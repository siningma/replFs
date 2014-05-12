#ifndef CLIENTINSTANCE_H
#define CLIENTINSTANCE_H

#include "network.h"

class ClientInstance : public NetworkInstance {
public: 
	int packetLoss;
	int numServers;
	int socket;
	uint32_t clientId;

	int recvInitAckServerCount;
	int serverId[numServers];

	ClientInstance();
};

#endif