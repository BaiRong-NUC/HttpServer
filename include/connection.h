#pragma once
#include "./public.h"
#include "./socket.h"
#include "./channel.h"
#include "./buffer.h"
#include "./any.h"
// 对连接管理,封装关于连接的所有操作

// 连接状态枚举
enum class ConnectState
{
    CONNECTING, // 已连接
    CLOSE       // 已关闭
};

class Connection
{
private:
    uint64_t _id;        // 连接ID,可以是文件描述符或者其他唯一标识符
    Socket _sock;        // 连接的套接字对象
    Channel _channel;    // 连接的Channel对象,用于监控连接的事件
    Buffer _in_buffer;   // 输入缓冲区,保存从socket中读取的数据
    Buffer _out_buffer;  // 输出缓冲区,保存要发送到socket的数据
    Any _context;        // 连接的上下文,可以保存连接相关的任意数据,如HTTP请求信息等
    ConnectState _state; // 连接状态,如已连接;正在关闭;已关闭等
};