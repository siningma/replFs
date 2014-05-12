/************************/
/* Your Name: Sining Ma */
/* Date: 05/10/2014     */
/* CS 244B	            */
/* Spring 2013	        */
/************************/

#include "serverInstance.h"

int main(int argc, char *argv[]) {

	unsigned short port = (unsigned short)atoi(argv[2]);
	server.filepath = argv[4];
	server.packetLoss = (uint16_t)atoi(argv[6]);

	srand(time(NULL));
	server.serverId = (uint32_t)rand();

	int err = mkdir(filepath, S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH);
	if (err == -1) {
		if (error == ENOENT) {
			RFSError("machine already in use");
		} else {
			RFSError("create filepath directory error");
		}
	}

	int socket = rfs_netInit(port);

	execute(socket);
	return 0;
}

void ServerInstance:: execute() {
	char buf[BUF_SIZE];

	while(1) {
		memset(buf, 0, BUF_SIZE);

		ssize_t status = rfs_recvFrom(this->socket, buf, BUF_SIZE);

		if (status > 0) {
			convert_incoming(buf);


		}
	}
}