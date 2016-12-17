#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>
#include <sys/shm.h>
#include <time.h>
#include <errno.h>
#include <sys/types.h>
#include <signal.h>

#include "FIFO.h"
#include "helpers.h"


#define memsizefor(n) (sizeof(data)+sizeof(char)*_size)
#define seed (time(NULL))

#define MAX_SIZE _SIZE
#define SLEEP 100000
#define true 1
#define false 0
#define N 10
unsigned int _size = MAX_SIZE;
typedef int bool;

typedef struct data
{
    FIFO buff;
    bool read_a;
    bool read_b;
    bool read_c;

}data;

typedef struct shared_memory
{
    data * dt;
    sem_t * mutex;
    sem_t * full;
    sem_t * empty;
    sem_t * a_sem;
    sem_t * b_sem;
    sem_t * c_sem;
}shmem;

void init_data(void* adr)
{
    data * dt = (data*)adr;
    Init(&dt->buff);
    dt->read_a = false;
    dt->read_b = false;
    dt->read_c = false;
}

shmem get_shmem(void* adr)
{
    shmem m;
    // local mapping to shared memory
    m.dt = (data*)adr;
    m.full = get_semaphore("/t3full", 0);
    m.mutex = get_semaphore("/t3mutex", 1);
    m.a_sem = get_semaphore("/t3a_sem", 1);
    m.b_sem = get_semaphore("/t3b_sem", 1);
    m.c_sem = get_semaphore("/t3c_sem", 1);
    m.empty = get_semaphore("/t3empty", MAX_SIZE);
    return m;
}

void RandomSleep()
{
    int x;
    x = (1000 + rand()%100)*50;
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
    sem_unlink("/t3full");
    sem_unlink("/t3mutex");
    sem_unlink("/t3a_read");
    sem_unlink("/t3b_read");
    sem_unlink("/t3c_read");
    shm_unlink("/t3empty");
}

void Read(bool *flag, char name, bool met_time, shmem *m) // flag - flag to change | name - consument name, val - value to read |first_time - first time read the letter
{
    char val;
    *flag = (*flag+1)%2;
    printf("Consumer %c read letter ", name);
    if (met_time)
    {
        val = First(&m->dt->buff);
        printf("%c for 1 time\n", val);
    }
    else
    {
        val = Pop(&m->dt->buff);
        printf("%c for 2 time\n");
    }
}

void producer()
{
    shmem mem = get_shmem(get_shared_mem("/t3shmem", memsizefor(_size)));
    printf("%d %d %s", mem.dt->read_a, mem.dt->read_b, "lol");
    char letter;
    while (true)
    {
        RandomSleep();
        letter = RandomLetter();
        sem_wait(mem.empty);
        sem_wait(mem.mutex);

        Push(&mem.dt->buff, letter);
        printf("%c %c this letters has been added to our queue", First(&(mem.dt->buff)), mem.dt->buff.arr[0]);

        sem_post(mem.mutex);
        sem_post(mem.full);
    }
}

void consumer_a()
{
    shmem mem = get_shmem(get_shared_mem("/t3shmem", memsizefor(_size)));
    while(true)
    {
        RandomSleep();
        sem_wait(mem.a_sem);
        if (mem.dt->read_c)
            continue;
        if (mem.dt->read_b)
        {
            sem_wait(mem.full);
            sem_wait(mem.mutex);
            Read(&mem.dt->read_a, 'A', 0, &mem);
            sem_post(mem.mutex);
            sem_post(mem.empty);

            if (mem.dt->read_a) {mem.dt->read_a=false;sem_post((mem.a_sem));}
            if (mem.dt->read_b) {mem.dt->read_b=false;sem_post((mem.b_sem));}
        }
        else
        {
            sem_wait(mem.mutex);
            Read(&mem.dt->read_a, 'A', 1, &mem);
            sem_post(mem.mutex);
        }
    }
}

void consumer_b()
{
    shmem mem = get_shmem(get_shared_mem("/t3shmem", memsizefor(_size)));
    while(true)
    {
        RandomSleep();
        sem_wait(mem.b_sem);

        if (mem.dt->read_a || mem.dt->read_c)
        {
            sem_wait(mem.full);
            sem_wait(mem.mutex);
            Read(&mem.dt->read_b, 'B', 0, &mem);
            sem_post(mem.mutex);
            sem_post(mem.empty);

            if (mem.dt->read_a) {mem.dt->read_a=false;sem_post((mem.a_sem));}
            if (mem.dt->read_b) {mem.dt->read_b=false;sem_post((mem.b_sem));}
            if (mem.dt->read_c) {mem.dt->read_c=false;sem_post((mem.c_sem));}

        }
        else
        {
            sem_wait(mem.mutex);
            Read(&mem.dt->read_b, 'B', 1, &mem);
            sem_post(mem.mutex);
        }
    }
}

void consumer_c()
{
    shmem mem = get_shmem(get_shared_mem("/t3shmem", memsizefor(_size)));
    while(true)
    {
        RandomSleep();
        sem_wait(mem.c_sem);
        if (mem.dt->read_a)
            continue;
        if (mem.dt->read_b)
        {
            sem_wait(mem.full);
            sem_wait(mem.mutex);
            Read(&mem.dt->read_c, 'C', 0, &mem);
            sem_post(mem.mutex);
            sem_post(mem.empty);

            if (mem.dt->read_c) {mem.dt->read_c=false;sem_post((mem.c_sem));}
            if (mem.dt->read_b) {mem.dt->read_b=false;sem_post((mem.b_sem));}
        }
        else
        {
            sem_wait(mem.mutex);
            Read(&mem.dt->read_c, 'C', 1, &mem);
            sem_post(mem.mutex);
        }
    }
}

int main()
{
    char c;
    pid_t pid;
    pid_t children[4];
    int i;
    int k = 0;
    srandom(seed);

    init_data(get_shared_mem("/t3shmem", memsizefor(_size)));

    printf("Let us begin...\n");

    switch(pid = fork())
    {
        case 0:
            printf("Producer is born!\n");
            producer();
            return 0;
        case -1:
            printf("Could not fork, errno %d\n", errno);
            break;
        default:
            children[0] = pid;
            break;
    }

    switch(pid = fork())
    {
        case 0:
            printf("A is born!\n");
            consumer_a();
            return 0;
        case -1:
            printf("Could not fork, errno %d\n", errno);
            break;
        default:
            children[1] = pid;
            break;
    }

    switch(pid = fork())
    {
        case 0:
            printf("B is born!\n");
            consumer_b();
            return 0;
        case -1:
            printf("Could not fork, errno %d\n", errno);
            break;
        default:
            children[2] = pid;
            break;
    }

    switch(pid = fork())
    {
        case 0:
            printf("C is born!\n");
            consumer_c();
            return 0;
        case -1:
            printf("Could not fork, errno %d\n", errno);
            break;
        default:
            children[3] = pid;
            break;
    }

    sleep(1);

    while((c = getchar()) != 'q');

    for (i = 0; i < 4; ++i)
    {
        kill(children[i], SIGKILL);
    }
    cleanup();

    return 0;
}
