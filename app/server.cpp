#include <protocol/http/http_server.h>

int main(int argc, char const *argv[])
{
    SetLogLevel(INFO);
    // 获取计算机CPU核心数量,作为从属线程数量
    int thread_num = std::thread::hardware_concurrency();
    HttpServer server("./wwwroot", 8085, DEFAULT_INACTIVE_TIMEOUT, thread_num);
    server.Get("/hello",
               [](const HttpRequest &req, HttpResponse &resp)
               {
                   // req对象是客户端请求解析结果,这里用不到
                   // resp对象是服务器响应,用于设置响应内容,发送给服务器
                   resp.SetBody("<html><body><h1>Hello, World!</h1></body></html>", "text/html");
                   resp.status_code = 200;  // OK
               });
    server.Post("/login",
                [](const HttpRequest &req, HttpResponse &resp)
                {
                    // 处理登录请求,这里只是示例,实际应用中需要验证用户名和密码等
                    resp.SetBody("<html><body><h1>Login Successful!</h1></body></html>", "text/html");
                    resp.status_code = 200;  // OK
                });
    // 注册GET请求处理函数
    server.Listen();
}
