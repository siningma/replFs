include <stdio.h>
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
static int nservers = 2;

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

int
main() {
     InitReplFs(port, packetloss, nservers);
     appl2();
}
