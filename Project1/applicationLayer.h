#include <errno.h>
#include "linkLayer.h"


typedef struct {
    int fd;
    int status;
} ApplicationLayer;


int receiver(int port);
int getFileName(unsigned char * data, char * fileName);
int getFileSize(unsigned char * data, int * fileSize);
int writeFileData(unsigned char * data, int fd);

int transmitter(int port, const char *fileName);
