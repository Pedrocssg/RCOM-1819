#include "linkLayer.h"

struct termios oldtio, newtio;

volatile int STOP=FALSE;
int REJ = FALSE;
int flag=1, counter=1;

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

void timeout() {
    printf("alarm # %d\n", counter);
    flag=1;
    counter++;
}

int setLinkLayer(int status){

  char *end;
  char buf[255];
  int n, valid;

  printf("Setting up\n");

  linkLayer.sequenceNumber = I0_C;

  printf("Baudrate (110, 150, 300, 1200, 2400, 4800, 9600, 19200, 38400, 57600, 115200, 230400): ");

  do {
      valid = FALSE;
      if (!fgets(buf, sizeof buf, stdin))
          break;

      buf[strlen(buf) - 1] = 0;

      n = strtol(buf, &end, 10);

      if(n == 110 || n == 150 || n == 300 || n == 1200 || n == 2400 || n == 4800 || n == 9600 || n == 19200 || n == 38400 || n == 57600 || n == 115200 || n == 230400)
          valid = TRUE;
      else
          printf("Please enter a valid baudrate: ");

  }while (end != buf + strlen(buf) || !valid);

  switch (n) {
    case 110:
        linkLayer.baudRate = B110;
        break;
    case 150:
        linkLayer.baudRate = B150;
        break;
    case 300:
        linkLayer.baudRate = B300;
        break;
    case 1200:
        linkLayer.baudRate = B1200;
        break;
    case 2400:
        linkLayer.baudRate = B2400;
        break;
    case 4800:
        linkLayer.baudRate = B4800;
        break;
    case 9600:
        linkLayer.baudRate = B9600;
        break;
    case 19200:
        linkLayer.baudRate = B19200;
        break;
    case 38400:
        linkLayer.baudRate = B38400;
        break;
    case 57600:
        linkLayer.baudRate = B57600;
        break;
    case 115200:
        linkLayer.baudRate = B115200;
        break;
    case 230400:
        linkLayer.baudRate = B230400;
        break;
  }

  printf("Timeout (in seconds): ");

  do {
       valid = FALSE;
       if (!fgets(buf, sizeof buf, stdin))
          break;

       buf[strlen(buf) - 1] = 0;

       n = strtol(buf, &end, 10);

       if(n > 0)
          valid = TRUE;
       else
          printf("Please enter a valid timeout: ");

  }while (end != buf + strlen(buf) || !valid);

  linkLayer.timeout = n;

  printf("Transmissions: ");

  do {
       valid = FALSE;
       if (!fgets(buf, sizeof buf, stdin))
          break;

       buf[strlen(buf) - 1] = 0;

       n = strtol(buf, &end, 10);

       if(n > 0)
          valid = TRUE;
       else
          printf("Please enter a valid transmission value: ");

  }while (end != buf + strlen(buf) || !valid);

  linkLayer.numTransmissions = n;

  printf("Maximum frame size: ");

  do {
       valid = FALSE;
       if (!fgets(buf, sizeof buf, stdin))
          break;

       buf[strlen(buf) - 1] = 0;

       n = strtol(buf, &end, 10);

       if(n < 0xffff)
          valid = TRUE;
       else
          printf("Please enter a valid maximum frame size value: ");

  }while (end != buf + strlen(buf) || !valid);

  linkLayer.maxFrameSize = n;

  if (status == TRANSMITTER) {
      printf("Random error (y/n): ");

      do {
           valid = FALSE;
           if (!fgets(buf, sizeof buf, stdin))
              break;

           buf[strlen(buf) - 1] = 0;

           if(*buf == 'y' || *buf == 'Y'){
               valid = TRUE;
               linkLayer.randomError = 1;
           }
           else if(*buf == 'n' || *buf == 'N'){
               valid = TRUE;
               linkLayer.randomError = 0;
           }
           else
               printf("Please enter a y or n: ");

      }while (!valid);

      if(linkLayer.randomError == TRUE){
        printf("Random error probability (1 in x times): ");
        do {
             valid = FALSE;
             if (!fgets(buf, sizeof buf, stdin))
                break;

             buf[strlen(buf) - 1] = 0;

             n = strtol(buf, &end, 10);

             if(n > 1)
                valid = TRUE;
             else
                printf("Please enter a valid probability value: ");

        }while (end != buf + strlen(buf) || !valid);

        linkLayer.errorProb = n;
      }
  }

  return 0;
}


int llopen(int port, int status) {
    if ( tcgetattr(port, &oldtio) == -1) {
      perror("tcgetattr");
      return -1;
    }

    bzero(&newtio, sizeof(newtio));
    newtio.c_cflag = linkLayer.baudRate | CS8 | CLOCAL | CREAD;
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
    flag = 1;
    counter = 1;

    signal(SIGALRM, timeout);

    while (STOP == FALSE && counter < (linkLayer.numTransmissions + 1)) {
        if(flag){
            alarm(linkLayer.timeout);
            flag=0;
        }

        if((ret = stateMachineSupervision(port,&i,set)) == -1)
            return -1;
    }

    alarm(0);

    if(counter < (linkLayer.numTransmissions + 1))
        printf("SET received\n");
    else{
        printf("SET not received\n");
        return -1;
    }

    if ((res = write(port,ua_rec,5)) == -1) {
        printf("An error has occured.\n");
        return -1;
    }


    printf("UA sent, %d bytes written\n", res);

    return 0;
}

int llopenTransmitterHandler(int port){
    int i = 0, res, ret;

    signal(SIGALRM, timeout);

    while(counter < (linkLayer.numTransmissions + 1) && STOP == FALSE){
        if(flag){
            alarm(linkLayer.timeout);
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

    if (counter != (linkLayer.numTransmissions + 1)) {
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
    unsigned char packet[linkLayer.maxFrameSize - FRAME_HEADER];

    size_t i;
    for (i = 0; i < packetSize; i++)
        packet[i] = frame[i];

    frame[0] = FLAG;
    frame[1] = A;
    frame[2] = linkLayer.sequenceNumber;
    frame[3] = A^linkLayer.sequenceNumber;

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

unsigned char randomError() {
    return ((rand()%linkLayer.errorProb) == 1) ? 1 : 0;
}

int llwrite(int port, unsigned char *buf, int *length) {
    counter = 1;
    flag = 1;
    STOP = FALSE;
    int repeated = FALSE;
    int i = 0, res, ret;

    *length = createFrame(buf, *length);

    while(counter < (linkLayer.numTransmissions + 1) && (STOP == FALSE || repeated == TRUE)) {
        repeated = FALSE;

        if(flag){
            alarm(linkLayer.timeout);
            flag=0;

            int oldBcc;
            if (linkLayer.randomError) {
                oldBcc = buf[*length - 2];
                buf[*length - 2] += randomError();
            }

            if ((res = write(port, buf, *length)) == -1) {
                printf("An error has occured writing the message.\n");
                return -1;
            }

            if (linkLayer.randomError) {
                buf[*length - 2] = oldBcc;
            }

            printf("Info sent, %d bytes written.\n", res);
        }

        ret = stateMachineInfoAnswer(port, &i);
        if (ret == -1)
            exit(-1);
        else if (ret == -2)
            repeated = TRUE;
    }

    if (counter == (linkLayer.numTransmissions + 1)) {
        printf("Exceeded maximum attempts.\n");
        return -1;
    }

    alarm(0);

    linkLayer.sequenceNumber ^= I1_C;

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
              else if (buf == FLAG){
                  printf("Nothing received\n");
              }
              else{
                  *state = START;
                  printf("Nothing received\n");
              }
              break;
          case A_RCV:
              if (buf == FLAG) {
                  *state = FLAG_RCV;
                  printf("Nothing received\n");
              }
              else if (buf == RR_C_N0 || buf == RR_C_N1 || buf == REJ_C_N0 || buf == REJ_C_N1) {
                  *state = C_RCV;
                  c = buf;
              }
              else{
                  *state = START;
                  printf("Nothing received\n");
              }
              break;
          case C_RCV:
              if (buf == FLAG) {
                  *state = FLAG_RCV;
                  printf("Nothing received\n");
              }
              else if ((buf == RR0_BCC && (A^c) == RR0_BCC) ||
                       (buf == RR1_BCC && (A^c) == RR1_BCC) ||
                       (buf == REJ0_BCC && (A^c) == REJ0_BCC) ||
                       (buf == REJ1_BCC && (A^c) == REJ1_BCC))
                  *state = BCC_OK;
              else{
                  *state = START;
                  printf("Nothing received\n");
              }
              break;
          case BCC_OK:
              if (buf == FLAG && (c == RR_C_N0 || c == RR_C_N1)){
                  STOP = TRUE;
                  printf("Rr received\n");
              }
              else if (buf == FLAG && (c == REJ_C_N0 || c == REJ_C_N1)){
                  *state = START;
                  printf("Rej received\n");
                  return -2;
              }
              else{
                  *state = START;
                  printf("Nothing received\n");
              }
              break;
          default:
              printf("There was an error or the message is not valid.\n");
              return -1;
              break;
        }
    }

    return 0;
}


int llread(int port, unsigned char *data){
    int res;
    int size = 0;
    unsigned char buf;
    int state = 0;
    int stopData = FALSE;
    int repeated = FALSE;
    STOP = FALSE;
    flag = 1;
    counter = 1;

    while(STOP == FALSE && counter < (linkLayer.numTransmissions + 1)) {
        if(flag){
            alarm(linkLayer.timeout);
            flag=0;
        }

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
                  if (buf == linkLayer.sequenceNumber){
                      state = C_RCV;
                  }
                  else if (buf == (linkLayer.sequenceNumber ^ I1_C)) {
                      repeated = TRUE;
                      state = C_RCV;
                  }
                  else
                      state = START;
                  break;
              case C_RCV:
                  if ((repeated == FALSE && buf == (A^linkLayer.sequenceNumber)) ||
                      (repeated == TRUE && buf == (A^(linkLayer.sequenceNumber^I1_C))))
                      state = BCC_OK;
                  else if (buf == FLAG)
                      state = FLAG_RCV;
                  else
                      state=START;
                  break;
              case BCC_OK:
                  if (buf == START_FRAME || buf == END_FRAME){
                    size = processBoundFrame(port, data, buf);
                    if(size > 0) {
                        if(REJ == TRUE && repeated == TRUE)
                            repeated = FALSE;

                        STOP = TRUE;
                        if(buf == END_FRAME)
                          stopData = TRUE;
                    }
                    else if(size == 0 || size == -4){
                        REJ = TRUE;
                        STOP = TRUE;
                    }
                    else
                        return -1;
                  }
                  else if (buf == INFO_FRAME){
                    size = processInfoFrame(port, data, buf);
                    if(size > 0){
                        if(REJ == TRUE && repeated == TRUE)
                            repeated = FALSE;

                        REJ = FALSE;
                        STOP = TRUE;
                    }
                    else if (size == 0 || size == -4){
                        REJ = TRUE;
                        STOP = TRUE;
                    }
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

    alarm(0);

    if(counter < (linkLayer.numTransmissions + 1))
        printf("Info received\n");
    else{
        printf("Info not received\n");
        return -1;
    }

    unsigned char * answer;

    if (repeated == FALSE && REJ == FALSE)
        linkLayer.sequenceNumber ^= I1_C;

    if(REJ == TRUE){
        if(linkLayer.sequenceNumber == I0_C)
            answer = rej0;
        else if(linkLayer.sequenceNumber == I1_C)
            answer = rej1;

        printf("Rej sent, ");
    }
    else{
        if(linkLayer.sequenceNumber == I0_C)
            answer = rr0;
        else if(linkLayer.sequenceNumber == I1_C)
            answer = rr1;

        printf("Rr sent, ");
    }

    if((res = write(port,answer,5)) == -1) {
        printf("An error has occured.\n");
        return -1;
    }

    printf("%d bytes written\n", res);


    if (stopData == TRUE)
        return -2;
    else if (repeated == TRUE)
        return -3;
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
          return -4;
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
          return -4;
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


int llcloseReceiver(int port) {
    int res, ret, i = 0;
    STOP = FALSE;
    flag = 1;
    counter = 1;

    while (STOP == FALSE && counter < (linkLayer.numTransmissions + 1)) {

        if(flag){
          flag = 0;
          alarm(linkLayer.timeout);
        }

        if((ret = stateMachineSupervision(port,&i,disc_em)) == -1)
        return -1;
    }

    alarm(0);

    if(counter < (linkLayer.numTransmissions + 1))
        printf("DISC received\n");
    else{
        printf("DISC not received\n");
        return -1;
    }

    if ((res = write(port,disc_rec,5)) == -1) {
        printf("An error has occured.\n");
        return -1;
    }

    printf("DISC sent, %d bytes written\n", res);


    flag = 1;
    counter = 1;
    STOP = FALSE;
    while (STOP == FALSE && counter < (linkLayer.numTransmissions + 1)) {

        if(flag){
          flag = 0;
          alarm(linkLayer.timeout);
        }

        if((ret = stateMachineSupervision(port,&i,ua_em)) == -1)
            return -1;
    }

    if(counter < (linkLayer.numTransmissions + 1))
        printf("UA received\n");
    else{
        printf("UA not received\n");
        return -1;
    }

    if(tcsetattr(port,TCSANOW,&oldtio) == -1){
        perror("tcsetattr");
        return -1;
    }

    close(port);

    return 0;
}

int llcloseTransmitter(int port) {
    counter = 1;
    flag = 1;
    STOP = FALSE;
    int i = 0, res, ret;

    while(counter < (linkLayer.numTransmissions + 1) && STOP == FALSE){
        if(flag){
            alarm(linkLayer.timeout);
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

    if (counter != (linkLayer.numTransmissions + 1)) {
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

    sleep(1);

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
