/* some struct that we're sending from one process to another */
struct Message {
    uint64_t timestamp;
    uint64_t id;
    int msg_len;
    char buf[1024];
};
