/*

backend.c  后端程序

程序流程：
1. 连接命名管道和消息队列
2. 向前端发送准备就绪信号
3. 执行主循环
    3.1. 接收命令
    3.2. 执行命令
    3.3. 向前端发送就绪信号

*/

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/msg.h>
#include <signal.h>

#define NAMED_PIPE_PATH         "/tmp/mypipe"       // 命名管道路径
#define MESSAGE_QUEUE_KEY       0x12345678          // 消息队列的key，其实应该生成，但是我懒了
#define MESSAGE_DATA_SIZE       256                 // 消息队列数据部分的大小
#define RESULT_BUFFER_SIZE      256                 // 接收执行结果的缓冲区大小

// 消息类型
// 因为后端需要接收 1 和 2 ，消息队列中可以接收小于等于某个类型的消息，因此这两个放在最前面

#define MESSAGE_TYPE_COMMAND    1   // 发出命令
#define MESSAGE_TYPE_EXIT       2   // 退出程序
#define MESSAGE_TYPE_READY      3   // 后端就绪

#define MESSAGE_TYPE_BACKEND_ACCEPTABLE -2  // 后端在主循环可以接收 1 和 2

int msq_id = -1;    // 消息队列的 ID
int pipe_fd = -1;   // 命名管道的文件描述符

// 消息队列中的消息
struct message {
    long type;                              // 消息类型
    char data[MESSAGE_DATA_SIZE];           // 消息内容
};

// 信号处理函数
void sig_handler(int sig_type) {
    switch (sig_type) {
    case SIGINT:
        // 关闭命名管道
        close(pipe_fd);
        // 异常退出
        exit(EXIT_FAILURE);
        break;
    default:
        break;
    }
}

// 后端
int main(int argc, char *argv[]) {
    int res;                                // 存放各种系统调用的返回结果
    struct message msg_buffer;              // 读写消息时的缓冲区
    char res_buffer[RESULT_BUFFER_SIZE];    // 接收执行结果的缓冲区
    int res_size;                           // 执行结果的读取字节数

    ////////////////////////////////////////
    //
    // 1. 连接命名管道和消息队列
    //
    ////////////////////////////////////////

    // 获取消息队列
    msq_id = msgget(MESSAGE_QUEUE_KEY, 0666);
    if (msq_id == -1) {
        perror("后端获取消息队列");
        exit(EXIT_FAILURE);
    }
    // 后端以只写方式打开管道
    pipe_fd = open(NAMED_PIPE_PATH, O_WRONLY);
    if (pipe_fd == -1) {
        perror("后端打开管道文件");
        exit(EXIT_FAILURE);
    }

    // 注册信号处理函数
    signal(SIGINT, sig_handler);

    ////////////////////////////////////////
    //
    // 2. 向前端发送准备就绪信号
    //
    ////////////////////////////////////////

    // 这里要和前端采用的方法相对应
    // 这里的前端使用了就绪消息
    msg_buffer.type = MESSAGE_TYPE_READY;
    res = msgsnd(msq_id, &msg_buffer, MESSAGE_DATA_SIZE, 0);
    if (res == -1) {
        // 错误处理，虽然不关闭管道操作系统也会帮着关
        perror("后端发送消息");
        close(pipe_fd);
        exit(EXIT_FAILURE);
    }

    ////////////////////////////////////////
    //
    // 3. 执行主循环
    //
    ////////////////////////////////////////

    while (1) {
        
        ////////////////////////////////////////
        //
        // 3.1. 接收命令
        //
        ////////////////////////////////////////

        // 从消息队列接收消息
        res = msgrcv(msq_id, &msg_buffer, MESSAGE_DATA_SIZE, MESSAGE_TYPE_BACKEND_ACCEPTABLE, 0);
        if (res == -1) {
            // 错误处理
            perror("后端接收消息");
            close(pipe_fd);
            exit(EXIT_FAILURE);
        }

        ////////////////////////////////////////
        //
        // 3.2. 执行命令
        //
        ////////////////////////////////////////

        if (msg_buffer.type == MESSAGE_TYPE_COMMAND) {
            // 通过popen执行命令
            FILE *pp = popen(msg_buffer.data, "r");
            if (pp == NULL) {
                // 错误处理，这里不是系统调用错误，perror不一定正确，所以要自己写错误信息
                fprintf(stderr, "后端执行命令失败");
                close(pipe_fd);
                exit(EXIT_FAILURE);
            }

            // 写入管道
            while ((res_size = fread(res_buffer, 1, RESULT_BUFFER_SIZE, pp)) > 0) {
                write(pipe_fd, res_buffer, res_size);
            }

            // 写入完毕后要关闭通过popen打开的管道
            pclose(pp);
        }
        else if (msg_buffer.type == MESSAGE_TYPE_EXIT) {
            // 前端发送退出信号，关闭管道，正常退出
            close(pipe_fd);
            exit(EXIT_SUCCESS);
        }

        ////////////////////////////////////////
        //
        // 3.3. 向前端发送就绪信号
        //
        ////////////////////////////////////////
        
        msg_buffer.type = MESSAGE_TYPE_READY;
        res = msgsnd(msq_id, &msg_buffer, MESSAGE_DATA_SIZE, 0);
        if (res == -1) {
            // 错误处理
            perror("后端发送消息");
            close(pipe_fd);
            exit(EXIT_FAILURE);
        }

    }

    return 0;
}
