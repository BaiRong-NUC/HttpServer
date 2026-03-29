#include <protocol/http/http_server.h>
#include <curl/curl.h>
#include <iostream>
#include <stdexcept>

static size_t WriteCallback(void *contents, size_t size, size_t nmemb, void *userp)
{
    ((std::string *)userp)->append((char *)contents, size * nmemb);
    return size * nmemb;
}

static std::string call_model_service(const std::string &json_body,
                                      const std::string &url = "http://127.0.0.1:8000/predict")
{
    CURL *curl = curl_easy_init();
    if (!curl)
    {
        throw std::runtime_error("curl init failed");
    }

    std::string response;
    struct curl_slist *headers = nullptr;
    headers = curl_slist_append(headers, "Content-Type: application/json");

    curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
    curl_easy_setopt(curl, CURLOPT_POST, 1L);
    curl_easy_setopt(curl, CURLOPT_POSTFIELDS, json_body.c_str());
    curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response);

    CURLcode res = curl_easy_perform(curl);
    curl_slist_free_all(headers);
    curl_easy_cleanup(curl);

    if (res != CURLE_OK)
    {
        throw std::runtime_error(std::string("curl perform failed: ") + curl_easy_strerror(res));
    }
    return response;
}

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

    // POST /ml_predict  将请求体（JSON）转发给本地运行的 Python 模型服务，并返回模型响应
    server.Post("/ml_predict",
                [](const HttpRequest &req, HttpResponse &resp)
                {
                    if (req.body.empty())
                    {
                        resp.SetBody("{\"error\": \"empty body\"}", "application/json");
                        resp.status_code = 400;
                        return;
                    }
                    try
                    {
                        std::string model_resp = call_model_service(req.body);
                        resp.SetBody(model_resp, "application/json");
                        resp.status_code = 200;
                    }
                    catch (const std::exception &e)
                    {
                        std::string err = std::string("{\"error\": \"") + e.what() + "\"}";
                        resp.SetBody(err, "application/json");
                        resp.status_code = 500;
                    }
                });
    // server.Get("/overdate",
    //             [](const HttpRequest &req, HttpResponse &resp)
    //             {
    //                 // req对象是客户端请求解析结果,这里用不到
    //                 // resp对象是服务器响应,用于设置响应内容,发送给服务器
    //                 resp.SetBody(req.body.empty() ? "<html><body><h1>Hello, World!</h1></body></html>"
    //                                                 : "<html><body><h1>Hello, World!</h1><p>request body: " +
    //                                                 req.body +
    //                                                     "</p></body></html>",
    //                             "text/html");
    //                 sleep(10); //模拟业务处理超瓶颈
    //                 resp.status_code = 200;  // OK
    //             });
    server.Listen();
}
