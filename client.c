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

    /****************************************************/
    /* Initialize network access, local state, etc.     */
    /****************************************************/

    int socket = rfs_netInit(portNum);
    srand (time(NULL));
    uint32_t nodeId = (uint32_t)rand();
    client = new ClientInstance(packetLoss, socket, nodeId, numServers);

    struct timeval first;
    struct timeval last;
    struct timeval now;

    getCurrentTime(&first);
    while(1) {
        printf("in loop\n");

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

            int status = rfs_recvFrom(socket, buf, HEADER_SIZE);
            client->procInitAckMessage(buf);
        }

    }

    printf("serverId count: %d\n", client->serverIds.size());
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

/* ------------------------------------------------------------------ */




