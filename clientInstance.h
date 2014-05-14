#ifndef CLIENTINSTANCE_H
#define CLIENTINSTANCE_H

#include "network.h"

class ClientInstance : public NetworkInstance {
public: 
	int numServers;
	std::vector<uint32_t> serverIds;

	ClientInstance(int packetLoss, uint32_t nodeId, int numServers, unsigned int group);

	void sendInitMessage();
	int procInitAckMessage(char *buf);
	void sendOpenFileMessage(uint32_t fileId, char* filename);
	int procOpenFileAckMessage(char *buf);
};

#endif