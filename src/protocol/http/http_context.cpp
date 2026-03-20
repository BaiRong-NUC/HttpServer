#include <protocol/http/http_context.h>

HttpContext::HttpContext() : _response_status(200), _accept_status(HttpAcceptStatus::ACCEPTING_REQUEST_LINE)
{
    this->_http_request_line_re = std::regex("^([A-Z]+)\\s+(\\S+)\\s+HTTP/([0-9\\.]+)$");
}

int HttpContext::GetResponseStatus() const { return this->_response_status; }
HttpAcceptStatus HttpContext::GetAcceptStatus() const { return this->_accept_status; }
HttpRequest &HttpContext::GetRequest() { return this->_request; }
HttpResponse &HttpContext::GetResponse() { return this->_response; }

bool HttpContext::ParseRequest(Buffer &buffer)
{
    // 1. 解析请求行
    std::string request_line = buffer.ReadLine(false);  // 不足一行先不读出来
    auto readSize = buffer.GetReadableSize();
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
    // 去掉行尾可能存在的 '\r'（处理 CRLF）
    if (!request_line.empty() && request_line.back() == '\r')
    {
        request_line.pop_back();
    }
    // 读取到完整的请求行
    if (request_line.size() > MAX_LINE_SIZE)
    {
        // 请求行过长,超过最大限制,返回414 Request-URI Too Long错误
        this->_response_status = 414;
        this->_accept_status = HttpAcceptStatus::ACCEPTING_ERROR;
        return false;
    }
    // 解析请求行,提取请求方法、URI和HTTP版本等信息
    std::smatch matchs;
    if (std::regex_match(request_line, matchs, this->_http_request_line_re))
    {
        this->_request.method = matchs[1].str();  // 请求方法
        std::string full_uri = matchs[2].str();   // 包含 path 和可选 query
        // 分离 path 与 query,这里可能处理多个?,把第一个之后的都当作 query
        size_t qpos = full_uri.find('?');
        std::string path = (qpos == std::string::npos) ? full_uri : full_uri.substr(0, qpos);
        std::string query_str = (qpos == std::string::npos) ? "" : full_uri.substr(qpos + 1);
        this->_request.uri = Utils::UrlDecode(path, false);

        if (!query_str.empty())
        {
            // 先按 '&' 拆分每个参数片段，再对 key/value 单独解码
            std::vector<std::string> parts = Utils::Split(query_str, "&");
            for (const auto &param : parts)
            {
                if (param.empty()) continue;
                size_t eq_pos = param.find('=');
                if (eq_pos != std::string::npos)
                {
                    std::string key = param.substr(0, eq_pos);
                    std::string value = param.substr(eq_pos + 1);
                    key = Utils::UrlDecode(key, true);
                    value = Utils::UrlDecode(value, true);
                    this->_request.query_params[key] = value;
                }
                else
                {
                    // 没有 '=', 当作 key 但值为空
                    std::string key = Utils::UrlDecode(param, true);
                    this->_request.query_params[key] = "";
                }
            }
        }
        this->_request.version = matchs[3].str();  // HTTP版本
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
