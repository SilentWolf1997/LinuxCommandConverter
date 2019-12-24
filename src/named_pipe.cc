#include "named_pipe.hh"

#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

void named_pipe::make_pipe(const char* pipe_path) {
    if (access(pipe_path, F_OK) == 0) {
        unlink(pipe_path);
    }
    int res = mkfifo(pipe_path, 0666);
    if (res != 0) {
        // TODO: Error handling
        perror("make pipe");
        exit(EXIT_FAILURE);
    }
}

named_pipe::named_pipe(const char* pipe_path, int mode) {
    pipe_fd = open(pipe_path, mode);
    if (pipe_fd == -1) {
        // TODO: Error handling
        perror("open pipe");
        exit(EXIT_FAILURE);
    }
}

named_pipe::~named_pipe() {
    close(pipe_fd);
}

void named_pipe::pipe_from(FILE *fp) {
    pip_buf_t buf;
    unsigned int read_size;
    while((read_size = fread(buf->data, sizeof(char), PIPE_BUFFER_SIZE, fp)) > 0) {
        write(pipe_fd, buf->data, read_size);
    }
}

void named_pipe::pipe_to(FILE *fp) {
    pip_buf_t buf;
    int read_size;
    while((read_size = read(pipe_fd, buf->data, PIPE_BUFFER_SIZE)) > 0) {
        fwrite(buf->data, sizeof(char), read_size, fp);
    }
}

