/* Source code for the process that writes data. */
#include "sys/ipc.h"
#include "sys/shm.h"
#include "stdint.h"
#include "stdio.h"
#include "stdlib.h"
#include "string.h"
#include "unistd.h"

#define SHM_KEY 42069

int main(int argc, char** argv)
{
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

  char buff[1024];
  strcpy(buff, shared_memory);
  printf("  %s\n", buff);

  shmdt(shared_memory);

  return 0;
}
