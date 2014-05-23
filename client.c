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

    printf("Client Init phase...\n");
    if (client->execute(INIT_OP, SHORT_TIMEOUT, NULL, 0, NULL) == ErrorReturn)
        return ErrorReturn;

    printf("Client Init phase is done\n");
    return (NormalReturn);
}

/* ------------------------------------------------------------------ */

int
OpenFile( char * fileName ) {

    if (fileName == NULL || strlen(fileName) >= MAXFILENAMESIZE)
        return ErrorReturn;
    if ((int)client->serverIds.size() < client->numServers) {
        printf("Client error: Not enough servers are available\n");
        return ErrorReturn;
    }

#ifdef DEBUG
    printf( "OpenFile: Opening File '%s'\n", fileName );
#endif
    int fd = client->nextFd;
    client->nextFd = getNextNum(client->nextFd);

    printf("Client OpenFile phase...\n");
    std::set<uint32_t> recvServerId;
    if (client->execute(OPEN_OP, LONG_TIMEOUT, &recvServerId, fd, fileName) == ErrorReturn)
        return ErrorReturn;

    client->isFileOpen = true;
    printf("Client OpenFile phase is done\n");
    return fd;
}

/* ------------------------------------------------------------------ */

int
WriteBlock( int fd, char * buffer, int byteOffset, int blockSize ) {
    if( fd < 0 )
        return ErrorReturn;
    if (fd != (client->nextFd - 1)) // check if passing open file fd
        return ErrorReturn;
    if ( byteOffset < 0 || byteOffset > MAXFILESIZE)
        return ErrorReturn;
    if ( buffer == NULL)
        return ErrorReturn;
    if ( blockSize < 0 || blockSize > MaxBlockLength )
        return ErrorReturn;
    if (!client->isFileOpen)
        return ErrorReturn;
    if ((int)client->serverIds.size() < client->numServers) {
        printf("Client error: Not enough servers are available\n");
        return ErrorReturn;
    }

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
    if (fd < 0)
        return ErrorReturn;
    if (fd != (client->nextFd - 1)) // check if passing open file fd
        return ErrorReturn;
    if (!client->isFileOpen)
        return ErrorReturn;
    if ((int)client->serverIds.size() < client->numServers) {
        printf("Client error: Not enough servers are available\n");
        return ErrorReturn;
    }

#ifdef DEBUG
    printf( "Commit: FD=%d\n", fd );
#endif

  	/****************************************************/
  	/* Prepare to Commit Phase			    */
  	/* - Check that all writes made it to the server(s) */
  	/****************************************************/
    printf("Client Vote phase...\n");
    std::set<uint32_t> recvVoteServerId;
    while(1) {
        int ret = client->execute(VOTE_OP, LONG_TIMEOUT, &recvVoteServerId, fd, NULL);
        if (ret == ErrorReturn) {
            // error happens on servers, abort all file updates
            Abort(fd);
            return (ErrorReturn);
        } else if (ret == 0)
            break;
    }
    printf("Client Vote phase is done\n");

  	/****************/
  	/* Commit Phase */
  	/****************/
    printf("Client Commit phase...\n");
    std::set<uint32_t> recvCommitServerId;
    if (client->execute(COMMIT_OP, LONG_TIMEOUT, &recvCommitServerId, fd, NULL) == ErrorReturn) {
        printf("File commit error, rollback all updates in fileId: %d\n", fd);
        Abort(fd);
        return ErrorReturn;
    }
    
    printf("Client Commit phase is done\n");
    client->reset();
    return (NormalReturn);
}

/* ------------------------------------------------------------------ */

int
Abort( int fd )
{
    if (fd < 0)
        return ErrorReturn;
    if (fd != (client->nextFd - 1)) // check if passing open file fd
        return ErrorReturn;
    if (!client->isFileOpen)
        return ErrorReturn;
    if ((int)client->serverIds.size() < client->numServers) {
        printf("Client error: Not enough servers are available\n");
        return ErrorReturn;
    }

#ifdef DEBUG
    printf( "Abort: FD=%d\n", fd );
#endif

    /*************************/
    /* Abort the transaction */
    /*************************/
    printf("Client Abort phase...\n");
    std::set<uint32_t> recvServerId;
    if (client->execute(ABORT_OP, LONG_TIMEOUT, &recvServerId, fd, NULL) == ErrorReturn) {
        client->reset();       
        return ErrorReturn;
    }

    printf("Client Abort phase is done\n");
    client->reset();
    return (NormalReturn);
}

/* ------------------------------------------------------------------ */

int
CloseFile( int fd ) {
    if (fd < 0)
        return ErrorReturn;
    if (fd != (client->nextFd - 1)) // check if passing open file fd
        return ErrorReturn;
    if (!client->isFileOpen)
        return ErrorReturn;
    if ((int)client->serverIds.size() < client->numServers) {
        printf("Client error: Not enough servers are available\n");
        return ErrorReturn;
    }

#ifdef DEBUG
    printf( "Close: FD=%d\n", fd );
#endif

  	/*****************************/
  	/* Check for Commit or Abort */
  	/*****************************/
    printf("Client Close phase...\n");
    std::set<uint32_t> recvServerId;
    int ret = client->execute(CLOSE_OP, SHORT_TIMEOUT, &recvServerId, fd, NULL);
    if (ret == ErrorReturn)
        return ErrorReturn;
    else if (ret == 1) {
        // server has uncommitted updates, do commit
        printf("Servers have uncommitted file updates\n");
        if (Commit(fd) == ErrorReturn)
            return ErrorReturn;

        // try to close file again
        if (client->execute(CLOSE_OP, SHORT_TIMEOUT, &recvServerId, fd, NULL) == ErrorReturn)
            return ErrorReturn;
    }

    client->isFileOpen = false;
    printf("Client CloseFile phase is done\n\n");
    return (NormalReturn);
}

/*  ------------------------------------------------------------------ */
