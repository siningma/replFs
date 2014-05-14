/************************/
/* Your Name: Sining Ma */
/* Date: 05/10/2014     */
/* CS 244B	            */
/* Spring 2013	        */
/************************/

#include "network.h"

void RFSError(char *s) {
	fprintf(stderr, "%s\n", s);
	perror("CS244BRFSError");
	exit(-1);
}

void getCurrentTime(struct timeval *tv) {
	memset(tv, 0, sizeof(struct timeval));
	gettimeofday(tv, NULL);  	
}

uint32_t getMsgSeqNum() {
	if (msgSeqNum == 10000000) {
		msgSeqNum = 0;
		return msgSeqNum;
	} else {
		return msgSeqNum++;
	}
}

bool isTimeOut(struct timeval *curr, struct timeval *last, uint32_t millisecond) {
	return ((curr->tv_sec - last->tv_sec) * 1000 + (curr->tv_usec - last->tv_usec) / 1000) >= millisecond;
}

bool isDrop(int packetLoss) {
	return ((unsigned int)rand() % 100) < packetLoss;
}

bool isDropPacket(int packetLoss) { 
	if (isDrop(packetLoss)) {
		printf(" Drop\n");
		return true;
	} else {
		return false;
	}
}

int rfs_netInit(unsigned short port) {
	Sockaddr		nullAddr;
	int				reuse;
	u_char          ttl;
	struct ip_mreq  mreq;

	int sockfd = socket(AF_INET, SOCK_DGRAM, 0);
	if (sockfd < 0)
	  RFSError("can't get socket");

	/* SO_REUSEADDR allows more than one binding to the same
	   socket - you cannot have more than one player on one
	   machine without this */
	reuse = 1;
	if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &reuse,
		   sizeof(reuse)) < 0) {
		RFSError("setsockopt failed (SO_REUSEADDR)");
	}

	nullAddr.sin_family = AF_INET;
	nullAddr.sin_addr.s_addr = htonl(INADDR_ANY);
	nullAddr.sin_port = htons(port);
	if (bind(sockfd, (struct sockaddr *)&nullAddr,
		 sizeof(nullAddr)) < 0)
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
	if (setsockopt(sockfd, IPPROTO_IP, IP_MULTICAST_TTL, &ttl,
		   sizeof(ttl)) < 0) {
		RFSError("setsockopt failed (IP_MULTICAST_TTL)");
	}

	/* join the multicast group */
	mreq.imr_multiaddr.s_addr = htonl(RFSGROUP);
	mreq.imr_interface.s_addr = htonl(INADDR_ANY);
	if (setsockopt(sockfd, IPPROTO_IP, IP_ADD_MEMBERSHIP, (char *)
		   &mreq, sizeof(mreq)) < 0) {
		RFSError("setsockopt failed (IP_ADD_MEMBERSHIP)");
	}

	/* Get the multi-cast address ready to use in SendData()
           calls. */
	memcpy(&groupAddr, &nullAddr, sizeof(Sockaddr));
	groupAddr.sin_addr.s_addr = htonl(RFSGROUP);

	return sockfd;
}

ssize_t rfs_sendTo(int sockfd, char *buf, int length) {
	ssize_t cc = sendto(sockfd, buf, length, 0, 
		(struct sockaddr *)&groupAddr, sizeof(Sockaddr));

	return cc;
}

bool rfs_recvData(int sockfd, int pollTimeout) {
    struct pollfd udp;
    udp.fd = sockfd;
    udp.events = POLLIN;

    int ret = poll(&udp, 1, pollTimeout);
    if (ret < 0) {
        RFSError("poll error"); 
        return false;  
    } else {
        if (udp.revents & POLLIN)
        	return true;
        else
        	return false;
    }
}

ssize_t rfs_recvFrom(int sockfd, char* buf, int length) {
	socklen_t fromLen = sizeof(Sockaddr);
	ssize_t cc = recvfrom(sockfd, buf, length, 0, 
		(struct sockaddr *)&groupAddr, &fromLen);
	
	printf("recv data: %d\n", cc);
	if (cc < 0 && errno != EINTR)
		perror("event recvfrom");

	return cc;
}

NetworkInstance:: NetworkInstance(int packetLoss, int sockfd, uint32_t nodeId) {
	this->packetLoss = packetLoss;
	this->sockfd = sockfd;
	this->nodeId = nodeId;
}