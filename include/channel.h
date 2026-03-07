#include "./public.h"
#include "./socket.h"
#pragma once
// 对文件描述符的事件监控
class Channel
{
private:
    Socket _sock;                          // 监控的文件描述符
    uint32_t _events;                      // 关注的事件
    uint32_t _revents;                     // 当前触发的事件
    using Anction = std::function<void()>; // 事件回调函数类型
public:
    Channel(Socket sock);
    // 可读事件回调函数
    Anction readAnction;
    // 可写事件回调函数
    Anction writeAnction;
    // 错误事件回调函数
    Anction errorAnction;
    // 连接断开事件回调函数
    Anction closeAnction;
    // 任意事件触发回调函数
    Anction eventAnction;
    // 当前是否监控可读
    bool ReadAble();
    // 当前是否监控可写
    bool WriteAble();
    // 启动可读事件监控
    void EnableRead();
    // 启动可写事件监控
    void EnableWrite();
    // 关闭可读事件监控
    void DisableRead();
    // 关闭可写事件监控
    void DisableWrite();

    // 关闭所有事件监控
    void DisableAll();

    // 移除监控
    void Remove();

    // 事件处理
    void HandleEvent(); // 根据当前触发的事件调用对应的回调函数

    // 获取文件描述符
    int GetSocketFd();

    // 设置就绪事件
    void SetRevents(uint32_t revents);
};