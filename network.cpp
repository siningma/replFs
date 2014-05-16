/************************/
/* Your Name: Sining Ma */
/* Date: 05/10/2014     */
/* CS 244B	            */
/* Spring 2013	        */
/************************/

#include "network.h"

void RFSError(const char *s) {
	fprintf(stderr, "%s\n", s);
	perror("CS244BRFSError");
	exit(-1);
}

void getCurrentTime(struct timeval *tv) {
	memset(tv, 0, sizeof(struct timeval));
	gettimeofday(tv, NULL);  	
}

bool isTimeOut(struct timeval *curr, struct timeval *last, uint32_t millisecond) {
	return ((curr->tv_sec - last->tv_sec) * 1000 + (curr->tv_usec - last->tv_usec) / 1000) >= millisecond;
}

bool isDropPacket(int packetLoss) {
	int prob = rand() % 100;
	// printf("drop prob: %d\n", prob);

	return prob < packetLoss;
}

uint32_t getNextNum(uint32_t num) {
	if (num == (uint32_t)~0) {
		return 0;
	} else {
		return ++num;
	}
}

int isFileExist(const char *filename) {
    struct stat st;
    int result = stat(filename, &st);
    return result == 0;
}

NetworkInstance:: NetworkInstance(int packetLoss, uint32_t nodeId) {
	this->packetLoss = packetLoss;
	this->nodeId = nodeId;
	this->msgSeqNum = 0;
	memset(&this->groupAddr, 0, sizeof(Sockaddr));
}

void NetworkInstance:: rfs_NetInit(unsigned short port) {
	Sockaddr		nullAddr;
	int				reuse;
	u_char          ttl;
	struct ip_mreq  mreq;

	sockfd = socket(AF_INET, SOCK_DGRAM, 0);
	if (sockfd < 0)
	  	RFSError("can't get socket");

	/* SO_REUSEADDR allows more than one binding to the same
	   socket - you cannot have more than one player on one
	   machine without this */
	reuse = 1;
	if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(reuse)) < 0) {
		RFSError("setsockopt failed (SO_REUSEADDR)");
	}

	nullAddr.sin_family = AF_INET;
	nullAddr.sin_addr.s_addr = htonl(INADDR_ANY);
	nullAddr.sin_port = htons(port);
	if (bind(sockfd, (struct sockaddr *)&nullAddr, sizeof(nullAddr)) < 0)
	  RFSError("netInit binding");

	/* Multicast TTL:
	   0 restricted to the same host
	   1 restricted to the same subnet
	   32 restricted to the same site
	   64 restricted to the same region
	   128 restricted to the same continent
	   255 unrestricted

	   DO NOT use a value > 32. If possible, use a value of 1 when
	   testing.
	*/

	ttl = 1;
	if (setsockopt(sockfd, IPPROTO_IP, IP_MULTICAST_TTL, &ttl, sizeof(ttl)) < 0) {
		RFSError("setsockopt failed (IP_MULTICAST_TTL)");
	}

	/* join the multicast group */
	mreq.imr_multiaddr.s_addr = htonl(RFS_GROUP);
	mreq.imr_interface.s_addr = htonl(INADDR_ANY);
	if (setsockopt(sockfd, IPPROTO_IP, IP_ADD_MEMBERSHIP, (char *)&mreq, sizeof(mreq)) < 0) {
		RFSError("setsockopt failed (IP_ADD_MEMBERSHIP)");
	}

	/* Get the multi-cast address ready to use in SendData()
           calls. */
	memcpy(&groupAddr, &nullAddr, sizeof(Sockaddr));
	groupAddr.sin_addr.s_addr = htonl(RFS_GROUP);
}

ssize_t NetworkInstance:: rfs_SendTo(char *buf, int length) {
	ssize_t cc = sendto(sockfd, buf, length, 0, 
		(struct sockaddr *)&groupAddr, sizeof(Sockaddr));

	if (cc < 0) {
		perror("sendto()");
	}
	return cc;
}

bool NetworkInstance:: rfs_IsRecvPacket() {
    fd_set	fdmask;
    struct timeval timeout;
	FD_ZERO(&fdmask);
  	FD_SET(sockfd, &fdmask);
  	if (nodeType == CLIENT_NODE) 	// non blocking IO
  		timeout.tv_sec = 0;
  	else	// server blocks 5 seconds
  		timeout.tv_sec = 5;
	timeout.tv_usec = 0;

	int ret = select(sockfd + 1, &fdmask, NULL, NULL, &timeout);
	if (ret == -1) {
		perror("select()");
		return false;
	} else if (!ret) {
		if (nodeType == SERVER_NODE)
			printf("Server receives no data in 5 seconds\n");

		return false;
	} else {
		return FD_ISSET(sockfd, &fdmask);
	}
}

ssize_t NetworkInstance:: rfs_RecvFrom(char* buf, int length) {
	socklen_t fromLen = sizeof(Sockaddr);

	ssize_t cc = recvfrom(sockfd, buf, length, 0, 
		(struct sockaddr *)&groupAddr, &fromLen);
	
	if (cc < 0 && errno != EINTR)
		perror("event recvfrom");

	return cc;
}

void NetworkInstance:: sendMessage(Message *msg, int len) {
	char buf[len];
	memset(buf, 0, len);
	msg->serialize(buf);

	ssize_t cc = rfs_SendTo(buf, len);
	printf("Send Message size: %d, ", (int)cc);
	msg->print();
}

bool NetworkInstance:: isMessageSentByMe(char *buf) {
	uint32_t msg_nodeId = 0;
	memcpy(&msg_nodeId, buf + 2, 4);
	unsigned char msgType = buf[0];
	printf("Match msgType: 0x%02x, nodeId: %010u\n", msgType, msg_nodeId);
	return this->nodeId == ntohl(msg_nodeId);
}
