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

/* https://lo.calho.st/posts/black-magic-buffer/ */
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
    const size_t shmlen = sizeof(struct Ring_Buffer)+rb->size;

    munmap(rb, shmlen);
    printf("destroyed ring buffer\n");
  }
}

struct Ring_Buffer* create_ring_buffer(const char* identifier, size_t size)
{
  if (size % getpagesize() != 0) {
    fprintf(stderr, "create_ring_buffer: size must be a multiple of %d\n", getpagesize());
    return NULL;
  }

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

  struct Ring_Buffer* rb = mmap(NULL, shmlen+size, PROT_READ|PROT_WRITE, MAP_SHARED, fd, 0);
  if (rb == MAP_FAILED) {
    fprintf(stderr, "create_ring_buffer: mmap failed\n");
    return NULL;
  }
  // Map the buffer at that address
  mmap(rb, shmlen, PROT_READ | PROT_WRITE, MAP_SHARED | MAP_FIXED, rb->fd, 0);
  // Now map it again, in the next virtual page
  mmap(rb->shmem + size, size, PROT_READ | PROT_WRITE, MAP_SHARED | MAP_FIXED, rb->fd, 0);

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

int write_to_ring_buffer(struct Ring_Buffer* rb, const void* data, size_t size)
{
  if (size >= rb->size) {
    fprintf(stderr, "ring buffer is smaller than ring buffer size\n");
    return -1;
  }

  pthread_mutex_lock(&rb->mutex);
  if (rb->size - (rb->tail-rb->head) < size) {
    pthread_mutex_unlock(&rb->mutex);
    return 1;
  }

  memcpy(&rb->shmem[rb->tail], data, size);
  rb->tail += size;

  pthread_mutex_unlock(&rb->mutex);
  return 0;
}

void* read_from_ring_buffer(struct Ring_Buffer* rb, size_t* size)
{
  void* data = NULL;
  pthread_mutex_lock(&rb->mutex);
  if (rb->head == rb->tail) {
    *size = 0;
  } else {
    *size = rb->tail - rb->head;
    data = &rb->shmem[rb->head];
    rb->head = rb->tail;

    if(rb->head > rb->size) {
       rb->head -= rb->size;
       rb->tail -= rb->size;
    }
  }
  pthread_mutex_unlock(&rb->mutex);
  return data;
}

#endif // ADIL_RING_BUFFER_H
