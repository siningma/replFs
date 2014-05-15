/************************/
/* Your Name: Sining Ma */
/* Date: 05/10/2014     */
/* CS 244B	            */
/* Spring 2013	        */
/************************/

#include "clientInstance.h"

ClientInstance:: ClientInstance(int packetLoss, uint32_t nodeId, int numServers): NetworkInstance(packetLoss, nodeId) {
	this->numServers = numServers;
	this->nodeType = CLIENT_NODE;
	this->updateId = 0;
}

void ClientInstance:: sendInitMessage() {
	InitMessage initMsg(nodeId, getMsgSeqNum());
	
	dropOrSendMessage(&initMsg, HEADER_SIZE);
}

int ClientInstance:: procInitAckMessage(char *buf) {
	InitAckMessage initAckMessage;
	initAckMessage.deserialize(buf);

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

void ClientInstance:: sendOpenFileMessage(int fileId, char* filename) {
	OpenFileMessage openFileMsg(nodeId, getMsgSeqNum(), fileId, filename);
	
	dropOrSendMessage(&openFileMsg, HEADER_SIZE + 4 + strlen(filename));
}

int ClientInstance:: procOpenFileAckMessage(char *buf) {
	OpenFileAckMessage openFileAckMessage;
	openFileAckMessage.deserialize(buf);

	openFileAckMessage.print();

	if (openFileAckMessage.fileDesc < 0)
		return -1;
	else
		return openFileAckMessage.fileDesc;
}

void ClientInstance:: sendWriteBlockMessage(int fileId, uint32_t updateId, int byteOffset, int blockSize, char *buffer) {
	WriteBlockMessage writeBlockMsg(nodeId, getMsgSeqNum(), fileId, updateId, byteOffset, blockSize, buffer);

	Update update;
	update.byteOffset = byteOffset;
	update.blockSize = blockSize;
	update.buffer = buffer;
	updateMap.insert(std::make_pair(updateId, update));
	dropOrSendMessage(&writeBlockMsg, HEADER_SIZE + 16 + blockSize);
}

void ClientInstance:: sendCloseMessage(int fileId) {
	CloseMessage closeMsg(nodeId, getMsgSeqNum(), fileId);
	
	dropOrSendMessage(&closeMsg, HEADER_SIZE + 4);
}

int ClientInstance:: procCloseAckMessage(char *buf) {
	CloseAckMessage closeAckMessage;
	closeAckMessage.deserialize(buf);

	closeAckMessage.print();

	return closeAckMessage.fileDesc;
}

uint32_t ClientInstance:: getUpdateId() {
	if (updateId == (uint32_t)~0) {
		updateId = 0;
		return (uint32_t)~0;
	} else {
		return updateId++;
	}
}