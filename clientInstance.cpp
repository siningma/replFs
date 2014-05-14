/************************/
/* Your Name: Sining Ma */
/* Date: 05/10/2014     */
/* CS 244B	            */
/* Spring 2013	        */
/************************/

#include "clientInstance.h"

ClientInstance:: ClientInstance(int packetLoss, uint32_t nodeId, int numServers, unsigned int group): NetworkInstance(packetLoss, nodeId, group) {
	this->numServers = numServers;
}

void ClientInstance:: sendInitMessage() {
	InitMessage initMsg(this->nodeId, getMsgSeqNum());

	char buf[HEADER_SIZE];
	memset(buf, 0, HEADER_SIZE);
	initMsg.serialize(buf);
	
	if (isDropPacket(packetLoss)) {
		printf("Drop Message: ");
		initMsg.print();
	} else {
		printf("Send Message: ");
		initMsg.print();
		rfs_sendTo(buf, HEADER_SIZE);
	}
}

int ClientInstance:: procInitAckMessage(char *buf) {
	InitAckMessage initAckMessage;
	initAckMessage.deserialize(buf);

	printf("Recv Message: ");
	initAckMessage.print();

	if ((int)serverIds.size() == numServers)
		return 0;

	for (unsigned int i = 0; i < serverIds.size(); i++) {
		if (serverIds[i] == initAckMessage.nodeId)
			return 0;
	}
	serverIds.push_back(initAckMessage.nodeId);

	return 0;
}

void ClientInstance:: sendOpenFileMessage() {
	OpenFileMessage openFileMsg(this->nodeId, getMsgSeqNum());

	char buf[HEADER_SIZE];
	memset(buf, 0, HEADER_SIZE);
	initMsg.serialize(buf);
	
	if (isDropPacket(packetLoss)) {
		printf("Drop Message: ");
		initMsg.print();
	} else {
		printf("Send Message: ");
		initMsg.print();
		rfs_sendTo(buf, HEADER_SIZE);
	}
}

void ClientInstance:: sendInitMessage() {
	InitMessage initMsg(this->nodeId, getMsgSeqNum());

	char buf[HEADER_SIZE];
	memset(buf, 0, HEADER_SIZE);
	initMsg.serialize(buf);
	
	if (isDropPacket(packetLoss)) {
		printf("Drop Message: ");
		initMsg.print();
	} else {
		printf("Send Message: ");
		initMsg.print();
		rfs_sendTo(buf, HEADER_SIZE);
	}
}

void ClientInstance:: sendOpenFileMessage(uint32_t fileId, char* filename) {
	OpenFileMessage openFileMsg(this->nodeId, getMsgSeqNum(), fileId, filename);

	char buf[HEADER_SIZE + 4 + strlen(filename)];
	memset(buf, 0, HEADER_SIZE + 4 + strlen(filename));
	openFileMsg.serialize(buf);
	
	if (isDropPacket(packetLoss)) {
		printf("Drop Message: ");
		openFileMsg.print();
	} else {
		printf("Send Message: ");
		openFileMsg.print();
		rfs_sendTo(buf, HEADER_SIZE + 4 + strlen(filename));
	}
}

int ClientInstance:: procOpenFileAckMessage() {
	OpenFileAckMessage openFileAckMessage;
	openFileAckMessage.deserialize(buf);

	printf("Recv Message: ");
	openFileAckMessage.print();

	if (openFileAckMessage.fileDesc < 0)
		return -1;
	else
		return openFileAckMessage.fileDesc;
}
