#include "receiver.h"

// Sender frames
unsigned char set[5] = {FLAG,A,SET_C,SET_BCC,FLAG};
unsigned char ua_em[5] = {FLAG,A_ALT,UA_C,UA_BCC_EM,FLAG};
unsigned char disc_em[5] = {FLAG,A,DISC_C,DISC_BCC_EM,FLAG};

// Receiver frames
unsigned char ua_rec[5] = {FLAG,A,UA_C,UA_BCC_REC,FLAG};
unsigned char disc_rec[5] = {FLAG,A_ALT,DISC_C,DISC_BCC_REC ,FLAG};
unsigned char rr0[5] = {FLAG,A,RR_C_N0,RR0_BCC,FLAG};
unsigned char rr1[5] = {FLAG,A,RR_C_N1,RR1_BCC,FLAG};
unsigned char rej0[5] = {FLAG, A, REJ_C_N0, REJ0_BCC, FLAG};
unsigned char rej1[5] = {FLAG, A, REJ_C_N1, REJ1_BCC, FLAG};

unsigned char controlField = I0_C;

int stopData = FALSE;

int main(int argc, char const *argv[]) {
  if ( (argc < 2) || ((strcmp("/dev/ttyS0", argv[1])!=0) && (strcmp("/dev/ttyS1", argv[1])!=0) )) {
    printf("Usage:\tnserial SerialPort\n\tex: nserial /dev/ttyS0\n");
    exit(1);
  }

    unsigned char filedata[MAX_INFO_SIZE*2];

    int fileSize;
    char fileName[255*2];

    ApplicationLayer appLayer;

    /*
    Open serial port device for reading and writing and not as controlling tty
    because we don't want to get killed if linenoise sends CTRL-C.
  */


    appLayer.fd = open(argv[1], O_RDWR | O_NOCTTY );
    if (appLayer.fd <0) {perror(argv[1]); exit(-1); }

    if(llopen(&appLayer) == -1)
        return -1;

    int size;
    if((size = llread(appLayer.fd, filedata))== -1)
        return -1;

    printf("start size: %d\n",size);

    getFileName(filedata, fileName);

    getFileSize(filedata, &fileSize);

    printf("File name: %s\n", fileName);
    printf("File size: %d\n", fileSize);

    int i;
    for(i = 0; i < size - 1; i++)
      printf("start[%d]: %x\n",i, filedata[i]);

    FILE *file = fopen("pinguim.gif", "ab+");
    int messageSize;
    do {
      if((messageSize = llread(appLayer.fd, filedata)) == -1)
          return -1;

      if(!stopData)
          writeFileData(filedata, file);

    }while(!stopData);


    printf("end size: %d\n",messageSize);

    for(i = 0; i < size - 1; i++)
      printf("end[%d]: %x\n",i, filedata[i]);

    if(llclose(&appLayer) == -1)
        return -1;


  return 0;
}


int llopen(ApplicationLayer *appLayer) {

    int res, ret, i = 0;

    if ( tcgetattr((*appLayer).fd,&(*appLayer).oldtio) == -1) { /* save current port settings */
      perror("tcgetattr");
      return -1;
    }

    bzero(&(*appLayer).newtio, sizeof((*appLayer).newtio));
    (*appLayer).newtio.c_cflag = BAUDRATE | CS8 | CLOCAL | CREAD;
    (*appLayer).newtio.c_iflag = IGNPAR;
    (*appLayer).newtio.c_oflag = 0;

    /* set input mode (non-canonical, no echo,...) */
    (*appLayer).newtio.c_lflag = 0;

    (*appLayer).newtio.c_cc[VTIME]    = 1;   /* inter-character timer unused */
    (*appLayer).newtio.c_cc[VMIN]     = 0;   /* blocking read until 5 chars received */



  /*
    VTIME e VMIN devem ser alterados de forma a proteger com um temporizador a
    leitura do(s) pr�ximo(s) caracter(es)
  */

    tcflush((*appLayer).fd, TCIOFLUSH);

    if ( tcsetattr((*appLayer).fd,TCSANOW,&(*appLayer).newtio) == -1) {
      perror("tcsetattr");
      return -1;
    }

    printf("New termios structure set\n");

    while (STOP == FALSE) {

         if((ret = stateMachineSupervision((*appLayer).fd,&i,set)) == -1)
            return -1;
    }

    printf("SET received\n");

    if ((res = write((*appLayer).fd,ua_rec,5)) == -1) {
        printf("An error has occured.\n");
        return -1;
    }

    printf("UA sent, %d bytes written\n", res);


  return 0;
}


int llread(int port, unsigned char *data){
    int res;
    int rej = FALSE;
    int size;
    unsigned char buf;
    int state = 0;
    STOP = FALSE;

    while(STOP == FALSE) {

        res = read(port, &buf, 1);
        //printf("%x\n", buf);
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

                  printf("BCC_OK\n");
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
          printf("BCC wrong\n");
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
            data[size++] = buf;
          }
          else if(buf2 == (ESC ^ 0x20)){
            bccFinal ^= ESC;
            data[size++] = buf;
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
    printf("INFO FRAME:%x\n", buf);
      if(buf == FLAG){
        if(bccFinal == 0){
          return size;
        }
        else{
          printf("BCC wrong\n");
          return -2;
        }
      }
      else if(buf == ESC){
        res2 = read(port, &buf2, 1);

        printf("INFO FRAME ESC:%x\n", buf2);
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
            data[size++] = buf;
          }
          else if(buf2 == (ESC ^ 0x20)){
            bccFinal ^= ESC;
            data[size++] = buf;
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

int getFileName(unsigned char * data, char * fileName) {

    int lengthSize = (int) data[2];
    int lengthName = (int) data[4+lengthSize];
    int i;


    for (i = 0; i < lengthName; i++)
        fileName[i] = data[5 + lengthSize + i];

    return 0;
}

int getFileSize(unsigned char * data, int * fileSize) {

  int length = (int) data[2];
  int i;
  *fileSize = 0;

  for (i = 0; i < length; i++){
      *fileSize += data[3+i] << (8*(length-(i+1)));
  }

  return 0;

}

int writeFileData(unsigned char * data, FILE * fd) {

    int l1 = (int) data[3];
    int l2 = (int) data[2];
    int k = 256*l2 + l1;
    int i;

    for(i = 0; i < k; i++) {
        if((fwrite(&data[4+i], sizeof(unsigned char), 1, fd)) == 0)
            return -1;
    }

    return 0;
}

int llclose(ApplicationLayer *appLayer) {

    int res, ret, i = 0;
    STOP = FALSE;

    while (STOP == FALSE) {

        if((ret = stateMachineSupervision((*appLayer).fd,&i,disc_em)) == -1)
        return -1;
    }

    printf("DISC received\n");

    if ((res = write((*appLayer).fd,disc_rec,5)) == -1) {
        printf("An error has occured.\n");
        return -1;
    }

    printf("DISC sent, %d bytes written\n", res);

    while (STOP == FALSE) {

    if((ret = stateMachineSupervision((*appLayer).fd,&i,ua_em)) == -1)
    return -1;
    }

    printf("UA received\n");


  if(tcsetattr((*appLayer).fd,TCSANOW,&(*appLayer).oldtio) == -1){
      perror("tcsetattr");
      return -1;
  }

  close((*appLayer).fd);

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
                *state=START;
            break;
        case A_RCV:
            if (buf == frame[2])
                *state = C_RCV;
            else if (buf == frame[0])
                *state = FLAG_RCV;
            else
                *state=START;
            break;
        case C_RCV:
            if (buf == frame[3])
                *state = BCC_OK;
            else if (buf == frame[0])
                *state = FLAG_RCV;
            else
                *state=START;
            break;
        case BCC_OK:
            if (buf == frame[4])
                STOP = TRUE;
            else
                *state=START;
            break;
        default:
            printf("There was an error or the message is not valid.\n");
            return -1;
            break;
      }
  }

  return 0;
}
