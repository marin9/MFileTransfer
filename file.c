#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>
#include <unistd.h>
#include <sys/stat.h>
#include "file.h"


void addToStack(File **root, char *name, char isDir, char isExec, unsigned long size){
	File *f=(File*)malloc(sizeof(File));
	strncpy(f->name, name, NAMELEN);
	f->size=size;
	f->isDir=isDir;
	f->isExec=isExec;
	f->next=(*root);
	*root=f;
}

File* listFiles(char *path){
	DIR *dir=opendir(path);
	
	if(dir==NULL) return NULL;

	File *stack=NULL;	
	struct dirent *ent;
	char fullPath[NAMELEN];

	while((ent=readdir(dir))!=NULL){
		if(ent->d_name[0]=='.') continue;
		snprintf(fullPath, NAMELEN, "%s/%s", path, ent->d_name);

		if(isDir(fullPath)) addToStack(&stack, ent->d_name, 1, 0, 0);
		else addToStack(&stack, ent->d_name, 0,isExecutable(fullPath) , fileSize(fullPath));
    }
    
    closedir(dir);
    return stack;
}

void freeFiles(File *files){
	if(files==NULL) return;

	do{
		File *f=files->next;
		free(files);
		files=f;
	}while(files!=NULL);
}

long fileSize(char *path){
	struct stat st;
	if(stat(path, &st)<0) return -1;
	return st.st_size;
}

int isDir(char *path){
	struct stat st;
	if(stat(path, &st)<0) return -1;
	return S_ISDIR(st.st_mode);
}

int isExecutable(char *path){ 
	struct stat st;
	if(stat(path, &st)<0) return -1;
	return st.st_mode & S_IXUSR;
}

int dirExist(char *dir){
	DIR* d=opendir(dir);
	if(d){
    	closedir(d);
		return 1;
	}else{
		return 0;
	}
}

int isFileExist(char *path){
	int s=access(path, F_OK);
	
	if(!s) return 1;		
	else return 0;	
}

void sizeToH(unsigned long size, char *buff, size_t max_len){
	char mj[3]={0, 'B', 0};
	double s;

	if(size>1000000000){
		s=(double)size/1000000000;
		mj[0]='G';

	}else if(size>1000000){
		s=(double)size/1000000;
		mj[0]='M';

	}else if(size>1000){
		s=(double)size/1000;
		mj[0]='k';

	}else{
		s=size;
		mj[0]=' ';
	}
	snprintf(buff, max_len, "%4.1f %s", s, mj);
}
