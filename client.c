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

static ClientInstance *client;

/* ------------------------------------------------------------------ */

int
InitReplFs( unsigned short portNum, int packetLoss, int numServers ) {
#ifdef DEBUG
    printf( "InitReplFs: Port number %d, packet loss %d percent, %d servers\n", 
        portNum, packetLoss, numServers );
#endif

    printf("max: %u\n", (uint32_t)~0);

    /****************************************************/
    /* Initialize network access, local state, etc.     */
    /****************************************************/

    srand (time(NULL));
    uint32_t nodeId = (uint32_t)rand();

    client = new ClientInstance(packetLoss, nodeId, numServers, RFS_GROUP);
    printf("Client port: %u, packetLoss: %d, numServers: %d, nodeId: %010u\n", portNum, packetLoss, numServers, nodeId);

    client->rfs_netInit(portNum);
    printf("Client sockfd: %d\n", client->sockfd);

    struct timeval first;
    struct timeval last;
    struct timeval now;

    memset(&last, 0, sizeof(struct timeval));
    getCurrentTime(&first);
    while(1) {
        getCurrentTime(&now);
        if (isTimeOut(&now, &last, SEND_MSG_INTERVAL)) {
            client->sendInitMessage();
            getCurrentTime(&last);
        }

        getCurrentTime(&now);
        if (isTimeOut(&now, &first, 2000)) {
              break;
        } else {
            char buf[HEADER_SIZE];
            memset(buf, 0, HEADER_SIZE);

            if (client->rfs_recvData(100)) {    // client is non-blocking IO
                int status = client->rfs_recvFrom(buf, HEADER_SIZE);
                if (client->isMessageSentByMe(buf))
                    continue;

                if (status > 0) {
                    client->procInitAckMessage(buf);
                }
            }
        }

    }

    printf("Receive serverId count: %d\n", (int)client->serverIds.size());
    if ((int)client->serverIds.size() < numServers)
        return ( ErrorReturn );
    else
        return( NormalReturn );  
}

/* ------------------------------------------------------------------ */

int
OpenFile( char * fileName ) {
    int fd;

    ASSERT( fileName );

#ifdef DEBUG
    printf( "OpenFile: Opening File '%s'\n", fileName );
#endif

    fd = open( fileName, O_WRONLY|O_CREAT, S_IRUSR|S_IWUSR );

#ifdef DEBUG
    if ( fd < 0 )
        perror( "OpenFile" );
#endif

    return( fd );
}

/* ------------------------------------------------------------------ */

int
WriteBlock( int fd, char * buffer, int byteOffset, int blockSize ) {
    //char strError[64];
    int bytesWritten;

    ASSERT( fd >= 0 );
    ASSERT( byteOffset >= 0 );
    ASSERT( buffer );
    ASSERT( blockSize >= 0 && blockSize < MaxBlockLength );

#ifdef DEBUG
    printf( "WriteBlock: Writing FD=%d, Offset=%d, Length=%d\n",
  	fd, byteOffset, blockSize );
#endif

    if ( lseek( fd, byteOffset, SEEK_SET ) < 0 ) {
        perror( "WriteBlock Seek" );
        return(ErrorReturn);
    }

    if ( ( bytesWritten = write( fd, buffer, blockSize ) ) < 0 ) {
        perror( "WriteBlock write" );
        return(ErrorReturn);
    }

    return( bytesWritten );

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

    if ( close( fd ) < 0 ) {
        perror("Close");
        return(ErrorReturn);
    }

    return(NormalReturn);
  }

/*  ------------------------------------------------------------------ */




