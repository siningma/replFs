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

    client->execute(INIT_OP, SHORT_TIMEOUT, NULL, NULL); 

    printf("Receive serverId count: %d\n", (int)client->serverIds.size());
    printf("Server Ids: ");
    for (std::set<uint32_t>::iterator it = client->serverIds.begin(); it != client->serverIds.end(); ++it) {
        printf("%010u, ", *it);
    }
    printf("\n");

    if ((int)client->serverIds.size() < numServers)
        return ( ErrorReturn );
    else
        return( NormalReturn ); 
}

/* ------------------------------------------------------------------ */

int
OpenFile( char * fileName ) {
    ASSERT( fileName );

    if (fileName == NULL || strlen(fileName) >= MAXFILENAMESIZE)
        return -1;

#ifdef DEBUG
    printf( "OpenFile: Opening File '%s'\n", fileName );
#endif

    std::set<uint32_t> recvServerId;
    client->execute(OPEN_OP, LONG_TIMEOUT, &recvServerId, fileName);
    
    if ((int)recvServerId.size() != client->numServers)
        return ( ErrorReturn );
    else {
        int fd = client->fd;
        client->fd = getNextNum(client->fd);
        return( fd );
    }
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
        return -1;

#ifdef DEBUG
    printf( "WriteBlock: Writing FD=%d, Offset=%d, Length=%d\n", fd, byteOffset, blockSize );
#endif
    client->sendWriteBlockMessage(fd, client->updateId, byteOffset, blockSize, buffer);
    client->updateId = getNextNum(client->updateId);

    // if ( lseek( fd, byteOffset, SEEK_SET ) < 0 ) {
    //     perror( "WriteBlock Seek" );
    //     return(ErrorReturn);
    // }

    // if ( ( bytesWritten = write( fd, buffer, blockSize ) ) < 0 ) {
    //     perror( "WriteBlock write" );
    //     return(ErrorReturn);
    // }

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

  	/****************/
  	/* Commit Phase */
  	/****************/

    return( NormalReturn );

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

    return(NormalReturn);
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
    std::set<uint32_t> recvServerId;
    client->execute(CLOSE_OP, SHORT_TIMEOUT, &recvServerId, NULL);

    if ((int)recvServerId.size() != client->numServers)
        return ( ErrorReturn );
    else {
        return(NormalReturn);
    }
}

/*  ------------------------------------------------------------------ */




