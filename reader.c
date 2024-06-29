/* Source code for the process that reads data. */
#include "ringbuffer.h"
#include "message.h"

#define SHM_KEY "/adil-shmem"
#define CPU_ID 1

/* reads messages from ring buffer */
static void read_messages(struct Ring_Buffer* buffer);

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

  struct Ring_Buffer buffer;
  if (get_ring_buffer(&buffer, SHM_KEY, rb_size) != 0) {
    fprintf(stderr, "failed to create ring buffer...\n");
    return -2;
  }

  printf("Ring buffer: fd=%d size=%lu refcount=%lu id=%s\n", buffer.impl->fd, buffer.impl->size, buffer.impl->refcount, buffer.impl->identifier);

  read_messages(&buffer);

  detach_ring_buffer(buffer.impl);
  return 0;
}

void read_messages(struct Ring_Buffer* buffer)
{
  size_t sz;
  struct Message* msg = read_from_ring_buffer(buffer, &sz);

  while (sz > 0) {
    printf("Message %lu: time=%lu \"%.*s\"\n", msg->id, msg->timestamp, msg->msg_len, msg->buf);

    size_t bytes = sizeof(msg->timestamp)+sizeof(msg->id)+sizeof(msg->msg_len)+msg->msg_len;
    msg = (struct Message*)((uint8_t*)msg + bytes);
    sz -= bytes;
  }
}
