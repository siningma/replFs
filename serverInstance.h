/************************/
/* Your Name: Sining Ma */
/* Date: 05/10/2014     */
/* CS 244B	            */
/* Spring 2013	        */
/************************/

#ifndef SERVERINSTANCE_H
#define SERVERINSTANCE_H

#include "network.h"

class ServerInstance: public NetworkInstance {
public:
	std::string mount;
	FILE* fp;
	bool isFileOpen;
	std::map<uint32_t, Update> updateMap;
	uint32_t nextUpdateId;

	ServerInstance(int packetLoss, uint32_t nodeId, std::string mount);

	void execute();
	void sendInitAckMessage();
	void procInitMessage(char *buf);

	void sendOpenFileAckMessage(int fileDesc);
	void procOpenFileMessage(char *buf);

	void procWriteBlockMessage(char *buf);

	void sendVoteAckMessage(int fileDesc, uint32_t updateId);
	void procVoteMessage(char *buf);

	void sendCommitAckMessage(int fileDesc);
	void procCommitMessage(char *buf);

	void sendAbortAckMessage(int fileDesc);
	void procAbortMessage(char *buf);

	void sendCloseAckMessage(int fileDesc);
	void procCloseMessage(char *buf);
};

#endif