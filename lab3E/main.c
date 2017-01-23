#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>
#include <sys/shm.h>
#include <time.h>
#include <errno.h>
#include <sys/types.h>
#include <signal.h>

#include "helpers.h"
#define memsizefor(n) (sizeof(SharedFifo) + sizeof(int)*(n))
#define seed (time(NULL))

#define MAX_SIZE 10
#define SLEEP 100000
#define true 1
#define false 0
#define N 10
unsigned int _size = MAX_SIZE;
typedef int bool;

typedef struct shfifo
{
    int size;//queue size
    int head;//where is our head
    int tail;//where is our tail
    bool first;//has already somebody read this letter for first time?
 } SharedFifo;

typedef struct fifo
{
    SharedFifo* sf;
    int* buf;
    sem_t* empty;//free places to push new letters
    sem_t* full;//can we pop sth
    sem_t* mutex;//critical section
    sem_t* a_sem;//Can A read new letter
    sem_t* b_sem;//  ----//----
    sem_t* block;//semaphore to create an atomic section
} Fifo;

// initialize shared variables
void init_shfifo(void* adr, int size)
{
    SharedFifo* sf = (SharedFifo*)adr;
    sf->size = _size;
    sf->head = 0;
    sf->tail = 0;
    sf->first = true;
}

Fifo get_fifo(void* adr)
{
    Fifo mem;int temp;
    // local mapping to shared memory
    mem.sf = (SharedFifo*)adr;
    mem.buf = (int*)(mem.sf + 1);
    //semaphores initialization
    mem.empty = get_semaphore("empty", mem.sf->size-1);
    mem.full = get_semaphore("full", 0);
    mem.mutex = get_semaphore("mutex", 1);
    mem.a_sem = get_semaphore("a_sem", 1);
    mem.b_sem = get_semaphore("b_sem", 1);
    mem.block = get_semaphore("block", 1);
    return mem;
}

void RandomSleep(int n)
{
    int x;
    x = ( rand()%(1000*n))*500;
    usleep(x);
}

char RandomLetter()
{
    char rdmlet;
    rdmlet = 'A' + (rand()%26);
    return rdmlet;
}

void cleanup()
{
    sem_unlink("full");
    sem_unlink("mutex");
    sem_unlink("a_sem");
    sem_unlink("b_sem");
    sem_unlink("empty");
    sem_unlink("block");

    shm_unlink("shmem");
}

void Read(char name, bool met_time, Fifo * m) //flag - flag to change | name - consument name, val - value to read |first_time - first time read the letter
{
    char val;
    printf("Consumer %c read letter ", name);
    if (met_time)
    {
        val = m->buf[m->sf->head];
        printf("%c for 1 time\n", val);
    }
    else
    {
        val = m->buf[m->sf->head];
        m->sf->head = (m->sf->head+1) % m->sf->size;
        printf("%c for 2 time\n", val);
    }
}

void producer_a()
{
    int temp=0,i,a;
    Fifo mem = get_fifo(get_shared_mem("shmem", memsizefor(_size)));
    char letter;

    while (temp!=100)
    {
        RandomSleep(10);
        letter = RandomLetter();

    	sem_wait(mem.block);
		sem_wait(mem.empty);
        	sem_wait(mem.mutex);
		
        mem.buf[mem.sf->tail] = letter;
        printf("%c has been added to our queue\n",  mem.buf[mem.sf->tail]);
        mem.sf->tail = (mem.sf->tail+1) % mem.sf->size;
        printf("QUEUE :");
        for(i= mem.sf->head;i!=mem.sf->tail;i = (i+1)%mem.sf->size)
            printf("%c ", mem.buf[i]);
        printf(" %d %d\n", mem.sf->head, mem.sf->tail);
		
        	sem_post(mem.mutex);
		sem_getvalue(mem.empty, &a);
		if (a<(mem.sf->size-4))
        		sem_post(mem.full);
	sem_post(mem.block);

        temp++;
    }
}

void producer_b()
{
    int temp=0,i,a;
    Fifo mem = get_fifo(get_shared_mem("shmem", memsizefor(_size)));
    char letter;
	
    while (temp!=100)
    {
        RandomSleep(10);

        sem_wait(mem.block);
     		sem_wait(mem.empty);
		sem_wait(mem.empty);
 	        sem_wait(mem.mutex);

        letter = RandomLetter();
        mem.buf[mem.sf->tail] = letter;
        printf("%c  has been added to our queue\n",  mem.buf[mem.sf->tail]);
        mem.sf->tail = (mem.sf->tail+1) % mem.sf->size;
        letter = RandomLetter();
        mem.buf[mem.sf->tail] = letter;
        printf("%c  has been added to our queue\n",  mem.buf[mem.sf->tail]);
        mem.sf->tail = (mem.sf->tail+1) % mem.sf->size;

        printf("QUEUE :");
        for(i= mem.sf->head;i!=mem.sf->tail;i = (i+1)%mem.sf->size)
            printf("%c ", mem.buf[i]);
        printf(" %d %d\n", mem.sf->head, mem.sf->tail);

        	sem_post(mem.mutex);
		sem_getvalue(mem.empty, &a);	
		if (a<(mem.sf->size-4))
		{
        		sem_post(mem.full);
	        	sem_post(mem.full);
	        }
	sem_post(mem.block);

        temp++;
    }
}

void consumer_a()
{
    Fifo mem = get_fifo(get_shared_mem("shmem", memsizefor(_size)));
    while(true)
    {
	RandomSleep(2);
        sem_wait(mem.a_sem);

	sem_wait(mem.full);
	sem_wait(mem.mutex);
        if (mem.sf->first)
        {
            Read('A', 1, &mem);
            mem.sf->first = false;
            sem_post(mem.mutex);
            sem_post(mem.full);
        }
        else
        {
            Read('A', 0, &mem);
            mem.sf->first = true;

            sem_post(mem.mutex);
            sem_post(mem.empty);

            sem_post(mem.a_sem);
            sem_post(mem.b_sem);
        }
    }
}

void consumer_b()
{
    Fifo mem = get_fifo(get_shared_mem("shmem", memsizefor(_size)));
    while(true)
    {
	RandomSleep(2);
        sem_wait(mem.b_sem);

	sem_wait(mem.full);
	sem_wait(mem.mutex);
        
        if (mem.sf->first)
        {
            Read('B', 1, &mem);
            mem.sf->first = false;
            sem_post(mem.mutex);
            sem_post(mem.full);

        }
        else
        {
            Read('B', 0, &mem);
            mem.sf->first = true;
            sem_post(mem.mutex);
            sem_post(mem.empty);

            sem_post(mem.a_sem);
            sem_post(mem.b_sem);
        }
    }
}

int main()
{
    cleanup();
    char c;
    pid_t pid;
    pid_t children[4];
    int i;
    int k = 0;
    srandom(seed);

    init_shfifo(get_shared_mem("shmem", memsizefor(_size)), _size);

    pid = fork();
    if (pid==0)
    {
        printf("Producer A is born!\n");
        producer_a();
    }
    else
    {
        children[0] = pid;
    }

    pid = fork();
    if (pid==0)
    {
        printf("Producer B is born!\n");
        producer_b();
    }
    else
    {
        children[1] = pid;
    }

    pid = fork();
    if (pid==0)
    {
        printf("Consummer A is born!\n");
        consumer_a();
    }
    else
    {
        children[2] = pid;
    }

    pid = fork();
    if (pid==0)
    {
        printf("Consummer B is born!\n");
        consumer_b();
    }
    else
    {
        children[3] = pid;
    }

    while((c = getchar()) != 'q'); 
	   
    sleep(10);

    for (i = 0; i < 4; ++i)
    {
        kill(children[i], SIGKILL);
    }
    cleanup();

    return 0;
}
