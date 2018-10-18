#include "macros.h"

unsigned char set[5] = {FLAG,A,SET_C,SET_BCC,FLAG};
unsigned char ua[5] = {FLAG,A,UA_C,UA_BCC,FLAG};
volatile int STOP=FALSE;
int flag=1, conta=1;

void atende()                   // atende alarme
{
  printf("alarme # %d\n", conta);
  flag=1;
  conta++;
}

int main(int argc, char const *argv[]) {
  int fd,c, res;
  struct termios oldtio,newtio;
  unsigned char buf[255];
  int i, sum = 0, speed = 0;

  if ( (argc < 2) ||
       ((strcmp("/dev/ttyS0", argv[1])!=0) &&
        (strcmp("/dev/ttyS1", argv[1])!=0) )) {
    printf("Usage:\tnserial SerialPort\n\tex: nserial /dev/ttyS1\n");
    exit(1);
  }

  (void) signal(SIGALRM, atende);  // instala  rotina que atende interrupcao

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

  newtio.c_cc[VTIME]    = 0;   /* inter-character used */
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

  i=0;
  while(conta < 4 && STOP == FALSE){
      if(flag){
          alarm(3);                 // activa alarme de 3s
          flag=0;

          if ((res = write(fd,set,5)) == -1) {
              printf("An error has occured writing the message.\n");
              exit(-1);
          }

          printf("%d bytes written\n", res);
      }

      res = read(fd, &buf[i], 1);

      
      if (res < 0) {
          printf("There was an error while reading the buffer.\n");
          exit(-1);
      }
      else if (res == 0) {
          continue;
      }
      else {
          switch (i) {
            case 0:
                if (buf[i] == ua[i])
                    i++;
                break;
            case 1:
                if (buf[i] == ua[i])
                    i++;
                else
                    i--;
                break;
            case 2:
                if (buf[i] == ua[i])
                    i++;
                else
                    i--;
                break;
            case 3:
                if (buf[i] == ua[i])
                    i++;
                else
                    i--;
                break;
            case 4:
                if (buf[i] == ua[i])
                    STOP = TRUE;
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

  if ( tcsetattr(fd,TCSANOW,&oldtio) == -1) {
    perror("tcsetattr");
    exit(-1);
  }

  close(fd);
  return 0;
}
