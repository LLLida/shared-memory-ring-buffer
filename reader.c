/* Source code for the process that reads data. */
#include "ringbuffer.h"
#include "message.h"

#include "signal.h"

#define SHM_KEY "/adil-shmem"
#define CPU_ID 1

struct Measurement {
  uint64_t send_sec;
  uint64_t send_nsec;
  uint64_t retrieve_sec;
  uint64_t retrieve_nsec;
};

static size_t measure_counter = 0;
/* reads messages from ring buffer */
static void read_messages(struct Ring_Buffer* buffer, struct Measurement* measurements);

static volatile int keep_running = 1;
static void interruption_handler(int dummy);

int main(int argc, char** argv)
{
  int64_t rb_size;
  const char* specified_path = NULL;
  if (argc != 2 && argc != 3) {
    printf("Usage: %s SIZE [PATH]\n", argv[0]);
    printf("  where SIZE is size of ring buffer in bytes.\n");
    printf("  where PATH is filepath where measurements will be saved(optional).\n");
    return 1;
  }
  rb_size = atoll(argv[1]);
  if (argc == 3) {
    specified_path = argv[2];
  }

  signal(SIGINT, &interruption_handler);

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

  struct Measurement* measure_buffer = malloc(sizeof(struct Measurement) * 128*1024*1024);

  while (keep_running) {
    read_messages(&buffer, measure_buffer);
  }

  /* save measures to .csv file  */
  char filepath[256];
  if (specified_path == NULL) {
  time_t t = time(NULL);
  struct tm tm = *localtime(&t);
  snprintf(filepath, sizeof(filepath), "results_%d_%02d_%02d_%02d%02d%02d.csv", tm.tm_year+1900, tm.tm_mon+1, tm.tm_mday, tm.tm_hour, tm.tm_min, tm.tm_sec);
  } else {
    strncpy(filepath, specified_path, sizeof(filepath)-1);
  }

  FILE* results = fopen(filepath, "w");
  fprintf(results, "send_sec,send_nsec,retrieve_sec,retrieve_nsec,\n");
  for (size_t i = 0; i < measure_counter; i++) {
    fprintf(results, "%lu,%lu,%lu,%lu,\n",
	    measure_buffer[i].send_sec, measure_buffer[i].send_nsec, measure_buffer[i].retrieve_sec, measure_buffer[i].retrieve_nsec);
  }
  fclose(results);
  free(measure_buffer);
  printf("Saved measure results to '%s'\n", filepath);

  detach_ring_buffer(buffer.impl);
  return 0;
}

void read_messages(struct Ring_Buffer* buffer, struct Measurement* measurements)
{
  size_t sz;
  struct Message* msg = read_from_ring_buffer(buffer, &sz);

  while (sz > 0) {
    struct timespec tp;
    clock_gettime(CLOCK_REALTIME, &tp);

#if 1
    measurements[measure_counter++] = (struct Measurement) {
      .send_sec = msg->sec,
      .send_nsec = msg->ns,
      .retrieve_sec = tp.tv_sec,
      .retrieve_nsec = tp.tv_nsec,
    };
#else
    if (tp.tv_nsec < msg->ns) {
      tp.tv_nsec += 1000000000;
      tp.tv_sec -= 1;
    }
    uint64_t d_ns = tp.tv_nsec - msg->ns;
    uint64_t d_s = tp.tv_sec - msg->sec;

    printf("Message %lu: time=%lu.%lu \"%.*s\" (delay = %f)\n", msg->id, msg->sec, msg->ns/1000000, msg->msg_len, msg->buf, d_s + d_ns/1e9);
#endif

    size_t bytes = sizeof(msg->sec)+sizeof(msg->ns)+sizeof(msg->id)+sizeof(msg->msg_len)+msg->msg_len;
    msg = (struct Message*)((uint8_t*)msg + bytes);
    sz -= bytes;
  }
}

void interruption_handler(int dummy)
{
  (void)dummy;
  printf("\ninterrupt\n");
  keep_running = 0;
}
