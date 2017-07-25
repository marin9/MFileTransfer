#ifndef MFT_H
#define MFT_H

#define READ	0
#define WRITE	1
#define LIST	2
#define REMOVE	3

#define OK		4
#define ERR		5

#define BUFFLEN		16384
#define REQLEN		256


void ClientHandler(int sock, char *wdir, int ewrite);
void ReadFile(int sock, char *path);
void WriteFile(int sock, char *path, long size, int ewrite);
void SendDir(int sock, char *path);
void ServerRemoveFile(int sock, char *path, int ewrite);
void ServerRemoveFile(int sock, char *path, int ewrite);
void LogAction(int sock, char* path, int action);
void SendError(int sock, char *msg);


void SendFile(struct sockaddr_in *addr, char *name);
void ReceiveFile(struct sockaddr_in *addr, char *name);
void RemoveFile(struct sockaddr_in *addr, char *name);
void PrintFiles(struct sockaddr_in *addr);

#endif
