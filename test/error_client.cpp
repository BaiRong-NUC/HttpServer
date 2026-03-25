// 模拟各种错误请求
#include <server/tcp_server.h>

// 1. 请求不完整
void ClientTest1()
{
    Socket clientSocket;
    clientSocket.CreateClient("127.0.0.1", 8085);
    int times = 0;
    // 请求不完整,Content-Length是10,但实际只发送了5个字节
    std::string req = "GET / HTTP/1.1\r\nConnection: keep-alive\r\nContent-Length: 10\r\n\r\n12345";
    while (true)
    {
        /**
         * Server会等直到请求体全部收到并把状态设为 ACCEPTED
         * HTTP 层才会处理并生成响应;在此之前服务器不会给出完整响应
         * 因为一共只有5个字节,且设置了超时机制,所以服务器会在超时后关闭连接
         */
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

// 2. 连续发送多次不完整的请求
void ClientTest2()
{
    Socket clientSocket;
    clientSocket.CreateClient("127.0.0.1", 8085);
    int times = 0;
    // 请求不完整,Content-Length是10,但实际只发送了5个字节
    std::string req = "GET / HTTP/1.1\r\nConnection: keep-alive\r\nContent-Length: 10\r\n\r\n12345";
    while (true)
    {
        /**
         * 多次发送请求有两种可能的结果:
         * 如果多次发送足够数据 -> 请求最终被完整接收并处理;最后服务器解析错误会主动关闭连接
         * 如果只发送部分然后静默 -> 超时后服务器关闭连接.(回到第一种情况)
         */
        int ret = clientSocket.Send(req.c_str(), req.size());
        clientSocket.Send(req.c_str(), req.size());
        clientSocket.Send(req.c_str(), req.size());
        clientSocket.Send(req.c_str(), req.size());
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
    }
    clientSocket.Close();
}

// 3. 单次业务处理达到服务器处理瓶颈(超时时间),可能导致其他连接没有刷新而释放
/**
 * 当客户套接字定时器描述符因为瓶颈,导致被释放.会释放客户套接字,最后服务器使用套接字时出错崩溃
 * 所以Connection释放时机应该在event_loop在所有事件执行完毕后(已修改)
 */

int main(int argc, char const *argv[])
{
    // ClientTest1();
    ClientTest2();
    return 0;
}
