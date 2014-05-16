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
	std::string mount(argv[4]);
	int packetLoss = atoi(argv[6]);
	if (mount[mount.size() - 1] != '/')
		mount += '/';

	srand(time(NULL));
	uint32_t nodeId = (uint32_t)rand();

	printf("Server port: %u, mount: %s, packetLoss: %d, nodeId: %010u\n", port, mount.c_str(), packetLoss, nodeId);
	ServerInstance *server = new ServerInstance(packetLoss, nodeId, mount.c_str());

	int err = mkdir(mount.c_str(), S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH);
	if (err == -1) {
		if (errno == EEXIST) {
			RFSError("machine already in use");
		} else {
			RFSError("create mount directory error");
		}
	}

	server->rfs_NetInit(port);

	server->execute();
	
	return 0;
}

ServerInstance:: ServerInstance(int packetLoss, uint32_t nodeId, std::string mount): NetworkInstance(packetLoss, nodeId) {
	this->mount = mount;
	this->nodeType = SERVER_NODE;
	this->isFileOpen = false;
	this->latestValidUpdateId = 0;
}

void ServerInstance:: execute() {
	while(1) {
		if (rfs_IsRecvPacket()) {

			char buf[MAXBUFSIZE];
			memset(buf, 0, MAXBUFSIZE);

			ssize_t status = rfs_RecvFrom(buf, MAXBUFSIZE);
			
			printf("Recv message size: %d, ", (int)status);
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
					case CLOSE:
					{
						procCloseMessage(buf);
					}
					default:
					break;
				}
			}
		}
	}
}

void ServerInstance:: sendInitAckMessage() {
	InitAckMessage initAckMsg(nodeId, msgSeqNum);
	msgSeqNum = getNextNum(msgSeqNum);

	sendMessage(&initAckMsg, HEADER_SIZE);
}

void ServerInstance:: procInitMessage(char *buf) {
	InitMessage initMessage;
	initMessage.deserialize(buf);

	initMessage.print();

	sendInitAckMessage();
}

void ServerInstance:: sendOpenFileAckMessage(int fileDesc) {
	OpenFileAckMessage openFileAckMessage(nodeId, msgSeqNum, fileDesc);
	msgSeqNum = getNextNum(msgSeqNum);

	sendMessage(&openFileAckMessage, HEADER_SIZE);
}

void ServerInstance:: procOpenFileMessage(char *buf) {
	OpenFileMessage openFileMessage;
	openFileMessage.deserialize(buf);

	openFileMessage.print();

	if (isFileOpen) {
		sendOpenFileAckMessage(1);
		return;
	}

	std::string filename(openFileMessage.filename);
	std::string fileFullname = mount + filename;
	
	if (!isFileExist(fileFullname.c_str()))
		fp = fopen(fileFullname.c_str(), "w+b");
	else
		fp = fopen(fileFullname.c_str(), "r+b");

	if (!fp) {
		isFileOpen = false;
		printf("Create filename %s fail\n", fileFullname.c_str());
		sendOpenFileAckMessage(-1);
	} else {
		isFileOpen = true;
		printf("Create filename %s success\n", fileFullname.c_str());
		sendOpenFileAckMessage(1);
	}
}

void ServerInstance:: procWriteBlockMessage(char *buf) {
	WriteBlockMessage writeBlockMessage;
	writeBlockMessage.deserialize(buf);

	writeBlockMessage.print();
	std::map<uint32_t, Update>::iterator it = updateMap.find(writeBlockMessage.updateId);
	if (it == updateMap.end()) {
		Update update;
		update.byteOffset = writeBlockMessage.byteOffset;
		update.blockSize = writeBlockMessage.blockSize;
		update.buffer = new char[writeBlockMessage.blockSize];
		memset(update.buffer, 0, writeBlockMessage.blockSize);
		memcpy(update.buffer, writeBlockMessage.buffer, writeBlockMessage.blockSize);
		
		updateMap.insert(std::make_pair(writeBlockMessage.updateId, update));
	}

	// for (uint32_t i = 0;;i++)
	// 	std::map<uint32_t, Update>::iterator iter = updateMap.find(i);
	// 	if (iter == updateMap.end()) {
	// 		break;
	// 	} else {
	// 		latestValidUpdateId = i;
	// 	}
	// }
}

void ServerInstance:: sendCloseAckMessage(int fileDesc) {
	CloseAckMessage closeAckMessage(nodeId, msgSeqNum, fileDesc);
	msgSeqNum = getNextNum(msgSeqNum);

	sendMessage(&closeAckMessage, HEADER_SIZE);
}

void ServerInstance:: procCloseMessage(char *buf) {
	CloseMessage closeMsg;
	closeMsg.deserialize(buf);

	closeMsg.print();

	if (isFileOpen == true) {
		int ret = fclose(fp);
		if (ret == 0)
			sendCloseAckMessage(0);
		else
			sendCloseAckMessage(-1);
		isFileOpen = false;
	} else {
		sendCloseAckMessage(-1);
	}
}

