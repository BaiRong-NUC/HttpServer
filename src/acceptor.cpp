#include "../include/acceptor.h"

Acceptor::Acceptor(EventLoop *event_loop, uint16_t port) : _event_loop(event_loop)
{
    Socket listen_sock;
    listen_sock.CreateServer(port);  // 创建监听套接字
    this->_listen_channel =
        std::make_shared<Channel>(event_loop, std::move(listen_sock));  // 创建监听套接字的Channel对象

    this->_listen_channel->readAction = std::bind(&Acceptor::_HandleRead, this);  // 设置监听套接字的可读事件回调函数
    this->_listen_channel->EnableRead();                                          // 启动监听套接字的
}

void Acceptor::_HandleRead()
{
    std::string clientIp;
    uint16_t clientPort;
    Socket clientSock;
    if (this->_listen_channel->GetSocket().Accept(clientIp, clientPort, clientSock))
    {
        LOG(INFO, "New connection from " + clientIp + ":" + std::to_string(clientPort));
        if (this->new_connection_callback)
        {
            // 调用新连接事件回调函数,将新连接的套接字对象作为参数传递
            this->new_connection_callback(clientSock);
        }
    }
    else
    {
        LOG(ERROR, "Failed to accept new connection");
    }
}