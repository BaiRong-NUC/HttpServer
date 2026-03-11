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