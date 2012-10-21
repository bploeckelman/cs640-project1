#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <ctype.h>
#include <errno.h>
#include <time.h>
#include <strings.h>
#include <string.h>
#include <assert.h>

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>

#include "utilities.h"
#include "tracker.h"
#include "packet.h"


int main(int argc, char **argv) {
    // ------------------------------------------------------------------------
    // Handle commandline arguments
    if (argc != 5) {
        printf("usage: requester -p <port> -o <file option>\n");
        exit(1);
    }

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

    // DEBUG
    printf("Port: %s\n", portStr);
    printf("File: %s\n", fileOption);

    // Convert program args to values
    int requesterPort = atoi(portStr);

    // Validate the argument values
    if (requesterPort <= 1024 || requesterPort >= 65536)
        ferrorExit("Invalid requester port");

    // ------------------------------------------------------------------------

    // Parse the tracker file for parts corresponding to the specified file
    struct file_info *fileParts = parseTracker(fileOption);
    assert(fileParts != NULL && "Invalid file_info struct");

    // ------------------------------------------------------------------------
    // TODO: this will eventually be moved into the send/recv loop
    // Setup sender address info 
    struct addrinfo hints;
    bzero(&hints, sizeof(struct addrinfo));
    hints.ai_family   = AF_INET;
    hints.ai_socktype = SOCK_DGRAM;
    hints.ai_flags    = 0;

    // Convert the sender's port # to a string
    struct file_part *part = fileParts->parts;
    char senderPortStr[6] = "\0\0\0\0\0\0";
    sprintf(senderPortStr, "%d", part->sender_port);

    // Get the sender's address info
    struct addrinfo *senderinfo, *p;
    int errcode = getaddrinfo(part->sender_hostname, senderPortStr, &hints, &senderinfo);
    if (errcode != 0) {
        fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(errcode));
        exit(EXIT_FAILURE);
    }

    // Loop through all the results of getaddrinfo and try to create a socket
    int sockfd;
    for(p = senderinfo; p != NULL; p = p->ai_next) {
        sockfd = socket(p->ai_family, p->ai_socktype, p->ai_protocol);
        if (sockfd == -1) {
            perror("Socket error");
            continue;
        }

        break;
    }
    if (p == NULL) perrorExit("Socket creation failed");
    else           printNameInfo(p);
    // ------------------------------------------------------------------------

    // Start sending and receiving packets
    struct packet *pkt = NULL;
    time_t t = time(NULL);

    for (;;) {
        // For now only send a test request packet every 2 seconds
        if (difftime(time(NULL), t) > 2) {
            t = time(NULL);
        } else continue;

        // Construct a request packet
        pkt = malloc(sizeof(struct packet));
        bzero(pkt, sizeof(struct packet));
        pkt->type = 'R';
        pkt->seq  = 0; // 0 for request pkts, otherwise: part->id;
        pkt->len  = strlen(fileOption) + 1;
        strcpy(pkt->payload, fileOption);

        // Send the serialized request packet
        size_t bytesSent = sendto(sockfd, serializePacket(pkt),
            sizeof(struct packet), 0, p->ai_addr, p->ai_addrlen);
        if (bytesSent == -1)
            perrorExit("Send error");
        else {
            printf("[Sent %lu payload bytes]\n", pkt->len);
            printf("  payload: \"%s\"\n", pkt->payload);
            printf("Requester waiting for response...\n");
        }

        // Receive a response message 
        void *msg = malloc(sizeof(struct packet));
        size_t bytesRecvd = recv(sockfd, msg, sizeof(struct packet), 0);
        if (bytesRecvd == -1) perrorExit("Receive error");

        // Deserialize the response message into a packet
        pkt = malloc(sizeof(struct packet));
        bzero(pkt, sizeof(struct packet));
        deserializePacket(msg, pkt);

        // TODO: print some statistics about the packet
        printf("[Received packet @ %llu ms]", getTimeMS());
        printPacketInfo(pkt, (struct sockaddr_storage *)p->ai_addr);

        // Cleanup packets
        free(msg);
        free(pkt);
   }

    // Got what we came for, shut it down
    if (close(sockfd) == -1) perrorExit("Close error");
    else                     puts("Connection closed.\n");

    // Cleanup address and file info data 
    freeaddrinfo(senderinfo);
    freeFileInfo(fileParts);

    // All done!
    exit(EXIT_SUCCESS);
}

