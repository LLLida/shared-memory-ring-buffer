/* Source code for the process that reads data. */
#define _GNU_SOURCE
#include "pthread.h"
#include "sys/ipc.h"
#include "sys/shm.h"
#include "stddef.h"
#include "stdint.h"
#include "stdio.h"
#include "stdlib.h"
#include "string.h"
#include "unistd.h"

#include "ringbuffer.h"

#ifndef RING_BUFFER_SIZE
#define RING_BUFFER_SIZE 1024
#endif
#define SHM_KEY 42069
#define CPU_ID 1

int main(int argc, char** argv)
{
  cpu_set_t cpuset;
  CPU_ZERO(&cpuset);
  CPU_SET(CPU_ID, &cpuset);
  if (pthread_setaffinity_np(pthread_self(), sizeof(cpuset), &cpuset) != 0) {
    fprintf(stderr, "pthread_setaffinity_np faield\n");
    return -1;
  }

  struct Ring_Buffer* rb = get_ring_buffer(SHM_KEY, RING_BUFFER_SIZE);

  printf("Ring buffer: shmid=%d size=%lu refcount=%lu\n", rb->shmid, rb->size, rb->refcount);

  detach_ring_buffer(rb);

  return 0;
}
