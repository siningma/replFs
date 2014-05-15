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
	char *filePath;
	FILE *pf;

	ServerInstance(int packetLoss, uint32_t nodeId, char* filePath);

	void execute();
	void sendInitAckMessage();
	int procInitMessage(char *buf);
	void sendOpenFileAckMessage(int fileDesc);
	int procOpenFileMessage(char *buf);
};

#endif