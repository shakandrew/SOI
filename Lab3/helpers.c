#include "helpers.h"

void* get_shared_mem(char* name, size_t size)
/* returns the address the memory was mapped to */
{
	void* mem; /* the mapped address */
	int shm; /* file descriptor of obtained shared memory */
	
	shm = shm_open(name, O_CREAT | O_RDWR, S_IRWXU | S_IRWXO); /* create or obtain existing shared memory */
	if(shm == -1) return NULL;
	
	ftruncate(shm, size); /* truncate it to size */
	
	mem = mmap(NULL, size, PROT_WRITE | PROT_READ, MAP_SHARED, shm, 0); /* map it to our virtual address space */
	if (mem == MAP_FAILED)
	{
		shm_unlink(name);
		return NULL;
	}
	return mem; /* return the mapped address */
}

sem_t* get_semaphore(char* name, int value)
/* returns a semaphore with given intial value */
{
	return sem_open(name, O_CREAT, S_IRWXU | S_IRWXO, value);
}
