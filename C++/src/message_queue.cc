#include "message_queue.hh"

#include <cstring>
#include <sys/msg.h>

_message_data::_message_data()
    : type(0), data{ 0 } { }

_message_data::_message_data(long msg_type, std::string msg_data)
    : type(msg_type) {
    msg_data.copy(data, MESSAGE_DATA_SIZE);
}

_message_data::_message_data(const _message_data& other)
    : type(other.type) {
    std::memcpy(data, other.data, MESSAGE_DATA_SIZE);
}

message_queue::message_queue(key_t key) : msg_key(key) {
    msg_id = msgget(key, 0666 | IPC_CREAT);
    if (msg_id == -1) {
        // TODO: Error handling
        perror("Message queue");
        exit(EXIT_FAILURE);
    }
}

void message_queue::send(long msg_type, std::string msg_data) {
    msg_buf_t buf(msg_type, msg_data);
    int res = msgsnd(msg_id, buf, MESSAGE_DATA_SIZE, 0);
    if (res == -1) {
        // TODO: Error handling
        perror("Message send");
        exit(EXIT_FAILURE);
    }
}

std::tuple<long, std::string> message_queue::receive(long type) {
    msg_buf_t buf;
    int res = msgrcv(msg_id, buf, MESSAGE_DATA_SIZE, type, 0);
    if (res == -1) {
        // TODO: Error handling
        perror("Message receive");
        exit(EXIT_FAILURE);
    }
    return std::make_tuple(buf->type, buf->data);
}

void message_queue::destroy() {
    int res = msgctl(msg_id, IPC_RMID, nullptr);
    if (res == -1) {
        // TODO: Error handling
        perror("msgctl");
        exit(EXIT_FAILURE);
    }
}
