#include <protocol/http/http_context.h>

HttpContext::HttpContext() : _response_status(200), _accept_status(HttpAcceptStatus::ACCEPTING_REQUEST_LINE)
{
    this->_http_request_line_re = std::regex("([A-Z]+)\\s+(/[^\\s\\?]+)(?:\\?([^\\s]+))?\\s+HTTP/([0-9\\.]+)");
}

int HttpContext::GetResponseStatus() const { return this->_response_status; }
HttpAcceptStatus HttpContext::GetAcceptStatus() const { return this->_accept_status; }
HttpRequest &HttpContext::GetRequest() { return this->_request; }
HttpResponse &HttpContext::GetResponse() { return this->_response; }

bool HttpContext::ParseRequest(Buffer &buffer)
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
    std::smatch matchs;
    if (std::regex_search(request_line, matchs, this->_http_request_line_re))
    {
        this->_request.method = matchs[1];  // 请求方法
        this->_request.uri = matchs[2];     // 请求URI
        SetLogLevel(INFO);
        for (int i = 0; i < matchs.size(); ++i)
        {
            LOG(INFO, "matchs[" << i << "]: " << matchs[i]);
        }
        if (matchs[3].matched)
        {
            // 存在查询字符串,解析查询参数
            std::string query_str = matchs[3];
            size_t pos = 0;
            while ((pos = query_str.find('&')) != std::string::npos)
            {
                std::string param = query_str.substr(0, pos);
                size_t eq_pos = param.find('=');
                if (eq_pos != std::string::npos)
                {
                    std::string key = param.substr(0, eq_pos);
                    std::string value = param.substr(eq_pos + 1);
                    this->_request.query_params[key] = value;
                }
                query_str.erase(0, pos + 1);
            }
            // 处理最后一个参数
            size_t eq_pos = query_str.find('=');
            if (eq_pos != std::string::npos)
            {
                std::string key = query_str.substr(0, eq_pos);
                std::string value = query_str.substr(eq_pos + 1);
                this->_request.query_params[key] = value;
            }
        }
        this->_request.version = matchs[4];  // HTTP版本
    }
    else
    {
        // 请求行格式错误,返回400 Bad Request错误
        this->_response_status = 400;
        this->_accept_status = HttpAcceptStatus::ACCEPTING_ERROR;
        return false;
    }

    // 请求行解析成功,进入接受请求头部阶段
    this->_accept_status = HttpAcceptStatus::ACCEPTING_HEADERS;

    return true;
}
