#include "linkLayer.h"

struct termios oldtio, newtio;

volatile int STOP=FALSE;
int flag=1, conta=1;
unsigned char controlField = I0_C;
int currentFrame = 1;

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

void atende() {                  // atende alarme
    printf("alarme # %d\n", conta);
    flag=1;
    conta++;
}

int llopen(int port, int status) {
    int i = 0, res, ret;

    if ( tcgetattr(port, oldtio) == -1) { /* save current port settings */
      perror("tcgetattr");
      return -1;
    }

    bzero(newtio, sizeof(newtio));
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

    if ( tcsetattr(port, TCSANOW, newtio) == -1) {
      perror("tcsetattr");
      return -1;
    }

    printf("New termios structure set\n");

    while(conta < 4 && STOP == FALSE){
        if(flag){
            alarm(3);                 // activa alarme de 3s
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

    int frameSize = byteStuffing(bound, (currentPosition+1));

    return frameSize;
}

int createInfoFrame(unsigned char *message, int messageSize, unsigned char *infoFrame) {
    infoFrame[0] = FLAG;
    infoFrame[1] = A;
    infoFrame[2] = controlField;
    infoFrame[3] = A^controlField;
    infoFrame[4] = INFO_FRAME;

    infoFrame[5] = currentFrame % 255;
    infoFrame[6] = (messageSize >> 8) & 0xFF;
    infoFrame[7] = messageSize & 0xFF;

    unsigned char bccFinal = infoFrame[4]^infoFrame[5]^infoFrame[6]^infoFrame[7];

    int currentPosition = 8;
    for (size_t i = 0; i < messageSize; i++) {
        infoFrame[currentPosition++] = message[i];
        bccFinal ^= message[i];
    }

    infoFrame[currentPosition++] = bccFinal;
    infoFrame[currentPosition] = FLAG;

    currentFrame++;

    int frameSize = byteStuffing(infoFrame, (currentPosition+1));

    return frameSize;
}


int byteStuffing(unsigned char* frame, int frameSize) {
    unsigned char tempFrame[frameSize*2];
    int newSize = frameSize;

    tempFrame[0] = FLAG;

    size_t j = 1;
    for (size_t i = 1; i < frameSize-1; i++) {
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

    for (size_t i = 0; i < newSize; i++) {
        frame[i] = tempFrame[i];
    }

    return newSize;
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


int llclose(int port) {
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

    if (tcsetattr(port, TCSANOW, oldtio) == -1) {
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
