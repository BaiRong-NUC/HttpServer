#include"../../include/channel.h"

void testChannel()
{
    // 创建一个socket
    Socket sock;
    sock.Create();
    Channel channel(sock);
    // 设置回调函数
    channel.readAnction = []() { std::cout << "Read event triggered!" << std::endl; };
    channel.writeAnction = []() { std::cout << "Write event triggered!" << std::endl; };
    channel.errorAnction = []() { std::cout << "Error event triggered!" << std::endl; };
    channel.closeAnction = []() { std::cout << "Close event triggered!" << std::endl; };
    channel.eventAnction = []() { std::cout << "Some event triggered!" << std::endl; };

    // 模拟事件触发
    channel.SetRevents(EPOLLIN | EPOLLOUT | EPOLLERR | EPOLLHUP);
    channel.HandleEvent();
}

int main(int argc, char const *argv[])
{
    testChannel();
    return 0;
}
