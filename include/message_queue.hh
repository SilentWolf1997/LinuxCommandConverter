#ifndef __MESSAGE_QUEUE_HH__
#define __MESSAGE_QUEUE_HH__

#include <string>
#include <sys/types.h>
#include <tuple>

#include "definations.hh"
#include "buffer.hh"

typedef struct _message_data {
    long type;
    char data[MESSAGE_DATA_SIZE];

    _message_data();
    _message_data(long msg_type, std::string msg_data);
    _message_data(const _message_data& other);
} msg_data_t;

using msg_buf_t = buffer<msg_data_t>;

class message_queue {
private:
    key_t msg_key;
    int msg_id;
public:
    message_queue(key_t key);

    void send(long msg_type, std::string msg_data);
    std::tuple<long, std::string> receive(long type = 0);

    void destroy();
};

#endif
