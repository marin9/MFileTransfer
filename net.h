#ifndef NET_H
#define NET_H

#define TIMEOUT		5


int SocketTCP();
void Bind(int sock, unsigned short port);
void Listen(int sock);
int Accept(int sock, struct sockaddr_in *addr, socklen_t *len);

int Send(int sock, char* buff, int len);
int Recv(int sock, char* buff, int len);

int EqualsAddr(struct sockaddr_in* addr1, struct sockaddr_in* addr2);
void GetIpFromName(char *name, struct sockaddr_in* addr);
void GetNameFromIP(struct sockaddr_in *addr, char *name);

#endif
