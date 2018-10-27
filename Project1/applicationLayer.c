#include "applicationLayer.h"

int main(int argc, char const *argv[]) {
    if ( (argc < 4) || ((strcmp("/dev/ttyS0", argv[1])!=0) && (strcmp("/dev/ttyS1", argv[1])!=0) )) {
        printf("Usage:\tnserial SerialPort\n\tex: nserial /dev/ttyS0\n");
        exit(1);
    }

    ApplicationLayer appLayer;

    signal(SIGALRM, atende);

    const char *fileName = argv[2];
    int file;
    if ((file = open(fileName, O_RDONLY)) == -1) {
        printf("Program ended with error n. %d\n", errno);
        return -1;
    }

    long fileSize = lseek(file, (size_t)0, SEEK_END);

    close(file);

    if ((appLayer.fd = open(argv[1], O_RDWR | O_NOCTTY)) < 0) {
        perror(argv[1]);
        return -1;
    }

    if (llopen(&appLayer) == -1)
        return -1;

    unsigned char boundFrame[255];
    int frameSize;
    frameSize = createBoundFrame(boundFrame, fileSize, fileName, START_FRAME);

    if (llwrite(appLayer.fd, boundFrame, frameSize) == -1)
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

        if (llwrite(appLayer.fd, infoFrame, frameSize) == -1)
            return -1;
    } while (messageSize == MAX_MSG_SIZE);

    close(file);

    frameSize = createBoundFrame(boundFrame, fileSize, fileName, END_FRAME);

    if (llwrite(appLayer.fd, boundFrame, frameSize) == -1)
        return -1;

    if (llclose(&appLayer) == -1)
        return -1;

    return 0;
}
