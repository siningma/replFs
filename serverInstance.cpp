/************************/
/* Your Name: Sining Ma */
/* Date: 05/10/2014     */
/* CS 244B	            */
/* Spring 2013	        */
/************************/

#include "serverInstance.h"

ServerInstance *server;

int main(int argc, char *argv[]) {
	if (argc != 7)
		RFSError("Invalid command. Example: replFsServer -port 4137 -mount /folder1/fs244b -drop 3");

	unsigned short port = (unsigned short)atoi(argv[2]);
	char *filePath = argv[4];
	int packetLoss = atoi(argv[6]);

	srand(time(NULL));
	uint32_t nodeId = (uint32_t)rand();

	printf("Server port: %u, filePath: %s, packetLoss: %d, nodeId: %u\n", port, filePath, packetLoss, nodeId);
	server = new ServerInstance(packetLoss, nodeId, filePath);

	int err = mkdir(filePath, S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH);
	if (err == -1) {
		if (errno == ENOENT) {
			RFSError("machine already in use");
		} else {
			RFSError("create filepath directory error");
		}
	}

	server->rfs_netInit(port);

	server->execute();
	
	return 0;
}

ServerInstance:: ServerInstance(int packetLoss, uint32_t nodeId, char* filePath): NetworkInstance(packetLoss, nodeId) {
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

	if (isDropPacket(packetLoss)) {
		printf("Drop Message: ");
		initAckMsg.print();
	} else {
		printf("Send Message: ");
		initAckMsg.print();
		rfs_sendTo(buf, HEADER_SIZE);
	}
}

int ServerInstance:: procInitMessage(char *buf) {
	InitMessage initMessage;
	initMessage.deserialize(buf);

	printf("Receive Message: ");
	initMessage.print();

	sendInitAckMessage();
	return 0;
}

void ServerInstance:: execute() {
	char buf[BUF_SIZE];

	while(1) {
		memset(buf, 0, BUF_SIZE);

		if (rfs_recvData(-1)) {	// server is blocking IO

			ssize_t status = rfs_recvFrom(buf, BUF_SIZE);
			if (isDropPacket(server->packetLoss))
				continue;

			if (status > 0) {
				unsigned char msgType = buf[0];

				switch(msgType) {
					case INIT:
					{
						server->procInitMessage(buf);
						break;
					}
					default:
					break;
				}
			}
		}
	}
}