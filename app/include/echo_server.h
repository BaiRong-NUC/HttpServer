#pragma once
#include "../../include/tcp_server.h"

// 应用,专门的回显服务器
class EchoServer
{
   private:
    TcpServer _server;  // TCP服务器对象
   public:
    EchoServer(uint16_t port, int thread_num = 0, bool reseAddr = true, bool noBlock = true,
               const std::string &ip = "0.0.0.0");

    void Run();
};