#include <sys/eventfd.h>
#include <stdint.h>
#include <stdio.h>
#include <unistd.h>
// count: 事件计数器的初始值，flags: 事件fd的标志位
// 创建一个文件描述符用于实现事件通知,文件内部的值为通知次数
// EFD_CLOEXEC: 禁止进程复制
// EFD_NONBLOCK: 非阻塞模式
// 返回文件描述符

// 注意: eventfd read/write 操作的值必须是8字节的整数,否则会返回EINVAL错误
// int eventfd(unsigned int count, int flags);
int main(int argc, char const *argv[])
{
    int efd = eventfd(0, EFD_CLOEXEC | EFD_NONBLOCK);
    if (efd < 0)
    {
        perror("eventfd");
        return -1;
    }
    uint64_t value = 1;
    write(efd, &value, sizeof(value)); // 发送事件通知
    write(efd, &value, sizeof(value)); // 发送事件通知
    write(efd, &value, sizeof(value)); // 发送事件通知

    uint64_t count;
    read(efd, &count, sizeof(count));             // 接收事件通知
    printf("Received event count: %lu\n", count); // 输出接收到的事件

    close(efd);
    return 0;
}