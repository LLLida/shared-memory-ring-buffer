#ifndef ADIL_RING_BUFFER_H
#define ADIL_RING_BUFFER_H

struct Ring_Buffer {
  /* pointer to begining of ring buffer (shared memory) */
  uint8_t* shmem;
  /* size of ring buffer in bytes */
  size_t size;
  size_t begin;
  size_t end;
  int shmid;
  _Atomic size_t refcount;
};

struct Ring_Buffer* create_ring_buffer(key_t shmem_key, size_t size)
{
  int shmid = shmget(shmem_key, size, IPC_CREAT|0666);
  if (shmid == -1) {
    fprintf(stderr, "shmget failed\n");
    return NULL;
  }

  struct Ring_Buffer* rb = shmat(shmid, NULL, 0);
  if (rb == NULL) {
    fprintf(stderr, "shmat failed\n");
    return NULL;
  }

  rb->shmem = (uint8_t*)&rb[1];
  rb->size = size;
  rb->begin = 0;
  rb->end = 0;
  rb->shmid = shmid;
  rb->refcount = 1;

  return rb;
}

void detach_ring_buffer(struct Ring_Buffer* rb)
{
  int shmid = rb->shmid;
  size_t val = --rb->refcount;
  shmdt(rb);

  if (val == 0) {
    shmctl(shmid, IPC_RMID, NULL);
    printf("destroyed ring buffer\n");
  }
}

struct Ring_Buffer* get_ring_buffer(key_t shmem_key, size_t size)
{
  int shmid = shmget(shmem_key, size, IPC_CREAT|0666);
  if (shmid == -1) {
    fprintf(stderr, "shmget failed\n");
    return NULL;
  }

  struct Ring_Buffer* rb = shmat(shmid, NULL, 0);
  if (rb == NULL) {
    fprintf(stderr, "shmat failed\n");
    return NULL;
  }
  rb->refcount++;

  if (size != rb->size) {
    fprintf(stderr, "invalid size passed (expected %lu, got %lu)", rb->size, size);
    detach_ring_buffer(rb);
    return NULL;
  }

  return rb;
}

#endif // ADIL_RING_BUFFER_H
