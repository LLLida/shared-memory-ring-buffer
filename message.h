/* some struct that we're sending from one process to another */
struct Message {
  int64_t sec;
  int64_t ns;
  uint64_t id;
  int msg_len;
  char buf[1024];
};
