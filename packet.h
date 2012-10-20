#ifndef _PACKET_H_
#define _PACKET_H_

#define MAX_PAYLOAD 5000


struct packet {
    char type;
    unsigned long seq;
    unsigned long len;
    char payload[MAX_PAYLOAD];
} __attribute__((packed));

void *serializePacket(struct packet *pkt);
void deserializePacket(void *msg, struct packet *pkt);

void sendPacket(int sockfd, struct packet *pkt);
void recvPacket(int sockfd, struct packet *pkt);

#endif

