#include "../include/echo_server.h"

EchoServer::EchoServer(uint16_t port, int thread_num, bool reseAddr, bool noBlock, const std::string &ip)
    : _server(port, thread_num, reseAddr, noBlock, ip)
{
    this->_server.SetInactiveRelease(true, 10);
    this->_server.connected_callback = [](const PtrConnection &conn)
    {
        LOG(INFO, "\nNew connection established: \n\tconnection Address: " << conn << ", Loop thread Id:"
                                                                           << conn->GetLoopThreadId());
    };

    this->_server.closed_callback = [](const PtrConnection &conn)
    {
        LOG(INFO,
            "\nConnection closed: \n\tconnection Address: " << conn << ", Loop thread Id:" << conn->GetLoopThreadId());
    };

    this->_server.message_callback = [](const PtrConnection &conn, Buffer *buffer)
    {
        // 回显接受缓冲区内容
        LOG(INFO, "Received message , id: " << conn->GetConnectionId()
                                            << ", \nmessage: " << buffer->Read(buffer->GetReadableSize()));
        // 客户端套接字可写时发送
        conn->Send("Server Send: Hello Client");

        // 测试: 通信一次直接关闭连接
        // conn->Close();
    };
}

void EchoServer::Run() { this->_server.Run(); }