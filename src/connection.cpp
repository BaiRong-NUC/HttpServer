#include "../include/connection.h"

void Connection::_HandleRead()
{
    char buffer[BUFFER_DEFAULT_SIZE] = {0};
    int ret = this->_sock.Recv(buffer, BUFFER_DEFAULT_SIZE, MSG_DONTWAIT);
    if (ret < 0)
    {
        LOG(WARNING, "Failed to read from socket, connection will be closed");
        // 查看缓冲区是否有数据再决定删除
        this->Close();
        return;
    }
    else if (ret == 0)
    {
        LOG(WARNING, "No more data to read");
        return;
    }
    else
    {
        // 将读取到的数据写入输入缓冲区
        this->_in_buffer.Write(buffer, ret);

        // 调用业务处理回调函数
        if (this->_message_callback && this->_in_buffer.GetReadableSize() > 0)
        {
            this->_message_callback(shared_from_this(), &this->_in_buffer);
        }
    }
}

void Connection::_HandleWrite()
{
    if (this->_out_buffer.GetReadableSize() == 0)
    {
        // 没有数据需要发送,关闭可写事件监控,否则对端缓冲区有空间会频繁触发写事件,但是没有数据可写
        this->_channel.DisableWrite();
        if (this->_state == ConnectState::DISCONNECTING)
        {
            // 连接处于待关闭状态且没有数据需要发送,可以真正关闭连接了
            this->_Release();
        }
        return;
    }

    std::string buffer = this->_out_buffer.Read(this->_out_buffer.GetReadableSize());
    int ret = this->_sock.Send(buffer.c_str(), buffer.size(), MSG_DONTWAIT);
    if (ret < 0)
    {
        LOG(WARNING, "Failed to write to socket, connection will be closed");
        // 处理缓冲区数据
        if (this->_in_buffer.GetReadableSize() > 0)
        {
            // 连接异常,但输入缓冲区还有数据未处理,调用业务处理回调函数处理剩余数据
            if (this->_message_callback)
            {
                this->_message_callback(shared_from_this(), &this->_in_buffer);
            }
        }
        // this->Close();
        // 没有数据,可以直接真正删除
        this->_Release();
        return;
    }
    else if (ret == 0)
    {
        // 发送数据缓冲区为0,提示
        LOG(WARNING, "No more data can be sent");
        return;
    }
    else
    {
        // TODO: 待处理
    }
}

void Connection::_HandleClose()
{
    // 处理缓冲区数据
    if (this->_in_buffer.GetReadableSize() > 0)
    {
        // 输入缓冲区还有数据未处理,调用业务处理回调函数处理剩余数据
        if (this->_message_callback)
        {
            this->_message_callback(shared_from_this(), &this->_in_buffer);
        }
    }
    // 没有数据,可以直接真正删除
    this->_Release();
}

void Connection::_HandleError()
{
    LOG(ERROR, "Connection error occurred, connection will be closed");
    this->_HandleClose();
}

void Connection::_HandleEvent()
{
    // 刷新活跃度,调用组件使用者的任意事件
    if (this->_inactive_release == true)
    {
        // 刷新连接度活跃度,默认延时添加定时器任务时设置的时间,也可以自己指定
        this->_event_loop->RefreshTimerTask(this->_id);
    }

    // 调用组件使用者的任意事件回调函数
    if (this->_event_callback)
    {
        this->_event_callback(shared_from_this());
    }
}

// 连接建立完成,修改连接状态,启动连接读事件监控
void Connection::Established()
{
    assert(this->_state == ConnectState::CONNECTING);
    this->_state = ConnectState::CONNECTED;
    this->_channel.EnableRead();

    // 用户设置的连接建立回调
    if (this->_connected_callback)
    {
        this->_connected_callback(shared_from_this());
    }
}

// 释放链接
void Connection::_Release()
{
    if (this->_state == ConnectState::DISCONNECTED)
        return;

    // 修改连接状态
    this->_state = ConnectState::DISCONNECTED;

    // 移除事件监控
    this->_channel.Remove(); // 从EventLoop的监控列表中移除当前Channel

    // 关闭描述符
    this->_sock.Close();

    // 取消定时器任务
    if (this->_inactive_release == true)
    {
        this->_event_loop->CancelTimerTask(this->_id);
    }

    // 用户设置的连接关闭回调
    if (this->_closed_callback)
    {
        this->_closed_callback(shared_from_this());
    }

    // 移除服务器内部对连接的管理,从连接列表中移除连接对象,必须先调用用户设置的函数
    if (this->_server_closed_callback)
    {
        this->_server_closed_callback(shared_from_this());
    }
}