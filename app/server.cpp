#include <protocol/http/http_server.h>

int main(int argc, char const *argv[])
{
    SetLogLevel(WARNING);
    // 获取计算机CPU核心数量,作为从属线程数量
    int thread_num = std::thread::hardware_concurrency();
    HttpServer server("./wwwroot", 8085, DEFAULT_INACTIVE_TIMEOUT, thread_num);
    // HttpServer server("./wwwroot", 8085, 10, thread_num);  // 测试
    server.Get("/hello",
               [](const HttpRequest &req, HttpResponse &resp)
               {
                   // req对象是客户端请求解析结果,这里用不到
                   // resp对象是服务器响应,用于设置响应内容,发送给服务器
                   resp.SetBody(req.body.empty() ? "<html><body><h1>Hello, World!</h1></body></html>"
                                                 : "<html><body><h1>Hello, World!</h1><p>request body: " + req.body +
                                                       "</p></body></html>",
                                "text/html");
                   resp.status_code = 200;  // OK
               });
    server.Post("/login",
                [](const HttpRequest &req, HttpResponse &resp)
                {
                    // 处理登录请求,这里只是示例,实际应用中需要验证用户名和密码等
                    // 将POST 上传的用户与密码也展示出来
                    std::string body = req.body;
                    resp.SetBody(
                        "<html><body><h1>Login Successful! </h1><p>request body: " + body + "</p></body></html>",
                        "text/html");
                    resp.status_code = 200;  // OK
                });
    server.Put("/update",
               [](const HttpRequest &req, HttpResponse &resp)
               {
                   // 处理更新请求,这里只是示例,实际应用中需要根据URI和请求内容进行相应的更新操作
                   std::string body = req.body;
                   resp.SetBody(
                       "<html><body><h1>Update Successful! </h1><p>request body: " + body + "</p></body></html>",
                       "text/html");
                   resp.status_code = 200;  // OK
               });

    server.Delete("/delete",
                  [](const HttpRequest &req, HttpResponse &resp)
                  {
                      // 处理删除请求,这里只是示例,实际应用中需要根据URI和请求内容进行相应的删除操作
                      std::string body = req.body;
                      resp.SetBody(
                          "<html><body><h1>Delete Successful! </h1><p>request body: " + body + "</p></body></html>",
                          "text/html");
                      resp.status_code = 200;  // OK
                  });
    // server.Get("/overdate",
    //             [](const HttpRequest &req, HttpResponse &resp)
    //             {
    //                 // req对象是客户端请求解析结果,这里用不到
    //                 // resp对象是服务器响应,用于设置响应内容,发送给服务器
    //                 resp.SetBody(req.body.empty() ? "<html><body><h1>Hello, World!</h1></body></html>"
    //                                                 : "<html><body><h1>Hello, World!</h1><p>request body: " + req.body +
    //                                                     "</p></body></html>",
    //                             "text/html");
    //                 sleep(10); //模拟业务处理超瓶颈
    //                 resp.status_code = 200;  // OK
    //             });
    server.Listen();
}
