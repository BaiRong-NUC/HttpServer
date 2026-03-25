// 长连接测试,创建一个客户端连接到服务器,持续发送数据,直到超时时间
#include <server/tcp_server.h>

int main(int argc, char const *argv[])
{
    Socket clientSocket;
    clientSocket.CreateClient("127.0.0.1", 8085);
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
        char buffer[1024];
        ret = clientSocket.Recv(buffer, sizeof(buffer) - 1);
        if (ret < 0)
        {
            std::cerr << "Failed to receive response from server" << std::endl;
            break;
        }
        sleep(3);
    }
    clientSocket.Close();
    return 0;
}