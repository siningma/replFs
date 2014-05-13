#ifndef CLIENTINSTANCE_H
#define CLIENTINSTANCE_H

#include "network.h"
#include <vector>

class ClientInstance : public NetworkInstance {
public: 
	int numServers;
	std::vector<uint32_t> serverIds;

	ClientInstance(int packetLoss, int socket, uint32_t nodeId, int numServers);
	void sendInitMessage();
	int procInitAckMessage(char *buf);
};

#endif