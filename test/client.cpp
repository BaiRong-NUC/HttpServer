// 长连接测试,创建一个客户端连接到服务器,持续发送数据,直到超时时间
#include <server/tcp_server.h>
#include <thread>
#include <atomic>
#include <unistd.h>

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

// 非活跃连接,发送一次数据后不再发送数据,等待服务器超时关闭连接
void ClientTest3()
{
    Socket clientSocket;
    clientSocket.CreateClient("127.0.0.1", 8085);
    std::string req = "GET / HTTP/1.1\r\nConnection: keep-alive\r\nContent-Length: 0\r\n\r\n";
    int ret = clientSocket.Send(req.c_str(), req.size());
    std::cout << "Sent request to server: " << req;
    // 等待服务器后续响应或服务器主动关闭连接
    char buffer[1024] = {0};
    std::atomic<bool> done(false);
    std::thread timer(
        [&]()
        {
            int times = 0;
            while (!done.load())
            {
                sleep(3);
                if (done.load()) break;
                std::cout << "Run Time: " << (++times) * 3 << "s\n===========" << std::endl;
            }
        });

    while (true)
    {
        ret = clientSocket.Recv(buffer, sizeof(buffer) - 1);
        if (ret > 0)
        {
            buffer[ret] = '\0';
            // std::cout << "Received from server: " << buffer << std::endl;
            // 继续等待服务器关闭（模拟非活跃客户端）
            continue;
        }
        else if (ret == 0)
        {
            std::cout << "Server closed the connection (recv returned 0)." << std::endl;
            break;
        }
        else
        {
            std::cerr << "Recv error or socket closed unexpectedly." << std::endl;
            break;
        }
    }

    done.store(true);
    if (timer.joinable()) timer.join();

    clientSocket.Close();
}

// 连续发送多条请求
void ClientTest4()
{
    Socket clientSocket;
    clientSocket.CreateClient("127.0.0.1", 8085);
    int times = 0;
    // 连续请求两条数据
    std::string req = "GET / HTTP/1.1\r\nConnection: keep-alive\r\nContent-Length: 5\r\n\r\n12345";
    req += "GET / HTTP/1.1\r\nConnection: keep-alive\r\nContent-Length: 5\r\n\r\n12345";
    int ret = clientSocket.Send(req.c_str(), req.size());
    if (ret < 0)
    {
        std::cerr << "Failed to send request to server" << std::endl;
        return;
    }
    std::cout << "Sent request to server: " << req;
    char buffer[1024] = {0};
    ret = clientSocket.Recv(buffer, sizeof(buffer) - 1);
    if (ret < 0)
    {
        std::cerr << "Failed to receive response from server" << std::endl;
        return;
    }
    std::cout << "Received response from server: " << buffer << std::endl;
     // 继续发送请求,测试服务器是否正确处理多条请求
    clientSocket.Close();
}
int main(int argc, char const *argv[])
{
    // 服务器默认超时时间为30s
    // ClientTest1();
    // ClientTest2();
    // ClientTest3();
    ClientTest4();
    return 0;
}