#include "definations.hh"
#include "message_queue.hh"
#include "named_pipe.hh"
#include "converter.hh"

#include <iostream>
#include <unistd.h>
#include <fcntl.h>
#include <sys/wait.h>

void app(bool verbose = false);
void frontend(pid_t pid, bool verbose);

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
    app(verbose);
    return 0;
}

void app(bool verbose) {
    if (verbose) {
        std::cout << "[FE] Preparing named pipe..." << std::endl;
    }
    named_pipe::make_pipe(NAMED_PIPE_PATH);

    if (verbose) {
        std::cout << "[FE] Making child process..." << std::endl;
    }
    pid_t pid = fork();
    if (pid < 0) {
        perror("fork");
        exit(EXIT_FAILURE);
    }
    else if (pid == 0) {
        if (verbose) {
            execl(BACKEND_PATH, BACKEND_NAME, "-v", NULL);
        }
        else {
            execl(BACKEND_PATH, BACKEND_NAME, NULL);
        }
    }
    else {
        frontend(pid, verbose);
    }
}

void frontend(pid_t pid, bool verbose) {
    converter conv;
    std::string command;

    if (verbose) {
        std::cout << "[FE] Preparing IPC..." << std::endl;
    }
    named_pipe np(NAMED_PIPE_PATH, O_RDONLY | O_NONBLOCK);
    message_queue msq(MESSAGE_QUEUE_KEY);

    if (verbose) {
        std::cout << "[FE] Waiting for backend..." << std::endl;
    }
    msq.receive(MESSAGE_TYPE_READY);

    if (verbose) {
        std::cout << "[FE] Starting main loop..." << std::endl;
    }
    while (true) {
        std::cout << PROMPT << " ";
        std::getline(std::cin, command);
        if (command == "exit") {
            if (verbose) {
                std::cout << "[FE] Sending shutdown message to backend..." << std::endl;
            }
            msq.send(MESSAGE_TYPE_EXIT, "");
            
            if (verbose) {
                std::cout << "[FE] Waiting for backend..." << std::endl;
            }
            int stat;
            waitpid(pid, &stat, 0);

            if (verbose) {
                std::cout << "[FE] Cleaning up..." << std::endl;
            }
            msq.destroy();
            exit(EXIT_SUCCESS);
        }
        else {
            command = conv.convert(command);
            if (verbose) {
                std::cout << "[FE] Converted command: '" << command << "'" << std::endl;
            }
            msq.send(MESSAGE_TYPE_REQUEST, command);
            
            if (verbose) {
                std::cout << "[FE] Waiting for backend response..." << std::endl;
            }
            msq.receive(MESSAGE_TYPE_RESPONSE);

            if (verbose) {
                std::cout << "[FE] Receving backend response. Printing..." << std::endl;
            }
            np.pipe_to(stdout);
        }
    }
}
