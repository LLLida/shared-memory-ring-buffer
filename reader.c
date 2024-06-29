/* Source code for the process that reads data. */
#include "ringbuffer.h"

#define SHM_KEY "/adil-shmem"
#define CPU_ID 1

int main(int argc, char** argv)
{
  int64_t rb_size;
  if (argc != 2) {
    printf("Usage: %s SIZE\n", argv[0]);
    printf("  where SIZE is size of ring buffer in bytes.\n");
    return 1;
  }
  rb_size = atoll(argv[1]);

  cpu_set_t cpuset;
  CPU_ZERO(&cpuset);
  CPU_SET(CPU_ID, &cpuset);
  if (pthread_setaffinity_np(pthread_self(), sizeof(cpuset), &cpuset) != 0) {
    fprintf(stderr, "pthread_setaffinity_np faield\n");
    return -1;
  }

  struct Ring_Buffer* rb = get_ring_buffer(SHM_KEY, rb_size);
  if (rb == NULL) {
    fprintf(stderr, "failed to create ring buffer...\n");
    return -2;
  }

  printf("Ring buffer: fd=%d size=%lu refcount=%lu id=%s\n", rb->fd, rb->size, rb->refcount, rb->identifier);

  detach_ring_buffer(rb);
  return 0;
}
