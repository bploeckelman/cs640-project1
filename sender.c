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
#include <netdb.h>

#include "utilities.h"
#include "packet.h"


int main(int argc, char **argv) {
    // ------------------------------------------------------------------------
    // Handle commandline arguments
    if (argc < 11) {
        printf("usage: sender -p <port> -g <requester port> ");
        printf("-r <rate> -q <seq_no> -l <length>\n");
        exit(1);
    }

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

    // Convert program args to values
    int senderPort    = atoi(portStr);
    int requesterPort = atoi(reqPortStr);
//  int sendRate      = atoi(rateStr);
    int sequenceNum   = atoi(seqNumStr);
    int payloadLen    = atoi(lenStr);

    // Validate the argument values
    if (senderPort <= 1024 || senderPort >= 65536)
        ferrorExit("Invalid sender port");
    if (requesterPort <= 1024 || requesterPort >= 65536)
        ferrorExit("Invalid requester port");
    puts("");

    // ------------------------------------------------------------------------

    // Setup sender address info
    struct addrinfo hints;
    bzero(&hints, sizeof(struct addrinfo));
    hints.ai_family   = AF_INET;
    hints.ai_socktype = SOCK_DGRAM;
    hints.ai_flags    = 0;

    // Get the sender's address info
    struct addrinfo *senderinfo, *p;
    int errcode = getaddrinfo(NULL, portStr, &hints, &senderinfo);
    if (errcode != 0) {
        fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(errcode));
        exit(EXIT_FAILURE);
    }

    // Loop through all the getaddrinfo results and bind the first usable one
    int sockfd;
    for(p = senderinfo; p != NULL; p = p->ai_next) {
        // Try to create a socket
        sockfd = socket(p->ai_family, p->ai_socktype, p->ai_protocol);
        if (sockfd == -1) {
            perror("Socket error");
            continue;
        }

        // Try to bind the socket
        if (bind(sockfd, p->ai_addr, p->ai_addrlen) == -1) {
            perror("Bind error");
            close(sockfd);
            continue;
        }

        break;
    }
    if (p == NULL) perrorExit("Bind failed"); 
    else           printNameInfo(p);

    // ------------------------------------------------------------------------
    puts("Sender waiting for request packet...\n");

    // Setup the requester address structure
    struct sockaddr_storage requesterAddr;
    bzero(&requesterAddr, sizeof(requesterAddr));
    socklen_t len = sizeof(struct sockaddr_storage);

    // Receive and discard packets until a REQUEST packet arrives
    char *filename = NULL;
    for (;;) {
        void *msg = malloc(sizeof(struct packet));
        bzero(msg, sizeof(struct packet));

        // Receive a message
        size_t bytesRecvd = recvfrom(sockfd, msg, sizeof(struct packet), 0,
            (struct sockaddr *)&requesterAddr, &len);
        if (bytesRecvd == -1) {
            perror("Recvfrom error");
            fprintf(stderr, "Failed/incomplete receive: ignoring\n");
            continue;
        }

        // Deserialize the message into a packet 
        struct packet *pkt = malloc(sizeof(struct packet));
        bzero(pkt, sizeof(struct packet));
        deserializePacket(msg, pkt);

        // Check for REQUEST packet
        if (pkt->type == 'R') {
            // Print some statistics for the recvd packet
            printf("<- [Received REQUEST]: ");
            printPacketInfo(pkt, &requesterAddr);

            // Grab a copy of the filename
            filename = strdup(pkt->payload);

            // Cleanup packets
            free(pkt);
            free(msg);
            break;
        }

        // Cleanup packets
        free(pkt);
        free(msg);
    }

    // ------------------------------------------------------------------------
    // Got REQUEST packet, start sending DATA packets

    // Note: this is just for testing END packet handling
    int numPacketsToEcho = 10;

    // TODO: open file for reading
    FILE *file = fopen(filename, "r");
    if (file == NULL) perrorExit("File open error");
    else              printf("Opened file \"%s\" for reading.\n", filename);

    for (;;) {
        // Create DATA packet
        struct packet *pkt = malloc(sizeof(struct packet));
        bzero(pkt, sizeof(struct packet));
        pkt->type = 'D';
        pkt->seq  = sequenceNum;
        pkt->len  = payloadLen;

        // TODO: chunk file into packet payloads
        const char *testStr = "DATA-foobar\0";
        memcpy(pkt->payload, testStr, sizeof(testStr));

        // Update sequence number for next packet
        sequenceNum += payloadLen;

        // Send the DATA packet to the requester 
        size_t bytesSent = sendto(sockfd, serializePacket(pkt),
                                  sizeof(struct packet), 0,
                                  (struct sockaddr *)&requesterAddr,
                                  sizeof(struct sockaddr));
        if (bytesSent == -1) {
            perror("Sendto error");
            fprintf(stderr, "Error sending response\n");
        } else {
            printf("-> [Sent DATA packet] ");
            printPacketInfo(pkt, &requesterAddr);
        }

        // Cleanup packets
        free(pkt);
        //free(msg);

        // TODO: Test of END packet 
        if (--numPacketsToEcho <= 0) {
            // Create END packet and send it
            pkt = malloc(sizeof(struct packet));
            bzero(pkt, sizeof(struct packet));
            pkt->type = 'E';
            pkt->seq  = 0;
            pkt->len  = 0;
            bytesSent = sendto(sockfd, serializePacket(pkt),
                               sizeof(struct packet), 0,
                               (struct sockaddr *)&requesterAddr,
                               sizeof(struct sockaddr));
            if (bytesSent == -1) {
                perror("Sendto error");
                fprintf(stderr, "Error sending response\n");
            } else {
                puts("-> *** [Sent END packet] ***");
            }

            free(pkt);
            break;
        }
    }

    // Cleanup the file
    if (fclose(file) != 0) fprintf(stderr, "Failed to close file \"%s\"\n", filename);
    else                   printf("File \"%s\" closed.\n", filename);
    free(filename);


    // Got what we came for, shut it down
    if (close(sockfd) == -1) perrorExit("Close error");
    else                     puts("Connection closed.\n");

    // Cleanup address info data
    freeaddrinfo(senderinfo);

    // All done!
    exit(EXIT_SUCCESS);
}

