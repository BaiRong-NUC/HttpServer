#include "../../include/tcp_server.h"
#include <utility>

int main(int argc, char const *argv[])
{
    SetLogLevel(INFO);
    // 获取计算机CPU核心数量,作为从属线程数量
    int thread_num = std::thread::hardware_concurrency();
    TcpServer server(8085, thread_num);
    server.SetInactiveRelease(true, 10);
    server.connected_callback = [](const PtrConnection &conn)
    {
        LOG(INFO, "\nNew connection established: \n\tconnection Address: " << conn << ", Loop thread Id:"
                                                                           << conn->GetLoopThreadId());
    };

    server.closed_callback = [](const PtrConnection &conn)
    {
        LOG(INFO,
            "\nConnection closed: \n\tconnection Address: " << conn << ", Loop thread Id:" << conn->GetLoopThreadId());
    };

    server.message_callback = [](const PtrConnection &conn, Buffer *buffer)
    {
        // 回显接受缓冲区内容
        LOG(INFO, "Received message , id: " << conn->GetConnectionId()
                                            << ", \nmessage: " << buffer->Read(buffer->GetReadableSize()));
        // 客户端套接字可写时发送
        conn->Send("Server Send: Hello Client");

        // 测试: 通信一次直接关闭连接
        // conn->Close();
    };

    server.Run();

    return 0;
}
