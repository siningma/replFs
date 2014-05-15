/************************/
/* Your Name: Sining Ma */
/* Date: 05/10/2014     */
/* CS 244B	            */
/* Spring 2013	        */
/************************/

#include "serverInstance.h"

int main(int argc, char *argv[]) {
	if (argc != 7)
		RFSError("Invalid command. Example: replFsServer -port 4137 -mount /folder1/fs244b -drop 3");

	unsigned short port = (unsigned short)atoi(argv[2]);
	char *filePath = argv[4];
	int packetLoss = atoi(argv[6]);

	srand(time(NULL));
	uint32_t nodeId = (uint32_t)rand();

	printf("Server port: %u, filePath: %s, packetLoss: %d, nodeId: %010u\n", port, filePath, packetLoss, nodeId);
	ServerInstance *server = new ServerInstance(packetLoss, nodeId, filePath);

	int err = mkdir(filePath, S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH);
	if (err == -1) {
		if (errno == EEXIST) {
			RFSError("machine already in use");
		} else {
			RFSError("create filepath directory error");
		}
	}

	server->rfs_NetInit(port);

	server->execute();
	
	return 0;
}

ServerInstance:: ServerInstance(int packetLoss, uint32_t nodeId, char* filePath): NetworkInstance(packetLoss, nodeId) {
	int len = strlen(filePath) + 1;
	this->filePath = new char[len];
	memset(this->filePath, 0, len);
	memcpy(this->filePath, filePath, len);
}

void ServerInstance:: execute() {
	while(1) {
		if (rfs_IsRecvPacket(TRUE)) {

			char buf[MAXBUFSIZE];
			memset(buf, 0, MAXBUFSIZE);

			ssize_t status = rfs_RecvFrom(buf, sizeof(buf));
			
			printf("Server recv message size: %d ", (int)status);
			if (isMessageSentByMe(buf))
				continue;

			if (status > 0) {
				unsigned char msgType = buf[0];

				if (isDropPacket(packetLoss)) {
					printf("Drop Message: Recv Message: MsgType: 0x%02x\n", msgType);
					continue;
				}

				switch(msgType) {
					case INIT:
					{
						procInitMessage(buf);
						break;
					}
					case OPENFILE:
					{
						procOpenFileMessage(buf);
						break;
					}
					default:
					break;
				}
			}
		}
	}
}

void ServerInstance:: sendInitAckMessage() {
	InitAckMessage initAckMsg(nodeId, getMsgSeqNum());

	dropOrSendMessage(&initAckMsg, HEADER_SIZE);
}

int ServerInstance:: procInitMessage(char *buf) {
	InitMessage initMessage;
	initMessage.deserialize(buf);

	initMessage.print();

	sendInitAckMessage();
	return 0;
}

void ServerInstance:: sendOpenFileAckMessage(int fileDesc) {
	OpenFileAckMessage openFileAckMessage(nodeId, getMsgSeqNum(), fileDesc);

	dropOrSendMessage(&openFileAckMessage, HEADER_SIZE);
}

int ServerInstance:: procOpenFileMessage(char *buf) {
	OpenFileMessage openFileMessage;
	openFileMessage.deserialize(buf);

	openFileMessage.print();

	char *fileFullname = strcat(filePath, openFileMessage.filename);
	pf = fopen(fileFullname, "r+b");
	if (!pf) {
		sendOpenFileAckMessage(-1);
		return -1;
	} else {
		sendOpenFileAckMessage(0);
		return 0;
	}
}

