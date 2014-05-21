/************************/
/* Your Name: Sining Ma */
/* Date: 05/10/2014     */
/* CS 244B	            */
/* Spring 2013	        */
/************************/

#include "clientInstance.h"

ClientInstance:: ClientInstance(int packetLoss, uint32_t nodeId, int numServers): NetworkInstance(packetLoss, nodeId) {
	this->numServers = numServers;
	this->nodeType = CLIENT_NODE;
	this->nextFd = 0;
	this->updateId = 0;
}

int ClientInstance:: execute(int opCode, int timeout, std::set<uint32_t> *recvServerId, uint32_t fd, char *fileName) {
	struct timeval first;
    struct timeval last;
    struct timeval now;

    memset(&last, 0, sizeof(struct timeval));
    getCurrentTime(&first);
    while(1) {
        getCurrentTime(&now);
        if (isTimeOut(&now, &last, SEND_MSG_INTERVAL)) {
        	switch(opCode) {
        		case INIT_OP:
        		sendInitMessage();
        		break;
        		case OPEN_OP:
        		sendOpenFileMessage(fd, fileName);
        		break;
        		case VOTE_OP:
        		sendVoteMessage(fd);
                break;
                case COMMIT_OP:
                sendCommitMessage(fd);
                break;
                case ABORT_OP:
                sendAbortMessage(fd);
                break;
        		case CLOSE_OP:
        		sendCloseMessage(fd);
        		break;
        		default:
        		break;
        	}

            getCurrentTime(&last);
        }

        if (isTimeOut(&now, &first, timeout)) {
              break;
        } else {
            if (rfs_IsRecvPacket()) {
                char buf[MAXBUFSIZE];
                memset(buf, 0, MAXBUFSIZE);
                int status = rfs_RecvFrom(buf, MAXBUFSIZE);

                if (status > 0) {
                    if (status < HEADER_SIZE)
                        continue;

                    if (isMessageSentByMe(buf))
                        continue;

                    unsigned char msgType = buf[0];
                    if (isDropPacket(packetLoss)) {
                        printf("Drop Message: MsgType: 0x%02x\n", msgType);
					
						uint32_t msg_nodeId = 0;
						memcpy(&msg_nodeId, buf + 2, 4);
						msg_nodeId = ntohl(msg_nodeId);
						uint32_t msg_seqNum = 0;
						memcpy(&msg_seqNum, buf + 6, 4);
						msg_seqNum = ntohl(msg_seqNum);
						printf("nodeId: %010u, msgSeqNum: %u\n", msg_nodeId, msg_seqNum);
                        continue;
                    }

                    printf("Recv message size: %d, ", (int)status);
                    switch(opCode) {
                    	case INIT_OP:
                    	procInitAckMessage(buf);
                    	break;
                    	case OPEN_OP:
                    	if (procOpenFileAckMessage(buf, recvServerId) == -1)
                    		return ( ErrorReturn );
                        break;
                        case VOTE_OP:
                        {
                        	int ret = procVoteAckMessage(buf, recvServerId, fd);
	                        if (ret == -1)	// Error happens on servers
	                        	return (ErrorReturn);
	                        else if (ret == 1)	// need to do retransmission
	                        	return 1;
                    	}
                    	break;
                        case COMMIT_OP:
                        if (procCommitAckMessage(buf, recvServerId) == -1)
                        	return (ErrorReturn);
                        break;
                        case ABORT_OP:
                        if (procAbortAckMessage(buf, recvServerId) == -1) 
                        	return (ErrorReturn);
                        break;
                        case CLOSE_OP:
                        if (procCloseAckMessage(buf, recvServerId) == -1)
                        	return ( ErrorReturn );
                    	break;
                    	default:
                    	break;
                    }  

                    switch(opCode) {
                    	case OPEN_OP:
                    	case VOTE_OP:
                    	case COMMIT_OP:
                    	case ABORT_OP:
                    	case CLOSE_OP:
                    	if ((int)recvServerId->size() == numServers)
                    		return NormalReturn;
                    	break;
                    	default:
                    	break;
                    }                  
                }
            }
        }
    }

    // if one server is unavailable when receiving messages timeout, return error 
    if (recvServerId != NULL && (int)recvServerId->size() < numServers)
        return ( ErrorReturn );
    else 
    	return (NormalReturn);
}

void ClientInstance:: reset() {
	// reset all, except numServers and serverIds
	for (std::map<uint32_t, Update>::iterator it = updateMap.begin(); it != updateMap.end(); ++it) {
		char *buff = it->second.buffer;
		delete[] buff;
	}
	updateMap.clear();
	updateId = 0;
	recvServerUpdateId.clear();
}

void ClientInstance:: sendInitMessage() {
	InitMessage initMsg(nodeId, msgSeqNum);
	msgSeqNum = getNextNum(msgSeqNum);
	
	sendMessage(&initMsg, HEADER_SIZE);
}

int ClientInstance:: procInitAckMessage(char *buf) {
	InitAckMessage initAckMessage;
	initAckMessage.deserialize(buf);

	initAckMessage.print();

	if ((int)serverIds.size() == numServers)
		return 0;

	std::set<uint32_t>::iterator it = serverIds.find(initAckMessage.nodeId);
	if (it == serverIds.end())
		serverIds.insert(initAckMessage.nodeId);

	return 0;
}

void ClientInstance:: sendOpenFileMessage(uint32_t fileId, char* filename) {
	OpenFileMessage openFileMsg(nodeId, msgSeqNum, fileId, filename);
	msgSeqNum = getNextNum(msgSeqNum);
	
	sendMessage(&openFileMsg, HEADER_SIZE + 4 + strlen(filename));
}

int ClientInstance:: procOpenFileAckMessage(char *buf, std::set<uint32_t> *recvServerId) {
	OpenFileAckMessage openFileAckMessage;
	openFileAckMessage.deserialize(buf);

	openFileAckMessage.print();

	if (openFileAckMessage.fileDesc < 0)
		return -1;
	else {
		std::set<uint32_t>::iterator it = recvServerId->find(openFileAckMessage.nodeId);
		std::set<uint32_t>::iterator iter = serverIds.find(openFileAckMessage.nodeId);

		// message id can be found in serverIds set, but not in recvServerId set
		if (iter != serverIds.end() && it == recvServerId->end()) { 
			recvServerId->insert(openFileAckMessage.nodeId);
			// printf("Recv OpenFileAck Message from server: %010u\n", openFileAckMessage.nodeId);
		}
		return 0;
	}
}

void ClientInstance:: sendWriteBlockMessage(uint32_t fileId, uint32_t updateId, int byteOffset, int blockSize, char *buffer, int isStore) {
	WriteBlockMessage writeBlockMsg(nodeId, msgSeqNum, fileId, updateId, byteOffset, blockSize, buffer);
	msgSeqNum = getNextNum(msgSeqNum);

	if (isStore) {
		Update update;
		update.byteOffset = byteOffset;
		update.blockSize = blockSize;
		update.buffer = new char[blockSize];
		memset(update.buffer, 0, blockSize);
		memcpy(update.buffer, buffer, blockSize);

		updateMap.insert(std::make_pair(updateId, update));
	}

	sendMessage(&writeBlockMsg, HEADER_SIZE + 16 + blockSize);
}

void ClientInstance:: sendVoteMessage(uint32_t fileId) {
	VoteMessage voteMsg(nodeId, msgSeqNum, fileId);
	msgSeqNum = getNextNum(msgSeqNum);

	sendMessage(&voteMsg, HEADER_SIZE + 4);
}

int ClientInstance:: procVoteAckMessage(char *buf, std::set<uint32_t> *recvServerId, uint32_t fd) {
	VoteAckMessage voteAckMessage;
	voteAckMessage.deserialize(buf);

	voteAckMessage.print();

	if (voteAckMessage.fileDesc < 0)
		return -1;
	else {
		std::set<uint32_t>::iterator it = recvServerId->find(voteAckMessage.nodeId);
		std::set<uint32_t>::iterator iter = serverIds.find(voteAckMessage.nodeId);

		// message id can be found in serverIds set, but not in recvServerId set
		if (iter != serverIds.end() && it == recvServerId->end()) { 
			recvServerId->insert(voteAckMessage.nodeId);
		}

		// update receive servers updateId
		std::map<uint32_t, uint32_t>::iterator iterUpdateId = recvServerUpdateId.find(voteAckMessage.nodeId);
		if (iterUpdateId == recvServerUpdateId.end())
			recvServerUpdateId.insert(std::make_pair(voteAckMessage.nodeId, voteAckMessage.updateId));
		else
			iterUpdateId->second = voteAckMessage.updateId;

		// check if retransmission is needed
		if ((int)recvServerId->size() == numServers) {
			uint32_t smallestUpdateId = std::numeric_limits<uint32_t>::max();

			for (std::map<uint32_t, uint32_t>::iterator itTemp = recvServerUpdateId.begin(); itTemp != recvServerUpdateId.end(); ++itTemp) {
				if (smallestUpdateId > itTemp->second)
					smallestUpdateId = itTemp->second;
			}

			if (smallestUpdateId == updateId) {
				printf("Servers receive all updates till updateId: %u\n", updateId);
				return 0;
			}

            printf("Client retransmit file updates from updateId: %u\n", smallestUpdateId);
			for (uint32_t i = smallestUpdateId; i < updateId; i++) {
				std::map<uint32_t, Update>::iterator it = updateMap.find(i);
				int byteOffset = it->second.byteOffset;
				int blockSize = it->second.blockSize;
				char *buffer = it->second.buffer;

				sendWriteBlockMessage(fd, i, byteOffset, blockSize, buffer, 0);
			}
			return 1;	// need to send vote again
		}
	}
	return 0;
}

void ClientInstance:: sendCommitMessage(uint32_t fileId) {
	CommitMessage commitMsg(nodeId, msgSeqNum, fileId);
	msgSeqNum = getNextNum(msgSeqNum);

	sendMessage(&commitMsg, HEADER_SIZE + 4);
}

int ClientInstance:: procCommitAckMessage(char *buf, std::set<uint32_t> *recvServerId) {
	CommitAckMessage commitAckMessage;
	commitAckMessage.deserialize(buf);

	commitAckMessage.print();

	if (commitAckMessage.fileDesc < 0)
		return -1;
	else {
		std::set<uint32_t>::iterator it = recvServerId->find(commitAckMessage.nodeId);
		std::set<uint32_t>::iterator iter = serverIds.find(commitAckMessage.nodeId);

		// message id can be found in serverIds set, but not in recvServerId set
		if (iter != serverIds.end() && it == recvServerId->end()) { 
			recvServerId->insert(commitAckMessage.nodeId);
		}
		return 0;
	}
}

void ClientInstance:: sendAbortMessage(uint32_t fileId) {
	AbortMessage abortMsg(nodeId, msgSeqNum, fileId);
	msgSeqNum = getNextNum(msgSeqNum);

	sendMessage(&abortMsg, HEADER_SIZE + 4);
}

int ClientInstance:: procAbortAckMessage(char *buf, std::set<uint32_t> *recvServerId) {
	AbortAckMessage abortAckMessage;
	abortAckMessage.deserialize(buf);

	abortAckMessage.print();

	if (abortAckMessage.fileDesc < 0)
		return -1;
	else {
		std::set<uint32_t>::iterator it = recvServerId->find(abortAckMessage.nodeId);
		std::set<uint32_t>::iterator iter = serverIds.find(abortAckMessage.nodeId);

		// message id can be found in serverIds set, but not in recvServerId set
		if (iter != serverIds.end() && it == recvServerId->end()) { 
			recvServerId->insert(abortAckMessage.nodeId);
		}
		return 0;
	}
}

void ClientInstance:: sendCloseMessage(uint32_t fileId) {
	CloseMessage closeMsg(nodeId, msgSeqNum, fileId);
	msgSeqNum = getNextNum(msgSeqNum);
	
	sendMessage(&closeMsg, HEADER_SIZE + 4);
}

int ClientInstance:: procCloseAckMessage(char *buf, std::set<uint32_t> *recvServerId) {
	CloseAckMessage closeAckMessage;
	closeAckMessage.deserialize(buf);

	closeAckMessage.print();

	if (closeAckMessage.fileDesc < 0)
		return -1;
	else {
		std::set<uint32_t>::iterator it = recvServerId->find(closeAckMessage.nodeId);
		std::set<uint32_t>::iterator iter = serverIds.find(closeAckMessage.nodeId);

		// message id can be found in serverIds set, but not in recvServerId set
		if (iter != serverIds.end() && it == recvServerId->end()) { 
			recvServerId->insert(closeAckMessage.nodeId);
		}
		return 0;
	}
}