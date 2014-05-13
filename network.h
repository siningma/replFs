/************************/
/* Your Name: Sining Ma */
/* Date: 05/10/2014     */
/* CS 244B	            */
/* Spring 2013	        */
/************************/

#ifndef NETWORK_H
#define NETWORK_H

#include <unistd.h>
#include <errno.h>
#include <netdb.h>
#include <time.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#ifndef	TRUE
#define	TRUE		1
#define	FALSE		0
#endif	/* TRUE */

#define RFSGROUP       0xe0010101

#define INIT 			0xC0
#define INITACK 		0xC1
#define OPENFILE 		0xC2
#define OPENFILEACK 	0xC3
#define WRITEBLOCK 		0xC4
#define VOTE 			0xC5
#define VOTEACK 		0xC6
#define COMMIT 			0xC7
#define COMMITACK 		0xC8
#define ABORT 			0xC9
#define ABORTACK 		0xCA
#define CLOSE 			0xCB
#define CLOSEACK 		0xCC

#define HEADER_SIZE 	10	

#define SEND_MSG_INTERVAL 	200

static Sockaddr         groupAddr;
static uint32_t msgSeqNum = 0;

class NetworkInstance {
public:
	int packetLoss;
	int socket;
	uint32_t nodeId;

	NetworkInstance(int packetLoss, int socket, uint32_t nodeId);
};

class Message {
public:
	unsigned char msgType;
	unsigned char reserved;
	uint32_t nodeId;
	uint32_t seqNum;

	Message() {}

	Message(unsigned char msgType, uint32_t nodeId, uint32_t seqNum) {
		this->mesType = msgType;
		this->nodeId = nodeId;
		this->seqNum = seqNum;
	}

	virtual void serialize(char *buf) {
		buf[0] = msgType;
		memcpy(buf + 2, htonl(nodeId), 4);
		memcpy(buf + 6, htonl(seqNum), 4);
	}

	virtual void deserialize(char *buf) {
		this->msgType = buf[0];
		this->reserved = 0;
		uint32_t nodeId = 0;
		memcpy(nodeId, buf + 2, 4);
		this->nodeId = ntohl(nodeId);
		uint32_t seqNum = 0;
		memcpy(seqNum, buf + 6, 4);
		this->seqNum = ntohl(seqNum);
	}

	virtual void print() {
		printf("Message Type: 0x%2x, NodeId: %u, seqNum: %u", this->mesType, this->nodeId, this->seqNum);
	}
};

class InitMessage: public Message {
public:
	InitMessage() {}
	InitMessage(uint32_t nodeId, uint32_t seqNum): Message(INIT, nodeId, seqNum) {}
	
	void serialize(char *buf) { Message::serialize(buf); }
	void deserialize(char *buf) { Message::deserialize(buf); }
	void print() { Message::print(); }
};

class InitAckMessage: public Message {
public:
	InitAckMessage() {}
	InitAckMessage(uint32_t nodeId, uint32_t seqNum): Message(INITACK, nodeId, seqNum) {}

	void serialize(char *buf) { Message::serialize(buf); }
	void deserialize(char *buf) { Message::deserialize(buf); }
	void print() { Message::print(); }
};

void RFSError(char *s);
void getCurrentTime(struct timeval *tv);
uint32_t getMsgSeqNum();
bool isTimeOut(struct timeval *curr, struct timeval *last, uint32_t millisecond);
bool isDrop(int packetLoss);

int rfs_netInit(unsigned short port);
ssize_t rfs_sendTo(int socket, char *buf, int length);
ssize_t rfs_recvFrom(int socket, char* buf, int length);

#endif