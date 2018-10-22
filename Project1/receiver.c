#include "receiver.h"

// Sender frames
unsigned char set[5] = {FLAG,A,SET_C,SET_BCC,FLAG};
unsigned char ua_em[5] = {FLAG,A_ALT,UA_C,UA_BCC,FLAG};
unsigned char disc_em[5] = {FLAG,A,DISC_C,SET_BCC,FLAG};

// Receiver frames
unsigned char disc_rec[5] = {FLAG,A_ALT,DISC_C,SET_BCC,FLAG};
unsigned char ua_rec[5] = {FLAG,A,UA_C,UA_BCC,FLAG};

int main(int argc, char const *argv[]) {
  if ( (argc < 2) || ((strcmp("/dev/ttyS0", argv[1])!=0) && (strcmp("/dev/ttyS1", argv[1])!=0) )) {
    printf("Usage:\tnserial SerialPort\n\tex: nserial /dev/ttyS0\n");
    exit(1);
  }

    int fd;

    ApplicationLayer appLayer;
    
    /*
    Open serial port device for reading and writing and not as controlling tty
    because we don't want to get killed if linenoise sends CTRL-C.
  */


    appLayer.fd = open(argv[1], O_RDWR | O_NOCTTY );
    if (appLayer.fd <0) {perror(argv[1]); exit(-1); }

    if(llopen(&appLayer) == -1)
        return -1;

    if(llread(&appLayer) == -1)
        return -1;

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

    (*appLayer).newtio.c_cc[VTIME]    = 0;   /* inter-character timer unused */
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

int llread(int port, char *buf){
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