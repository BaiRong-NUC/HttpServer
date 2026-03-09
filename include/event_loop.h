#pragma once
#include "./public.h"
#include "./poller.h"
#include "./channel.h"
#include "./log.h"
#include "./socket.h"
class EventLoop
{
private:
    // 保证描述符的任务处理在同一线程中执行
    std::thread::id _thread_id;           // 与EventLoop模块绑定的线程ID
    Poller _poller;                       // 描述符事件监控
    Channel _efd_channel;                 // 监控 _efd 的事件,当 _efd 可读时说明有任务需要处理
    using Action = std::function<void()>; // 事件回调函数类型
    std::vector<Action> _tasks_queue;     // 线程安全的队列
    std::mutex _queue_mutex;              // 保护任务队列的互斥锁

    void _RunAllTasks(); // 执行任务队列中的所有任务
public:
    EventLoop();

    void AddTask(const Action &task); // 将任务添加到队列中

    void RunTask(const Action &task); // 在与EventLoop模块绑定的线程ID的线程中执行函数

    bool InLoop(); // 判断当前线程是否是与EventLoop模块绑定的线程

    void AddChannel(Channel *channel); // 添加Channel在Poller中的事件监控

    void RemoveChannel(Channel *channel); // 从Poller中移除Channel的事件监控

    void Start(); // 事件监控->就绪事件处理->执行任务

    Poller &GetPoller() { return this->_poller; } // 为了兼容之前的测试
};