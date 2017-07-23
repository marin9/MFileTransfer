#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <syslog.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <errno.h>
#include "file.h"
#include "net.h"
#include "mft.h"


void ClientHandler(int sock, struct sockaddr_in *addr, char *wdir, int ewrite){
	char path[NAMELEN];	
	char buffer[REQLEN];

	if(!Recv(sock, buffer, REQLEN)){
		close(sock);
		return;
	}

	if((strlen(wdir)+strlen(buffer+1))>=(NAMELEN-1)) return;
	else{
		strcpy(path, wdir);
		if(path[strlen(path)-1]!='/') strcat(path, "/");
		strcat(path, buffer+1);
	}
	
	if(buffer[0]==READ) ReadFile(sock, path);
	else if(buffer[0]==WRITE) WriteFile(sock, path, ewrite);
	else if(buffer[0]==LIST) SendDir(sock, wdir);
	else if(buffer[0]==REMOVE) ServerRemoveFile(sock, path, ewrite);
	else SendError(sock, "Illegal command");
}

void ReadFile(int sock, char *path){
	//TODO
}

void WriteFile(int sock, char *path, int ewrite){
	//TODO
}

void SendDir(int sock, char *path){
	File *files=listFiles(path);
	
	char buffer[REQLEN];
	File *f=files;
	while(f!=NULL){
		if(f->isDir){
			f=f->next;
			continue;
		}
		
		strcpy(buffer, f->name);
		strcat(buffer, "\t\t\t");
		char size[16];
		sizeToH(f->size, size, 16);
		strcat(buffer, size);
		
		Send(sock, buffer, REQLEN);
		
		f=f->next;
	}
	
	buffer[0]=0;
	Send(sock, buffer, REQLEN);
	
	freeFiles(files);
	close(sock);	
}

void ServerRemoveFile(int sock, char *path, int ewrite){
	if(!ewrite || strstr(path, "..")!=NULL){
		SendError(sock, "Access violation");
		return;
	}
	
	if(!remove(path)){
		struct sockaddr_in addr;
		socklen_t len=sizeof(addr);
		
		if(getsockname(sock, (struct sockaddr*)&addr, &len)==-1){
			syslog(LOG_ERR, "getsockname() fail: %s.\n", strerror(errno));
			
		}else{
			char ip[INET_ADDRSTRLEN];
			inet_ntop(AF_INET, &(addr.sin_addr), ip, INET_ADDRSTRLEN);
			syslog(LOG_INFO, "%s:remove:%s", ip, path);
		}	
		
		char ack[REQLEN]={0};
		Send(sock, ack, REQLEN);
		close(sock);	
		
	}else{
		SendError(sock, strerror(errno));
	}
}

void SendError(int sock, char *msg){
	char buffer[REQLEN];
	strcpy(buffer, msg);
	Send(sock, buffer, REQLEN);
	close(sock);
}




void SendFile(struct sockaddr_in *addr, char *name){
	//TODO
}

void ReceiveFile(struct sockaddr_in *addr, char *name){
	//TODO
}

void RemoveFile(struct sockaddr_in *addr, char *name){	
	int sock=SocketTCP();

	int s=connect(sock, (struct sockaddr*)addr, sizeof(struct sockaddr_in));
	if(s!=0){
		printf("\x1B[33mWARNING:\x1B[0m %s\n", strerror(errno));
		return;
	}
	
	char buffer[REQLEN];
	buffer[0]=REMOVE;
	strcpy(buffer+1, name);

	if(!Send(sock, buffer, REQLEN)){
		printf("\x1B[33mWARNING:\x1B[0m send() %s\n", strerror(errno));
		close(sock);
		return;
	}
		
	if(!Recv(sock, buffer, REQLEN)){
		printf("\x1B[33mWARNING:\x1B[0m recv() %s\n", strerror(errno));
		close(sock);
		return;
	}
		
	if(buffer[0]!=0) printf("\x1B[33mWARNING:\x1B[0m %s.\n", buffer);
	else printf("\x1B[32mFINISH:\x1B[0m File: %s removed.\n", name);
	
	close(sock);
}

void PrintFiles(struct sockaddr_in *addr){
	int sock=SocketTCP();
	
	int s=connect(sock, (struct sockaddr*)addr, sizeof(struct sockaddr_in));
	if(s!=0){
		printf("\x1B[33mWARNING:\x1B[0m %s\n", strerror(errno));
		return;
	}
	
	char buffer[REQLEN];
	buffer[0]=LIST;
	
	if(!Send(sock, buffer, REQLEN)){
		printf("\x1B[33mWARNING:\x1B[0m send() %s\n", strerror(errno));
		close(sock);
		return;
	}
	
	while(1){
		if(!Recv(sock, buffer, REQLEN)){
			printf("\x1B[33mWARNING:\x1B[0m recv() %s\n", strerror(errno));
			close(sock);
			return;
		}
		
		if(buffer[0]==0) break;
		printf("%s\n", buffer);
	}
	
	printf("\x1B[32mFINISH\x1B[0m \n");
	close(sock);
}

