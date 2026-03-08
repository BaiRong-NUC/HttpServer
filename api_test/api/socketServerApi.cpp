#include "../include/socket.h"
#include "../include/poller.h"
#include "../include/channel.h"

#include <utility>

// 客户端关闭事件处理函数
void ClientCloseHandler(Channel *client_channel)
{
    LOG(INFO, "Close Client Connection: " << client_channel->GetSocket().GetSocketFd());
    client_channel->Remove();
    delete client_channel;
}

// 客户端可读事件处理函数
void ClientReadHandler(Channel *client_channel)
{
    Socket &clientSock = client_channel->GetSocket();
    char buffer[1024] = {0};
    // 接受客户端数据
    ssize_t recvLen = clientSock.Recv(buffer, sizeof(buffer) - 1);
    if (recvLen < 0)
    {
        LOG(WARNING, "Recv failed");
        ClientCloseHandler(client_channel);
        return;
    }
    // 打印
    LOG(INFO, std::string(buffer, recvLen));

    // 开启客户端套接字的可写事件监控,服务器发送数据给客户端
    client_channel->EnableWrite();
}

// 客户端可写事件处理函数
void ClientWriteHandler(Channel *client_channel)
{
    Socket &clientSock = client_channel->GetSocket();
    const char *response = "Server Response: Hello Client!";
    ssize_t sendLen = clientSock.Send(response, strlen(response));
    if (sendLen < 0)
    {
        LOG(WARNING, "Send failed");
        ClientCloseHandler(client_channel);
        return;
    }

    // 发送完成后关闭可写事件监控,服务器只发送一次数据
    client_channel->DisableWrite();
}

// 客户端处理事件函数
void ClientEventHandler(Channel *client_channel)
{
    LOG(INFO, "Client Event Triggered");
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

    Poller poller;
    Channel server_channel(&poller, std::move(listenSock));
    // 监听套接字监听客户端连接事件
    server_channel.readAnction = [&]()
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
        Channel *client_channel = new Channel(&poller, std::move(clientSocket));

        // 监听客户端套接字的可读事件(客户端发送数据给服务器)
        client_channel->readAnction = std::bind(ClientReadHandler, client_channel);
        client_channel->writeAnction = std::bind(ClientWriteHandler, client_channel);
        client_channel->closeAnction = std::bind(ClientCloseHandler, client_channel);
        client_channel->eventAnction = std::bind(ClientEventHandler, client_channel);
        client_channel->errorAnction = std::bind(ClientCloseHandler, client_channel); // 错误事件也当作连接关闭处理
        client_channel->EnableRead();
    };
    server_channel.EnableRead();

    // 启动事件循环
    while (true)
    {
        std::vector<Channel *> activeChannels = poller.Poll(-1);
        for (Channel *channel : activeChannels)
        {
            channel->HandleEvent();
        }
    }
    return 0;
}
