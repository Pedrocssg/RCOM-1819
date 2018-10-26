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

unsigned char controlField = I0_C;

int main(int argc, char const *argv[]) {
  if ( (argc < 2) || ((strcmp("/dev/ttyS0", argv[1])!=0) && (strcmp("/dev/ttyS1", argv[1])!=0) )) {
    printf("Usage:\tnserial SerialPort\n\tex: nserial /dev/ttyS0\n");
    exit(1);
  }

    unsigned char * filedata = (unsigned char *) malloc(0);

    int fileSize;
    char * fileName = (char *) malloc(0);

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

    printf("start size:%d\n",size);

    getFileName(filedata, fileName);

    getFileSize(filedata, &fileSize);

    printf("%s\n", fileName);
    printf("%d\n", fileSize);

    int i;
    for(i = 0; i < size - 1; i++)
      printf("start[%d]: %x\n",i, filedata[i]);

    FILE *file = fopen("pinguim.gif", "ab+");
    int messageSize;
    do {
      if((messageSize = llread(appLayer.fd, filedata)) == -1)
          return -1;

    }while(messageSize < fileSize);



    if(llread(appLayer.fd, filedata) == -1)
        return -1;

    printf("end size:%d\n",size);

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
    int size;
    unsigned char buf;
    int state = 0;
    STOP = FALSE;

    while(STOP == FALSE) {

        res = read(port, &buf, 1);
        printf("%x\n", buf);
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

                  printf("FLAG\n");
                  break;
              case A_RCV:
                  printf("CONTROL:%x\n",controlField);
                  printf("BUF:%x\n",buf);
                  if (buf == controlField){
                      state = C_RCV;
                  }
                  else if (buf == (controlField ^ I1_C))
                      return 1; // QUANDO ESTA A RECEBER UMA TRAMA REPETIDA, ENVIAR RR
                  else
                      state=START;

                  printf("A\n");
                  break;
              case C_RCV:
                  if (buf == (A^controlField))
                      state = BCC_OK;
                  else if (buf == FLAG)
                      state = FLAG_RCV;
                  else
                      state=START;

                  printf("C\n");
                  break;
              case BCC_OK:
                  if (buf == INFO_FRAME){
                    processInfo(data);
                    state = DATA_OK;
                  }
                  else if (buf == START_FRAME || buf == END_FRAME){
                    if((size = processBounds(port, data, buf)) >= 0)
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

    unsigned char * receiverReady;
    controlField = controlField ^ I1_C;

    if(controlField == I0_C)
        receiverReady = rr0;
    else if(controlField == I1_C)
        receiverReady = rr1;

    if((res = write(port,receiverReady,5)) == -1) {
        printf("An error has occured.\n");
        return -1;
    }

    printf("Receiver ready sent, %d bytes written\n", res);



    return size;
}

int processInfo(unsigned char * data){
  return 0;
}

int processBounds(int port, unsigned char * data, unsigned char c){
  int res, size = 0;
  unsigned char buf;
  unsigned char bccFinal = c;
  data = (unsigned char *) realloc(data, 1);
  data[size++] = c;

  printf("teste\n");

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
          return -1;
        }
      }
      else{
        bccFinal ^= buf;

        data = (unsigned char *)realloc(data, ++size);
        data[size-1] = buf;
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
