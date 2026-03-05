// Linux系统定时器
#include <sys/timerfd.h>
#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <cstdint>

int main(int argc, char const *argv[])
{
    // CLOCK_MONOTONIC: 系统启动时间为基准,阻塞
    int timerfd = timerfd_create(CLOCK_MONOTONIC, 0);
    if (timerfd == -1)
    {
        perror("timerfd_create error");
        return -1;
    }
    struct itimerspec new_value;
    new_value.it_value.tv_sec = 5; // 初始过期时间为5
    new_value.it_value.tv_nsec = 0;

    new_value.it_interval.tv_sec = 2; // 间隔时间为2
    new_value.it_interval.tv_nsec = 0;

    // TFD_TIMER_ABSTIME: 表示定时器的超时时间是绝对时间;
    // 如果未设置此标志,超时时间是相对时间(从当前时间开始计算)
    if (timerfd_settime(timerfd, 0, &new_value, NULL) == -1)
    {
        perror("timerfd_settime error");
        return -1;
    }

    while (true)
    {
        uint64_t expirations; // 8字节
        // 每次超时向文件描述符中写入8字节数据
        ssize_t s = read(timerfd, &expirations, sizeof(expirations));
        if (s != sizeof(expirations))
        {
            perror("read error");
            return -1;
        }
        printf("Timer expired %llu times\n", (unsigned long long)expirations);
    }
    return 0;
}
