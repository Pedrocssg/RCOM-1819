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
int createBoundPacket(unsigned char *bound, long fileSize, const char *fileName, unsigned char frame);
int createInfoPacket(unsigned char *message, int messageSize, unsigned char *infoFrame);
