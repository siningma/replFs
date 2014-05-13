/************************/
/* Your Name: Sining Ma */
/* Date: 05/10/2014     */
/* CS 244B	            */
/* Spring 2013	        */
/************************/

#include "serverInstance.h"

ServerInstance *server;

int main(int argc, char *argv[]) {

	unsigned short port = (unsigned short)atoi(argv[2]);
	char *filepath = argv[4];
	int packetLoss = atoi(argv[6]);

	srand(time(NULL));
	uint32_t nodeId = (uint32_t)rand();

	int err = mkdir(filepath, S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH);
	if (err == -1) {
		if (error == ENOENT) {
			RFSError("machine already in use");
		} else {
			RFSError("create filepath directory error");
		}
	}

	int socket = rfs_netInit(port);

	server = new ServerInstance(packetLoss, socket, nodeId, filePath);
	execute();
	return 0;
}

ServerInstance:: ServerInstance(int packetLoss, int socket, uint32_t nodeId, char* filePath): NetworkInstance(packetLoss, socket, nodeId) {
	int len = strlen(filePath) + 1;

	this->filePath = new char[len];
	memset(this->filePath, 0, len);
	memcpy(this->filePath, filePath, len);
}

void ServerInstance:: sendInitAckMessage() {
	InitAckMessage initAckMsg(this->nodeId, getMsgSeqNum());

	char buf[HEADER_SIZE];
	memset(buf, 0, HEADER_SIZE);
	initAckMsg.serialize(buf);
	initAckMsg.print();

	if (isDropPacket(packetLoss))
		return;
	rfs_sendTo(this->socket, buf, HEADER_SIZE);
}

int ServerInstance:: procInitMessage(char *buf) {
	InitMessage initMessage;
	initMessage.deserialize(buf);
	initMessage.print();

	sendInitAckMessage();
	return 0;
}

void execute() {
	char buf[BUF_SIZE];

	while(1) {
		memset(buf, 0, BUF_SIZE);

		ssize_t status = rfs_recvFrom(server->socket, buf, BUF_SIZE);
		if (isDropPacket(server->packetLoss))
			continue;

		if (status > 0) {
			unsigned char msgType = buf[0];

			switch(msgType) {
				case INIT:
				{
					server->processInitMessage(buf);
					break;
				}
				default:
				break;
			}
		}
	}
}