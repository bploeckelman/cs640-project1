#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#include <arpa/inet.h>

#include "packet.h"


void *serializePacket(struct packet *pkt) {
    if (pkt == NULL) {
        fprintf(stderr, "Serialize: invalid packet\n");
        return NULL;
    }

    struct packet *spkt = malloc(sizeof(struct packet));
    bzero(spkt, sizeof(struct packet));

    spkt->type = pkt->type;
    spkt->seq  = htonl(pkt->seq);
    spkt->len  = htonl(pkt->len);
    memcpy(spkt->payload, pkt->payload, MAX_PAYLOAD);

    return spkt;
}

void deserializePacket(void *msg, struct packet *pkt) {
    if (msg == NULL) {
        fprintf(stderr, "Deserialize: invalid message\n");
        return;
    }
    if (pkt == NULL) {
        fprintf(stderr, "Deserialize: invalid packet\n");
        return;
    }

    struct packet *p = (struct packet *)msg;
    pkt->type = p->type;
    pkt->seq  = ntohl(p->seq);
    pkt->len  = ntohl(p->len);
    memcpy(pkt->payload, p->payload, MAX_PAYLOAD);
}

void sendPacket(int sockfd, struct packet *pkt) {
    // TODO
}

void recvPacket(int sockfd, struct packet *pkt) {
    // TODO
}

