/*

frontend.c  前端程序

程序流程：
1. 创建命名管道和消息队列
2. 启动后端
3. 等待后端就绪
4. 执行主循环
    4.1. 输入命令
    4.2. 转换命令
    4.3. 将转换后的命令发送至后端
    4.4. 等待后端执行
    4.5. 输出执行结果

*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/msg.h>
#include <sys/wait.h>
#include <signal.h>

#define NAMED_PIPE_PATH         "/tmp/mypipe"       // 命名管道路径
#define MESSAGE_QUEUE_KEY       0x12345678          // 消息队列的key，其实应该生成，但是我懒了
#define MESSAGE_DATA_SIZE       256                 // 消息队列数据部分的大小
#define RESULT_BUFFER_SIZE      256                 // 接收执行结果的缓冲区大小
#define BACKEND_PATH            "./backend"         // 后端路径
#define BACKEND_NAME            "backend"           // 后端名称（作为命令行参数传递）
#define PROMPT                  "$"                 // 提示符

// 消息类型
// 因为后端需要接收 1 和 2 ，消息队列中可以接收小于等于某个类型的消息，因此这两个放在最前面

#define MESSAGE_TYPE_COMMAND    1   // 发出命令
#define MESSAGE_TYPE_EXIT       2   // 退出程序
#define MESSAGE_TYPE_READY      3   // 后端就绪

int msq_id = -1;    // 消息队列的 ID
int pipe_fd = -1;   // 命名管道的文件描述符
pid_t child_pid = -1;

// 消息队列中的消息
struct message {
    long type;                              // 消息类型
    char data[MESSAGE_DATA_SIZE];           // 消息内容
};

// 信号处理函数，在主函数里面会说明作用
void sig_handler(int sig_type) {
    int status;
    switch (sig_type) {
    case SIGCHLD:
        printf("\n");
        // 因为只有一个子进程，因此可以直接等待
        wait(&status);
        // 无论如何，都要回收资源
        if (pipe_fd > -1) {
            close(pipe_fd);                 // 关闭管道
            unlink(NAMED_PIPE_PATH);
        }
        if (msq_id > -1) {
            msgctl(msq_id, IPC_RMID, NULL); // 删除消息队列
        }
        if (status == 0) {
            // 子进程正常退出，父进程也正常退出
            exit(EXIT_SUCCESS);
        }
        else {
            // 子进程异常退出，父进程也异常退出
            exit(EXIT_FAILURE);
        }
        break;
    case SIGINT:
        // 收到中断信号，要传递给后端
        // 然后当后端结束的时候会收到 SIGCHLD 从而结束整个程序
        kill(child_pid, SIGINT);
        // 等待子进程结束
        pause();
        break;
    default:
        break;
    }
}

// 前端
int main(int argc, char *argv[]) {
    int res;                                // 存放各种系统调用的返回结果
    char command[MESSAGE_DATA_SIZE];        // 存放输入命令的缓冲区
    char args[MESSAGE_DATA_SIZE];           // 存放命令参数的缓冲区
    struct message msg_buffer;              // 读写消息时的缓冲区
    char res_buffer[RESULT_BUFFER_SIZE];    // 接收执行结果的缓冲区
    int res_size;                           // 执行结果的读取字节数

    ////////////////////////////////////////
    //
    // 1. 创建消息队列和命名管道
    //
    ////////////////////////////////////////
    
    // 获取消息队列
    msq_id = msgget(MESSAGE_QUEUE_KEY, 0666 | IPC_CREAT);
    if (msq_id == -1) {
        // 错误处理
        perror("前端获取消息队列");
        exit(EXIT_FAILURE);
    }
    // 如果命名管道文件（同名文件）已经存在，删除
    if (access(NAMED_PIPE_PATH, F_OK) == 0) {
        res = unlink(NAMED_PIPE_PATH);
    }
    // 创建命名管道文件
    res = mkfifo(NAMED_PIPE_PATH, 0666);
    if (res == -1) {
        // 错误处理
        perror("前端创建管道文件");
        exit(EXIT_FAILURE);
    }
    // 前端以只读非阻塞方式打开管道
    // 如果把打开管道这一步放在启动后端之后则不用非阻塞方式，并且不用单独等待后端就绪
    pipe_fd = open(NAMED_PIPE_PATH, O_RDONLY | O_NONBLOCK);
    if (pipe_fd == -1) {
        // 错误处理
        perror("前端打开管道文件");
        exit(EXIT_FAILURE);
    }

    ////////////////////////////////////////
    //
    // 2. 启动后端
    //
    ////////////////////////////////////////

    // 创建子进程
    child_pid = fork();
    if (child_pid == -1) {
        // 错误处理
        perror("前端创建子进程");
        exit(EXIT_FAILURE);
    }
    else if (child_pid == 0) {
        // 子进程启动后端
        execl(BACKEND_PATH, BACKEND_NAME, NULL);
    }
    // 否则为父进程，继续执行前端
    // 从这里开始要注意，前端因为错误而异常退出时要通知后端，同理，后端异常退出时前端也要退出
    // 因此在这里注册信号处理函数
    signal(SIGINT, sig_handler);
    signal(SIGCHLD, sig_handler);

    ////////////////////////////////////////
    //
    // 3. 等待后端就绪
    //
    ////////////////////////////////////////

    // 这里等待有几种方法：
    // 1. 把上面的打开管道改成阻塞方式然后放到这里，当前后端都打开了管道的时候继续
    // 2. 等待后端发来特定信号
    // 3. 等待后端发来就绪消息
    // 这里采用第三种，反正就绪消息后面还要用，也不浪费
    res = msgrcv(msq_id, &msg_buffer, MESSAGE_DATA_SIZE, MESSAGE_TYPE_READY, 0);
    if (res == -1) {
        // 错误处理，上面已经注册了信号处理函数，因此可以直接给后端发中断让它异常退出
        perror("前端接收消息");
        kill(child_pid, SIGINT);
        pause();
    }

    ////////////////////////////////////////
    //
    // 4. 执行主循环
    //
    ////////////////////////////////////////

    while (1) {
        
        ////////////////////////////////////////
        //
        // 4.1. 输入命令
        //
        ////////////////////////////////////////

        printf("%s ", PROMPT);
        //
        // 这个方法有问题
        //
        // res = scanf("%s%*[ ]%[^\n]", command, args);
        // getchar();
        // if (res <= 1) {
        //     args[0] = '\0';
        // }

        // 从输入读取一行
        // 用gets也行，但是因为 C11 标准把 gets 删掉了因此编译的时候要加上 -std=C99
        fgets(command, MESSAGE_DATA_SIZE, stdin);
        int i, j, flag = 0;

        // 查找输入中除去前导空格外的第一个空格
        for (i = 0; i < MESSAGE_DATA_SIZE; i++) {
            if (command[i] == ' ') {
                if (flag) {
                    // 找到后从这里分割
                    command[i] = '\0';
                    j = 0;
                    for (i = i + 1; i < MESSAGE_DATA_SIZE; i++) {
                        if (command[i] == '\n') {
                            command[i] = '\0';
                        }
                        args[j++] = command[i];
                        if (command[i] == '\0'){
                            break;
                        }
                    }
                    break;
                }
            }
            else if (command[i] == '\n') {
                // 在找到空格前找到了换行符，命令结束，参数为空
                command[i] = '\0';
                args[0] = '\0';
                break;
            }
            else {
                flag = 1;
            }
        }

        ////////////////////////////////////////
        //
        // 4.2. 转换命令
        //
        ////////////////////////////////////////

        if (strcmp(command, "dir") == 0) {
            strcpy(msg_buffer.data, "ls");
            if (strlen(args) > 0) {
                msg_buffer.data[2] = ' ';
                strcpy(msg_buffer.data + 3, args);
            }
        }
        else if (strcmp(command, "rename") == 0) {
            strcpy(msg_buffer.data, "mv ");
            strcpy(msg_buffer.data + 3, args);
        }
        else if (strcmp(command, "move") == 0) {
            strcpy(msg_buffer.data, "mv ");
            strcpy(msg_buffer.data + 3, args);
        }
        else if (strcmp(command, "del") == 0) {
            strcpy(msg_buffer.data, "rm ");
            strcpy(msg_buffer.data + 3, args);
        }
        else if (strcmp(command, "cd") == 0) {
            if (strlen(args) > 0) {
                strcpy(msg_buffer.data, "cd ");
                strcpy(msg_buffer.data + 3, args);
            }
            else {
                strcpy(msg_buffer.data, "pwd");
            }
        }
        else {
            strcpy(msg_buffer.data, command);
            if (strlen(args) > 0) {
                msg_buffer.data[strlen(command)] = ' ';
                strcpy(msg_buffer.data + strlen(command) + 1, args);
            }
        }

        ////////////////////////////////////////
        //
        // 4.3. 将转换后的命令发送至后端
        //
        ////////////////////////////////////////

        if (strcmp(command, "exit") == 0) {
            msg_buffer.type = MESSAGE_TYPE_EXIT;
        }
        else {
            msg_buffer.type = MESSAGE_TYPE_COMMAND;
        }
        res = msgsnd(msq_id, &msg_buffer, MESSAGE_DATA_SIZE, 0);
        if (res == -1) {
            // 错误处理
            perror("前端发送消息");
            kill(child_pid, SIGINT);
            pause();
        }

        ////////////////////////////////////////
        //
        // 4.4. 等待后端执行
        //
        ////////////////////////////////////////

        res = msgrcv(msq_id, &msg_buffer, MESSAGE_DATA_SIZE, MESSAGE_TYPE_READY, 0);
        if (res == -1) {
            // 错误处理
            perror("前端接收消息");
            kill(child_pid, SIGINT);
            pause();
        }

        ////////////////////////////////////////
        //
        // 4.5. 输出执行结果
        //
        ////////////////////////////////////////

        while ((res_size = read(pipe_fd, res_buffer, RESULT_BUFFER_SIZE)) > 0) {
            fwrite(res_buffer, 1, res_size, stdout);
        }

    } // 主循环结束

    return 0;
}