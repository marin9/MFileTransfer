#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <syslog.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <errno.h>
#include "net.h"

int syslog_on=0;


int SocketTCP(){
	int sock;
	if((sock=socket(PF_INET, SOCK_STREAM, 0))==-1){
		if(syslog_on){
			syslog(LOG_ERR, "socket() return %d: %s", sock, strerror(errno));
			closelog();
		}
		else printf("\x1B[31mERROR:\x1B[0m socket() return %d: %s\n", sock, strerror(errno));
		exit(20);
	}
	
	int on=1;
	if(setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, &on, sizeof(int))==-1){
		if(syslog_on){
			syslog(LOG_ERR, "setsockopt() reuse fail: %s\n", strerror(errno));
			closelog();
		}
		else printf("\x1B[31mERROR:\x1B[0m setsockopt() reuse fail: %s\n", strerror(errno)); 
		exit(21);
	}
	
	struct timeval timeout;      
	timeout.tv_sec=TIMEOUT;
	timeout.tv_usec=0;
	if(setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, (char*)&timeout, sizeof(timeout))<0){
		if(syslog_on){
			syslog(LOG_ERR, "setsockopt() timeout fail: %s\n", strerror(errno));
			closelog();
		}
		else printf("\x1B[31mERROR:\x1B[0m setsockopt() timeout fail: %s.\n", strerror(errno));
		exit(22);
	}
	return sock;
}

void Bind(int sock, unsigned short port){
	struct sockaddr_in addr;	
	addr.sin_family=AF_INET;
	addr.sin_port=htons(port);
	addr.sin_addr.s_addr=INADDR_ANY;

	if(bind(sock, (struct sockaddr*)&addr, sizeof(addr))<0){
		if(syslog_on){
			syslog(LOG_ERR, "bind() fail: %s\n", strerror(errno));
			closelog();
		}
		else printf("\x1B[31mERROR:\x1B[0m bind() fail: %s\n", strerror(errno));
		exit(23);
	}
}

void Listen(int sock){
	if(listen(sock, 5)==-1){
		if(syslog_on){
			syslog(LOG_ERR, "listen() fail: %s\n", strerror(errno));
			closelog();
		}
		else printf("\x1B[31mERROR:\x1B[0m listen() fail: %s\n", strerror(errno));
		exit(24);
	}	
}

int Accept(int sock, struct sockaddr_in *addr, socklen_t *len){
	int csock;
	if((csock=accept(sock, (struct sockaddr*)addr, len))==-1){
		if(errno==EAGAIN || errno==EINTR) return -1;
		if(syslog_on){
			syslog(LOG_ERR, "accept() fail: %s\n", strerror(errno));
			closelog();
		}
		else printf("\x1B[31mERROR:\x1B[0m accept() fail: %s\n", strerror(errno));
		exit(25);
	}
	return csock;
}

int Send(int sock, char* buff, int len){
	char *b=(char*)buff;

    while(len>0){
        int n=send(sock, b, len, MSG_NOSIGNAL);
        
        if(n<1) return 0;
        b+=n;
        len-=n;
    }
    return 1;
}

int Recv(int sock, char* buff, int len){
	char *b=(char*)buff;
	
    while(len>0){
        int n=recv(sock, b, len, MSG_NOSIGNAL);
        
        if(n<1) return 0;
        b+=n;
        len-=n;
    }
    return 1;
}


int EqualsAddr(struct sockaddr_in* addr1, struct sockaddr_in* addr2){
	unsigned short port1=addr1->sin_port;
	unsigned short port2=addr2->sin_port;
	
	if(port1!=port2) return 0;
	
	char host1[NI_MAXHOST];
	char host2[NI_MAXHOST];	
	inet_ntop(AF_INET, &(addr1->sin_addr), host1, INET_ADDRSTRLEN);
	inet_ntop(AF_INET, &(addr2->sin_addr), host2, INET_ADDRSTRLEN);
	
	if(strcmp(host1, host2)!=0) return 0;
	else return 1;
}

void GetIpFromName(char *name, struct sockaddr_in* addr){ 
    struct addrinfo hints, *res;
	
	memset(addr, 0, sizeof(struct sockaddr_in));
	memset(&hints, 0, sizeof(hints));
	hints.ai_family=AF_INET; 
	hints.ai_flags|=AI_CANONNAME;

	int s;
	if((s=getaddrinfo(name, NULL, &hints, &res))!=0){
		printf("\x1B[33mWARNING:\x1B[0m %s\n", gai_strerror(s));
		return;
	}
	
	if(res){
		memcpy(&(addr->sin_addr), &((struct sockaddr_in*)res->ai_addr)->sin_addr, sizeof(struct sockaddr_in));
		freeaddrinfo(res);	
	}	
}

void GetNameFromIP(struct sockaddr_in *addr, char *name){
    memset(name, 0, NI_MAXHOST); 

	int res;
    if((res=getnameinfo((struct sockaddr*)addr, sizeof(struct sockaddr), name, NI_MAXHOST, NULL, 0, NI_NAMEREQD))!=0){
        printf("\x1B[33mWARNING:\x1B[0m %s\n", gai_strerror(res));
        name[0]=0;     
    }
}

