#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <ctype.h>
#include <errno.h>
#include <time.h>
#include <strings.h>
#include <string.h>

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include "utilities.h"
#include "tracker.h"

#define LOOPBACK "127.0.0.1"
#define MAX_BUF 1024


int main(int argc, char **argv) {
    if (argc != 5) {
        printf("usage: requester -p <port> -o <file option>\n");
        exit(1);
    }

    // ------------------------------------------------------------------------

    char *portStr    = NULL;
    char *fileOption = NULL;

    int cmd;
    while ((cmd = getopt(argc, argv, "p:o:")) != -1) {
        switch(cmd) {
            case 'p': portStr    = optarg; break;
            case 'o': fileOption = optarg; break;
            case '?':
                if (optopt == 'p' || optopt == 'o')
                    fprintf(stderr, "Option -%c requires an argument.\n", optopt);
                else if (isprint(optopt))
                    fprintf(stderr, "Unknown option -%c.\n", optopt);
                else
                    fprintf(stderr, "Unknown option character '\\x%x'.\n", optopt);
                exit(EXIT_FAILURE);
            break;
            default: 
                printf("Unhandled argument: %d\n", cmd);
                exit(EXIT_FAILURE); 
        }
    }

    printf("Port: %s\n", portStr);
    printf("File: %s\n", fileOption);

    // ------------------------------------------------------------------------

    // TODO: write some code to traverse the lists and free memory
    struct file_info *files = parseTracker();
    printFileInfo(files); // DEBUG

    // ------------------------------------------------------------------------
    // Convert program args to values
    int requesterPort = atoi(portStr);

    // Validate the argument values
    if (requesterPort <= 1024 || requesterPort >= 65536)
        ferrorExit("Invalid requester port");

    // Create a socket
    int sock = socket(AF_INET, SOCK_DGRAM, 0);
    if (sock < 0) perrorExit("Socket error");
    else          printf("Created socket.\n");

    // Setup the sender address structure
    struct sockaddr_in senderAddr;
    bzero((char *)&senderAddr, sizeof(senderAddr));
    senderAddr.sin_family = AF_INET;
    senderAddr.sin_port = htons(requesterPort);
    //senderAddr.sin_addr.s_addr = htonl(INADDR_ANY);
    inet_aton(LOOPBACK, &senderAddr.sin_addr);

    // Connect to the sender
    int connectResult = connect(sock,
        (struct sockaddr *)&senderAddr, sizeof(senderAddr));
    if (connectResult < 0) perrorExit("Connect error");
    else                   printf("Connection established.\n");

    // Send a request for bytes from file specified at commandline
    size_t bytesSent = sendto(sock, fileOption, strlen(fileOption), 0,
        (struct sockaddr *)&senderAddr, sizeof(senderAddr));
    if (bytesSent < 0) perrorExit("Send error");
    else               printf("Sent %d bytes: %s\n", (int)bytesSent, fileOption);

    // Receive bytes back from the sender
    char buf[MAX_BUF]; 
    bzero(&buf, sizeof(buf));
    size_t bytesRecvd = recvfrom(sock, buf, MAX_BUF, 0, NULL, NULL);
    if (bytesRecvd < 0) perrorExit("Receive error");
    else                printf("Received %d bytes: %s\n", (int)bytesRecvd, buf);

    // Clear buffer if it will be used again
    //bzero(&buf, sizeof(buf));

    // Got what we came for, shut it down
    if (close(sock) < 0) perrorExit("Close error");
    else                 puts("Connection closed.\n");

    exit(EXIT_SUCCESS);
}

