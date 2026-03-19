#include <protocol/http/http_context.h>

HttpContext::HttpContext() : _response_status(200), _accept_status(HttpAcceptStatus::ACCEPTING_REQUEST_LINE) {}

int HttpContext::GetResponseStatus() const { return this->_response_status; }
HttpAcceptStatus HttpContext::GetAcceptStatus() const { return this->_accept_status; }
HttpRequest &HttpContext::GetRequest() { return this->_request; }
HttpResponse &HttpContext::GetResponse() { return this->_response; }

bool HttpContext::RecvRequest(Buffer &buffer)
{
    // 1. 解析请求行
    std::string request_line = buffer.ReadLine(false);  // 不足一行先不读出来
    int readSize = buffer.GetReadableSize();
    if (request_line.empty())
    {
        if (readSize > MAX_LINE_SIZE)
        {
            // 请求行过长,超过最大限制,返回414 Request-URI Too Lon{g错误
            this->_response_status = 414;
            this->_accept_status = HttpAcceptStatus::ACCEPTING_ERROR;
            return false;
        }
        // 数据不为空,但没有完整的请求行,继续等待数据
        this->_accept_status = HttpAcceptStatus::ACCEPTING_REQUEST_LINE;
        return true;
    }
    // 读取到完整的请求行
    if (readSize > MAX_LINE_SIZE)
    {
        // 请求行过长,超过最大限制,返回414 Request-URI Too Long错误
        this->_response_status = 414;
        this->_accept_status = HttpAcceptStatus::ACCEPTING_ERROR;
        return false;
    }
    // 解析请求行,提取请求方法、URI和HTTP版本等信息
    
}
