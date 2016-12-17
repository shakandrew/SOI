#ifndef SOI_T3_HELPERS
#define SOI_T3_HELPERS

#include <sys/mman.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <semaphore.h>
#include <stddef.h>

void* get_shared_mem(char* name, size_t size);
sem_t* get_semaphore(char* name, int value);
#endif
