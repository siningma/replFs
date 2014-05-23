#include <stdio.h>
#include <string.h>

#include <client.h>
#include <appl.h>

/* ------------------------------------------------------------------ */
static int 
openFile(char *file)
{
       int fd = OpenFile(file);
         if (fd < 0) printf("OpenFile(%s): failed (%d)\n", file, fd);
           return fd; 
}

static int 
commit(int fd) 
{
       int result = Commit(fd);
         if (result < 0) printf("Commit(%d): failed (%d)\n", fd, result);
           return fd; 
}

static int 
closeFile(int fd) 
{
       int result = CloseFile(fd);
         if (result < 0) printf("CloseFile(%d): failed (%d)\n", fd, result);
           return fd; 
}

// The port number to pass to InitReplFs().
// It is set by the -port command-line argument.
static int port = 44023;

// The number of servers to pass to InitReplFs().
// It is set by the -servers command-line argument.
static int nservers = 4;

// The packet loss percentage to pass to InitReplFs().
// It is set by the -packetloss command-line argument.
static int packetloss = 20; 

static void
appl1() {
   // simple case - just commit a single write update.

    int fd; 
    int retVal;

    fd = openFile( "file1" );
    retVal = WriteBlock( fd, "abcdefghijkl", 0, 12 );
    retVal = commit( fd );
    retVal = closeFile( fd );
}

static void
appl2() {
    // simple case - just commit a single write update using closeFile.

    int fd; 
    int retVal;

    fd = openFile( "file2" );
    retVal = WriteBlock( fd, "abcdefghijkl", 0, 12 );
    retVal = closeFile( fd ); // should commit the changes
}

static void
appl3() {

    //  simple case - just abort a single update on a new file
    //  the file  should not be in the mount directory at the end given
    //  it was not existing to start with. Script should check that


    int fd;
    int retVal;

    fd = openFile( "file3" );
    retVal = WriteBlock( fd, "abcdefghijkl", 0, 12 );
    Abort( fd );
    retVal = closeFile( fd ); // should abort and remove the file
}

static void
appl4() {

    //  simple case - just do a single commit and then single abort
    //  the file should be in the  mount directory with the committed
    //  write only thus no "mno"in it


    int fd;
    int retVal;

    fd = openFile( "file4" );
    retVal = WriteBlock( fd, "abcdefghijkl", 0, 12 );
    retVal = commit( fd );
    retVal = WriteBlock( fd, "mno", 12, 3 );
    Abort( fd );
    retVal = closeFile( fd );
}

static void
appl5() {

    // checks simple overwrite case

    int fd;
    int retVal;

    fd = openFile( "file5" );
    retVal = WriteBlock( fd, "aecdefghijkl", 0, 12 );
    retVal = WriteBlock( fd, "b", 1, 1 );
    retVal = commit( fd );
    retVal = closeFile( fd );
}

static void
appl6() {
    // try out commit - abort with multiple writes
    // No XXX should be in the file

    int fd;
    int retVal;
    int i;
    char * commitStr = "deadbeef";
    char * abortStr = "XXX";

    int commitStrLength = strlen( commitStr );
    int abortStrLength = strlen( abortStr );


    fd = openFile( "file6" );
    for (i = 0; i < 29; i++)
        retVal = WriteBlock( fd, commitStr, i * commitStrLength ,
                             commitStrLength );
    retVal = commit( fd );

    for (i = 0; i < 7; i++)
        retVal = WriteBlock( fd, abortStr, i * abortStrLength ,
                             abortStrLength );
    Abort( fd );

    retVal = closeFile( fd );

}


static void
appl7() {
    // not provided
}

static void
appl8() {
    // multiple openFiles of the same file. As a consequence,
    // this also checks that
    // when a file exists in the mount directory, they should openFile it
    // and not create a new one.

    int fd;
    int retVal;
    int i;
    char commitStrBuf[512];

    for( i = 0; i < 512; i++ )
        commitStrBuf[i] = '1';

    fd = openFile( "file8" );

    // write first transaction starting at offset 512
    for (i = 0; i < 50; i++)
        retVal = WriteBlock( fd, commitStrBuf, 512 + i * 512 , 512 );

    retVal = commit( fd );
    retVal = closeFile( fd );

    for( i = 0; i < 512; i++ )
        commitStrBuf[i] = '2';

    fd = openFile( "file8" );

    // write second transaction starting at offset 0
    retVal = WriteBlock( fd, commitStrBuf, 0 , 512 );

    retVal = commit( fd );
    retVal = closeFile( fd );


    for( i = 0; i < 512; i++ )
        commitStrBuf[i] = '3';

    fd = openFile( "file8" );

    // write third transaction starting at offset 50*512
    for (i = 0; i < 100; i++)
        retVal = WriteBlock( fd, commitStrBuf, 50 * 512 + i * 512 , 512 );

    retVal = commit( fd );
    retVal = closeFile( fd );
}



static void
appl9() {
    // not provided
}


// void
// static appl10() {
//     // test that if a server is crashed at write updates time,
//     // the library aborts the transaction at commit time
//     // the file should have only 0's in it.

//     int fd;
//     int retVal;
//     int i;
//     char commitStrBuf[512];

//     for( i = 0; i < 256; i++ )
//         commitStrBuf[i] = '0';

//     fd = openFile( "file10" );

//     // zero out the file first
//     for (i = 0; i < 100; i++)
//         retVal = WriteBlock( fd, commitStrBuf, i * 256 , 256 );

//     retVal = commit( fd );
//     retVal = closeFile( fd );

//     fd = openFile( "file10" );
//     retVal = WriteBlock( fd, "111111", 0 , 6 );

//     // KILL ONE OF THE  SERVERS HERE BY ISSUING A SYSTEM CALL
//     log("killing server\n");
//     stopServer(0);
//     log("committing\n");

//     retVal = commit( fd ); // this should return in abort
//     retVal = closeFile( fd );
// }



// Some lower priority error cases for coding sanity

static void
appl11() {

    // checks that a WriteBlock to a non-openFile file descriptor
    // is skipped
    // There should be only 12 0's at the end in the file

    int fd;
    int retVal;

    fd = openFile( "file11" );
    retVal = WriteBlock( fd, "000000000000", 0, 12 );

    // the following should not be performed due to wrong fd
    retVal = WriteBlock( fd + 1, "abcdefghijkl", 0, 12 );

    retVal = commit( fd );
    retVal = closeFile( fd );

}


static void
appl12() {

    // checks that funky order of operations does not cause crash or
    // bad behavior
    // there should be only 12 0's in the file

    int fd;
    int retVal;

    fd = openFile( "file12" );
    retVal = WriteBlock( fd, "000000000000", 0, 12 );
    retVal = closeFile( fd );

    // the following should not be performed due to file not openFile
    retVal = WriteBlock( fd, "abcdefghijkl", 0, 12 );
    retVal = commit( fd );
    retVal = closeFile( fd );

}

static void
appl13() {
    // not provided!
}

int
main() {
    InitReplFs(port, packetloss, nservers);
    appl1();

    InitReplFs(port, packetloss, nservers);
    appl2();

    InitReplFs(port, packetloss, nservers);
    appl3();

    InitReplFs(port, packetloss, nservers);
    appl4();

    InitReplFs(port, packetloss, nservers);
    appl5();

    InitReplFs(port, packetloss, nservers);
    appl6();

    InitReplFs(port, packetloss, nservers);
    appl8();

    InitReplFs(port, packetloss, nservers);
    appl11();

    InitReplFs(port, packetloss, nservers);
    appl12();
}
