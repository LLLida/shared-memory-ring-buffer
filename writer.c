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

  int shmid = shmget(SHM_KEY, 1024, IPC_CREAT|0666);
  if (shmid == -1) {
    fprintf(stderr, "shmget failed\n");
    return -1;
  }

  uint8_t* shared_memory = shmat(shmid, NULL, 0);
  if (shared_memory == NULL) {
    fprintf(stderr, "shmat failed\n");
    return -1;
  }

  strcpy((char*)shared_memory, "Hello from writer process!");

  sleep(10);

  shmdt(shared_memory);
  shmctl(shmid, IPC_RMID, NULL);

  return 0;
}
