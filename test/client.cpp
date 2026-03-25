// 长连接测试,创建一个客户端连接到服务器,持续发送数据,直到超时时间
#include <server/tcp_server.h>

// 长连接,不会断开,持续发送数据,会不断刷新服务器上连接的活跃度,一直保持连接
void ClientTest1()
{
    Socket clientSocket;
    clientSocket.CreateClient("127.0.0.1", 8085);
    int times = 0;
    std::string req = "GET / HTTP/1.1\r\nConnection: keep-alive\r\nContent-Length: 0\r\n\r\n";
    while (true)
    {
        int ret = clientSocket.Send(req.c_str(), req.size());
        if (ret < 0)
        {
            std::cerr << "Failed to send request to server" << std::endl;
            break;
        }
        std::cout << "Sent request to server: " << req;
        char buffer[1024] = {0};
        ret = clientSocket.Recv(buffer, sizeof(buffer) - 1);
        if (ret < 0)
        {
            std::cerr << "Failed to receive response from server" << std::endl;
            break;
        }
        sleep(3);
        std::cout << "Send Times: " << ++times << " Run Time:" << times * 3 << "s\n===========" << std::endl;
    }
    clientSocket.Close();
}

// 短连接,发送一次数据后断开连接
void ClientTest2()
{
    Socket clientSocket;
    clientSocket.CreateClient("127.0.0.1", 8085);
    std::string req = "GET / HTTP/1.1\r\nConnection: close\r\nContent-Length: 0\r\n\r\n";
    int ret = clientSocket.Send(req.c_str(), req.size());
    if (ret < 0)
    {
        std::cerr << "Failed to send request to server" << std::endl;
    }
    std::cout << "Sent request to server: " << req;
    char buffer[1024] = {0};
    ret = clientSocket.Recv(buffer, sizeof(buffer) - 1);
    if (ret < 0)
    {
        std::cerr << "Failed to receive response from server" << std::endl;
    }
    std::cout << "Received response from server: " << buffer << std::endl;
    clientSocket.Close();
}

int main(int argc, char const *argv[])
{
    // ClientTest1();
    ClientTest2();
    return 0;
}