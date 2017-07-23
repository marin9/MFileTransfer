CC=gcc
FLAGS=-Wall -Wextra -c
OBJC=client.o net.o file.o mft.o
OBJS=server.o net.o file.o mft.o


all : mft mftserver

mft : $(OBJC)
	$(CC) $(OBJC) -o mft
	
mftserver : $(OBJS)
	$(CC) $(OBJS) -o mftserver


client.o : client.c
	$(CC) $(FLAGS) client.c

net.o : net.c net.h
	$(CC) $(FLAGS) net.c
	
file.o : file.c file.h
	$(CC) $(FLAGS) file.c
	
mft.o : mft.c mft.h
	$(CC) $(FLAGS) mft.c


clean :
	rm *.o
	rm mft
	rm mftserver
