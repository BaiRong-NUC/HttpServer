#include <protocol/http/http_server.h>

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

        HttpRequest server_request;    // 服務器構造請求行
        HttpResponse server_response;  // 服務器構造請求正文
        // 2. 通過上下文對緩衝區的數據進行解析,得到HttpRequest對象
        context->ParseRequest(buffer);
        if (context->GetAcceptStatus() == HttpAcceptStatus::ACCEPTING_ERROR)
        {
            // 解析過程中出現錯誤,根據響應狀態碼設置響應內容
            if (this->error_response != nullptr)
            {
                // 用戶提供的函數構造了請求行和正文
                this->error_response(server_request, server_response);
            }
            else
            {
                // 默認響應: 返回 404 Not Found
                server_request.method = "GET";
                server_request.uri = "/";
                server_request.version = "HTTP/1.1";
                server_request.SetHeader("Host", "localhost");

                server_response.status_code = 404;
                std::string body = "<html><body><h1>404 Not Found</h1></body></html>";
                server_response.SetBody(body);
                // 明確設置內容長度與關閉連接
                server_response.SetHeader("Content-Length", std::to_string(server_response.GetBody().size()));
                server_response.SetHeader("Connection", "close");
            }

            // 構造HTTP并發送響應
            this->SendResponse(conn, server_request, server_response);
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
        HttpResponse &client_response = context->GetResponse();

        // 3. 判斷請求路由+業務處理

        // 4. 組織HTTP報文,發送響應

        // 5. 重置上下文
        context->Reset();

        // 5. 根據長短連接決定是否關閉連接
        if (client_response.IsKeepAlive() == false)
        {
            conn->Close();
            return;
        }
    }
}