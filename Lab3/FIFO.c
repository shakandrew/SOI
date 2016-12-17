#include "FIFO.h"
#include <stdio.h>
#include <stdlib.h>

void Init(FIFO *val)
{
    val->arr = (char*)malloc(sizeof(char)*_SIZE);
    val->amount = 0;
    val->read = 0;
    val->write = 0;
}

void Push(FIFO *val, char c)
{
	if(val->amount == _SIZE)
	{
		printf("Queue is full, cannot push!\n");
		return;
	}
	val->arr[val->write] = c;
	val->write = (val->write+1) % _SIZE;
	val->amount++;
	return;
}

char Pop(FIFO *val)
{
	char c;
	if(val->amount == 0)
	{
		printf("Queue is empty, cannot pop!\n");
		return 0;
	}
	c = val->arr[val->read];
	val->read = (val->read + 1) % _SIZE;
	val->amount--;
	return c;
}

char First(FIFO *val)
{
	char c;
	if(val->amount == 0)
	{
		printf("Queue is empty, cannot top!\n");
		return 0;
	}
	c = val->arr[val->read];
	return c;
}

int GetAmount(FIFO *val)
{
	return val->amount;
}
