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
	this->fd = 0;
	this->updateId = 0;
}

void ClientInstance:: sendInitMessage() {
	InitMessage initMsg(nodeId, msgSeqNum);
	msgSeqNum = getNextNum(msgSeqNum);
	
	sendMessage(&initMsg, HEADER_SIZE);
}

int ClientInstance:: procInitAckMessage(char *buf) {
	InitAckMessage initAckMessage;
	initAckMessage.deserialize(buf);

	initAckMessage.print();

	if ((int)serverIds.size() == numServers)
		return 0;

	std::set<uint32_t>::iterator it = serverIds.find(initAckMessage.nodeId);
	if (it == serverIds.end())
		serverIds.insert(initAckMessage.nodeId);

	return 0;
}

void ClientInstance:: sendOpenFileMessage(uint32_t fileId, char* filename) {
	OpenFileMessage openFileMsg(nodeId, msgSeqNum, fileId, filename);
	msgSeqNum = getNextNum(msgSeqNum);
	
	sendMessage(&openFileMsg, HEADER_SIZE + 4 + strlen(filename));
}

int ClientInstance:: procOpenFileAckMessage(char *buf, std::set<uint32_t> *recvServerId) {
	OpenFileAckMessage openFileAckMessage;
	openFileAckMessage.deserialize(buf);

	openFileAckMessage.print();

	if (openFileAckMessage.fileDesc < 0)
		return -1;
	else {
		std::set<uint32_t>::iterator it = recvServerId->find(openFileAckMessage.nodeId);
		std::set<uint32_t>::iterator iter = serverIds.find(openFileAckMessage.nodeId);

		// message id can be found in serverIds set, but not in recvServerId set
		if (iter != serverIds.end() && it == recvServerId->end()) { 
			recvServerId->insert(openFileAckMessage.nodeId);
		}
		return 0;
	}
}

void ClientInstance:: sendWriteBlockMessage(int fileId, uint32_t updateId, int byteOffset, int blockSize, char *buffer) {
	WriteBlockMessage writeBlockMsg(nodeId, msgSeqNum, fileId, updateId, byteOffset, blockSize, buffer);
	msgSeqNum = getNextNum(msgSeqNum);

	Update update;
	update.byteOffset = byteOffset;
	update.blockSize = blockSize;
	update.buffer = buffer;
	updateMap.insert(std::make_pair(updateId, update));
	sendMessage(&writeBlockMsg, HEADER_SIZE + 16 + blockSize);
}

void ClientInstance:: sendCloseMessage(int fileId) {
	CloseMessage closeMsg(nodeId, msgSeqNum, fileId);
	msgSeqNum = getNextNum(msgSeqNum);
	
	sendMessage(&closeMsg, HEADER_SIZE + 4);
}

int ClientInstance:: procCloseAckMessage(char *buf, std::set<uint32_t> *recvServerId) {
	CloseAckMessage closeAckMessage;
	closeAckMessage.deserialize(buf);

	closeAckMessage.print();

	if (closeAckMessage.fileDesc < 0)
		return -1;
	else {
		std::set<uint32_t>::iterator it = recvServerId->find(closeAckMessage.nodeId);
		std::set<uint32_t>::iterator iter = serverIds.find(closeAckMessage.nodeId);

		// message id can be found in serverIds set, but not in recvServerId set
		if (iter != serverIds.end() && it == recvServerId->end()) { 
			recvServerId->insert(closeAckMessage.nodeId);
		}
		return 0;
	}
}