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
    // Setup requester address info 
    struct addrinfo rhints;
    bzero(&rhints, sizeof(struct addrinfo));
    rhints.ai_family   = AF_INET;
    rhints.ai_socktype = SOCK_DGRAM;
    rhints.ai_flags    = 0;

    // Get the requester's address info
    struct addrinfo *requesterinfo;
    int errcode = getaddrinfo(NULL, reqPortStr, &rhints, &requesterinfo);
    if (errcode != 0) {
        fprintf(stderr, "requester getaddrinfo: %s\n", gai_strerror(errcode));
        exit(EXIT_FAILURE);
    }

    // Loop through all the results of getaddrinfo and try to create a socket for requester
    int reqsockfd;
    struct addrinfo *rp;
    for(rp = requesterinfo; rp != NULL; rp = rp->ai_next) {
        reqsockfd = socket(rp->ai_family, rp->ai_socktype, rp->ai_protocol);
        if (reqsockfd == -1) {
            perror("Socket error");
            continue;
        }

        break;
    }
    if (rp == NULL) perrorExit("Request socket creation failed");
    else            { printf("Requester socket: "); printNameInfo(rp); }

    // ------------------------------------------------------------------------
    // Setup sender address info 
    struct addrinfo shints;
    bzero(&shints, sizeof(struct addrinfo));
    shints.ai_family   = AF_INET;
    shints.ai_socktype = SOCK_DGRAM;
    shints.ai_flags    = 0;

    // Get the sender's address info
    struct addrinfo *senderinfo;
    errcode = getaddrinfo(LOOPBACK, portStr, &shints, &senderinfo);
    if (errcode != 0) {
        fprintf(stderr, "sender getaddrinfo: %s\n", gai_strerror(errcode));
        exit(EXIT_FAILURE);
    }

    // Loop through all the results of getaddrinfo and try to create a socket for sender
    int sendsockfd;
    struct addrinfo *sp;
    for(sp = senderinfo; sp != NULL; sp = sp->ai_next) {
        // Try to create a new socket
        sendsockfd = socket(sp->ai_family, sp->ai_socktype, sp->ai_protocol);
        if (sendsockfd == -1) {
            perror("Socket error");
            continue;
        }

        // Try to bind the socket
        if (bind(sendsockfd, sp->ai_addr, sp->ai_addrlen) == -1) {
            perror("Bind error");
            close(sendsockfd);
            continue;
        }

        break;
    }
    if (sp == NULL) perrorExit("Send socket creation failed");
    else            { printf("Sender socket: "); printNameInfo(sp); }

    // ------------------------------------------------------------------------
    puts("Sender waiting for request packet...\n");

    struct sockaddr_storage requesterAddr;
    bzero(&requesterAddr, sizeof(struct sockaddr_storage));
    socklen_t len = sizeof(requesterAddr);

    // Receive and discard packets until a REQUEST packet arrives
    char *filename = NULL;
    for (;;) {
        void *msg = malloc(sizeof(struct packet));
        bzero(msg, sizeof(struct packet));

        // Receive a message
        size_t bytesRecvd = recvfrom(sendsockfd, msg, sizeof(struct packet), 0,
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
            printPacketInfo(pkt, (struct sockaddr_storage *)rp->ai_addr);

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
    // ------------------------------------------------------------------------

    // Open file for reading
    FILE *file = fopen(filename, "r");
    if (file == NULL) perrorExit("File open error");
    else              printf("Opened file \"%s\" for reading.\n", filename);

    struct packet *pkt;
    for (;;) {
        // Is file part finished?
        if (feof(file) != 0) {
            // Create END packet and send it
            // TODO: sometimes this packet isn't received 
            //       and the requester doesn't shutdown
            //       needs to be handled somehow... 
            //       FIN packet back from requester on receipt of END?
            pkt = malloc(sizeof(struct packet));
            bzero(pkt, sizeof(struct packet));
            pkt->type = 'E';
            pkt->seq  = 0;
            pkt->len  = 0;

            sendPacketTo(reqsockfd, pkt, (struct sockaddr *)rp->ai_addr);

            free(pkt);
            break;
        }

        // Create DATA packet
        pkt = malloc(sizeof(struct packet));
        bzero(pkt, sizeof(struct packet));
        pkt->type = 'D';
        pkt->seq  = sequenceNum;
        pkt->len  = payloadLen;

        // Chunk the next batch of file data into this packet
        char buf[payloadLen];
        fread(buf, 1, payloadLen, file); // TODO: check return value
        memcpy(pkt->payload, buf, sizeof(buf));

        // Send the DATA packet to the requester 
        sendPacketTo(reqsockfd, pkt, (struct sockaddr *)rp->ai_addr);

        // Cleanup packets
        free(pkt);

        // Update sequence number for next packet
        sequenceNum += payloadLen;
    }

    // Cleanup the file
    if (fclose(file) != 0) fprintf(stderr, "Failed to close file \"%s\"\n", filename);
    else                   printf("File \"%s\" closed.\n", filename);
    free(filename);


    // Got what we came for, shut it down
    if (close(reqsockfd) == -1) perrorExit("Close error");
    else                     puts("Connection closed.\n");
    if (close(sendsockfd) == -1) perrorExit("Close error");
    else                     puts("Connection closed.\n");

    // Cleanup address info data
    freeaddrinfo(senderinfo);
    freeaddrinfo(requesterinfo);

    // All done!
    exit(EXIT_SUCCESS);
}

