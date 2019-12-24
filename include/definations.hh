#ifndef __DEFINATIONS_HH__
#define __DEFINATIONS_HH__

constexpr const unsigned int MESSAGE_DATA_SIZE = 256;
constexpr const unsigned int PIPE_BUFFER_SIZE = 1024;

constexpr const auto NAMED_PIPE_PATH = "/tmp/mypipe";
constexpr const auto BACKEND_PATH = "./backend";
constexpr const auto BACKEND_NAME = "backend";
constexpr const int MESSAGE_QUEUE_KEY = 0x12345678;

constexpr const long MESSAGE_TYPE_EXIT = 1;
constexpr const long MESSAGE_TYPE_REQUEST = 2;
constexpr const long MESSAGE_TYPE_BACKEND_ACCEPTABLE = -2;
constexpr const long MESSAGE_TYPE_RESPONSE = 3;
constexpr const long MESSAGE_TYPE_READY = 4;

constexpr const auto PROMPT = "$";

#endif
