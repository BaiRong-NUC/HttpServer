#include <protocol/http/http_server.h>
#include <curl/curl.h>
#include <algorithm>
#include <cstdlib>
#include <cctype>
#include <iostream>
#include <stdexcept>

namespace
{
const char* DEFAULT_PYTHON_RESTORE_URL = "http://127.0.0.1:8091/restore";
constexpr int RESTORE_PROXY_INACTIVE_TIMEOUT = 900;

struct ProxyResult
{
    CURLcode curl_code = CURLE_OK;
    long status_code = 0;
    std::string body;
    std::string content_type;
    std::string output_url;
    char error_buffer[CURL_ERROR_SIZE] = {0};
};

size_t WriteToString(char* ptr, size_t size, size_t nmemb, void* userdata)
{
    size_t total_size = size * nmemb;
    auto* data = static_cast<std::string*>(userdata);
    data->append(ptr, total_size);
    return total_size;
}

std::string Trim(std::string value)
{
    auto is_space = [](unsigned char ch) { return std::isspace(ch); };
    value.erase(value.begin(),
                std::find_if(value.begin(), value.end(), [&](unsigned char ch) { return !is_space(ch); }));
    value.erase(std::find_if(value.rbegin(), value.rend(), [&](unsigned char ch) { return !is_space(ch); }).base(),
                value.end());
    return value;
}

std::string ToLower(std::string value)
{
    std::transform(value.begin(), value.end(), value.begin(), [](unsigned char ch) { return std::tolower(ch); });
    return value;
}

size_t CaptureHeader(char* buffer, size_t size, size_t nitems, void* userdata)
{
    size_t total_size = size * nitems;
    auto* result = static_cast<ProxyResult*>(userdata);
    std::string header_line(buffer, total_size);
    size_t colon_pos = header_line.find(':');
    if (colon_pos == std::string::npos)
    {
        return total_size;
    }

    std::string key = ToLower(Trim(header_line.substr(0, colon_pos)));
    std::string value = Trim(header_line.substr(colon_pos + 1));
    if (key == "x-replicate-output-url")
    {
        result->output_url = value;
    }
    return total_size;
}

std::string JsonEscape(const std::string& value)
{
    std::string escaped;
    escaped.reserve(value.size());
    for (char ch : value)
    {
        switch (ch)
        {
            case '\\':
                escaped += "\\\\";
                break;
            case '"':
                escaped += "\\\"";
                break;
            case '\n':
                escaped += "\\n";
                break;
            case '\r':
                escaped += "\\r";
                break;
            case '\t':
                escaped += "\\t";
                break;
            default:
                escaped += ch;
                break;
        }
    }
    return escaped;
}

std::string JsonError(const std::string& message)
{
    return std::string("{\"detail\":\"") + JsonEscape(message) + "\"}";
}

std::string PythonRestoreUrl()
{
    const char* configured_url = std::getenv("MOSAIC_PYTHON_URL");
    if (configured_url && configured_url[0] != '\0')
    {
        return configured_url;
    }
    return DEFAULT_PYTHON_RESTORE_URL;
}

void AppendQueryParam(CURL* curl, std::string& url, bool& has_query, const std::string& key, const std::string& value)
{
    char* escaped_key = curl_easy_escape(curl, key.c_str(), static_cast<int>(key.size()));
    char* escaped_value = curl_easy_escape(curl, value.c_str(), static_cast<int>(value.size()));
    if (!escaped_key || !escaped_value)
    {
        if (escaped_key) curl_free(escaped_key);
        if (escaped_value) curl_free(escaped_value);
        throw std::runtime_error("Failed to encode query parameter.");
    }
    url += has_query ? '&' : '?';
    url += escaped_key;
    url += '=';
    url += escaped_value;
    has_query = true;
    curl_free(escaped_key);
    curl_free(escaped_value);
}

ProxyResult ForwardRestoreRequest(const HttpRequest& request)
{
    ProxyResult result;
    CURL* curl = curl_easy_init();
    if (!curl)
    {
        result.curl_code = CURLE_FAILED_INIT;
        return result;
    }
    std::string url = PythonRestoreUrl();
    bool has_query = url.find('?') != std::string::npos;
    const std::vector<std::string> forwarded_params = {"upscale", "face_upsample", "background_enhance",
                                                       "codeformer_fidelity", "save"};
    try
    {
        for (const std::string& param : forwarded_params)
        {
            if (request.HasQueryParam(param))
            {
                AppendQueryParam(curl, url, has_query, param, request.GetQueryParam(param));
            }
        }
    }
    catch (const std::exception& error)
    {
        curl_easy_cleanup(curl);
        result.curl_code = CURLE_URL_MALFORMAT;
        std::string message = error.what();
        std::copy(message.begin(), message.end(), result.error_buffer);
        return result;
    }
    std::string content_type = request.GetHeader("Content-Type");
    if (content_type.empty())
    {
        content_type = "application/octet-stream";
    }
    struct curl_slist* headers = nullptr;
    std::string content_type_header = "Content-Type: " + content_type;
    headers = curl_slist_append(headers, content_type_header.c_str());
    headers = curl_slist_append(headers, "Accept: image/png, application/json");
    headers = curl_slist_append(headers, "Expect:");
    std::string forwarded_host = request.GetHeader("X-Forwarded-Host");
    if (forwarded_host.empty())
    {
        forwarded_host = request.GetHeader("Host");
    }
    std::string forwarded_proto = request.GetHeader("X-Forwarded-Proto");
    if (forwarded_proto.empty())
    {
        forwarded_proto = "http";
    }
    std::string forwarded_host_header;
    std::string forwarded_proto_header;
    if (!forwarded_host.empty())
    {
        forwarded_host_header = "X-Forwarded-Host: " + forwarded_host;
        headers = curl_slist_append(headers, forwarded_host_header.c_str());
    }
    if (!forwarded_proto.empty())
    {
        forwarded_proto_header = "X-Forwarded-Proto: " + forwarded_proto;
        headers = curl_slist_append(headers, forwarded_proto_header.c_str());
    }
    curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
    curl_easy_setopt(curl, CURLOPT_POST, 1L);
    curl_easy_setopt(curl, CURLOPT_POSTFIELDS, request.body.data());
    curl_easy_setopt(curl, CURLOPT_POSTFIELDSIZE_LARGE, static_cast<curl_off_t>(request.body.size()));
    curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteToString);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &result.body);
    curl_easy_setopt(curl, CURLOPT_HEADERFUNCTION, CaptureHeader);
    curl_easy_setopt(curl, CURLOPT_HEADERDATA, &result);
    curl_easy_setopt(curl, CURLOPT_ERRORBUFFER, result.error_buffer);
    curl_easy_setopt(curl, CURLOPT_CONNECTTIMEOUT, 5L);
    curl_easy_setopt(curl, CURLOPT_TIMEOUT, 600L);
    result.curl_code = curl_easy_perform(curl);
    curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &result.status_code);
    char* upstream_content_type = nullptr;
    curl_easy_getinfo(curl, CURLINFO_CONTENT_TYPE, &upstream_content_type);
    if (upstream_content_type)
    {
        result.content_type = upstream_content_type;
    }
    curl_slist_free_all(headers);
    curl_easy_cleanup(curl);
    return result;
}
}  // namespace

static size_t WriteCallback(void* contents, size_t size, size_t nmemb, void* userp)
{
    ((std::string*)userp)->append((char*)contents, size * nmemb);
    return size * nmemb;
}

static std::string call_model_service(const std::string& json_body,
                                      const std::string& url = "http://127.0.0.1:8000/predict")
{
    CURL* curl = curl_easy_init();
    if (!curl)
    {
        throw std::runtime_error("curl init failed");
    }

    std::string response;
    struct curl_slist* headers = nullptr;
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

int main(int argc, char const* argv[])
{
    SetLogLevel(WARNING);
    curl_global_init(CURL_GLOBAL_DEFAULT);
    int cpu_count = static_cast<int>(std::thread::hardware_concurrency());
    if (cpu_count <= 0)
    {
        cpu_count = 4;
    }
    int reactor_thread_num = std::max(1, cpu_count / 2);
    int business_thread_num = std::max(1, cpu_count - reactor_thread_num);
    HttpServer server("./wwwroot", 8085, DEFAULT_INACTIVE_TIMEOUT, reactor_thread_num, true, true, "0.0.0.0",
                      business_thread_num);
    server.Get("/hello",
               [](const HttpRequest& req, HttpResponse& resp)
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
                [](const HttpRequest& req, HttpResponse& resp)
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
               [](const HttpRequest& req, HttpResponse& resp)
               {
                   // 处理更新请求,这里只是示例,实际应用中需要根据URI和请求内容进行相应的更新操作
                   std::string body = req.body;
                   resp.SetBody(
                       "<html><body><h1>Update Successful! </h1><p>request body: " + body + "</p></body></html>",
                       "text/html");
                   resp.status_code = 200;  // OK
               });

    server.Delete("/delete",
                  [](const HttpRequest& req, HttpResponse& resp)
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
                [](const HttpRequest& req, HttpResponse& resp)
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
                    catch (const std::exception& e)
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
    // 新增 MosaicRestored 图片修复代理接口
    server.Post(
        "^/api/restore$",
        [](const HttpRequest& request, HttpResponse& response)
        {
            if (request.body.empty())
            {
                response.status_code = 400;
                response.SetBody(JsonError("Request body must contain image bytes."), "application/json");
                return;
            }
            ProxyResult proxy_result = ForwardRestoreRequest(request);
            if (proxy_result.curl_code != CURLE_OK)
            {
                std::string message = proxy_result.error_buffer[0] != '\0' ? proxy_result.error_buffer
                                                                           : curl_easy_strerror(proxy_result.curl_code);
                response.status_code = 502;
                response.SetBody(JsonError("Python restore service request failed: " + message), "application/json");
                return;
            }
            response.status_code = proxy_result.status_code > 0 ? static_cast<int>(proxy_result.status_code) : 502;
            std::string content_type =
                proxy_result.content_type.empty() ? "application/octet-stream" : proxy_result.content_type;
            response.SetBody(proxy_result.body, content_type);
            if (!proxy_result.output_url.empty())
            {
                response.SetHeader("X-Output-Url", proxy_result.output_url);
            }
            if (!proxy_result.output_url.empty())
            {
                response.SetHeader("X-Replicate-Output-Url", proxy_result.output_url);
            }
        });
    server.Listen();
    curl_global_cleanup();
}
