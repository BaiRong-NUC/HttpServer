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

// 事件处理
void Channel::HandleEvent()
{
    if ((this->_revents & EPOLLIN) || (this->_revents & EPOLLRDHUP)||(this->_revents & EPOLLPRI))
    {
        // 可读事件或对端关闭连接或紧急数据事件触发可读回调
        if (this->readAnction)
            this->readAnction();
    }
    if (this->_revents & EPOLLOUT)
    {
        if (this->writeAnction)
            this->writeAnction();
    }
    if (this->_revents & EPOLLERR)
    {
        if (this->errorAnction)
            this->errorAnction();
    }
    if (this->_revents & EPOLLHUP)
    {
        // 文件描述符关闭
        if (this->closeAnction)
            this->closeAnction();
    }
    if (this->eventAnction)
        this->eventAnction();
}