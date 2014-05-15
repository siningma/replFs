#ifndef CLIENTINSTANCE_H
#define CLIENTINSTANCE_H

#include "network.h"

class ClientInstance : public NetworkInstance {
public: 
	int numServers;
	std::vector<uint32_t> serverIds;
	uint32_t updateId;
	std::map<uint32_t, Update> updateMap;

	ClientInstance(int packetLoss, uint32_t nodeId, int numServers);

	uint32_t getUpdateId();

	void sendInitMessage();
	int procInitAckMessage(char *buf);

	void sendOpenFileMessage(int fileId, char* filename);
	int procOpenFileAckMessage(char *buf);

	void sendWriteBlockMessage(int fileId, uint32_t updateId, int byteOffset, int blockSize, char *buffer);

	void sendCloseMessage(int fileId);
	int procCloseAckMessage(char *buf);
};

#endif