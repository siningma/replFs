
CFLAGS	= -g -Wall -DSUN
# CFLAGS	= -g -Wall -DDEC
CC	= g++
CCF	= $(CC) $(CFLAGS)

H	= .
C_DIR	= .

INCDIR	= -I$(H)
LIBDIRS = -L$(C_DIR)
LIBS    = -lclientReplFs

CLIENT_OBJECTS = client.o clientInstance.o network.o
SERVER_OBJECTS = serverInstance.o network.o

all:	appl	replFsServer

appl:	appl.o $(C_DIR)/libclientReplFs.a
	$(CCF) -o appl appl.o $(LIBDIRS) $(LIBS)

appl.o:	appl.c client.h appl.h
	$(CCF) -c $(INCDIR) appl.c

$(C_DIR)/libclientReplFs.a:	$(CLIENT_OBJECTS)
	ar cr libclientReplFs.a $(CLIENT_OBJECTS)
	ranlib libclientReplFs.a

client.o:	client.c client.h clientInstance.h
	$(CCF) -c $(INCDIR) client.c

clientInstance.o: 	clientInstance.cpp clientInstance.h network.h
	$(CCF) -c $(INCDIR) clientInstance.cpp	

replFsServer:	$(SERVER_OBJECTS)	
	$(CCF) -o replFsServer $(SERVER_OBJECTS) $(LIBDIRS) $(LIBS)

serverInstance.o: 	serverInstance.cpp server.h network.h
	$(CCF) -c $(INCDIR) serverInstance.cpp

network.o:	network.cpp network.h
	$(CCF) -c $(INCDIR) network.cpp

clean:
	rm -f appl replFsServer *.o *.a

