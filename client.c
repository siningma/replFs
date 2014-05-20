/************************/
/* Your Name: Sining Ma */
/* Date: 05/10/2014     */
/* CS 244B              */
/* Spring 2013          */
/************************/

#define DEBUG

#include <stdio.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <errno.h>
#include <unistd.h>
#include <assert.h>

#include <client.h>
#include "clientInstance.h"

ClientInstance *client;

/* ------------------------------------------------------------------ */

int
InitReplFs( unsigned short portNum, int packetLoss, int numServers ) {
#ifdef DEBUG
    printf( "InitReplFs: Port number %d, packet loss %d percent, %d servers\n", 
        portNum, packetLoss, numServers );
#endif

    /****************************************************/
    /* Initialize network access, local state, etc.     */
    /****************************************************/

    srand (time(NULL));
    uint32_t nodeId = (uint32_t)rand();

    client = new ClientInstance(packetLoss, nodeId, numServers);
    printf("Client port: %u, packetLoss: %d, numServers: %d, nodeId: %010u\n", portNum, packetLoss, numServers, nodeId);

    client->rfs_NetInit(portNum);

    if (client->execute(INIT_OP, SHORT_TIMEOUT, NULL, 0, NULL) == ErrorReturn)
        return ErrorReturn;

    printf("\nReceive serverId count: %d\n", (int)client->serverIds.size());
    printf("Server Ids: ");
    for (std::set<uint32_t>::iterator it = client->serverIds.begin(); it != client->serverIds.end(); ++it) {
        printf("%010u, ", *it);
    }
    printf("\n\n");

    return (NormalReturn);
}

/* ------------------------------------------------------------------ */

int
OpenFile( char * fileName ) {
    ASSERT( fileName );

    if (fileName == NULL || strlen(fileName) >= MAXFILENAMESIZE)
        return ErrorReturn;

#ifdef DEBUG
    printf( "OpenFile: Opening File '%s'\n", fileName );
#endif
    int fd = client->nextFd;
    client->nextFd = getNextNum(client->nextFd);

    std::set<uint32_t> recvServerId;
    if (client->execute(OPEN_OP, LONG_TIMEOUT, &recvServerId, fd, fileName) == ErrorReturn)
        return ErrorReturn;

    return (NormalReturn);
}

/* ------------------------------------------------------------------ */

int
WriteBlock( int fd, char * buffer, int byteOffset, int blockSize ) {
    //char strError[64];
    // int bytesWritten;

    ASSERT( fd >= 0 );
    ASSERT( byteOffset >= 0 );
    ASSERT( buffer );
    ASSERT( blockSize >= 0 && blockSize < MaxBlockLength );

    if (fd < 0 || byteOffset < 0 || buffer == NULL || blockSize < 0 || blockSize >= MaxBlockLength)
        return ErrorReturn;

#ifdef DEBUG
    printf( "WriteBlock: Writing FD=%d, Offset=%d, Length=%d\n", fd, byteOffset, blockSize );
#endif
    client->sendWriteBlockMessage(fd, client->updateId, byteOffset, blockSize, buffer, 1);
    client->updateId = getNextNum(client->updateId);

    return( fd );
}

/* ------------------------------------------------------------------ */

int
Commit( int fd ) {
    ASSERT( fd >= 0 );

#ifdef DEBUG
    printf( "Commit: FD=%d\n", fd );
#endif

  	/****************************************************/
  	/* Prepare to Commit Phase			    */
  	/* - Check that all writes made it to the server(s) */
  	/****************************************************/
    std::set<uint32_t> recvVoteServerId;
    while(1) {
        int ret = client->execute(VOTE_OP, LONG_TIMEOUT, &recvVoteServerId, fd, NULL);
        if (ret < 0) {
            // error happens on servers, abort all file updates
            Abort(fd);
            return (ErrorReturn);
        } else if (ret == 0)
            break;
    }

  	/****************/
  	/* Commit Phase */
  	/****************/
    std::set<uint32_t> recvCommitServerId;
    if (client->execute(COMMIT_OP, LONG_TIMEOUT, &recvCommitServerId, fd, NULL) == ErrorReturn)
        return ErrorReturn;
    
    return (NormalReturn);
}

/* ------------------------------------------------------------------ */

int
Abort( int fd )
{
    ASSERT( fd >= 0 );

#ifdef DEBUG
    printf( "Abort: FD=%d\n", fd );
#endif

    /*************************/
    /* Abort the transaction */
    /*************************/
    std::set<uint32_t> recvServerId;
    if (client->execute(ABORT_OP, LONG_TIMEOUT, &recvServerId, fd, NULL) == ErrorReturn)
        return ErrorReturn;

    return (NormalReturn);
}

/* ------------------------------------------------------------------ */

int
CloseFile( int fd ) {

    ASSERT( fd >= 0 );

#ifdef DEBUG
    printf( "Close: FD=%d\n", fd );
#endif

  	/*****************************/
  	/* Check for Commit or Abort */
  	/*****************************/
    // TODO:

    std::set<uint32_t> recvServerId;
    if (client->execute(CLOSE_OP, SHORT_TIMEOUT, &recvServerId, fd, NULL) == ErrorReturn)
        return ErrorReturn;

    return (NormalReturn);
}

/*  ------------------------------------------------------------------ */
