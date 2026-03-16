#include "../include/tcp_server.h"

TcpServer::TcpServer(uint16_t port, int thread_num, bool reseAddr, bool noBlock, const std::string &ip)
    : port(port),
      thread_num(thread_num),
      _connection_id(0),
      _acceptor(&_baseloop, port, reseAddr, noBlock, ip),
      _loop_thread_pool(&_baseloop, thread_num)

{
    this->_connection_id = 0;
    this->_timer_id = 0;
    this->connected_callback = nullptr;
    this->closed_callback = nullptr;
    this->event_callback = nullptr;
    this->message_callback = nullptr;
}

TcpServer::~TcpServer() {}

void TcpServer::SetInactiveRelease(bool enable, int timeout)
{
    this->_inactive_release = enable;
    this->_inactive_timeout = timeout;
}

void TcpServer::AddTimerTask(TimerAction &action, uint64_t expireTime)
{
    this->_baseloop.AddTimerTask(this->_timer_id++, expireTime, action);
}

void TcpServer::Run()
{
    this->_acceptor.new_connection_callback = [this](Socket &&clientSock)
    {
        EventLoop *loop = this->_loop_thread_pool.GetSubEventLoop();  // 轮询分配EventLoop对象
        PtrConnection clientConnection =
            std::make_shared<Connection>(loop, clientSock.GetSocketFd(), std::move(clientSock));
        this->_connections[clientConnection->GetConnectionId()] = clientConnection;

        // 关闭连接
        clientConnection->closed_callback = this->closed_callback;

        // 连接事件
        clientConnection->connected_callback = this->connected_callback;

        // 客户端套接字可读时,业务处理
        clientConnection->message_callback = this->message_callback;

        // 任意事件处理
        clientConnection->event_callback = this->event_callback;

        // 连接删除处理
        clientConnection->_server_closed_callback = [this](const PtrConnection &conn)
        {
            this->_baseloop.RunTask(
                [this, &conn]()
                {
                    // LOG(INFO, "Client disconnected, id: " << conn->GetConnectionId() << "\n\tconnection Address: "
                    //                                       << conn << ", Loop thread Id: " <<
                    //                                       conn->GetLoopThreadId());
                    this->_connections.erase(conn->GetConnectionId());
                });  // 从连接列表中移除连接对象
        };

        clientConnection->SetInactiveRelease(this->_inactive_release, this->_inactive_timeout);
        clientConnection->Established();  // 连接就绪初始化,启动可读监控
    };

    this->_acceptor.Listen();  // 启动监听套接字的可读事件监控,当可读时说明有新连接到来
    this->_baseloop.Start();   // 启动事件循环,监控事件
}