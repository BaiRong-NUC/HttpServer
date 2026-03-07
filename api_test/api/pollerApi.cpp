#include "../../include/poller.h"
#include "../../include/socket.h"
#include <memory>
#include <thread>
#include <chrono>

// 简单的回调函数
void TestRead() { std::cout << "Read event triggered!" << std::endl; }
void TestWrite() { std::cout << "Write event triggered!" << std::endl; }

int main(int argc, char const *argv[])
{
    // 创建监听Socket
    Socket listenSock;
    if (!listenSock.CreateServer(12345))
    {
        std::cout << "Failed to create server socket" << std::endl;
        return -1;
    }
    // 创建Channel
    Channel channel(listenSock);
    channel.readAnction = TestRead;
    channel.writeAnction = TestWrite;
    channel.EnableRead();
    // 创建Poller
    Poller poller;
    poller.AddChannel(&channel);
    std::cout << "Channel added to Poller." << std::endl;
    // 事件轮询
    std::vector<Channel *> activeChannels = poller.Poll(1000); // 1秒超时
    for (auto ch : activeChannels)
    {
        ch->HandleEvent();
    }
    poller.RemoveChannel(&channel);
    std::cout << "Channel removed from Poller." << std::endl;
    return 0;
}
