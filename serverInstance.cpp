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

	if( mkdir(mount.c_str(), S_IRUSR | S_IWUSR) < 0) {
		if (errno == EEXIST) {
			RFSError("machine already in use");
		} else {
			printf("create mount directory error = %s\n", strerror(errno));
			RFSError("ReplFsServer server exits");
		}
	}

	server->rfs_NetInit(port);

	server->execute();
	
	return 0;
}

ServerInstance:: ServerInstance(int packetLoss, uint32_t nodeId, std::string mount): NetworkInstance(packetLoss, nodeId) {
	this->mount = mount;
	this->nodeType = SERVER_NODE;
	this->nextUpdateId = 0;
	this->backup = NULL;
	this->fp = NULL;
}

void ServerInstance:: execute() {
	while(1) {
		bool isRecvPacket = rfs_IsRecvPacket();
		// printf("receive packets: %d\n", isRecvPacket);

		if (isRecvPacket) {
			char buf[MAXBUFSIZE];
			memset(buf, 0, MAXBUFSIZE);

			ssize_t status = rfs_RecvFrom(buf, MAXBUFSIZE);

			if (status > 0) {
				if (status < HEADER_SIZE)
                	continue;

				unsigned char msgType = buf[0];

				if (isMessageSentByMe(buf))
					continue;

				// message received not from client
				if (!isRecvClientMsg(msgType))
					continue;

				if (isDropPacket(packetLoss)) {
					uint32_t msg_nodeId = 0;
					memcpy(&msg_nodeId, buf + 2, 4);
					msg_nodeId = ntohl(msg_nodeId);
					uint32_t msg_seqNum = 0;
					memcpy(&msg_seqNum, buf + 6, 4);
					msg_seqNum = ntohl(msg_seqNum);
					printf("Drop Message: MsgType: 0x%02x, nodeId: %010u, msgSeqNum: %u\n", msgType, msg_nodeId, msg_seqNum);
					continue;
				}

				switch(msgType) {
					case INIT:
					printf("Recv message size: %d, ", (int)status);
					procInitMessage(buf);
					break;
					case OPENFILE:
					printf("Recv message size: %d, ", (int)status);
					procOpenFileMessage(buf);
					break;
					case WRITEBLOCK:
					printf("Recv message size: %d, ", (int)status);
					procWriteBlockMessage(buf);
					break;
					case VOTE:
					printf("Recv message size: %d, ", (int)status);
					procVoteMessage(buf);
					break;
					case COMMIT:
					printf("Recv message size: %d, ", (int)status);
					procCommitMessage(buf);
					break;
					case ABORT:
					printf("Recv message size: %d, ", (int)status);
					procAbortMessage(buf);
					break;
					case CLOSE:
					printf("Recv message size: %d, ", (int)status);
					procCloseMessage(buf);
					break;
					default:
					break;
				}
			}
		}
	}
}

void ServerInstance:: reset() {
	// delete memory
	if ((int)updateMap.size() != 0) {
		for (std::map<uint32_t, Update>::iterator it = updateMap.begin(); it != updateMap.end(); ++it) {
			char *buff = it->second.buffer;
			delete[] buff;
		}
		updateMap.clear();
	}
	nextUpdateId = 0;
}

void ServerInstance:: resetBackup() {
	if (backup != NULL) {
		delete[] backup; 	
		backup = NULL;
	}
}

bool ServerInstance:: isRecvClientMsg(unsigned char msgType) {
	switch(msgType) {
		case INIT:
		return true;
		break;
		case OPENFILE:
		return true;
		break;
		case WRITEBLOCK:
		return true;
		break;
		case VOTE:
		return true;
		break;
		case COMMIT:
		return true;
		break;
		case ABORT:
		return true;
		break;
		case CLOSE:
		return true;
		break;
		default:
		return false;
		break;
	}
	return false;
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

	sendMessage(&openFileAckMessage, HEADER_SIZE + 4);
}

void ServerInstance:: procOpenFileMessage(char *buf) {
	OpenFileMessage openFileMessage;
	openFileMessage.deserialize(buf);

	openFileMessage.print();

	// opening file only gets new filename
	// create or open the file when commit is done
	std::string filename(openFileMessage.filename);
	fileFullname.clear();
	fileFullname = mount + filename;

	printf("OpenFile phase: server receives new filename: %s\n", fileFullname.c_str());
	sendOpenFileAckMessage(0);
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
}

void ServerInstance:: sendVoteAckMessage(int fileDesc, uint32_t updateId) {
	VoteAckMessage voteAckMessage(nodeId, msgSeqNum, fileDesc, updateId);
	msgSeqNum = getNextNum(msgSeqNum);

	sendMessage(&voteAckMessage, HEADER_SIZE + 8);
}

void ServerInstance:: procVoteMessage(char *buf) {
	VoteMessage voteMsg;
	voteMsg.deserialize(buf);

	voteMsg.print();

	while(1) {
		std::map<uint32_t, Update>::iterator it = updateMap.find(nextUpdateId);
		if (it == updateMap.end())	// cannot found in memory
			break;
		else	// found in memory
			++nextUpdateId;
	}

	printf("Vote phase: server receives fileId: %u till updateId: %u\n", voteMsg.fileId, nextUpdateId);
	sendVoteAckMessage(0, nextUpdateId);
}

void ServerInstance:: sendCommitAckMessage(int fileDesc) {
	CommitAckMessage commitAckMessage(nodeId, msgSeqNum, fileDesc);
	msgSeqNum = getNextNum(msgSeqNum);

	sendMessage(&commitAckMessage, HEADER_SIZE + 4);
}

void ServerInstance:: procCommitMessage(char *buf) {
	CommitMessage commitMsg;
	commitMsg.deserialize(buf);

	commitMsg.print();

	printf("Commit phase: start to commit fileId: %u till updateId: %u\n", commitMsg.fileId, nextUpdateId);

	if (!isFileExist(fileFullname.c_str()))
		fp = fopen(fileFullname.c_str(), "w+b");
	else
		fp = fopen(fileFullname.c_str(), "r+b");

	if (!fp) {
		// open the file fails
		printf("Commit phase: Create filename %s fail\n", fileFullname.c_str());
		sendCommitAckMessage(-1);
		return;
	} else {
		// open the file successfully
		printf("Commit phase: Create filename %s successful\n", fileFullname.c_str());
	}

	// before the first commit, do backup
	fseek(fp, 0, SEEK_END);
	long fileSize = ftell(fp);
	if (fileSize > 0) {
		backup = new char[fileSize];
		memset(backup, 0, fileSize);
		fread(backup, 1, fileSize, fp);
	}

	// write from memory to the file
	for (uint32_t i = 0; i < nextUpdateId; i++) {
		std::map<uint32_t, Update>::iterator it = updateMap.find(i);

		int byteOffset = it->second.byteOffset;
		int blockSize = it->second.blockSize;
		char *buffer = it->second.buffer;

		// seek and write all updates to the file
		if (fseek (fp, byteOffset, SEEK_SET) != 0) {
			printf("Commit phase: fseek error on updateId: %u\n", i);
			sendCommitAckMessage(-1);
			return;
		}

		if ((int)fwrite (buffer , sizeof(char), blockSize, fp) != blockSize) {
			printf("Commit phase: fwrite error on updateId: %u\n", i);
			sendCommitAckMessage(-1);
			return;
		}
		fflush(fp);
	}

	printf("Commit phase: All commits are done update fileId: %u till updateId: %u\n", commitMsg.fileId, nextUpdateId);

	// commit is done
	resetBackup();
	reset();
	sendCommitAckMessage(0);
}

void ServerInstance:: sendAbortAckMessage(int fileDesc) {
	AbortAckMessage abortAckMessage(nodeId, msgSeqNum, fileDesc);
	msgSeqNum = getNextNum(msgSeqNum);

	sendMessage(&abortAckMessage, HEADER_SIZE + 4);
}

void ServerInstance:: procAbortMessage(char *buf) {
	AbortMessage abortMsg;
	abortMsg.deserialize(buf);

	abortMsg.print();

	if (backup != NULL) {
		printf("Abort phase: Server error happens during commit and rollback the file"); 
		if (remove(fileFullname.c_str()) != 0) {
			printf("Rollback delete filename %s fail\n", fileFullname.c_str());
			sendAbortAckMessage(-1);
			resetBackup();
			reset();
			return;
		}

		fp = fopen(fileFullname.c_str(), "w+b");
		if (!fp) {
			// fail to open file in order to do rollback
			printf("Rollback recreate filename %s fail\n", fileFullname.c_str());
			sendAbortAckMessage(-1);
			resetBackup();
			reset();
			return;
		}

		fwrite (backup , sizeof(char), sizeof(backup), fp);
		fflush(fp);
		resetBackup();
	}

	// abort is done
	reset();
	printf("Abort phase: Server aborts fileId: %u updates ok\n", abortMsg.fileId);
	sendAbortAckMessage(0);
}

void ServerInstance:: sendCloseAckMessage(int fileDesc) {
	CloseAckMessage closeAckMessage(nodeId, msgSeqNum, fileDesc);
	msgSeqNum = getNextNum(msgSeqNum);

	sendMessage(&closeAckMessage, HEADER_SIZE + 4);
}

void ServerInstance:: procCloseMessage(char *buf) {
	CloseMessage closeMsg;
	closeMsg.deserialize(buf);

	closeMsg.print();

	printf("Close phase: Server starts to close file\n");

	if ((int)updateMap.size() != 0) {
		printf("Close phase: Server has uncommitted file update\n");
		sendCloseAckMessage(1);
		return;
	}

	if (fp != NULL) {
		int ret = fclose(fp);
		if (ret == 0) {
			printf("Close phase: Server closes fileId: %u ok\n", closeMsg.fileId);
			sendCloseAckMessage(0);
			fp = NULL;
		}
		else {
			printf("Close phase: Server closes fileId: %u error\n", closeMsg.fileId);
			sendCloseAckMessage(-1);
		}
	} else {
		sendCloseAckMessage(0);
	}
}

