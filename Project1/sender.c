#include "sender.h"

// Sender frames
unsigned char set[5] = {FLAG, A, SET_C, SET_BCC, FLAG};
unsigned char ua_em[5] = {FLAG, A_ALT, UA_C, UA_BCC_EM, FLAG};
unsigned char disc_em[5] = {FLAG, A, DISC_C, DISC_BCC_EM, FLAG};

// Receiver frames
unsigned char ua_rec[5] = {FLAG, A, UA_C, UA_BCC_REC, FLAG};
unsigned char disc_rec[5] = {FLAG, A_ALT, DISC_C, DISC_BCC_REC, FLAG};
unsigned char rr0[5] = {FLAG, A, RR_C_N0, RR0_BCC, FLAG};
unsigned char rr1[5] = {FLAG, A, RR_C_N1, RR1_BCC, FLAG};

volatile int STOP=FALSE;
int flag=1, conta=1;
unsigned char controlField = I0_C;

void atende() {                  // atende alarme
    printf("alarme # %d\n", conta);
    flag=1;
    conta++;
}

int main(int argc, char const *argv[]) {
    if ( (argc < 3) || ((strcmp("/dev/ttyS0", argv[1])!=0) && (strcmp("/dev/ttyS1", argv[1])!=0) )) {
        printf("Usage:\tnserial SerialPort\n\tex: nserial /dev/ttyS0\n");
        exit(1);
    }

    ApplicationLayer appLayer;

    (void) signal(SIGALRM, atende);  // instala  rotina que atende interrupcao

    const char *fileName = argv[2];
    FILE *file;
    if ((file = fopen(fileName, "r")) == NULL) {
        printf("Program ended with error n. %d\n", errno);
        return -1;
    }

    fseek(file, 0L, SEEK_END);
    long fileSize = ftell(file);

    fclose(file);

    printf("size: %ld\n", fileSize);

    /*
    Open serial port device for reading and writing and not as controlling tty
    because we don't want to get killed if linenoise sends CTRL-C.
    */
    appLayer.fd = open(argv[1], O_RDWR | O_NOCTTY );
    if (appLayer.fd < 0) {
        perror(argv[1]);
        return -1;
    }

    if (llopen(&appLayer) == -1) {
        return -1;
    }

    unsigned char boundFrame[255];
    int frameSize = createBoundFrame(boundFrame, fileSize, fileName, START_FRAME);
    llwrite(appLayer.fd, boundFrame, frameSize);

    if ((file = fopen(fileName, "r")) == NULL) {
        printf("Program ended with error n. %d\n", errno);
        return -1;
    }

    int messageSize;
    unsigned char message[MAX_MSG_SIZE];
    unsigned char infoFrame[MAX_FRAME_SIZE];
    do {
        messageSize = fread(message, sizeof(char), MAX_MSG_SIZE, file);
        frameSize = createInfoFrame(message, messageSize, infoFrame);
        llwrite(appLayer.fd, infoFrame, frameSize);
    } while (messageSize != MAX_MSG_SIZE);


    frameSize = createBoundFrame(boundFrame, fileSize, fileName, END_FRAME);
    llwrite(appLayer.fd, boundFrame, frameSize);


    if (llclose(&appLayer) == -1) {
        return -1;
    }

    return 0;
}

int llopen(ApplicationLayer *appLayer) {
    int i = 0, res, ret;

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

    (*appLayer).newtio.c_cc[VTIME]    = 1;   /* inter-character used */
    (*appLayer).newtio.c_cc[VMIN]     = 0;   /* blocking read until n chars received */

    /*
      VTIME e VMIN devem ser alterados de forma a proteger com um temporizador a
      leitura do(s) proximo(s) caracter(es)
    */

    tcflush((*appLayer).fd, TCIOFLUSH);

    if ( tcsetattr((*appLayer).fd,TCSANOW,&(*appLayer).newtio) == -1) {
      perror("tcsetattr");
      return -1;
    }

    printf("New termios structure set\n");

    while(conta < 4 && STOP == FALSE){
        if(flag){
            alarm(3);                 // activa alarme de 3s
            flag=0;

            if ((res = write((*appLayer).fd,set,5)) == -1) {
                printf("An error has occured writing the message.\n");
                return -1;
            }

            printf("Start sent, %d bytes written.\n", res);
        }

        if ((ret = stateMachineSupervision((*appLayer).fd, &i, ua_rec)) == -1)
            return -1;
    }

    if (conta != 4) {
        printf("UA received.\n");
    }
    alarm(0);

    return 0;
}

int createBoundFrame(unsigned char *bound, long fileSize, const char *fileName, unsigned char frame) {
    bound[0] = FLAG;
    bound[1] = A;
    bound[2] = controlField;
    bound[3] = A^controlField;
    bound[4] = frame;

    bound[5] = T1;
    bound[6] = LONG_SIZE;
    bound[7] = (fileSize >> 24) & 0xFF;
    bound[8] = (fileSize >> 16) & 0xFF;
    bound[9] = (fileSize >> 8) & 0xFF;
    bound[10] = fileSize & 0xFF;

    bound[11] = T2;
    bound[12] = strlen(fileName);
    int currentPosition = 12;
    for (size_t i = 0; i < strlen(fileName); i++) {
        bound[++currentPosition] = fileName[i];
    }

    unsigned char bccFinal = frame;

    for (size_t i = 5; i <= currentPosition; i++) {
        bccFinal ^= bound[i];
    }

    bound[++currentPosition] = bccFinal;
    bound[++currentPosition] = FLAG;

    return (currentPosition+1);
}

int createInfoFrame(unsigned char *message, int messageSize, unsigned char *infoFrame) {
    infoFrame[0] = FLAG;
    infoFrame[1] = A;
    infoFrame[2] = controlField;
    infoFrame[3] = A^controlField;
    infoFrame[4] = INFO_FRAME;

    return 0;
}

int llwrite(int fd, unsigned char *buf, int length) {
    conta = 1;
    flag = 1;
    STOP = FALSE;
    int i = 0, res, ret;

    while(conta < 4 && STOP == FALSE){
        if(flag){
            alarm(3);                 // activa alarme de 3s
            flag=0;

            if ((res = write(fd,buf,length)) == -1) {
                printf("An error has occured writing the message.\n");
                return -1;
            }

            printf("Info sent, %d bytes written.\n", res);
        }

        if (controlField == I0_C) {
            if ((ret = stateMachineSupervision(fd, &i, rr1)) == -1)
                exit(-1);
        }
        else {
            if ((ret = stateMachineSupervision(fd, &i, rr0)) == -1)
                exit(-1);
        }
    }

    if (conta != 4) {
        printf("Answer received\n");
    }

    alarm(0);

    controlField ^= I1_C;

    return 0;
}

int llclose(ApplicationLayer *appLayer) {
    conta = 1;
    flag = 1;
    STOP = FALSE;
    int i = 0, res, ret;

    while(conta < 4 && STOP == FALSE){
        if(flag){
            alarm(3);                 // activa alarme de 3s
            flag=0;

            if ((res = write((*appLayer).fd,disc_em,5)) == -1) {
                printf("An error has occured writing the message.\n");
                return -1;
            }

            printf("DISC sent, %d bytes written.\n", res);
        }

        if ((ret = stateMachineSupervision((*appLayer).fd, &i, disc_rec)) == -1)
            return -1;
    }

    printf("DISC received.\n");
    alarm(0);

    if ((res = write((*appLayer).fd,ua_em,5)) == -1) {
        printf("An error has occured writing the message.\n");
        return -1;
    }

    printf("UA sent, %d bytes written.\n", res);

    if (tcsetattr((*appLayer).fd,TCSANOW,&(*appLayer).oldtio) == -1) {
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
