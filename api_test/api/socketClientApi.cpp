#include "../include/socket.h"

int main(int argc, char const *argv[])
{
    SetLogLevel(INFO);
    Socket client;
    bool ret = client.CreateClient("127.0.0.1", 8085);
    if (!ret)
    {
        LOG(ERROR, "Failed to create client socket");
        return -1;
    }
    std::string message = "Hello, Server!";
    ssize_t sendLen = client.Send(message.c_str(), message.size());
    if (sendLen < 0)
    {
        LOG(ERROR, "Failed to send data");
        return -1;
    }
    LOG(INFO, "Sent to Server: " + message);
    char buffer[1024] = {0};
    ssize_t recvLen = client.Recv(buffer, sizeof(buffer) - 1);
    if (recvLen < 0)
    {
        LOG(ERROR, "Failed to receive data");
        return -1;
    }
    LOG(INFO, "Received from Server: " + std::string(buffer, recvLen));
    return 0;
}
