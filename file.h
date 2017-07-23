#ifndef FILE_H
#define FILE_H

#define NAMELEN 256

typedef struct File File;
struct File{
	char name[NAMELEN];	
	char isDir;
	char isExec;
	unsigned long size;
	File *next;
};


void addToStack(File **root, char *name, char isDir, char isExec, unsigned long size);
File* listFiles(char *path);
int isExecutable(char *path);
int isDir(char *path);
int dirExist(char *dir);
long fileSize(char *path);
void freeFiles(File *files);
int isFileExist(char *path);
void sizeToH(unsigned long size, char *buff, size_t max_len);

#endif
