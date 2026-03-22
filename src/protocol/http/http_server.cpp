#include <protocol/http/http_server.h>

HttpServer::HttpServer(const std::string &root, uint16_t port, int timeout, int thread_num, bool reseAddr, bool noBlock,
                       const std::string &ip)
    : _tcp_server(port, thread_num, reseAddr, noBlock, ip), static_root(root)
{
    // 设置超时时间,单位为秒
    this->SetInactiveTimeout(timeout);
    // 設置TcpServer回調
    this->_tcp_server.connected_callback = [this](const PtrConnection &conn) { this->_OnConnected(conn); };
    this->_tcp_server.message_callback = [this](const PtrConnection &conn, Buffer *buf)
    {
        if (buf) this->_OnMessage(conn, buf);
    };

    // 初始化默认错误响应内容
    this->response_404.status_code = 404;
    this->response_404.SetBody(GetEmbedded404(), "text/html");
    this->response_405.status_code = 405;
    this->response_405.SetBody(GetEmbedded405(), "text/html");
    this->response_error.status_code = 500;
    this->response_error.SetBody(GetEmbeddedError(), "text/html");
}

// 設置TcpServer上下文
void HttpServer::_OnConnected(const PtrConnection &conn)
{
    conn->SetContext(HttpContext());
    LOG(INFO, "\nNew connection established: \n\tconnection Address: " << conn << ", Loop thread Id:"
                                                                       << conn->GetLoopThreadId());
}

// 設置TcpServer回調
void HttpServer::_OnMessage(const PtrConnection &conn, Buffer *buffer)
{
    while (buffer->GetReadableSize() > 0)
    {
        // 1. 獲取協議上下文
        HttpContext *context = conn->GetContext().Get<HttpContext>();

        // 2. 通過上下文對緩衝區的數據進行解析,得到HttpRequest對象
        context->ParseRequest(buffer);
        if (context->GetAcceptStatus() == HttpAcceptStatus::ACCEPTING_ERROR)
        {
            // 解析過程中出現錯誤,構造HTTP并發送響應
            HttpResponse error_response = this->_GetErrorResponse(context->GetResponseStatus());
            this->SendResponse(conn, context->GetRequest(), error_response);
            // 切斷連接
            conn->Close();
            return;
        }
        if (context->GetAcceptStatus() != HttpAcceptStatus::ACCEPTED)
        {
            // 請求還未完全解析
            return;
        }

        // 請求解析完畢,獲取請求HttpRequest對象
        HttpRequest &client_request = context->GetRequest();
        // 服务器响应对象
        HttpResponse server_response(context->GetResponseStatus());

        // 3. 判斷請求路由+業務處理
        this->_HandleRequest(conn, client_request, server_response);

        // 4. 組織HTTP報文,發送響應
        this->SendResponse(conn, client_request, server_response);

        // 5. 重置上下文,重置服务器响应
        context->Reset();

        // 6. 根據長短連接決定是否關閉連接
        if (client_request.IsKeepAlive() == false)
        {
            conn->Close();
            return;
        }
    }
}

void HttpServer::SendResponse(const PtrConnection &conn, const HttpRequest &client_request,
                              HttpResponse &server_response)
{
    // 设置响应头部,防止用户忘记设置必要的头部字段
    if (client_request.IsKeepAlive() == false)
    {
        server_response.SetHeader("Connection", "close");
    }
    else
    {
        server_response.SetHeader("Connection", "keep-alive");
    }
    if (server_response.body.empty() == false)
    {
        if (server_response.HasHeader("Content-Length") == false)
            server_response.SetHeader("Content-Length", std::to_string(server_response.body.size()));
        if (server_response.HasHeader("Content-Type") == false)
            server_response.SetHeader("Content-Type", "application/octet-stream");  // 外部没有设置,默认为二进制流
    }
    if (server_response.is_redirect == true)
    {
        if (server_response.HasHeader("Location") == false)
        {
            server_response.SetHeader("Location", server_response.redirect_location);
        }
    }

    // 构造HTTP响应报文并发送
    conn->Send(server_response.ToString());
}

void HttpServer::_HandleRequest(const PtrConnection &conn, HttpRequest &request, HttpResponse &response)
{
    // 判断资源是否是静态资源
    if (this->_IsStaticResource(request))
    {
        // 处理静态资源请求
        std::string file_path = request.uri;  // 已经在_IsStaticResource中将URI替换为实际文件路径
        Buffer file_content;
        if (Utils::GetFileContent(file_path, &file_content))
        {
            response.SetBody(file_content.Read(file_content.GetReadableSize()), Utils::GetMimeType(file_path));
            response.status_code = 200;  // OK
        }
        else
        {
            response = this->_GetErrorResponse(500);  // Internal Server Error
        }
        return;
    }

    // 非静态资源;根据请求方法分别进行不同的处理
    int status_code = 200;  // 默认状态码为200 OK
    HttpServer::HandlerFunc handler = this->_FindHandler(request.method, request.uri, status_code);
    if (handler)
    {
        // 找到处理函数,调用函数处理请求,得到响应内容和状态码,设置HttpResponse对象
        handler(request, response);
    }
    else
    {
        // 没有找到处理函数,即URI没有匹配的路由,根据状态码设置错误响应
        response = this->_GetErrorResponse(status_code);
    }
}

HttpServer::HandlerFunc HttpServer::_FindHandler(const std::string &method, const std::string &uri, int &status_code)
{
    auto method_it = this->_method_handlers.find(method);
    if (method_it != this->_method_handlers.end())
    {
        const auto &handlers = method_it->second;
        for (const auto &handler_pair : handlers)
        {
            const std::regex &pattern = this->_GetRegex(handler_pair.first);
            const HandlerFunc &handler = handler_pair.second;
            if (std::regex_match(uri, pattern))
            {
                status_code = 200;  // 找到匹配的处理函数,状态码为200 OK
                return handler;
            }
        }
    }
    else
    {
        // 不支持的HTTP方法
        status_code = 405;  // Method Not Allowed
        return nullptr;
    }
    // 没有找到匹配的处理函数,即URI没有匹配的路由
    status_code = 404;  // Not Found
    return nullptr;
}

std::regex HttpServer::_GetRegex(const std::string &pattern)
{
    auto it = this->_regex_cache.find(pattern);
    if (it != this->_regex_cache.end())
    {
        return it->second;
    }
    else
    {
        std::regex regex_pattern(pattern);
        this->_regex_cache[pattern] = regex_pattern;
        return regex_pattern;
    }
}

bool HttpServer::_IsStaticResource(HttpRequest &request)
{
    // 1. 必须设置了静态资源根目录
    if (this->static_root.empty())
    {
        return false;
    }
    // 2. 必须是GET/HEAD请求
    if (request.method != "GET" && request.method != "HEAD")
    {
        return false;
    }
    // 3. 必须是合法路径
    if (Utils::IsValidPath(request.uri) == false)
    {
        return false;
    }
    // 4. 请求文件合法
    std::string req_path = this->static_root + request.uri;
    if (req_path.back() == '/')
    {
        req_path += "index.html";  // 路径默认请求路径下的index.html
    }
    if (Utils::IsFile(req_path) == false)
    {
        return false;
    }
    // 请求合法,将请求URI替换为实际文件路径,方便后续处理
    request.uri = req_path;
    return true;
}

const HttpResponse &HttpServer::_GetErrorResponse(int status_code)
{
    if (status_code < 400 || status_code >= 600)
    {
        // 非错误状态码,返回500 Internal Server Error
        LOG(WARNING, "HTTP Error Response requested with non-error status code: "
                         << status_code << ". Defaulting to 500 Internal Server Error.");
    }
    if (status_code == 404)
    {
        return this->response_404;
    }
    else if (status_code == 405)
    {
        return this->response_405;
    }
    else
    {
        return this->response_error;  // 500 Internal Server Error 或其他错误
    }
}

void HttpServer::Get(const std::string &uri_pattern, HandlerFunc handler)
{
    this->_get_handlers[uri_pattern] = handler;
}

void HttpServer::Post(const std::string &uri_pattern, HandlerFunc handler)
{
    this->_post_handlers[uri_pattern] = handler;
}

void HttpServer::Put(const std::string &uri_pattern, HandlerFunc handler)
{
    this->_put_handlers[uri_pattern] = handler;
}

void HttpServer::Delete(const std::string &uri_pattern, HandlerFunc handler)
{
    this->_delete_handlers[uri_pattern] = handler;
}

void HttpServer::Listen() { this->_tcp_server.Run(); }

void HttpServer::SetInactiveTimeout(int timeout)
{
    if (timeout <= 0)
    {
        this->_tcp_server.SetInactiveRelease(false);  // 不自动释放非活跃连接
    }
    else
    {
        this->_tcp_server.SetInactiveRelease(true, timeout);  // 自动释放非活跃连接,超时时间为timeout秒
    }
}