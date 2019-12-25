#ifndef __NAMED_PIPE_HH__
#define __NAMED_PIPE_HH__

#include "definations.hh"
#include "buffer.hh"
#include <stdio.h>

typedef struct _pipe_buffer_data {
    char data[PIPE_BUFFER_SIZE];
} pip_buf_data_t;

using pip_buf_t = buffer<pip_buf_data_t>;

class named_pipe {
public:
    static void make_pipe(const char* pipe_path);
private:
    int pipe_fd;
    int open_mode;
public:
    named_pipe(const char* pipe_path, int mode);
    named_pipe(const named_pipe& other) = delete;
    named_pipe(named_pipe&& other) = delete;
    ~named_pipe();

    void pipe_from(FILE *fp);
    void pipe_to(FILE *fp);
};

#endif
