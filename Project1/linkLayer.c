#include "linkLayer.h"

struct termios oldtio, newtio;

volatile int STOP=FALSE;
int flag=1, conta=1;
unsigned char controlField = I0_C;

// Sender frames
unsigned char set[5] = {FLAG, A, SET_C, SET_BCC, FLAG};
unsigned char ua_em[5] = {FLAG, A_ALT, UA_C, UA_BCC_EM, FLAG};
unsigned char disc_em[5] = {FLAG, A, DISC_C, DISC_BCC_EM, FLAG};

// Receiver frames
unsigned char ua_rec[5] = {FLAG, A, UA_C, UA_BCC_REC, FLAG};
unsigned char disc_rec[5] = {FLAG, A_ALT, DISC_C, DISC_BCC_REC, FLAG};
unsigned char rr0[5] = {FLAG, A, RR_C_N0, RR0_BCC, FLAG};
unsigned char rr1[5] = {FLAG, A, RR_C_N1, RR1_BCC, FLAG};
unsigned char rej0[5] = {FLAG, A, REJ_C_N0, REJ0_BCC, FLAG};
unsigned char rej1[5] = {FLAG, A, REJ_C_N1, REJ1_BCC, FLAG};

void atende() {
    printf("alarme # %d\n", conta);
    flag=1;
    conta++;
}

int llopen(int port, int status) {
    if ( tcgetattr(port, &oldtio) == -1) {
      perror("tcgetattr");
      return -1;
    }

    bzero(&newtio, sizeof(newtio));
    newtio.c_cflag = BAUDRATE | CS8 | CLOCAL | CREAD;
    newtio.c_iflag = IGNPAR;
    newtio.c_oflag = 0;

    /* set input mode (non-canonical, no echo,...) */
    newtio.c_lflag = 0;

    newtio.c_cc[VTIME]    = 1;   /* inter-character used */
    newtio.c_cc[VMIN]     = 0;   /* blocking read until n chars received */

    /*
      VTIME e VMIN devem ser alterados de forma a proteger com um temporizador a
      leitura do(s) proximo(s) caracter(es)
    */

    tcflush(port, TCIOFLUSH);

    if ( tcsetattr(port, TCSANOW, &newtio) == -1) {
      perror("tcsetattr");
      return -1;
    }

    printf("New termios structure set\n");

    if (status == TRANSMITTER)
        return llopenTransmitterHandler(port);
    else if (status == RECEIVER)
        return llopenReceiverHandler(port);

    return 0;
}


int llopenReceiverHandler(int port){
    int res, ret, i = 0;

    while (STOP == FALSE) {
         if((ret = stateMachineSupervision(port,&i,set)) == -1)
            return -1;
    }

    printf("SET received\n");

    if ((res = write(port,ua_rec,5)) == -1) {
        printf("An error has occured.\n");
        return -1;
    }

    printf("UA sent, %d bytes written\n", res);

    return 0;
}


int llopenTransmitterHandler(int port){
    int i = 0, res, ret;

    signal(SIGALRM, atende);

    while(conta < 4 && STOP == FALSE){
        if(flag){
            alarm(3);
            flag=0;

            if ((res = write(port, set, 5)) == -1) {
                printf("An error has occured writing the message.\n");
                return -1;
            }

            printf("Set sent, %d bytes written.\n", res);
        }

        if ((ret = stateMachineSupervision(port, &i, ua_rec)) == -1)
            return -1;
    }

    if (conta != 4) {
        printf("UA received.\n");
    }
    else {
        printf("UA not received.\n");
        return -1;
    }

    alarm(0);

    return 0;
}


int createFrame(unsigned char *frame, int packetSize) {
    unsigned char packet[MAX_INFO_SIZE];

    size_t i;
    for (i = 0; i < packetSize; i++)
        packet[i] = frame[i];

    frame[0] = FLAG;
    frame[1] = A;
    frame[2] = controlField;
    frame[3] = A^controlField;

    int currentPosition = 3;
    for (i = 0; i < packetSize; i++) {
        frame[++currentPosition] = packet[i];
    }

    frame[++currentPosition] = FLAG;

    int frameSize = byteStuffing(frame, (currentPosition+1));

    return frameSize;
}


int byteStuffing(unsigned char* frame, int frameSize) {
    unsigned char tempFrame[frameSize*2];
    int newSize = frameSize;

    tempFrame[0] = FLAG;

    size_t j = 1;
    size_t i;
    for (i = 1; i < frameSize-1; i++) {
        if (frame[i] == FLAG) {
            tempFrame[j] = ESC;
            tempFrame[++j] = FLAG^0x20;
            newSize++;
        }
        else if (frame[i] == ESC) {
            tempFrame[j] = ESC;
            tempFrame[++j] = ESC^0x20;
            newSize++;
        }
        else {
            tempFrame[j] = frame[i];
        }

        j++;
    }

    tempFrame[newSize-1] = FLAG;

    for (i = 0; i < newSize; i++) {
        frame[i] = tempFrame[i];
    }

    return newSize;
}


int llread(int port, unsigned char *data){
    int res;
    int rej = FALSE;
    int size;
    unsigned char buf;
    int state = 0;
    int stopData = FALSE;
    STOP = FALSE;

    while(STOP == FALSE) {

        res = read(port, &buf, 1);
        if (res < 0) {
            printf("There was an error while reading the buffer.\n");
            return -1;
        }
        else if (res == 0) {
            continue;
        }
        else{
            switch (state) {
              case START:
                  if (buf == FLAG)
                      state = FLAG_RCV;

                  break;
              case FLAG_RCV:
                  if (buf == A)
                      state = A_RCV;
                  else if (buf != FLAG)
                      state=START;

                  break;
              case A_RCV:
                  if (buf == controlField){
                      state = C_RCV;
                  }
                  else if (buf == (controlField ^ I1_C))
                      STOP = TRUE;
                  else
                      state = START;

                  break;
              case C_RCV:
                  if (buf == (A^controlField))
                      state = BCC_OK;
                  else if (buf == FLAG)
                      state = FLAG_RCV;
                  else
                      state=START;

                  break;
              case BCC_OK:
                  if (buf == START_FRAME || buf == END_FRAME){
                    size = processBoundFrame(port, data, buf);
                    if(size >= 0) {
                        STOP = TRUE;
                        if(buf == END_FRAME)
                          stopData = TRUE;
                    }
                    else if(size == -2){
                        rej = TRUE;
                        STOP = TRUE;
                    }
                    else
                      return -1;
                  }
                  else if (buf == INFO_FRAME){
                    if((size = processInfoFrame(port, data, buf)) >= 0)
                        STOP = TRUE;
                    else
                        return -1;
                  }
                  else if (buf == FLAG)
                      state = FLAG_RCV;
                  else
                      state=START;
                  break;
              default:
                  printf("There was an error or the message is not valid.\n");
                  return -1;
                  break;
            }
        }
    }

    unsigned char * answer;

    controlField = controlField ^ I1_C;
    if(rej == TRUE){
        if(controlField == I0_C)
            answer = rej0;
        else if(controlField == I1_C)
            answer = rej1;
    }
    else{
        if(controlField == I0_C)
            answer = rr0;
        else if(controlField == I1_C)
            answer = rr1;
    }

    if((res = write(port,answer,5)) == -1) {
        printf("An error has occured.\n");
        return -1;
    }

    printf("Receiver ready sent, %d bytes written\n", res);

    if (stopData == TRUE)
        return -2;
    else
        return size;
}

int processBoundFrame(int port, unsigned char * data, unsigned char c){
  int res, res2, size = 0;
  unsigned char buf;
  unsigned char buf2;
  unsigned char bccFinal = c;
  data[size++] = c;

  while(TRUE){
    res = read(port, &buf, 1);
    if (res < 0) {
        printf("There was an error while reading the buffer.\n");
        return -1;
    }
    else if (res == 0) {
        return 0;
    }
    else
    {
      if(buf == FLAG){
        if(bccFinal == 0){
          return size;
        }
        else{
          return -2;
        }
      }
      else if(buf == ESC){
        res2 = read(port, &buf2, 1);
        if (res2 < 0) {
            printf("There was an error while reading the buffer.\n");
            return -1;
        }
        else if (res2 == 0) {
            return 0;
        }
        else{
          if(buf2 == (FLAG ^ 0x20)){
            bccFinal ^= FLAG;
            data[size++] = FLAG;
          }
          else if(buf2 == (ESC ^ 0x20)){
            bccFinal ^= ESC;
            data[size++] = ESC;
          }
          else{
            bccFinal ^= buf;
            data[size++] = buf;
            bccFinal ^= buf2;
            data[size++] = buf2;
          }
        }
      }
      else{
        bccFinal ^= buf;
        data[size++] = buf;
      }
    }
  }
}

int processInfoFrame(int port, unsigned char * data, unsigned char c){
  int res, res2, size = 0;
  unsigned char buf;
  unsigned char buf2;
  unsigned char bccFinal = c;
  data[size++] = c;

  while(TRUE){
    res = read(port, &buf, 1);
    if (res < 0) {
        printf("There was an error while reading the buffer.\n");
        return -1;
    }
    else if (res == 0) {
        return 0;
    }
    else
    {
      if(buf == FLAG){
        if(bccFinal == 0){
          return size;
        }
        else{
          return -2;
        }
      }
      else if(buf == ESC){
        res2 = read(port, &buf2, 1);
        if (res2 < 0) {
            printf("There was an error while reading the buffer.\n");
            return -1;
        }
        else if (res2 == 0) {
            return 0;
        }
        else{
          if(buf2 == (FLAG ^ 0x20)){
            bccFinal ^= FLAG;
            data[size++] = FLAG;
          }
          else if(buf2 == (ESC ^ 0x20)){
            bccFinal ^= ESC;
            data[size++] = ESC;
          }
          else{
            bccFinal ^= buf;
            data[size++] = buf;
            bccFinal ^= buf2;
            data[size++] = buf2;
          }
        }
      }
      else{
        bccFinal ^= buf;
        data[size++] = buf;
      }
    }
  }
}


int llwrite(int port, unsigned char *buf, int length) {
    conta = 1;
    flag = 1;
    STOP = FALSE;
    int i = 0, res, ret;

    while(conta < 4 && STOP == FALSE){
        if(flag){
            alarm(3);                 // activa alarme de 3s
            flag=0;

            if ((res = write(port, buf, length)) == -1) {
                printf("An error has occured writing the message.\n");
                return -1;
            }

            printf("Info sent, %d bytes written.\n", res);
        }

        if ((ret = stateMachineInfoAnswer(port, &i)) == -1)
            exit(-1);
    }

    if (conta != 4)
        printf("Answer received\n");
    else {
        printf("Answer not received\n");
        return -1;
    }

    alarm(0);

    controlField ^= I1_C;

    return 0;
}

int stateMachineInfoAnswer(int port, int *state) {
    int res;
    unsigned char buf;
    static int c;

    res = read(port, &buf, 1);

    if (res < 0) {
        printf("There was an error while reading the buffer.\n");
        return -1;
    }
    else if (res == 0) {
        return 0;
    }
    else {
        switch (*state) {
          case START:
              if (buf == FLAG)
                  *state = FLAG_RCV;
              break;
          case FLAG_RCV:
              if (buf == A)
                  *state = A_RCV;
              else if (buf != FLAG)
                  *state = START;
              break;
          case A_RCV:
              if (buf == FLAG) {
                  *state = FLAG_RCV;
                  break;
              }
              else if ((controlField == I1_C && buf == RR_C_N0) || (controlField == I0_C && buf == RR_C_N1)) {
                  *state = C_RCV;
                  c = buf;
              }
              else if ((controlField == I1_C && buf == REJ0_BCC) || (controlField == I0_C && buf == REJ1_BCC)) {
                  *state = C_RCV;
                  c = buf;
              }
              else
                  *state = START;
              break;
          case C_RCV:
              if (buf == FLAG)
                  *state = FLAG_RCV;
              else if (buf == RR0_BCC && (A^c) == RR0_BCC)
                  *state = BCC_OK;
              else if (buf == RR1_BCC && (A^c) == RR1_BCC)
                  *state = BCC_OK;
              else if (buf == REJ0_BCC && (A^c) == REJ0_BCC)
                  *state = BCC_OK;
              else if (buf == REJ1_BCC && (A^c) == REJ1_BCC)
                  *state = BCC_OK;
              else
                  *state = START;
              break;
          case BCC_OK:
              if (buf == FLAG && (c == RR_C_N0 || c == RR_C_N1))
                  STOP = TRUE;
              else
                  *state = START;
              break;
          default:
              printf("There was an error or the message is not valid.\n");
              return -1;
              break;
        }
    }

    return 0;
}

int llcloseReceiver(int port) {
    int res, ret, i = 0;
    STOP = FALSE;

    while (STOP == FALSE) {

        if((ret = stateMachineSupervision(port,&i,disc_em)) == -1)
        return -1;
    }

    printf("DISC received\n");

    if ((res = write(port,disc_rec,5)) == -1) {
        printf("An error has occured.\n");
        return -1;
    }

    printf("DISC sent, %d bytes written\n", res);

    while (STOP == FALSE) {

    if((ret = stateMachineSupervision(port,&i,ua_em)) == -1)
    return -1;
    }

    printf("UA received\n");

    if(tcsetattr(port,TCSANOW,&oldtio) == -1){
        perror("tcsetattr");
        return -1;
    }

    close(port);

    return 0;
}

int llcloseTransmitter(int port) {
    conta = 1;
    flag = 1;
    STOP = FALSE;
    int i = 0, res, ret;

    while(conta < 4 && STOP == FALSE){
        if(flag){
            alarm(3);                 // activa alarme de 3s
            flag=0;

            if ((res = write(port, disc_em, 5)) == -1) {
                printf("An error has occured writing the message.\n");
                return -1;
            }

            printf("DISC sent, %d bytes written.\n", res);
        }

        if ((ret = stateMachineSupervision(port, &i, disc_rec)) == -1)
            return -1;
    }

    alarm(0);

    if (conta != 4) {
        printf("DISC received.\n");
        alarm(0);

        if ((res = write(port, ua_em, 5)) == -1) {
            printf("An error has occured writing the message.\n");
            return -1;
        }

        printf("UA sent, %d bytes written.\n", res);
    }
    else {
        printf("DISC not received.\n");
        return -1;
    }

    if (tcsetattr(port, TCSANOW, &oldtio) == -1) {
        perror("tcsetattr");
        return -1;
    }

    close(port);

    return 0;
}


int stateMachineSupervision(int port, int *state, unsigned char *frame) {
    int res;
    unsigned char buf;

    res = read(port, &buf, 1);

    if (res < 0) {
        printf("There was an error while reading the buffer.\n");
        return -1;
    }
    else if (res == 0) {
        return 0;
    }
    else {
        switch (*state) {
          case START:
              if (buf == frame[0])
                  *state = FLAG_RCV;
              break;
          case FLAG_RCV:
              if (buf == frame[1])
                  *state = A_RCV;
              else if (buf != frame[0])
                  *state = START;
              break;
          case A_RCV:
              if (buf == frame[2])
                  *state = C_RCV;
              else if (buf == frame[0])
                  *state = FLAG_RCV;
              else
                  *state = START;
              break;
          case C_RCV:
              if (buf == frame[3])
                  *state = BCC_OK;
              else if (buf == frame[0])
                  *state = FLAG_RCV;
              else
                  *state = START;
              break;
          case BCC_OK:
              if (buf == frame[4])
                  STOP = TRUE;
              else
                  *state = START;
              break;
          default:
              printf("There was an error or the message is not valid.\n");
              return -1;
              break;
        }
    }

    return 0;
}
