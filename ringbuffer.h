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
#include "time.h"
#include "unistd.h"

/* per-process object */
struct Ring_Buffer {
  /* pointer to begining of ring buffer (shared memory)
     NOTE: this pointer is unique for each process, because processes
     have different virtual adress spaces
   */
  uint8_t* buffer;
  /* actual ring buffer object placed in shared memory */
  struct Ring_Buffer_Details* impl;
};

/* ring buffer object placed in shared memory
   Based on https://lo.calho.st/posts/black-magic-buffer/ but this
   implementation is designed for multi-processing */
struct Ring_Buffer_Details {
  /* size of ring buffer in bytes */
  size_t size;
  size_t head;
  size_t tail;
  int fd;
  char identifier[64];
  pthread_mutexattr_t mutex_attr;
  pthread_mutex_t mutex;
  pthread_condattr_t cond_attr;
  pthread_cond_t cond;
  size_t readers;
  size_t writers;
  size_t active_writers;
  _Atomic size_t refcount;
};

void detach_ring_buffer(struct Ring_Buffer_Details* rb)
{
  size_t val = --rb->refcount;

  if (val == 0) {
    const int pagesize = getpagesize();
    const size_t rb_size = (sizeof(struct Ring_Buffer_Details)+pagesize-1)/pagesize*pagesize;

    munmap(rb, rb_size+rb->size);
    printf("destroyed ring buffer\n");
  }
}

int create_ring_buffer(struct Ring_Buffer* ring_buffer, const char* identifier, size_t size)
{
  /* Memory layout

     +-----------------------+----------------+
     |         page 1        |   page 2..N    |
     +-------------+---------+----------------+
     | ring buffer |         |   ring buffer  |
     |    struct   | padding |      data      |

     Using virtual memory magic we make so that there are 2 blocks of
     pages that point to same physical memory.
   */

  const int pagesize = getpagesize();
  if (size % pagesize != 0) {
    fprintf(stderr, "create_ring_buffer: size must be a multiple of %d\n", getpagesize());
    return -1;
  }

  int fd = shm_open(identifier, O_CREAT|O_RDWR, S_IRUSR|S_IWUSR);
  if (fd == -1) {
    fprintf(stderr, "create_ring_buffer: shm_open failed\n");
    return -1;
  }

  /* align to page size */
  const size_t rb_size = (sizeof(struct Ring_Buffer_Details)+pagesize-1)/pagesize*pagesize;

  const size_t shmlen = rb_size+size;
  if (ftruncate(fd, shmlen) == -1) {
    fprintf(stderr, "create_ring_buffer: ftruncate failed\n");
    return -1;
  }

  struct Ring_Buffer_Details* rb = mmap(NULL, shmlen, PROT_READ|PROT_WRITE, MAP_SHARED, fd, 0);
  if (rb == MAP_FAILED) {
    fprintf(stderr, "create_ring_buffer: mmap failed\n");
    return -1;
  }
  uint8_t* buffer = (uint8_t*)rb + rb_size;
  // Map the buffer at that address
  mmap(buffer, size, PROT_READ | PROT_WRITE, MAP_SHARED | MAP_FIXED, fd, rb_size);
  // Now map it again, in the next virtual page
  mmap(buffer + size, size, PROT_READ | PROT_WRITE, MAP_SHARED | MAP_FIXED, fd, rb_size);

  rb->size = size;
  rb->head = 0;
  rb->tail = 0;
  rb->refcount = 1;
  strncpy(rb->identifier, identifier, sizeof(rb->identifier));
  rb->fd = fd;
  pthread_mutexattr_init(&rb->mutex_attr);
  pthread_mutexattr_setpshared(&rb->mutex_attr, PTHREAD_PROCESS_SHARED);
  if (pthread_mutex_init(&rb->mutex, &rb->mutex_attr) != 0) {
    fprintf(stderr, "create_ring_buffer: pthread mutex init failed\n");
    detach_ring_buffer(rb);
    return -1;
  }
  pthread_condattr_init(&rb->cond_attr);
  pthread_condattr_setpshared(&rb->cond_attr, PTHREAD_PROCESS_SHARED);
  if (pthread_cond_init(&rb->cond, &rb->cond_attr) != 0) {
    fprintf(stderr, "create_ring_buffer: pthread cond init failed\n");
    detach_ring_buffer(rb);
    return -1;
  }
  rb->readers = 0;
  rb->writers = 0;
  rb->active_writers = 0;

  *ring_buffer = (struct Ring_Buffer) {
    .buffer = buffer,
    .impl = rb
  };
  return 0;
}

int get_ring_buffer(struct Ring_Buffer* ring_buffer, const char* identifier, size_t size)
{
  int fd = shm_open(identifier, O_CREAT|O_RDWR, S_IRUSR|S_IWUSR);
  if (fd == -1) {
    fprintf(stderr, "create_ring_buffer: shm_open failed\n");
    return -1;
  }

  const int pagesize = getpagesize();
  const size_t rb_size = (sizeof(struct Ring_Buffer_Details)+pagesize-1)/pagesize*pagesize;

  struct Ring_Buffer_Details* rb = mmap(NULL, rb_size+size, PROT_READ|PROT_WRITE, MAP_SHARED, fd, 0);
  if (rb == MAP_FAILED) {
    fprintf(stderr, "create_ring_buffer: mmap failed\n");
    return -1;
  }
  uint8_t* buffer = (uint8_t*)rb + rb_size;
  // Map the buffer at that address
  mmap(buffer, size, PROT_READ | PROT_WRITE, MAP_SHARED | MAP_FIXED, fd, rb_size);
  // Now map it again, in the next virtual page
  mmap(buffer + size, size, PROT_READ | PROT_WRITE, MAP_SHARED | MAP_FIXED, fd, rb_size);

  rb->refcount++;

  if (size != rb->size) {
    fprintf(stderr, "get_ring_buffer: invalid size passed (expected %lu, got %lu)\n", rb->size, size);
    detach_ring_buffer(rb);
    return -1;
  }
  *ring_buffer = (struct Ring_Buffer) {
    .buffer = buffer,
    .impl = rb
  };

  return 0;
}

/* NOTE: there's should be only 1 writer at a time */
int write_to_ring_buffer(struct Ring_Buffer* ring_buffer, const void* data, size_t size)
{
  uint8_t* buffer = ring_buffer->buffer;
  struct Ring_Buffer_Details* rb = ring_buffer->impl;

  if (size >= rb->size) {
    fprintf(stderr, "ring buffer is smaller than ring buffer size\n");
    return -1;
  }

  pthread_mutex_lock(&rb->mutex);
  if (rb->size - (rb->tail-rb->head) < size) {
    pthread_mutex_unlock(&rb->mutex);
    return 1;
  }

  memcpy(&buffer[rb->tail], data, size);
  rb->tail += size;

  pthread_cond_signal(&rb->cond);
  pthread_mutex_unlock(&rb->mutex);
  return 0;
}

/* NOTE: there's should be only 1 reader at a time */
void* read_from_ring_buffer(struct Ring_Buffer* ring_buffer, size_t* size)
{
  uint8_t* buffer = ring_buffer->buffer;
  struct Ring_Buffer_Details* rb = ring_buffer->impl;

  void* data = NULL;
  pthread_mutex_lock(&rb->mutex);
  while(rb->head == rb->tail) {
    pthread_cond_wait(&rb->cond, &rb->mutex);
  }

  *size = rb->tail - rb->head;
  data = &buffer[rb->head];
  rb->head = rb->tail;

  if(rb->head > rb->size) {
    rb->head -= rb->size;
    rb->tail -= rb->size;
  }

  pthread_mutex_unlock(&rb->mutex);
  return data;
}

#endif // ADIL_RING_BUFFER_H
