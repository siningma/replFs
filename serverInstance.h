/************************/
/* Your Name: Sining Ma */
/* Date: 05/10/2014     */
/* CS 244B	            */
/* Spring 2013	        */
/************************/

#ifndef SERVERINSTANCE_H
#define SERVERINSTANCE_H

#include "network.h"

#define BUF_SIZE 1024

class ServerInstance: public NetworkInstance {
public:
	int packetLoss;
	char *filePath;
	int socket;
	uint32_t serverId;

	void execute();
};

#endif