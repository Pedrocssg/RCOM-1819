#include "sender.h"

// Sender frames
unsigned char set[5] = {FLAG,A,SET_C,SET_BCC,FLAG};
unsigned char ua_em[5] = {FLAG,A_ALT,UA_C,UA_BCC,FLAG};
unsigned char disc_em[5] = {FLAG,A,DISC_C,SET_BCC,FLAG};

// Receiver frames
unsigned char ua_rec[5] = {FLAG,A,UA_C,UA_BCC,FLAG};
unsigned char disc_rec[5] = {FLAG,A_ALT,DISC_C,SET_BCC,FLAG};
unsigned char rr0[5] = {FLAG,A,RR_C_N0,SET_BCC,FLAG};
unsigned char rr1[5] = {FLAG,A,RR_C_N1,SET_BCC,FLAG};

volatile int STOP=FALSE;
int flag=1, conta=1;

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

  // if (llopen(&appLayer) == -1) {
  //     return -1;
  // }

  unsigned char start[255] = {FLAG, A, I0_C, A^I0_C, START_FRAME};

  start[0] = FLAG;
  start[1] = A;
  start[2] = I0_C;
  start[3] = A^I0_C;
  start[4] = START_FRAME;

  start[5] = T1;
  start[6] = LONG_SIZE;
  start[7] = (fileSize >> 24) & 0xFF;
  start[8] = (fileSize >> 16) & 0xFF;
  start[9] = (fileSize >> 8) & 0xFF;
  start[10] = fileSize & 0xFF;

  start[11] = T2;
  start[12] = strlen(fileName);
  int currentPosition = 12;
  for (size_t i = 0; i < strlen(fileName); i++) {
      start[++currentPosition] = fileName[i];
  }

  unsigned char bccFinal = START_FRAME;

  for (size_t i = 5; i <= currentPosition; i++) {
      bccFinal = bccFinal^start[i];
  }

  start[++currentPosition] = bccFinal;
  start[++currentPosition] = FLAG;

  // if (llclose(&appLayer) == -1) {
  //     return -1;
  // }

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

  (*appLayer).newtio.c_cc[VTIME]    = 0;   /* inter-character used */
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

          printf("SET sent, %d bytes written.\n", res);
      }

      if ((ret = stateMachineSupervision((*appLayer).fd, &i, ua_rec)) == -1)
          return -1;
  }

  printf("UA received.\n");
  alarm(0);

  return 0;
}

int llwrite(int fd, char *buf, int length) {
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

      if ((ret = stateMachineSupervision(fd, &i, rr0)) == -1)
          return -1;
  }

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
