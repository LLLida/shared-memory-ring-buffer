#ifndef ADIL_RING_BUFFER_H
#define ADIL_RING_BUFFER_H

#define _GNU_SOURCE
#include "sys/mman.h"
#include "fcntl.h"
#include "pthread.h"
#include "sys/ipc.h"
#include "sys/shm.h"
#include "stdint.h"
#include "stdio.h"
#include "stdlib.h"
#include "string.h"
#include "unistd.h"


/* https://ferrous-systems.com/blog/lock-free-ring-buffer/ */
struct Ring_Buffer {
  /* pointer to begining of ring buffer (shared memory) */
  uint8_t* shmem;
  /* size of ring buffer in bytes */
  size_t size;
  size_t head;
  size_t tail;
  int fd;
  char identifier[64];
  pthread_mutex_t mutex;
  _Atomic size_t refcount;
};

void detach_ring_buffer(struct Ring_Buffer* rb)
{
  size_t val = --rb->refcount;

  if (val == 0) {
    const char* identifier = rb->identifier;
    const size_t shmlen = sizeof(struct Ring_Buffer)+rb->size;

    munmap(rb, shmlen);
    printf("destroyed ring buffer\n");
  }
}

struct Ring_Buffer* create_ring_buffer(const char* identifier, size_t size)
{
  int fd = shm_open(identifier, O_CREAT|O_RDWR, S_IRUSR|S_IWUSR);
  if (fd == -1) {
    fprintf(stderr, "create_ring_buffer: shm_open failed\n");
    return NULL;
  }

  const size_t shmlen = sizeof(struct Ring_Buffer)+size;
  if (ftruncate(fd, shmlen) == -1) {
    fprintf(stderr, "create_ring_buffer: ftruncate failed\n");
    return NULL;
  }

  struct Ring_Buffer* rb = mmap(NULL, shmlen, PROT_READ|PROT_WRITE, MAP_SHARED, fd, 0);
  if (rb == MAP_FAILED) {
    fprintf(stderr, "create_ring_buffer: mmap failed\n");
    return NULL;
  }

  rb->shmem = (uint8_t*)&rb[1];
  rb->size = size;
  rb->head = 0;
  rb->tail = 0;
  rb->refcount = 1;
  strncpy(rb->identifier, identifier, sizeof(rb->identifier));
  rb->fd = fd;
  if (pthread_mutex_init(&rb->mutex, NULL) != 0) {
    fprintf(stderr, "create_ring_buffer: pthread mutex init failed\n");
    detach_ring_buffer(rb);
    return NULL;
  }

  return rb;
}

struct Ring_Buffer* get_ring_buffer(const char* identifier, size_t size)
{
  int fd = shm_open(identifier, O_CREAT|O_RDWR, S_IRUSR|S_IWUSR);
  if (fd == -1) {
    fprintf(stderr, "create_ring_buffer: shm_open failed\n");
    return NULL;
  }

  const size_t shmlen = sizeof(struct Ring_Buffer)+size;
  struct Ring_Buffer* rb = mmap(NULL, shmlen, PROT_READ|PROT_WRITE, MAP_SHARED, fd, 0);
  if (rb == MAP_FAILED) {
    fprintf(stderr, "create_ring_buffer: mmap failed\n");
    return NULL;
  }

  rb->refcount++;

  if (size != rb->size) {
    fprintf(stderr, "get_ring_buffer: invalid size passed (expected %lu, got %lu)\n", rb->size, size);
    detach_ring_buffer(rb);
    return NULL;
  }

  return rb;
}

int write_to_ring_buffer(struct Ring_Buffer* rb, const void* mem, size_t size)
{
  if (size >= rb->size) {
    fprintf(stderr, "ring buffer is smaller than ring buffer size\n");
    return -1;
  }

  pthread_mutex_lock(&rb->mutex);

  pthread_mutex_unlock(&rb->mutex);
}

void* read_from_ring_buffer(struct Ring_Buffer* rb, size_t* size)
{
  void* data = NULL;
  pthread_mutex_lock(&rb->mutex);
  if (rb->head != rb->tail) {

  }
  pthread_mutex_unlock(&rb->mutex);
  return data;
}

#endif // ADIL_RING_BUFFER_H
