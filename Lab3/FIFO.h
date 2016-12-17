#ifndef __FIFO_h
#define __FIFO_h

#include <sys/shm.h>

#include <stdio.h>
#include <stdlib.h>
#define _SIZE 10

typedef struct FIFO
{
    char *arr;
	int amount;
	int read;
	int write;
}FIFO;

void Init(FIFO *val);
int GetAmount(FIFO *val);
char Pop(FIFO *val);
char First(FIFO *val);
void Push(FIFO *val, char c);

#endif
