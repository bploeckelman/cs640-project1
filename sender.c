#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <ctype.h>
#include <errno.h>
#include <time.h>
#include <fcntl.h>
#include <strings.h>
#include <string.h>

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include "utilities.h"

#define LOOPBACK "127.0.0.1"
#define MAX_BUF 1024


int main(int argc, char **argv) {
    if (argc < 11) {
        printf("usage: sender -p <port> -g <requester port> -r <rate> -q <seq_no> -l <length>\n");
        exit(1);
    }

    // ------------------------------------------------------------------------

    char *portStr    = NULL;
    char *reqPortStr = NULL;
    char *rateStr    = NULL;
    char *seqNumStr  = NULL;
    char *lenStr     = NULL;

    int cmd;
    while ((cmd = getopt(argc, argv, "p:g:r:q:l:")) != -1) {
        switch(cmd) {
            case 'p': portStr    = optarg; break;
            case 'g': reqPortStr = optarg; break;
            case 'r': rateStr    = optarg; break;
            case 'q': seqNumStr  = optarg; break;
            case 'l': lenStr     = optarg; break;
            case '?':
                if (optopt == 'p' || optopt == 'g' || optopt == 'r'
                 || optopt == 'q' || optopt == 'l')
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

    printf("Port           : %s\n", portStr);
    printf("Requester Port : %s\n", reqPortStr);
    printf("Rate           : %s\n", rateStr);
    printf("Sequence #     : %s\n", seqNumStr);
    printf("Length         : %s\n", lenStr);

    // ------------------------------------------------------------------------

    // Convert program args to values
    int senderPort    = atoi(portStr);
    int requesterPort = atoi(reqPortStr);
//  int sendRate      = atoi(rateStr);
//  int sequenceNum   = atoi(seqNumStr);
//  int payloadLen    = atoi(lenStr);

    // Validate the argument values
    if (senderPort <= 1024 || senderPort >= 65536)
        ferrorExit("Invalid sender port");
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
    senderAddr.sin_port = htons(senderPort);
    //senderAddr.sin_addr.s_addr = htonl(INADDR_ANY);
    inet_aton(LOOPBACK, &senderAddr.sin_addr);

    // Bind the socket to the sender address
    int bindResult = bind(sock,
        (struct sockaddr *)&senderAddr, sizeof(senderAddr));
    if (bindResult < 0) perrorExit("Bind error");
    else                printf("Bound socket to sender address.\n");

    // Setup the requester address structure
    struct sockaddr_in requesterAddr;
    bzero((char *)&requesterAddr, sizeof(requesterAddr));
    socklen_t len = sizeof(requesterAddr);

    // Setup a byte buffer
    char buf[MAX_BUF];
    bzero(&buf, sizeof(buf));

    // Get the filename to open from the requester
    recvfrom(sock, buf, MAX_BUF, 0,
        (struct sockaddr *)&requesterAddr, &len);

    // Open the requested file and read in the bytes
    int file = open(buf, O_RDONLY);
    if (file < 0) perror("Open error");
    else          printf("File opened: %s\n", buf);
    read(file, buf, MAX_BUF);

    // Send the bytes back to the requester
    sendto(sock, buf, strlen(buf), 0,
        (struct sockaddr *)&requesterAddr, sizeof(requesterAddr));

    // Print the bytes that were just sent
    printf("Data sent to requester: %s\n", buf);
    //bzero(&buf, sizeof(buf)); // clear if using buffer again

    // Got what we came for, shut it down
    close(sock);
    puts("Connection closed.\n");

    exit(EXIT_SUCCESS);
}

