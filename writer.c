/* Source code for the process that writes data. */
#define _GNU_SOURCE
#include "pthread.h"
#include "sys/ipc.h"
#include "sys/shm.h"
#include "stdint.h"
#include "stdio.h"
#include "stdlib.h"
#include "string.h"
#include "unistd.h"

#include "ringbuffer.h"

#define RING_BUFFER_SIZE 1024
#define SHM_KEY 42069
#define CPU_ID 0

int main(int argc, char** argv)
{
  cpu_set_t cpuset;
  CPU_ZERO(&cpuset);
  CPU_SET(CPU_ID, &cpuset);
  if (pthread_setaffinity_np(pthread_self(), sizeof(cpuset), &cpuset) != 0) {
    fprintf(stderr, "pthread_setaffinity_np faield\n");
    return -1;
  }

  struct Ring_Buffer* rb = create_ring_buffer(SHM_KEY, RING_BUFFER_SIZE);

  sleep(10);

  detach_ring_buffer(rb);

  return 0;
}
