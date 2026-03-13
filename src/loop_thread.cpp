#include "../include/loop_thread.h"

LoopThread::LoopThread()
    : _event_loop(nullptr),
      _thread(
          [this]()
          {
              EventLoop loop;  // 在线程中实例化EventLoop对象
              {
                  std::unique_lock<std::mutex> lock(this->_mutex);
                  this->_event_loop = &loop;     // 获取线程内的EventLoop对象
                  this->_cond_var.notify_all();  // 通知GetEventLoop()可以获取_event_loop了
              }
              loop.Start();  // 启动事件循环,死循环,不会退出,这样loop变量不会销毁_event_loop不会悬空
          })
{
}

LoopThread::~LoopThread()
{
    if (_thread.joinable())
    {
        _thread.detach();
    }
}

EventLoop* LoopThread::GetEventLoop()
{
    EventLoop* loop = nullptr;
    {
        std::unique_lock<std::mutex> lock(this->_mutex);
        while (this->_event_loop == nullptr)
        {
            this->_cond_var.wait(lock);  // 等待_event_loop实例化完成
        }
        loop = this->_event_loop;  // 获取线程内的EventLoop对象
    }
    return loop;
}