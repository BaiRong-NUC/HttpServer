#include "../include/channel.h"

// 构造函数
Channel::Channel(Socket sock) : _sock(sock), _events(0), _revents(0) {}

// 判断当前是否监控可读
bool Channel::ReadAble() { return this->_events & EPOLLIN; }

// 判断当前是否可监控可写
bool Channel::WriteAble() { return this->_events & EPOLLOUT; }

// 启动可读事件监控
void Channel::EnableRead() { this->_events |= EPOLLIN; }

// 启动可写事件监控
void Channel::EnableWrite() { this->_events |= EPOLLOUT; }

// 关闭可读事件监控
void Channel::DisableRead() { this->_events &= ~EPOLLIN; }

// 关闭可写事件监控
void Channel::DisableWrite() { this->_events &= ~EPOLLOUT; }

// 关闭所有事件监控
void Channel::DisableAll() { this->_events = 0; }

// 移除监控
void Channel::Remove() { this->_events = 0; }

// 设置就绪事件
void Channel::SetRevents(uint32_t revents) { this->_revents = revents; }

// 获取文件描述符
int Channel::GetSocketFd() { return this->_sock.GetSocketFd(); }

// 事件处理
/***
 * 先判断错误和断开事件(EPOLLERR、EPOLLHUP)
 * 一旦触发,优先处理并关闭 fd,后续事件就不再处理.
 * 提前return,保证只处理一次
 */
void Channel::HandleEvent()
{
    if (this->_revents & EPOLLERR)
    {
        // 任意事件触发回调在前面,因为错误事件可能导致连接关闭,如果先调用错误回调函数,可能会在回调函数中关闭连接,导致任意事件回调函数无法调用
        if (this->eventAnction)
            this->eventAnction();

        if (this->errorAnction)
            this->errorAnction();
        return;
    }

    if (this->_revents & EPOLLHUP)
    {
        // 任意事件触发回调在前面
        if (this->eventAnction)
            this->eventAnction();
        // 文件描述符关闭
        if (this->closeAnction)
            this->closeAnction();
        return;
    }

    if ((this->_revents & EPOLLIN) || (this->_revents & EPOLLRDHUP) || (this->_revents & EPOLLPRI))
    {
        // 可读事件或对端关闭连接或紧急数据事件触发可读回调
        if (this->readAnction)
            this->readAnction();

        // 任意事件触发回调
        if (this->eventAnction)
            this->eventAnction();
    }

    // 有可能导致连接错误的事件一次触发一个
    if (this->_revents & EPOLLOUT)
    {
        if (this->writeAnction)
            this->writeAnction();

        // 任意事件触发回调
        if (this->eventAnction)
            this->eventAnction();
    }
}