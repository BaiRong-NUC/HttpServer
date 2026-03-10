#include "../../include/socket.h"
#include "../../include/poller.h"
#include "../../include/channel.h"
#include "../../include/event_loop.h"
#include <utility>

// 客户端关闭事件处理函数
void ClientCloseHandler(EventLoop *loop, Channel *client_channel)
{
    int fd = client_channel->GetSocket().GetSocketFd();
    // 取消连接时需要取消定时器
    // 否则连接先被事件回调关闭并 delete，但定时器还持有同一个裸指针，超时后再次回调导致二次释放
    loop->CancelTimerTask(fd);
    LOG(INFO, "Close Client Connection: " << fd);
    client_channel->Remove();
    delete client_channel;
}

// 客户端可读事件处理函数
void ClientReadHandler(EventLoop *loop, Channel *client_channel)
{
    Socket &clientSock = client_channel->GetSocket();
    char buffer[1024] = {0};
    // 接受客户端数据
    ssize_t recvLen = clientSock.Recv(buffer, sizeof(buffer) - 1);
    if (recvLen < 0)
    {
        LOG(WARNING, "Recv failed");
        ClientCloseHandler(loop, client_channel);
        return;
    }
    // 打印
    LOG(INFO, std::string(buffer, recvLen));

    // 开启客户端套接字的可写事件监控,服务器发送数据给客户端
    client_channel->EnableWrite();
}

// 客户端可写事件处理函数
void ClientWriteHandler(EventLoop *loop, Channel *client_channel)
{
    Socket &clientSock = client_channel->GetSocket();
    const char *response = "Server Response: Hello Client!";
    ssize_t sendLen = clientSock.Send(response, strlen(response));
    if (sendLen < 0)
    {
        LOG(WARNING, "Send failed");
        ClientCloseHandler(loop, client_channel);
        return;
    }

    // 发送完成后关闭可写事件监控,服务器只发送一次数据
    client_channel->DisableWrite();
}

// 客户端处理事件函数
void ClientEventHandler(Channel *client_channel, EventLoop *loop)
{
    // 任意事件刷新定时器,说明客户端活跃,重置定时器到期时间
    loop->RefreshTimerTask(client_channel->GetSocket().GetSocketFd(), 10);
    LOG(INFO, "Client Event Timer Refresh: " << client_channel->GetSocket().GetSocketFd());
}

int main(int argc, char const *argv[])
{
    SetLogLevel(INFO);
    Socket listenSock;
    // bool ret = listenSock.CreateServer(8085);
    bool ret = listenSock.CreateServer(8085, true, false);
    if (!ret)
    {
        LOG(ERROR, "Failed to create server socket");
        return -1;
    }

    EventLoop loop;
    Channel server_channel(&loop, std::move(listenSock));
    // 监听套接字监听客户端连接事件
    server_channel.readAction = [&]()
    {
        std::string clientIp = "";
        uint16_t clientPort = 0;
        Socket clientSocket;
        bool accepted = server_channel.GetSocket().Accept(clientIp, clientPort, clientSocket);
        if (!accepted)
        {
            LOG(WARNING, "Accept failed");
            return;
        }
        LOG(INFO, "New Client Connection: " << clientIp << ":" << clientPort);
        Channel *client_channel = new Channel(&loop, std::move(clientSocket));

        // 非活跃连接定时器,如果客户端在10秒内没有发送数据则关闭连接,必须在客户端启动可读事件监控之前添加定时器
        loop.AddTimerTask(client_channel->GetSocket().GetSocketFd(), 10, [client_channel, &loop]()
                          {
            LOG(INFO, "Client Connection Timeout: " << client_channel->GetSocket().GetSocketFd());
            ClientCloseHandler(&loop, client_channel); });

        // 监听客户端套接字的可读事件(客户端发送数据给服务器)
        client_channel->readAction = std::bind(ClientReadHandler, &loop, client_channel);
        client_channel->writeAction = std::bind(ClientWriteHandler, &loop, client_channel);
        client_channel->closeAction = std::bind(ClientCloseHandler, &loop, client_channel);
        client_channel->eventAction = std::bind(ClientEventHandler, client_channel, &loop);
        client_channel->errorAction = std::bind(ClientCloseHandler, &loop, client_channel); // 错误事件也当作连接关闭处理
        client_channel->EnableRead();
    };
    server_channel.EnableRead();

    while (true)
    {
        loop.Start();
    }

    return 0;
}
