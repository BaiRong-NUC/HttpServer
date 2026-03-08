#include "../../include/poller.h"
#include "../../include/channel.h"

using namespace std;

void testPollReadableEvent()
{
    int fds[2] = {-1, -1};
    int ret = socketpair(AF_UNIX, SOCK_STREAM, 0, fds);
    assert(ret == 0);

    Poller poller;
    Socket monitored(fds[0]);
    Socket peer(fds[1]);
    Channel channel(&poller, monitored);

    bool readTriggered = false;
    bool eventTriggered = false;
    string received;

    channel.readAnction = [&]()
    {
        char buf[128] = {0};
        ssize_t n = channel.GetSocket().Recv(buf, sizeof(buf));
        if (n > 0)
        {
            received.assign(buf, static_cast<size_t>(n));
            readTriggered = true;
        }
    };
    channel.eventAnction = [&]()
    { eventTriggered = true; };

    channel.EnableRead();

    const string msg = "hello poller";
    ssize_t sendLen = peer.Send(msg.c_str(), msg.size());
    assert(sendLen == static_cast<ssize_t>(msg.size()));

    vector<Channel *> active = poller.Poll(1000);
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
