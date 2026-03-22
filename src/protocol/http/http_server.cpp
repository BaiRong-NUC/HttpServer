#include <protocol/http/http_server.h>

// HttpServer::HttpServer(const std::string& root) : static_root(root) {

// }

// 設置TcpServer上下文
void HttpServer::_OnConnected(const PtrConnection &conn)
{
    conn->SetContext(HttpContext());
    LOG(INFO, "\nNew connection established: \n\tconnection Address: " << conn << ", Loop thread Id:"
                                                                       << conn->GetLoopThreadId());
}

// 設置TcpServer回調
void HttpServer::_OnMessage(const PtrConnection &conn, Buffer &buffer)
{
    while (buffer.GetReadableSize() > 0)
    {
        // 1. 獲取協議上下文
        HttpContext *context = conn->GetContext().Get<HttpContext>();

        // 2. 通過上下文對緩衝區的數據進行解析,得到HttpRequest對象
        context->ParseRequest(buffer);
        if (context->GetAcceptStatus() == HttpAcceptStatus::ACCEPTING_ERROR)
        {
            // 解析過程中出現錯誤,構造HTTP并發送響應
            this->SendResponse(conn, context->GetRequest(), this->error_response);
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

        // 3. 判斷請求路由+業務處理

        // 4. 組織HTTP報文,發送響應

        // 5. 重置上下文
        context->Reset();

        // 5. 根據長短連接決定是否關閉連接
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