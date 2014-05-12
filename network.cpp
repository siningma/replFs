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

void getMsgSeqNum() {
	if (msgSeqNum == UINT32_MAX) {
		msgSeqNum = 0;
		return msgSeqNum;
	} else {
		return msgSeqNum++;
	}
}

bool isTimeOut(struct timeval *curr, struct timeval *last, uint32_t millisecond) {
	double currTime = curr->tv_sec * 1000 + curr->tv_usec / 1000;
	double lastTime = last->tv_sec * 1000 + last->tv_usec / 1000;

	return (currTime - lastTime) >= millisecond;
}

bool isDrop(int packetLoss) {
	return ((unsigned int)rand() % 100) < packetLoss;
}

int rfs_netInit(unsigned short port) {
	Sockaddr		nullAddr;
	Sockaddr		*thisHost;
	char			buf[128];
	int				reuse;
	u_char          ttl;
	struct ip_mreq  mreq;

	gethostname(buf, sizeof(buf));
	if ((thisHost = resolveHost(buf)) == (Sockaddr *) NULL)
	  RFSError("who am I?");
	bcopy((caddr_t) thisHost, (caddr_t) (M->myAddr()), sizeof(Sockaddr));

	int socket = socket(AF_INET, SOCK_DGRAM, 0);
	if (socket < 0)
	  RFSError("can't get socket");

	/* SO_REUSEADDR allows more than one binding to the same
	   socket - you cannot have more than one player on one
	   machine without this */
	reuse = 1;
	if (setsockopt(socket, SOL_SOCKET, SO_REUSEADDR, &reuse,
		   sizeof(reuse)) < 0) {
		RFSError("setsockopt failed (SO_REUSEADDR)");
	}

	nullAddr.sin_family = AF_INET;
	nullAddr.sin_addr.s_addr = htonl(INADDR_ANY);
	nullAddr.sin_port = htons(port);
	if (bind(socket, (struct sockaddr *)&nullAddr,
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
	if (setsockopt(socket, IPPROTO_IP, IP_MULTICAST_TTL, &ttl,
		   sizeof(ttl)) < 0) {
		RFSError("setsockopt failed (IP_MULTICAST_TTL)");
	}

	/* join the multicast group */
	mreq.imr_multiaddr.s_addr = htonl(RFSGROUP);
	mreq.imr_interface.s_addr = htonl(INADDR_ANY);
	if (setsockopt(socket, IPPROTO_IP, IP_ADD_MEMBERSHIP, (char *)
		   &mreq, sizeof(mreq)) < 0) {
		RFSError("setsockopt failed (IP_ADD_MEMBERSHIP)");
	}

	/*
	 * Now we can try to find a game to join; if none, start one.
	 */
	 
	printf("\n");

	/* Get the multi-cast address ready to use in SendData()
           calls. */
	memcpy(&groupAddr, &nullAddr, sizeof(Sockaddr));
	groupAddr.sin_addr.s_addr = htonl(RFSGROUP);

	return socket;
}

ssize_t rfs_sendTo(int socket, char *buf, int length) {
	ssize_t cc = sendto(socket, buf, length, 0, 
		(struct sockaddr *)&groupAddr, sizeof(Sockaddr));

	return cc;
}

ssize_t rfs_recvFrom(int socket, char* buf, int length) {
	ssize_t cc = recvfrom(socket, buf, length, 0, 
		(struct sockaddr *)&groupAddr, sizeof(Sockaddr));
	
	if (cc < 0 && errno != EINTR)
		perror("event recvfrom");

	return cc;
}

void convert_incoming(const char* buf) {
	unsigned char msgType = buf[0];

	switch(msgType) {
		case INIT:
		{
			InitMessage p;
			p.deserialize(buf);

			processInitMessage(&p);
			break;
		}
		case INITACK:
		{
			InitAckMessage p;
			p.deserialize(buf);

			processInitAckMessage(&p);
			break;
		}
		default:
		break;
	}
}

void sendInitMessage(int socket, uint32_t nodeId) {
	InitMessage msg(nodeId, getMsgSeqNum());

	char buf[HEADER_SIZE];
	memset(buf, 0, HEADER_SIZE);
	msg.serialize(buf);

	rfs_sendTo(socket, buf, HEADER_SIZE);
}

int processInitMessage(InitMessage *p) {
	sendInitAckMessage(socket)
}

int processInitAckMessage(InitAckMessage *p) {

}