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


void ClientHandler(int sock, char *wdir, int ewrite){
	char path[NAMELEN];	
	char buffer[REQLEN];
	int noffset=9;

	if(!Recv(sock, buffer, REQLEN)){
		close(sock);
		return;
	}

	if((strlen(wdir)+strlen(buffer+noffset))>=(NAMELEN-2)) return;
	else{
		strcpy(path, wdir);
		if(path[strlen(path)-1]!='/') strcat(path, "/");
		strcat(path, buffer+noffset);
	}
	
	long size=*((long*)(buffer+1));
	
	if(buffer[0]==READ) ReadFile(sock, path);
	else if(buffer[0]==WRITE) WriteFile(sock, path, size, ewrite);
	else if(buffer[0]==LIST) SendDir(sock, wdir);
	else if(buffer[0]==REMOVE) ServerRemoveFile(sock, path, ewrite);
	else SendError(sock, "Illegal command");
}

void ReadFile(int sock, char *path){
	if(strstr(path, "..")!=NULL){
		SendError(sock, "Access violation");
		return;
	}
	
	FILE *file=fopen(path, "rb");
	if(file==NULL){
		SendError(sock, strerror(errno));
		return;	
	}
	
	fseek(file, 0L, SEEK_END);
	long size=ftell(file);
	fseek(file, 0L, SEEK_SET);
	
	char buffer[BUFFLEN];
	buffer[0]=OK;
	*((long*)(buffer+1))=size;
	Send(sock, buffer, REQLEN);
	
	while(!feof(file)){
		int n=fread(buffer, 1, BUFFLEN, file);
		if(n==0) break;
		if(!Send(sock, buffer, n)) break;
	}	
	
	close(sock);
	fclose(file);
}

void WriteFile(int sock, char *path, long size, int ewrite){
	if(!ewrite || strstr(path, "..")!=NULL){
		SendError(sock, "Access violation");
		return;
	}
	
	if(isFileExist(path)){
		SendError(sock, "File already exist");
		return;
	}
	
	FILE *file=fopen(path, "wb");
	if(file==NULL){
		SendError(sock, strerror(errno));
		return;
	}
	
	char buffer[BUFFLEN];
	buffer[0]=OK;
	Send(sock, buffer, REQLEN);
	
	while(size!=0){
		int n=recv(sock, buffer, BUFFLEN, MSG_NOSIGNAL);
		if(n<1){
			close(sock);
			fclose(file);
			remove(path);
			return;
		}
		
		int w=fwrite(buffer, 1, n, file);
		if(w!=n){
			SendError(sock, strerror(errno));
			fclose(file);
			remove(path);
			return;	
		}
		
		size-=n;
	}
	
	LogAction(sock, path, WRITE);
	close(sock);
	fclose(file);
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
		LogAction(sock, path, REMOVE);
		
		char buffer[REQLEN];
		buffer[0]=OK;
		Send(sock, buffer, REQLEN);
		close(sock);	
		
	}else{
		SendError(sock, strerror(errno));
	}
}

void LogAction(int sock, char* path, int action){
	struct sockaddr_in addr;
	socklen_t len=sizeof(addr);
		
	if(getsockname(sock, (struct sockaddr*)&addr, &len)==-1){
		syslog(LOG_ERR, "getsockname() fail: %s.\n", strerror(errno));
			
	}else{
		char ip[INET_ADDRSTRLEN];
		inet_ntop(AF_INET, &(addr.sin_addr), ip, INET_ADDRSTRLEN);
		if(action==WRITE) syslog(LOG_INFO, "%s:write:%s", ip, path);
		else if(action==REMOVE) syslog(LOG_INFO, "%s:remove:%s", ip, path);
	}
}

void SendError(int sock, char *msg){
	char buffer[REQLEN];
	buffer[0]=ERR;
	strcpy(buffer+1, msg);
	Send(sock, buffer, REQLEN);
	close(sock);
}




void SendFile(struct sockaddr_in *addr, char *name){
	FILE *file=fopen(name, "rb");
	if(file==NULL){
		printf("\x1B[33mWARNING:\x1B[0m %s\n", strerror(errno));
		return;
	}
	
	int sock=SocketTCP();
	
	int s=connect(sock, (struct sockaddr*)addr, sizeof(struct sockaddr_in));
	if(s!=0){
		printf("\x1B[33mWARNING:\x1B[0m %s\n", strerror(errno));
		return;
	}
	
	fseek(file, 0L, SEEK_END);
	long size=ftell(file);
	fseek(file, 0L, SEEK_SET);
	
	char buffer[BUFFLEN];
	int noffset=9;
	buffer[0]=WRITE;
	*((long*)(buffer+1))=size;
	strcpy(buffer+noffset, name);
	
	if(!Send(sock, buffer, REQLEN)){
		printf("\x1B[33mWARNING:\x1B[0m %s\n", strerror(errno));
		close(sock);
		return;
	}
	
	if(!Recv(sock, buffer, REQLEN)){
		printf("\x1B[33mWARNING:\x1B[0m %s\n", strerror(errno));
		close(sock);
		return;
	}
	
	if(buffer[0]!=OK){
		printf("\x1B[33mWARNING:\x1B[0m %s\n", buffer+1);
		close(sock);
		return;
	}
	
	long Size=size;	
	int t=0;
	while(size!=0){
		int n=fread(buffer, 1, BUFFLEN, file);
		if(n<1){
			printf("\x1B[33mWARNING:\x1B[0m %s\n", strerror(errno));
			close(sock);
			fclose(file);
			return;
		}
		
		if(!Send(sock, buffer, n)){
			printf("\x1B[33mWARNING:\x1B[0m Connection closed.\n");
			close(sock);
			fclose(file);
			return;
		}
		size-=n;
		
		if(t%500==0){
			printf("\r%.2f %%", (float)(Size-size)*100/Size);
			fflush(stdout);
			t=0;
		}
		++t;
	}
	
	printf("\r\x1B[32mFINISH\x1B[0m \n");
	close(sock);
	fclose(file);
}

void ReceiveFile(struct sockaddr_in *addr, char *name){
	int sock=SocketTCP();
	
	int s=connect(sock, (struct sockaddr*)addr, sizeof(struct sockaddr_in));
	if(s!=0){
		printf("\x1B[33mWARNING:\x1B[0m %s\n", strerror(errno));
		return;
	}
	
	char buffer[BUFFLEN];
	int noffset=9;
	buffer[0]=READ;
	strcpy(buffer+noffset, name);
	
	if(!Send(sock, buffer, REQLEN)){
		printf("\x1B[33mWARNING:\x1B[0m %s\n", strerror(errno));
		close(sock);
		return;
	}
	
	if(!Recv(sock, buffer, REQLEN)){
		printf("\x1B[33mWARNING:\x1B[0m Connection closed.\n");
		close(sock);
		return;
	}
	
	if(buffer[0]!=OK){
		printf("\x1B[33mWARNING:\x1B[0m %s\n", buffer+1);
		close(sock);
		return;
	}
	
	long size=*((long*)(buffer+1));
	
	if(isFileExist(name)){
		printf("\x1B[33mWARNING:\x1B[0m  File already exist.\n");
		close(sock);
		return;
	}
	
	FILE *file=fopen(name, "wb");
	if(file==NULL){
		printf("\x1B[33mWARNING:\x1B[0m %s\n", strerror(errno));
		close(sock);
		return;
	}
	
	long Size=size;
	int t=0;
	while(size!=0){
		int n=recv(sock, buffer, BUFFLEN, MSG_NOSIGNAL);
		if(n<1){
			printf("\x1B[33mWARNING:\x1B[0m Connection closed.\n");
			close(sock);
			fclose(file);
			remove(name);
			return;
		}
		
		size-=n;		
		if(fwrite(buffer, 1, n, file)!=(unsigned int)n){
			printf("\x1B[33mWARNING:\x1B[0m %s\n", strerror(errno));
			close(sock);
			fclose(file);
			remove(name);
			return;
		}
					
		if(t%500==0){
			printf("\r%.2f %%", (float)(Size-size)*100/Size);
			fflush(stdout);
			t=0;
		}
		++t;
	}
	
	printf("\r\x1B[32mFINISH\x1B[0m \n");
	close(sock);
	fclose(file);
}

void RemoveFile(struct sockaddr_in *addr, char *name){	
	int sock=SocketTCP();

	int s=connect(sock, (struct sockaddr*)addr, sizeof(struct sockaddr_in));
	if(s!=0){
		printf("\x1B[33mWARNING:\x1B[0m %s\n", strerror(errno));
		return;
	}
	
	char buffer[REQLEN];
	int noffset=9;
	buffer[0]=REMOVE;
	strcpy(buffer+noffset, name);

	if(!Send(sock, buffer, REQLEN)){
		printf("\x1B[33mWARNING:\x1B[0m %s\n", strerror(errno));
		close(sock);
		return;
	}
		
	if(!Recv(sock, buffer, REQLEN)){
		printf("\x1B[33mWARNING:\x1B[0m %s\n", strerror(errno));
		close(sock);
		return;
	}
		
	if(buffer[0]!=OK) printf("\x1B[33mWARNING:\x1B[0m %s.\n", buffer+1);
	else printf("\x1B[32mFINISH:\x1B[0m %s removed.\n", name);
	
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
		printf("\x1B[33mWARNING:\x1B[0m %s\n", strerror(errno));
		close(sock);
		return;
	}
	
	while(1){
		if(!Recv(sock, buffer, REQLEN)){
			printf("\x1B[33mWARNING:\x1B[0m Connection closed.\n");
			close(sock);
			return;
		}
		
		if(buffer[0]==0) break;
		printf("%s\n", buffer);
	}
	
	printf("\x1B[32mFINISH\x1B[0m \n");
	close(sock);
}

