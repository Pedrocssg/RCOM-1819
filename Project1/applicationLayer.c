#include "applicationLayer.h"

int main(int argc, char const *argv[]) {
    if ((strcmp("/dev/ttyS0", argv[1])!=0) && (strcmp("/dev/ttyS1", argv[1])!=0)) {
        printf("Usage:\tnserial SerialPort\n\tex: nserial /dev/ttyS0\n");
        exit(1);
    }

    ApplicationLayer appLayer;

    if (argc == 2) {
        appLayer.status = RECEIVER;
        printf("Receiver ready.\n");
    }
    else if (argc == 3) {
        appLayer.status = TRANSMITTER;
        printf("Transmitter ready.\n");
    }
    else {
        printf("Number of arguments insufficient\n");
        exit(-1);
    }

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

    return 0;
}


int receiver(int port) {
    unsigned char filedata[MAX_INFO_SIZE*2];

    int fileSize;
    char fileName[BYTE_SIZE*2];

    int size;
    if((size = llread(port, filedata)) == -1)
        return -1;

    getFileName(filedata, fileName);

    getFileSize(filedata, &fileSize);

    printf("File name: %s\n", fileName);
    printf("File size: %d\n", fileSize);

    mode_t mode = S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH;
    int flags = O_WRONLY | O_CREAT | O_TRUNC;
    int file;
    if((file = open(fileName, flags, mode)) == -1)
        return -1;

    int messageSize;
    do {
        if((messageSize = llread(port, filedata)) == -1)
            return -1;

        if(messageSize != -2)
            writeFileData(filedata, file);
    }while(messageSize != -2);

    close(file);

    return 0;
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

int writeFileData(unsigned char * data, int fd) {

    int l1 = (int) data[3];
    int l2 = (int) data[2];
    int k = 256*l2 + l1;

    if((write(fd, &data[4], k)) == 0)
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

    unsigned char boundFrame[BYTE_SIZE];
    int frameSize;
    frameSize = createBoundFrame(boundFrame, fileSize, fileName, START_FRAME);

    if (llwrite(port, boundFrame, frameSize) == -1)
        return -1;

    if ((file = open(fileName, O_RDONLY)) == -1) {
        printf("Program ended with error n. %d\n", errno);
        return -1;
    }

    int messageSize;
    unsigned char message[MAX_MSG_SIZE];
    unsigned char infoFrame[MAX_FRAME_SIZE*2];
    do {
        messageSize = read(file, message, MAX_MSG_SIZE);
        frameSize = createInfoFrame(message, messageSize, infoFrame);

        if (llwrite(port, infoFrame, frameSize) == -1)
            return -1;
    } while (messageSize == MAX_MSG_SIZE);

    close(file);

    frameSize = createBoundFrame(boundFrame, fileSize, fileName, END_FRAME);

    if (llwrite(port, boundFrame, frameSize) == -1)
        return -1;

    return 0;
}
