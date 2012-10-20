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

    // Convert program args to values
    int requesterPort = atoi(portStr);

    // Validate the argument values
    if (requesterPort <= 1024 || requesterPort >= 65536)
        ferrorExit("Invalid requester port");

    // ------------------------------------------------------------------------

    // Parse the tracker file for parts corresponding to the specified file
    struct file_info *fileParts = parseTracker(fileOption);
    assert(fileParts != NULL && "Invalid file_info struct");

    // Setup structs for getaddrinfo
    struct addrinfo hints;
    bzero(&hints, sizeof(struct addrinfo));
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_DGRAM;
    hints.ai_flags = 0;
    hints.ai_protocol = 0;

    struct file_part *part = fileParts->parts;
    char senderPortStr[6] = "\0\0\0\0\0\0";
    sprintf(senderPortStr, "%d", part->sender_port);

    struct addrinfo *result, *rp;
    int errcode = getaddrinfo(part->sender_hostname, senderPortStr, &hints, &result);
    if (errcode != 0) {
        fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(errcode));
        exit(EXIT_FAILURE);
    }

    // Try each address from getaddrinfo until connect
    int sockfd;
    for(rp = result; rp != NULL; rp = rp->ai_next) {
        sockfd = socket(rp->ai_family, rp->ai_socktype, rp->ai_protocol);
        if (sockfd == -1)
            continue;
        if (connect(sockfd, rp->ai_addr, rp->ai_addrlen) != -1)
            break; // connected!
        close(sockfd);
    }
    if (rp == NULL) ferrorExit("Connect error");
    freeaddrinfo(result);

    // Construct a packet
    struct packet *pkt = malloc(sizeof(struct packet));
    bzero(pkt, sizeof(struct packet));
    pkt->type = 'R';
    pkt->seq  = part->id;
    pkt->len  = strlen(fileOption) + 1;
    strcpy(pkt->payload, fileOption);

    // Send the packet
    size_t bytesSent = send(sockfd, serializePacket(pkt), sizeof(struct packet), 0);
    if (bytesSent == -1)
        perrorExit("Send error");
    else
        printf("Sent %lu payload bytes: \"%s\"\n", pkt->len, pkt->payload);
    free(pkt);

    // Receive a response
    void *msg = NULL;
    size_t bytesRecvd = recv(sockfd, msg, sizeof(struct packet), 0);
    if (bytesRecvd == -1) perrorExit("Receive error");

    // Deserialize the response
    pkt = malloc(sizeof(struct packet));
    bzero(pkt, sizeof(struct packet));
    deserializePacket(msg, pkt);
    printf("Received %lu payload bytes.\n", pkt->len);

    // Print the response packet values
    printf("Response Packet:\n");
    printf("  type = %c\n", pkt->type);
    printf("  seq  = %lu\n", pkt->seq);
    printf("  len  = %lu\n", pkt->len);
    printf("  data = %s\n", pkt->payload);

    // Cleanup
    free(msg);
    free(pkt);

    // Got what we came for, shut it down
    if (close(sockfd) == -1) perrorExit("Close error");
    else                     puts("Connection closed.\n");

    freeFileInfo(fileParts);
    exit(EXIT_SUCCESS);
}

