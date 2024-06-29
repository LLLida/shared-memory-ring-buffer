/* Source code for the process that writes data. */
#include "ringbuffer.h"
#include "message.h"

#include "signal.h"

#define SHM_KEY "/adil-shmem"
#define CPU_ID 0

/* writes some message to ring buffer with a random string attached to it */
static void send_message(struct Ring_Buffer* buffer);

static volatile int keep_running = 1;
static void interruption_handler(int dummy);

int main(int argc, char** argv)
{
  srand(73);

  int64_t rb_size, msg_frequency;
  if (argc != 3) {
    printf("Usage: %s SIZE FREQ\n", argv[0]);
    printf("  where SIZE is size of ring buffer in bytes.\n");
    printf("  where FREQ is number of messages send in second.\n");
    return 1;
  }
  rb_size = atoll(argv[1]);
  msg_frequency = atoll(argv[2]);

  signal(SIGINT, &interruption_handler);

  cpu_set_t cpuset;
  CPU_ZERO(&cpuset);
  CPU_SET(CPU_ID, &cpuset);
  if (pthread_setaffinity_np(pthread_self(), sizeof(cpuset), &cpuset) != 0) {
    fprintf(stderr, "pthread_setaffinity_np faield\n");
    return -1;
  }

  struct Ring_Buffer buffer;
  if (create_ring_buffer(&buffer, SHM_KEY, rb_size) != 0) {
    fprintf(stderr, "failed to create ring buffer...\n");
    return -2;
  }
  printf("Ring buffer: fd=%d size=%lu refcount=%lu id=%s\n", buffer.impl->fd, buffer.impl->size, buffer.impl->refcount, buffer.impl->identifier);

  while (keep_running) {
    struct timespec ts;
    ts.tv_sec = 0;
    ts.tv_nsec = 1e9 / msg_frequency;
    nanosleep(&ts, NULL);

    send_message(&buffer);
  }

  detach_ring_buffer(buffer.impl);
  return 0;
}

void send_message(struct Ring_Buffer* buffer)
{
  const char* some_string = "privetabcdefghijkpoka";
  int some_len = strlen(some_string);

  static struct Message msg;

  static struct timespec tp;
  static uint64_t counter = 0;

  msg.msg_len = rand() % 10 + 5;
  for (int i = 0; i < msg.msg_len; i++) {
    msg.buf[i] = some_string[i%some_len];
  }

  clock_gettime(CLOCK_REALTIME, &tp);
  msg.timestamp = tp.tv_nsec;
  msg.id = counter++;

  write_to_ring_buffer(buffer, &msg,
		       sizeof(msg.timestamp)+sizeof(msg.id)+sizeof(msg.msg_len)+msg.msg_len);
}

static void interruption_handler(int dummy)
{
  (void)dummy;
  printf("\ninterrupt\n");
  keep_running = 0;
}
