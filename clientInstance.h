#ifndef CLIENTINSTANCE_H
#define CLIENTINSTANCE_H

#include "network.h"

#define MAXFILENAMESIZE 128

#define INIT_OP 0
#define OPEN_OP 1
#define VOTE_OP 2
#define COMMIT_OP 3
#define ABORT_OP 4
#define CLOSE_OP 5

enum {
  NormalReturn = 0,
  ErrorReturn = -1,
};

class ClientInstance : public NetworkInstance {
public: 
	int numServers;
	std::set<uint32_t> serverIds;

	bool isFileOpen;
	uint32_t nextFd;	// next file id, a monotonically increasing number
	uint32_t updateId;	// client total update count in one commit
	std::map<uint32_t, Update> updateMap;
	std::map<uint32_t, uint32_t> recvServerUpdateId;

	ClientInstance(int packetLoss, uint32_t nodeId, int numServers);

	int execute(int opCode, int timeout, std::set<uint32_t> *recvServerId, uint32_t fd, char *fileName);
	void reset();

	void sendInitMessage();
	int procInitAckMessage(char *buf);

	void sendOpenFileMessage(uint32_t fileId, char *fileName);
	int procOpenFileAckMessage(char *buf, std::set<uint32_t> *recvServerId);

	void sendWriteBlockMessage(uint32_t fileId, uint32_t updateId, int byteOffset, int blockSize, char *buffer, int isStore);

	void sendVoteMessage(uint32_t fileId);
	int procVoteAckMessage(char *buf, std::set<uint32_t> *recvServerId, uint32_t fd);

	void sendCommitMessage(uint32_t fileId);
	int procCommitAckMessage(char *buf, std::set<uint32_t> *recvServerId);	

	void sendAbortMessage(uint32_t fileId);
	int procAbortAckMessage(char *buf, std::set<uint32_t> *recvServerId);

	void sendCloseMessage(uint32_t fileId);
	int procCloseAckMessage(char *buf, std::set<uint32_t> *recvServerId);
};

#endif