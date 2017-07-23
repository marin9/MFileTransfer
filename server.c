#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <unistd.h>
#include <syslog.h>
#include <string.h>
#include <sys/wait.h>
#include <netdb.h>
#include <errno.h>
#include "file.h"
#include "net.h"
#include "mft.h"


pid_t CreateProcess();
void GetOpt(int argc, char **argv);
void RegisterClient(int csock, struct sockaddr_in *addr);
void sigchld_handler();
void SetSigActions();
void SetChildSigActions();
void Daemon();
void term();
void client_terminate();

extern int syslog_on;
char WDIR[NAMELEN];
int MAXCLIENT;
int EWRITE;
int PORT;
int sock;
pid_t *clients;


int main(int argc, char **argv){
	GetOpt(argc, argv);
	//Daemon(); //TODO  rpistartup.sh mft.c
	openlog("MFT_SERVER", LOG_PID, LOG_DAEMON);
	syslog(LOG_INFO, "MFT server started.");
	syslog_on=1;
	SetSigActions();

	int i;
	clients=(pid_t*)malloc(MAXCLIENT*sizeof(pid_t));
	for(i=0;i<MAXCLIENT;++i) clients[i]=-1;

	sock=SocketTCP();
	Bind(sock, PORT);
	Listen(sock);
	
	struct sockaddr_in addr;
	socklen_t len=sizeof(addr);
	
	while(1){
		int csock=Accept(sock, &addr, &len);
		if(csock<0) continue;

		RegisterClient(csock, &addr);
	}
	return 0;
}


void GetOpt(int argc, char **argv){
	MAXCLIENT=3;
	PORT=7999;
	WDIR[0]=0;
	EWRITE=0;
	int c, sum=0;

	while((c=getopt(argc, argv, "p:m:d:w"))!=-1){
    	switch(c){
     		case 'd':
				strncpy(WDIR, optarg, NAMELEN);
				sum+=2;
        		break;
      		case 'p':
       			PORT=atoi(optarg);
				sum+=2;
       			break;
			case 'm':
				MAXCLIENT=atoi(optarg);
				sum+=2;
				break;
			case 'w':
				EWRITE=1;
				sum+=1;
				break;
      		default:
        		printf("Usage: [-p port] [-m max_clients] [-d working_directory] [-w]\n");
				exit(1);
      	}
	}

	if(argc!=(sum+1)){
		printf("Usage: [-p port] [-m max_clients] [-d working_directory] [-w]\n");
		exit(1);
	}

	if(WDIR[0]!=0 && !dirExist(WDIR)){
		printf("Directory not exist.\n");
		exit(1);
	}

	if(WDIR[0]==0){
		WDIR[0]='/';
		WDIR[1]=0;
	}
}

pid_t CreateProcess(){
	int s=fork();
	if(s<0){
		syslog(LOG_ERR, "fork() fail: %s", strerror(errno));
		closelog();
		exit(10);
	}
	else return s;
}

void RegisterClient(int csock, struct sockaddr_in *addr){
	int i;

	for(i=0;i<MAXCLIENT;++i){
		if(clients[i]==-1){
			pid_t pid=CreateProcess();
			if(pid==0){
				close(sock);

				SetChildSigActions();
				ClientHandler(csock, addr, WDIR, EWRITE);

				exit(0);
			}else{
				clients[i]=pid;
				close(csock);
				break;
			}
		}
	}
}

void Daemon(){
	if(CreateProcess()) exit(0);
	if(setsid()<0) exit(1);
	signal (SIGHUP, SIG_IGN);

	if(CreateProcess()) exit(0);
	chdir("/");
	int i;
	for(i=0;i<64;i++) close (i);
}

void SetSigActions(){
	struct sigaction action;
    memset(&action, 0, sizeof(action));
    action.sa_handler=term;

    if(sigaction(SIGTERM, &action, NULL)){
		syslog(LOG_ERR, "sigaction() set SIGTERM action fail: %s", strerror(errno));
		closelog();
		exit(11);
	}

	struct sigaction sa;
	sa.sa_handler=sigchld_handler;
	sigemptyset(&sa.sa_mask);
	sa.sa_flags=SA_RESTART;

	if(sigaction(SIGCHLD, &sa, NULL)==-1){
		syslog(LOG_ERR, "sigaction() set SIGCHLD action fail: %s", strerror(errno));
		closelog();
		exit(12);
	}
}

void SetChildSigActions(){
	struct sigaction action;
    memset(&action, 0, sizeof(action));
    action.sa_handler=client_terminate;

    if(sigaction(SIGTERM, &action, NULL)){
		syslog(LOG_ERR, "sigaction() set child SIGTERM action fail: %s", strerror(errno));
		closelog();
		exit(13);
	}
}

void term(){
	close(sock);
	int i;
	for(i=0;i<MAXCLIENT;++i){
		if(clients[i]!=-1){
			if(kill(clients[i], SIGTERM)!=0){
				syslog(LOG_ERR, "kill() send SIGTERM to child fail: %s", strerror(errno));
			}else{
				int status;
				if(waitpid(clients[i], &status, 0)==-1){
					syslog(LOG_ERR, "waitpid() fail: Wait child process fail: %s", strerror(errno));
				}
			}
		}
	}
	syslog(LOG_INFO, "MFT server finish.");
    closelog();
	exit(0);
}

void client_terminate(){
	exit(0);
}

void sigchld_handler(){
	int i, pid;
	while((pid=waitpid(-1, NULL, WNOHANG))>0){
		for(i=0;i<MAXCLIENT;++i){
			if(clients[i]==pid){
				clients[i]=-1;
			}
		}
	}
}
