#include "macros.h"

unsigned char set[5] = {FLAG,A,SET_C,SET_BCC,FLAG};
unsigned char ua[5] = {FLAG,A,UA_C,UA_BCC,FLAG};
volatile int STOP=FALSE;

int main(int argc, char const *argv[]) {
  int fd,c, res;
  struct termios oldtio,newtio;
  char buf[255];
  int i, sum = 0, speed = 0;

  if ( (argc < 2) ||
       ((strcmp("/dev/ttyS0", argv[1])!=0) &&
        (strcmp("/dev/ttyS1", argv[1])!=0) )) {
    printf("Usage:\tnserial SerialPort\n\tex: nserial /dev/ttyS1\n");
    exit(1);
  }

/*
  Open serial port device for reading and writing and not as controlling tty
  because we don't want to get killed if linenoise sends CTRL-C.
*/

  fd = open(argv[1], O_RDWR | O_NOCTTY );
  if (fd < 0) {perror(argv[1]); exit(-1); }

  if ( tcgetattr(fd,&oldtio) == -1) { /* save current port settings */
    perror("tcgetattr");
    exit(-1);
  }

  bzero(&newtio, sizeof(newtio));
  newtio.c_cflag = BAUDRATE | CS8 | CLOCAL | CREAD;
  newtio.c_iflag = IGNPAR;
  newtio.c_oflag = 0;

  /* set input mode (non-canonical, no echo,...) */
  newtio.c_lflag = 0;

  newtio.c_cc[VTIME]    = 30;   /* inter-character timer unused */
  newtio.c_cc[VMIN]     = 0;   /* blocking read until n chars received */

/*
  VTIME e VMIN devem ser alterados de forma a proteger com um temporizador a
  leitura do(s) proximo(s) caracter(es)
*/

  tcflush(fd, TCIOFLUSH);

  if ( tcsetattr(fd,TCSANOW,&newtio) == -1) {
    perror("tcsetattr");
    exit(-1);
  }

  printf("New termios structure set\n");

  if ((res = write(fd,set,5)) == -1) {
      printf("An error has occured.\n");
      exit(-1);
  }

  printf("%d bytes written\n", res);

/*
  O ciclo FOR e as instrucoes seguintes devem ser alterados de modo a respeitar
  o indicado no guiao
*/
  i = 0;
  while (STOP == FALSE) {
      res = read(fd, &buf[i], 1);

      if (res < 0) {
          printf("You are dumb af...\n");
          exit(-1);
      }
      else {
          switch (i) {
            case 0:
                if (buf[i] == set[i])
                    i++;
                else
                    i--;
                break;
            case 1:
                if (buf[i] == set[i])
                    i++;
                else
                    i--;
                break;
            case 2:
                if (buf[i] == set[i])
                    i++;
                else
                    i--;
                break;
            case 3:
                if (buf[i] == set[i])
                    i++;
                else
                    i--;
                break;
            case 4:
                if (buf[i] == set[i]) {
                    i++;
                    STOP = TRUE;
                }
                else
                    i--;
                break;
            default:
                printf("There was an error or the message is not valid.\n");
                exit(-1);
                break;
          }
      }
  }

  printf("%s:%d\n", buf, i);

  if ( tcsetattr(fd,TCSANOW,&oldtio) == -1) {
    perror("tcsetattr");
    exit(-1);
  }

  close(fd);
  return 0;
}
