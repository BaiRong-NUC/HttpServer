#include "../../include/socket.h"
#include "../../include/poller.h"
#include "../../include/channel.h"
#include "../../include/event_loop.h"
#include "../../include/connection.h"
#include <utility>

using PtrConnection = std::shared_ptr<Connection>;

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
    // 连接列表,保存服务器内部对连接的管理,以连接ID为键,连接对象的智能指针为值
    std::unordered_map<int, PtrConnection> connections;

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

        PtrConnection clientConnection =
            std::make_shared<Connection>(&loop, clientSocket.GetSocketFd(), std::move(clientSocket));
        connections[clientConnection->GetConnectionId()] = clientConnection;

        // 关闭连接
        clientConnection->closed_callback = [&](const PtrConnection &conn)
        {
            LOG(INFO, "Client disconnected, id: " << conn->GetConnectionId());
            connections.erase(conn->GetConnectionId());  // 从连接列表中移除连接对象
        };

        // 连接事件
        clientConnection->connected_callback = [&](const PtrConnection &conn)
        { LOG(INFO, "Client connected, id: " << conn->GetConnectionId() << " Map: " << clientConnection); };

        // 客户端套接字可读时,将输入缓冲区内容放buffer里,同时调用业务处理回调
        clientConnection->message_callback = [&](const PtrConnection &conn, Buffer *in_buffer)
        {
            // 回显接受缓冲区内容
            LOG(INFO, "Received message , id: " << conn->GetConnectionId()
                                                << ", message: " << in_buffer->Read(in_buffer->GetReadableSize()));
            // 客户端套接字可写时发送
            conn->Send("Server Send: Hello Client");

            // 测试: 通信一次直接关闭连接
            // conn->Close();
        };

        clientConnection->SetInactiveRelease(true, 10);  // 设置连接不活跃时自动释放连接的机制,以s为单位
        clientConnection->Established();                 // 连接就绪初始化,启动可读监控
    };
    server_channel.EnableRead();

    while (true)
    {
        loop.Start();
    }

    return 0;
}
