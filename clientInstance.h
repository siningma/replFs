#ifndef CLIENTINSTANCE_H
#define CLIENTINSTANCE_H

#include "network.h"

class ClientInstance : public NetworkInstance {
public: 
	int numServers;
	std::set<uint32_t> serverIds;
	uint32_t fd;
	uint32_t updateId;
	std::map<uint32_t, Update> updateMap;

	ClientInstance(int packetLoss, uint32_t nodeId, int numServers);

	void sendInitMessage();
	int procInitAckMessage(char *buf);

	void sendOpenFileMessage(uint32_t fileId, char* filename);
	int procOpenFileAckMessage(char *buf, std::set<uint32_t> *recvServerId);

	void sendWriteBlockMessage(uint32_t fileId, uint32_t updateId, int byteOffset, int blockSize, char *buffer);

	void sendCloseMessage(uint32_t fileId);
	int procCloseAckMessage(char *buf, std::set<uint32_t> *recvServerId);
};

#endif