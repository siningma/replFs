/************************/
/* Your Name: Sining Ma */
/* Date: 05/10/2014     */
/* CS 244B	            */
/* Spring 2013	        */
/************************/

#ifndef SERVERINSTANCE_H
#define SERVERINSTANCE_H

#include "network.h"

#define MAXBUFSIZE 2048

class ServerInstance: public NetworkInstance {
public:
	char *filePath;
	std::map<int, FILE*> fileId_map;

	ServerInstance(int packetLoss, uint32_t nodeId, char* filePath);

	void execute();
	void sendInitAckMessage();
	void procInitMessage(char *buf);
	void sendOpenFileAckMessage(int fileDesc);
	void procOpenFileMessage(char *buf);

	void sendCloseAckMessage(int fileDesc);
	void procCloseMessage(char *buf);
};

#endif