#include "../include/socket.h"

int main(int argc, char const *argv[])
{
    SetLogLevel(INFO);
    Socket listenSock;
    // bool ret = listenSock.CreateServer(8085);
    bool ret = listenSock.CreateServer(8085, true, false);
    if (!ret)
    {
        LOG(ERROR, "Failed to create server socket");
        return -1;
    }
    while (true)
    {
        std::string clientIp = "";
        uint16_t clientPort = 0;
        Socket *clientSocket = listenSock.Accept(clientIp, clientPort);
        if (clientSocket == nullptr)
        {
            // LOG(WARNING, "Accept failed");
            LOG(WARNING, "Accept failed");
            continue;
        }
        // 处理客户端连接
        char buffer[1024] = {0};
        // 接受客户端数据
        ssize_t recvLen = clientSocket->Recv(buffer, sizeof(buffer) - 1);
        if (recvLen < 0)
        {
            LOG(WARNING, "Recv failed");
            delete clientSocket;
            continue;
        }
        // 打印
        LOG(INFO, "Received from " + clientIp + ":" + std::to_string(clientPort) + " - " + std::string(buffer, recvLen));
        // 回显
        clientSocket->Send(buffer, recvLen);
        LOG(INFO, "Send to Client Over");
        delete clientSocket;
    }
    return 0;
}
