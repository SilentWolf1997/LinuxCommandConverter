#include "definations.hh"
#include "message_queue.hh"
#include "named_pipe.hh"

#include <iostream>
#include <fcntl.h>
#include <unistd.h>
#include <tuple>

void backend(bool verbose = false);

int main(int argc, char *argv[]) {
    int ch;
    bool verbose = false;
    while ((ch = getopt(argc, argv, "v")) != -1) {
        switch (ch) {
        case 'v':
            verbose = true;
            break;
        default:
            std::cout << "Unknown argument: " << ch << std::endl;
            exit(EXIT_FAILURE);
        }
    }
    backend(verbose);
    return 0;
}

void backend(bool verbose) {
    if (verbose) {
        std::cout << "[BE] Preparing IPC..." << std::endl;
    }
    named_pipe np(NAMED_PIPE_PATH, O_WRONLY);
    message_queue msq(MESSAGE_QUEUE_KEY);
    long msg_type;
    std::string msg_data;

    if (verbose) {
        std::cout << "[BE] Ready. Starting main loop..." << std::endl;
    }
    msq.send(MESSAGE_TYPE_READY, "");

    while (true) {
        std::tie(msg_type, msg_data) = msq.receive(MESSAGE_TYPE_BACKEND_ACCEPTABLE);
        if (msg_type == MESSAGE_TYPE_REQUEST) {
            if (verbose) {
                std::cout << "[BE] Receiving message. Request: '" << msg_data << "'" << std::endl;
            }
            FILE *ppipe = popen(msg_data.c_str(), "r");
            if (!ppipe) {
                perror("popen");
                exit(EXIT_FAILURE);
            } else {
                np.pipe_from(ppipe);

                if (verbose) {
                    std::cout << "[BE] Command execution finished. Sending response..." << std::endl;
                }
                msq.send(MESSAGE_TYPE_RESPONSE, "");
            }
        }
        else if (msg_type == MESSAGE_TYPE_EXIT) {
            if (verbose) {
                std::cout << "[BE] Receiving message. Exiting..." << std::endl;
            }
            exit(EXIT_SUCCESS);
        }
    }
}
