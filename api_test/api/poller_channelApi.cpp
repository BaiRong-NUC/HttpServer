#include "../../include/poller.h"
#include "../../include/channel.h"
#include "../../include/event_loop.h"

#include <utility>

using namespace std;

void testPollReadableEvent()
{
    int fds[2] = {-1, -1};
    int ret = socketpair(AF_UNIX, SOCK_STREAM, 0, fds);
    assert(ret == 0);

    EventLoop loop;
    Socket monitored(fds[0]);
    Socket peer(fds[1]);

    // 监控monitored文件描述符事件
    Channel channel(&loop, std::move(monitored));

    Poller &poller = loop.GetPoller();

    bool readTriggered = false;
    bool eventTriggered = false;
    bool writeTriggered = false;
    string received;

    channel.readAction = [&]()
    {
        // 从对端接受数据
        char buf[128] = {0};
        ssize_t n = channel.GetSocket().Recv(buf, sizeof(buf));
        if (n > 0)
        {
            received.assign(buf, static_cast<size_t>(n));
            readTriggered = true;

            // 启动可写事件监控
            channel.EnableWrite();
        }
    };

    channel.writeAction = [&]()
    {
        // 发送响应给对端
        const string response = "received: " + received;
        ssize_t sendLen = channel.GetSocket().Send(response.c_str(), response.size());
        assert(sendLen == static_cast<ssize_t>(response.size()));

        writeTriggered = true;
        // 发送完成后关闭可写事件监控
        channel.DisableWrite();
    };

    channel.eventAction = [&]()
    { eventTriggered = true; };

    channel.EnableRead();

    const string msg = "hello poller";
    ssize_t sendLen = peer.Send(msg.c_str(), msg.size());
    assert(sendLen == static_cast<ssize_t>(msg.size()));
    vector<Channel *> active = poller.Poll(-1);
    assert(!active.empty());

    bool foundCurrentChannel = false;
    for (Channel *ch : active)
    {
        if (ch == &channel)
        {
            foundCurrentChannel = true;
        }
        ch->HandleEvent();
    }

    assert(foundCurrentChannel);
    assert(readTriggered);
    assert(eventTriggered);
    assert(received == msg);
    channel.Remove();

    // Remove 后不应再收到该 fd 的事件
    sendLen = peer.Send(msg.c_str(), msg.size());
    assert(sendLen == static_cast<ssize_t>(msg.size()));
    active = poller.Poll(100);
    assert(active.empty());
}

int main()
{
    testPollReadableEvent();
    cout << "poller_channelApi test passed" << endl;
    return 0;
}
