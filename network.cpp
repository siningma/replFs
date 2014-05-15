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
	return ((unsigned int)rand() % 100) < packetLoss;
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

	this->sockfd = socket(AF_INET, SOCK_DGRAM, 0);
	if (this->sockfd < 0)
	  RFSError("can't get socket");

	/* SO_REUSEADDR allows more than one binding to the same
	   socket - you cannot have more than one player on one
	   machine without this */
	reuse = 1;
	if (setsockopt(this->sockfd, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(reuse)) < 0) {
		RFSError("setsockopt failed (SO_REUSEADDR)");
	}

	nullAddr.sin_family = AF_INET;
	nullAddr.sin_addr.s_addr = htonl(INADDR_ANY);
	nullAddr.sin_port = htons(port);
	if (bind(this->sockfd, (struct sockaddr *)&nullAddr, sizeof(nullAddr)) < 0)
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
	if (setsockopt(this->sockfd, IPPROTO_IP, IP_MULTICAST_TTL, &ttl, sizeof(ttl)) < 0) {
		RFSError("setsockopt failed (IP_MULTICAST_TTL)");
	}

	/* join the multicast group */
	mreq.imr_multiaddr.s_addr = htonl(RFS_GROUP);
	mreq.imr_interface.s_addr = htonl(INADDR_ANY);
	if (setsockopt(this->sockfd, IPPROTO_IP, IP_ADD_MEMBERSHIP, (char *)&mreq, sizeof(mreq)) < 0) {
		RFSError("setsockopt failed (IP_ADD_MEMBERSHIP)");
	}

	/* Get the multi-cast address ready to use in SendData()
           calls. */
	memcpy(&this->groupAddr, &nullAddr, sizeof(Sockaddr));
	this->groupAddr.sin_addr.s_addr = htonl(RFS_GROUP);
}

ssize_t NetworkInstance:: rfs_SendTo(char *buf, int length) {
	ssize_t cc = sendto(this->sockfd, buf, length, 0, 
		(struct sockaddr *)&this->groupAddr, sizeof(Sockaddr));

	return cc;
}

bool NetworkInstance:: rfs_IsRecvPacket(bool nodeType) {
    fd_set	fdmask;
    struct timeval timeout;
	FD_ZERO(&fdmask);
  	FD_SET(sockfd, &fdmask);
  	if (!nodeType)
  		timeout.tv_sec = 0;
  	else
  		timeout.tv_sec = 5;
	timeout.tv_usec = 0;

	int ret = select(sockfd + 1, &fdmask, NULL, NULL, &timeout);
	if (ret == -1) {
		perror("select()");
		return false;
	}
	else if (!ret) {
		if (nodeType)
			printf("No Data receive in 5 seconds\n");
		return false;
	} else {
		return FD_ISSET(sockfd, &fdmask);
	}
}

ssize_t NetworkInstance:: rfs_RecvFrom(char* buf, int length) {
	socklen_t fromLen = sizeof(Sockaddr);

	ssize_t cc = recvfrom(this->sockfd, buf, length, 0, 
		(struct sockaddr *)&this->groupAddr, &fromLen);
	
	if (cc < 0 && errno != EINTR)
		perror("event recvfrom");

	return cc;
}

void NetworkInstance:: dropOrSendMessage(Message *msg, int len) {
	if (isDropPacket(packetLoss)) {
		printf("Drop Message: ");
		msg->print();
	} else {	
		char buf[len];
		memset(buf, 0, len);
		msg->serialize(buf);

		rfs_SendTo(buf, len);
		printf("Send Message: ");
		msg->print();
	}
}


uint32_t NetworkInstance:: getMsgSeqNum() {
	if (msgSeqNum == (uint32_t)~0) {
		msgSeqNum = 0;
		return (uint32_t)~0;
	} else {
		return msgSeqNum++;
	}
}

bool NetworkInstance:: isMessageSentByMe(char *buf) {
	uint32_t msg_nodeId = 0;
	memcpy(&msg_nodeId, buf + 2, 4);
	return this->nodeId == ntohl(msg_nodeId);
}
