#include "applicationLayer.h"

int currentFrame = 1;

int main(int argc, char const *argv[]) {
    if ((strcmp("/dev/ttyS0", argv[1])!=0) && (strcmp("/dev/ttyS1", argv[1])!=0)) {
        printf("Usage:\tnserial SerialPort\n\tex: nserial /dev/ttyS0\n");
        exit(1);
    }

    ApplicationLayer appLayer;

    setLinkLayer();

    if (argc == 2) {
        appLayer.status = RECEIVER;
        printf("Receiver OK.\n");
    }
    else if (argc == 3) {
        appLayer.status = TRANSMITTER;
        printf("Transmitter OK.\n");
    }
    else {
        printf("Number of arguments insufficient\n");
        exit(-1);
    }

    struct timeval start, stop;
    double secs = 0;
    gettimeofday(&start, NULL);

    if ((appLayer.fd = open(argv[1], O_RDWR | O_NOCTTY)) < 0) {
        perror(argv[1]);
        return -1;
    }

    if (llopen(appLayer.fd, appLayer.status) == -1)
        return -1;

    if (appLayer.status == TRANSMITTER){
        if (transmitter(appLayer.fd, argv[2]) == -1)
            return -1;
    }
    else if (appLayer.status == RECEIVER) {
        if (receiver(appLayer.fd) == -1)
            return -1;
    }

    if (appLayer.status == TRANSMITTER){
        if (llcloseTransmitter(appLayer.fd) == -1)
            return -1;
    }
    else if (appLayer.status == RECEIVER) {
        if (llcloseReceiver(appLayer.fd) == -1)
            return -1;
    }


    gettimeofday(&stop, NULL);
    secs = (double)(stop.tv_usec - start.tv_usec) / 1000000 + (double)(stop.tv_sec - start.tv_sec);
    printf("time taken %f\n",secs);

    return 0;
}


int receiver(int port) {
    unsigned char filedata[(linkLayer.maxFrameSize - PACKET_HEADER)*2];

    int fileSize;
    char * fileName = (char *) malloc(0);

    int size;
    if((size = llread(port, filedata)) == -1)
        return -1;

    getFileName(filedata, fileName);

    getFileSize(filedata, &fileSize);

    printf("Receiving '%s' with %d bytes\n", fileName, fileSize);

    mode_t mode = S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH;
    int flags = O_WRONLY | O_CREAT | O_TRUNC;
    int file;
    if((file = open(fileName, flags, mode)) == -1)
        return -1;

    int messageSize;
    do {
        if((messageSize = llread(port, filedata)) == -1)
            return -1;

        if(messageSize != -2 && messageSize != -3 && messageSize != -4 && messageSize > 0)
            if(writeFileData(filedata, file) == -1)
              return -1;
    }while(messageSize != -2);


    close(file);

    return 0;
}

int getFileName(unsigned char * data, char * fileName) {

    int lengthSize = (int) data[2];
    int lengthName = (int) data[4+lengthSize];
    int i;

    fileName = (char *) realloc(fileName, lengthName);

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

int writeFileData(unsigned char * data, int fd) {

    int l1 = (int) data[3];
    int l2 = (int) data[2];
    int k = 256*l2 + l1;

    int res;
    if((res = write(fd, &data[4], k)) == -1)
        return -1;

    return 0;
}


int transmitter(int port, const char *fileName) {
    int file;
    if ((file = open(fileName, O_RDONLY)) == -1) {
        printf("Program ended with error n. %d\n", errno);
        return -1;
    }

    long fileSize = lseek(file, (size_t)0, SEEK_END);

    close(file);


    unsigned char boundPacket[linkLayer.maxFrameSize - HEADER];
    int packetSize;
    packetSize = createBoundPacket(boundPacket, fileSize, fileName, START_FRAME);

    if (llwrite(port, boundPacket, &packetSize) == -1)
        return -1;

    if ((file = open(fileName, O_RDONLY)) == -1) {
        printf("Program ended with error n. %d\n", errno);
        return -1;
    }

    int messageSize;

    unsigned char message[linkLayer.maxFrameSize - HEADER];
    unsigned char infoPacket[linkLayer.maxFrameSize*2];
    do {
        messageSize = read(file, message, linkLayer.maxFrameSize - HEADER);
        packetSize = createInfoPacket(message, messageSize, infoPacket);


        if (llwrite(port, infoPacket, &packetSize) == -1)
            return -1;
    } while (messageSize == linkLayer.maxFrameSize - HEADER);

    close(file);

    packetSize = createBoundPacket(boundPacket, fileSize, fileName, END_FRAME);

    if (llwrite(port, boundPacket, &packetSize) == -1)
        return -1;

    return 0;
}

int createBoundPacket(unsigned char *bound, long fileSize, const char *fileName, unsigned char frame) {
    bound[0] = frame;

    bound[1] = T1;
    bound[2] = LONG_SIZE;
    bound[3] = (fileSize >> 24) & 0xFF;
    bound[4] = (fileSize >> 16) & 0xFF;
    bound[5] = (fileSize >> 8) & 0xFF;
    bound[6] = fileSize & 0xFF;

    bound[7] = T2;
    bound[8] = strlen(fileName);
    int currentPosition = 8;
    size_t i;
    for (i = 0; i < strlen(fileName); i++) {
        bound[++currentPosition] = fileName[i];
    }

    unsigned char bccFinal = frame;

    for (i = 1; i <= currentPosition; i++) {
        bccFinal ^= bound[i];
    }

    bound[++currentPosition] = bccFinal;

    return (currentPosition+1);
}

int createInfoPacket(unsigned char *message, int messageSize, unsigned char *infoFrame) {
    infoFrame[0] = INFO_FRAME;

    infoFrame[1] = currentFrame % BYTE_SIZE;
    infoFrame[2] = (messageSize >> 8) & 0xFF;
    infoFrame[3] = messageSize & 0xFF;

    unsigned char bccFinal = infoFrame[0]^infoFrame[1]^infoFrame[2]^infoFrame[3];

    int currentPosition = 4;
    size_t i;
    for (i = 0; i < messageSize; i++) {
        infoFrame[currentPosition++] = message[i];
        bccFinal ^= message[i];
    }

    infoFrame[currentPosition++] = bccFinal;

    currentFrame++;

    return currentPosition;
}
