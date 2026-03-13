#include "../../include/socket.h"
#include "../../include/poller.h"
#include "../../include/channel.h"
#include "../../include/event_loop.h"
#include "../../include/connection.h"
#include "../../include/log.h"
#include "../../include/acceptor.h"
#include "../../include/loop_thread.h"
#include <utility>

using PtrConnection = std::shared_ptr<Connection>;

int main(int argc, char const *argv[])
{
    SetLogLevel(INFO);

    // 连接列表,保存服务器内部对连接的管理,以连接ID为键,连接对象的智能指针为值
    std::unordered_map<int, PtrConnection> connections;
    // 创建多个LoopThread对象,每个对象内部都有一个EventLoop对象,实现多线程处理连接事件
    std::vector<LoopThread> loopThreads(2);  // (从属EventLoop)
    // server_loop负责监控新连接到来
    EventLoop server_loop;
    Acceptor acceptor(&server_loop, 8085);  // 创建Acceptor对象,监听8085端口

    int threadIndex = 0;
    acceptor.new_connection_callback = [&threadIndex, &connections, &loopThreads](Socket &&clientSock)
    {
        EventLoop *loop = loopThreads[threadIndex++ % loopThreads.size()].GetEventLoop();  // 轮询分配EventLoop对象
        PtrConnection clientConnection =
            std::make_shared<Connection>(loop, clientSock.GetSocketFd(), std::move(clientSock));
        connections[clientConnection->GetConnectionId()] = clientConnection;

        // 关闭连接
        clientConnection->closed_callback = [&connections](const PtrConnection &conn)
        {
            LOG(INFO, "Client disconnected, id: " << conn->GetConnectionId());
            connections.erase(conn->GetConnectionId());  // 从连接列表中移除连接对象
        };

        // 连接事件
        clientConnection->connected_callback = [](const PtrConnection &conn)
        { LOG(INFO, "Client connected, id: " << conn->GetConnectionId() << " Map: " << conn); };

        // 客户端套接字可读时,将输入缓冲区内容放buffer里,同时调用业务处理回调
        clientConnection->message_callback = [](const PtrConnection &conn, Buffer *in_buffer)
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

    acceptor.Listen();  // 启动监听套接字的可读事件监控,当可读时说明有新连接到来

    server_loop.Start();  // 启动事件循环,监控新连接到来
    return 0;
}
