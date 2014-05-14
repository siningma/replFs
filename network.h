/************************/
/* Your Name: Sining Ma */
/* Date: 05/10/2014     */
/* CS 244B	            */
/* Spring 2013	        */
/************************/

#ifndef NETWORK_H
#define NETWORK_H

#include <sys/types.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <sys/stat.h>
#include <sys/select.h>
#include <sys/poll.h>

#include <netinet/in.h>
#include <arpa/inet.h>
#include <time.h>

#include <stdint.h>
#include <unistd.h>
#include <errno.h>
#include <netdb.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <vector>

#ifndef	TRUE
#define	TRUE		1
#define	FALSE		0
#endif	/* TRUE */

#define RFS_GROUP       0xe0010101

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

typedef	struct sockaddr_in			Sockaddr;

class NetworkInstance {
public:
	int packetLoss;
	int sockfd;
	uint32_t nodeId;
	Sockaddr groupAddr;
	uint32_t msgSeqNum;

	NetworkInstance(int packetLoss, uint32_t nodeId);

	uint32_t getMsgSeqNum();
	void rfs_netInit(unsigned short port);
	ssize_t rfs_sendTo(char *buf, int length);
	bool rfs_recvData(int pollTimeout);
	ssize_t rfs_recvFrom(char* buf, int length);
};

class Message {
public:
	unsigned char msgType;
	unsigned char reserved;
	uint32_t nodeId;
	uint32_t seqNum;

	Message() {}

	Message(unsigned char msgType, uint32_t nodeId, uint32_t seqNum) {
		this->msgType = msgType;
		this->nodeId = nodeId;
		this->seqNum = seqNum;
	}

	virtual void serialize(char *buf) {
		buf[0] = msgType;
		uint32_t msg_nodeId = htonl(nodeId);
		uint32_t msg_seqNum = htonl(seqNum);
		memcpy(buf + 2, &msg_nodeId, 4);
		memcpy(buf + 6, &msg_seqNum, 4);
	}

	virtual void deserialize(char *buf) {
		this->msgType = buf[0];
		this->reserved = 0;
		uint32_t msg_nodeId = 0;
		memcpy(&msg_nodeId, buf + 2, 4);
		this->nodeId = ntohl(msg_nodeId);
		uint32_t msg_seqNum = 0;
		memcpy(&msg_seqNum, buf + 6, 4);
		this->seqNum = ntohl(msg_seqNum);
	}

	virtual void print() {
		printf("Message Type: 0x%02x, NodeId: %10u, seqNum: %u\n", this->msgType, this->nodeId, this->seqNum);
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
bool isTimeOut(struct timeval *curr, struct timeval *last, uint32_t millisecond);
bool isDrop(int packetLoss);
bool isDropPacket(int packetLoss);

#endif