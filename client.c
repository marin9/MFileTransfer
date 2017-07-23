#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include "net.h"
#include "mft.h"

#define PORT		7999
#define CM_MAXLEN	128

#define CM_HELP		0
#define CM_PORT		1
#define CM_SERVER	2
#define CM_STATUS	3
#define CM_LIST		4
#define CM_GET		5
#define CM_PUT		6
#define CM_REMOVE	7
#define CM_QUIT		8


int GetCommand(char *buff);
void Help();
void Port(struct sockaddr_in *addr, char *buff);		
void Server(struct sockaddr_in *addr, char *hostname, char *buff);
void Status(struct sockaddr_in addr, char *hostname);


int main(){
	int command;
	char buffer[CM_MAXLEN];
	char hostname[NI_MAXHOST];
	struct sockaddr_in addr;
	
	memset(buffer, 0, sizeof(buffer));
	memset(hostname, 0, sizeof(hostname));
	memset(&addr, 0, sizeof(struct sockaddr_in));
	addr.sin_family=AF_INET;	
	addr.sin_port=htons(7999);
	inet_pton(AF_INET, "127.0.0.1", &(addr.sin_addr));
	
	while(1){
		printf("\n>");		
		scanf("%s", buffer);
		command=GetCommand(buffer);
		
		if(command==CM_HELP) Help();
		else if(command==CM_PORT) Port(&addr, buffer);			
		else if(command==CM_SERVER) Server(&addr, hostname, buffer);
		else if(command==CM_STATUS) Status(addr, hostname);
		else if(command==CM_LIST) PrintFiles(&addr);
		else if(command==CM_GET) ReceiveFile(&addr, buffer);
		else if(command==CM_PUT) SendFile(&addr, buffer);
		else if(command==CM_REMOVE) RemoveFile(&addr, buffer);
		else if(command==CM_QUIT) break;
		else printf("Illegal command. Type 'help' for more info.\n");
	}
	return 0;
}


int GetCommand(char *buff){
	char command[11];
	int i, j;

	i=0;
	while(buff[i]!=':' && buff[i]!=0){
		if(i>=10) return -1;
		if(buff[i]>='A' && buff[i]<='Z') command[i]=buff[i]+32;
		else command[i]=buff[i];
		++i;
	}
	command[i]=0;
	++i;

	j=0;
	while(buff[i]!=0){
		if(i>=CM_MAXLEN-1) return -1;
		buff[j]=buff[i];
		++i;
		++j;
	}
	buff[j]=0;
	
	if(strcmp(command, "help")==0) return CM_HELP;
	else if(strcmp(command, "port")==0) return CM_PORT;
	else if(strcmp(command, "host")==0) return CM_SERVER;
	else if(strcmp(command, "stat")==0) return CM_STATUS;
	else if(strcmp(command, "ls")==0) return CM_LIST;
	else if(strcmp(command, "get")==0) return CM_GET;
	else if(strcmp(command, "put")==0) return CM_PUT;
	else if(strcmp(command, "rm")==0) return CM_REMOVE;
	else if(strcmp(command, "quit")==0) return CM_QUIT;
	else return -1;
}

void Help(){
	printf("\n");
	printf("port:[port] - set server port\n");
	printf("host:[hostname|ip] - set server address\n");
	printf("stat - print info (hostname, ip, port)\n");
	printf("ls - list server directory\n");
	printf("get:[filename] - get file 'filename'\n");
	printf("put:[filename] - put on server file 'filename'\n");
	printf("rm:[filename] - remove from server file 'filename'\n");
	printf("quit - exit from program\n");
	printf("Example: server:7999\n");
}

void Port(struct sockaddr_in *addr, char *buff){
	addr->sin_port=htons((unsigned short)atoi(buff));
}
		
void Server(struct sockaddr_in *addr, char *hostname, char *buff){
	int i;
	unsigned short port=ntohs(addr->sin_port);
	
	for(i=0;i<CM_MAXLEN || buff[i]!=0;++i){
		if((buff[i]>='a' && buff[i]<='z') || (buff[i]>='A' && buff[i]<='Z')){		
			strcpy(hostname, buff);
			
			GetIpFromName(buff, addr);
			addr->sin_family=AF_INET;
			addr->sin_port=htons(port);	
			return;
		}
	}
	
	addr->sin_family=AF_INET;
	inet_pton(AF_INET, buff, &(addr->sin_addr));
	GetNameFromIP(addr, hostname);
	addr->sin_port=htons(port);	
}

void Status(struct sockaddr_in addr, char *hostname){
	char ip[INET_ADDRSTRLEN];
	inet_ntop(AF_INET, &(addr.sin_addr), ip, sizeof(addr));
	
	printf("Host:    %s\n", hostname);
	printf("Host IP: %s\n", ip);
	printf("Port:    %d\n\n", ntohs(addr.sin_port));
}

