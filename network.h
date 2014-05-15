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
#include <set>
#include <map>

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
#define SHORT_TIMEOUT 	2000
#define LONG_TIMEOUT 	4000

#define CLIENT_NODE 	0
#define SERVER_NODE 	1

typedef	struct sockaddr_in			Sockaddr;

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
		msgType = buf[0];
		reserved = 0;
		uint32_t msg_nodeId = 0;
		memcpy(&msg_nodeId, buf + 2, 4);
		nodeId = ntohl(msg_nodeId);
		uint32_t msg_seqNum = 0;
		memcpy(&msg_seqNum, buf + 6, 4);
		seqNum = ntohl(msg_seqNum);
	}

	virtual void print() {
		printf("MsgType: 0x%02x, NodeId: %010u, seqNum: %u", msgType, nodeId, seqNum);
	}
};

class InitMessage: public Message {
public:
	InitMessage() {}
	InitMessage(uint32_t nodeId, uint32_t seqNum): Message(INIT, nodeId, seqNum) {}
	
	void serialize(char *buf) { Message::serialize(buf); }
	void deserialize(char *buf) { Message::deserialize(buf); }
	void print() { Message::print(); printf("\n"); }
};

class InitAckMessage: public Message {
public:
	InitAckMessage() {}
	InitAckMessage(uint32_t nodeId, uint32_t seqNum): Message(INITACK, nodeId, seqNum) {}

	void serialize(char *buf) { Message::serialize(buf); }
	void deserialize(char *buf) { Message::deserialize(buf); }
	void print() { Message::print(); printf("\n"); }
};

class OpenFileMessage: public Message {
public:
	int fileId;
	char filename[128];

	OpenFileMessage() {}
	OpenFileMessage(uint32_t nodeId, uint32_t seqNum, int fileId, char* filename): Message(OPENFILE, nodeId, seqNum) {
		this->fileId = fileId;
		memset(this->filename, 0, 128);
		memcpy(this->filename, filename, strlen(filename));
	}

	void serialize(char *buf) {
		Message::serialize(buf);
		uint32_t msg_fileId = htonl(fileId);
		memcpy(buf + HEADER_SIZE, &msg_fileId, 4);
		memcpy(buf + HEADER_SIZE + 4, filename, strlen(filename));
	}

	void deserialize(char *buf) { 
		Message::deserialize(buf);
		uint32_t msg_fileId = 0;
		memcpy(&msg_fileId, buf + HEADER_SIZE, 4);
		fileId = ntohl(msg_fileId);
		memcpy(filename, buf + HEADER_SIZE + 4, 128);
	}

	void print() { 
		Message::print();
		printf("FileId: %u, Filename: %s\n", fileId, filename);
	}
};

class OpenFileAckMessage: public Message {
public:
	int fileDesc;

	OpenFileAckMessage() {}
	OpenFileAckMessage(uint32_t nodeId, uint32_t seqNum, int fileDesc): Message(OPENFILEACK, nodeId, seqNum) {
		this->fileDesc = fileDesc;
	}

	void serialize(char *buf) {
		Message::serialize(buf);
		int msg_fileDesc = htonl(fileDesc);
		memcpy(buf + HEADER_SIZE, &msg_fileDesc, 4);
	}

	void deserialize(char *buf) { 
		Message::deserialize(buf);
		int msg_fileDesc = 0;
		memcpy(&msg_fileDesc, buf + HEADER_SIZE, 4);
		fileDesc = ntohl(msg_fileDesc);
	}

	void print() { 
		Message::print();
		printf("FileDesc: %d\n", fileDesc);
	}
};

class WriteBlockMessage: public Message {
public:
	int fileId;
	uint32_t updateId;
	int byteOffset;
	int blockSize;
	char *buffer;

	WriteBlockMessage() {}
	WriteBlockMessage(uint32_t nodeId, uint32_t seqNum, int fileId, 
		uint32_t updateId, int byteOffset, int blockSize, char *buffer): Message(WRITEBLOCK, nodeId, seqNum) {
		this->nodeId = nodeId;
		this->seqNum = seqNum;
		this->fileId = fileId;
		this->updateId = updateId;
		this->byteOffset = byteOffset;
		this->blockSize = blockSize;
		this->buffer = buffer;
	}

	void serialize(char *buf) {
		Message::serialize(buf);
		int msg_fileId = htonl(fileId);
		memcpy(buf + HEADER_SIZE, &msg_fileId, 4);
		uint32_t msg_updateId = htonl(updateId);
		memcpy(buf + HEADER_SIZE + 4, &msg_updateId, 4);
		int msg_byteOffset = htonl(byteOffset);
		memcpy(buf + HEADER_SIZE + 8, &msg_byteOffset, 4);
		int msg_blockSize = htonl(blockSize);
		memcpy(buf + HEADER_SIZE + 12, &msg_blockSize, 4);
		memcpy(buf + HEADER_SIZE + 16, buffer, blockSize);
	}

	void deserialize(char *buf) { 
		Message::deserialize(buf);
		int msg_fileId = 0;
		memcpy(&msg_fileId, buf + HEADER_SIZE, 4);
		fileId = ntohl(msg_fileId);
		int msg_updateId = 0;
		memcpy(&msg_updateId, buf + HEADER_SIZE + 4, 4);
		updateId = ntohl(msg_updateId);
		int msg_byteOffset = 0;
		memcpy(&msg_byteOffset, buf + HEADER_SIZE + 8, 4);
		byteOffset = ntohl(msg_byteOffset);
		int msg_blockSize = 0;
		memcpy(&msg_blockSize, buf + HEADER_SIZE + 12, 4);
		blockSize = ntohl(msg_blockSize);
		memcpy(buffer, buf + HEADER_SIZE + 16, blockSize);
	}

	void print() { 
		Message::print();
		printf("fileId: %d, updateId: %u, byteOffset: %d, blockSize: %d\n", fileId, updateId, byteOffset, blockSize);
	}
};

class CloseMessage: public Message {
public:
	int fileId;

	CloseMessage() {}
	CloseMessage(uint32_t nodeId, uint32_t seqNum, int fileId): Message(CLOSE, nodeId, seqNum){
		this->fileId = fileId;
	}

	void serialize(char *buf) {
		Message::serialize(buf);
		uint32_t msg_fileId = htonl(fileId);
		memcpy(buf + HEADER_SIZE, &msg_fileId, 4);
	}

	void deserialize(char *buf) { 
		Message::deserialize(buf);
		uint32_t msg_fileId = 0;
		memcpy(&msg_fileId, buf + 2, 4);
		fileId = ntohl(msg_fileId);
	}

	void print() { 
		Message::print();
		printf("FileId: %u\n", fileId);
	}
};

class CloseAckMessage: public Message {
public:
	int fileDesc;

	CloseAckMessage() {}
	CloseAckMessage(uint32_t nodeId, uint32_t seqNum, int fileDesc): Message(CLOSEACK, nodeId, seqNum) {
		this->fileDesc = fileDesc;
	}

	void serialize(char *buf) {
		Message::serialize(buf);
		int msg_fileDesc = htonl(fileDesc);
		memcpy(buf + HEADER_SIZE, &msg_fileDesc, 4);
	}

	void deserialize(char *buf) { 
		Message::deserialize(buf);
		int msg_fileDesc = 0;
		memcpy(&msg_fileDesc, buf + 2, 4);
		fileDesc = ntohl(msg_fileDesc);
	}

	void print() { 
		Message::print();
		printf("FileDesc: %d\n", fileDesc);
	}
};

class NetworkInstance {
public:
	int packetLoss;
	int sockfd;
	uint32_t nodeId;
	Sockaddr groupAddr;
	uint32_t msgSeqNum;

	int nodeType;

	NetworkInstance(int packetLoss, uint32_t nodeId);

	bool isMessageSentByMe(char *buf);
	void rfs_NetInit(unsigned short port);

	ssize_t rfs_SendTo(char *buf, int length);
	bool rfs_IsRecvPacket();
	ssize_t rfs_RecvFrom(char* buf, int length);

	void sendMessage(Message *msg, int len);
};

typedef struct _Update {
	int byteOffset;
	int blockSize;
	char *buffer;
} Update;

void RFSError(const char *s);
void getCurrentTime(struct timeval *tv);
bool isTimeOut(struct timeval *curr, struct timeval *last, uint32_t millisecond);
bool isDropPacket(int packetLoss);
uint32_t getNextNum(uint32_t num);

#endif