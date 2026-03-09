#include "../../include/socket.h"

int main(int argc, char const *argv[])
{
    SetLogLevel(INFO);
    Socket client;
    bool ret = client.CreateClient("127.0.0.1", 8085);
    if (!ret)
    {
        LOG(ERROR, "Failed to create client connection");
        return -1;
    }
    while (true)
    {
        const char *message = "Hello Server!";
        ssize_t sendLen = client.Send(message, strlen(message));
        if (sendLen < 0)
        {
            LOG(WARNING, "Send failed");
            continue;
        }
        char buffer[1024] = {0};
        ssize_t recvLen = client.Recv(buffer, sizeof(buffer) - 1);
        if (recvLen < 0)
        {
            LOG(WARNING, "Recv failed");
            continue;
        }
        LOG(INFO, std::string(buffer, recvLen));
        sleep(5); // 每隔5秒发送一次数据
    }
    return 0;
}
