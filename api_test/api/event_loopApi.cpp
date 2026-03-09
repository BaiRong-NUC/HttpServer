#include "../../include/event_loop.h"
#include <iostream>
#include <thread>
#include <atomic>
#include <cassert>

// 测试任务队列添加与执行
void TestAddTask()
{
    EventLoop loop;
    std::atomic<int> counter{0};
    loop.AddTask([&]()
                 { counter++; });
    loop.RunTask([&]()
                 { counter++; });
    loop.Start();
    // LOG(INFO, "counter:" << counter);
    assert(counter == 2);
    std::cout << "TestAddTask passed." << std::endl;
}

// 测试线程判断
void TestInLoop()
{
    EventLoop loop;
    assert(loop.InLoop());
    std::thread t([&]()
                  { assert(!loop.InLoop()); });
    t.join();
    std::cout << "TestInLoop passed." << std::endl;
}

// 测试Channel管理（适配新构造函数）
void TestChannel()
{
    EventLoop loop;
    Socket sock;
    sock.Create();
    Channel channel(&loop, std::move(sock));
    loop.AddChannel(&channel);
    loop.RemoveChannel(&channel);
    std::cout << "TestChannel passed." << std::endl;
}

// 测试Start方法（仅基本调用，复杂场景需补充事件模拟）
void TestStart()
{
    EventLoop loop;
    loop.AddTask([]()
                 { std::cout << "Task executed in Start." << std::endl; });
    loop.Start();
    std::cout << "TestStart passed." << std::endl;
}

int main()
{
    SetLogLevel(INFO);
    TestAddTask();
    TestInLoop();
    TestChannel();
    TestStart();
    std::cout << "All EventLoop API tests passed." << std::endl;
    return 0;
}
