#include <protocol/http/http_request.h>

void HttpRequest::SetHeader(const std::string &key, const std::string &value) { this->headers[key] = value; }

bool HttpRequest::HasHeader(const std::string &key) const { return this->headers.find(key) != this->headers.end(); }

std::string HttpRequest::GetHeader(const std::string &key) const
{
    if (this->HasHeader(key))
    {
        return this->headers.at(key);
    }
    return "";
}
void HttpRequest::SetQueryParams(const std::string &key, const std::string &value) { this->query_params[key] = value; }

bool HttpRequest::HasQueryParam(const std::string &key) const
{
    return this->query_params.find(key) != this->query_params.end();
}

std::string HttpRequest::GetQueryParam(const std::string &key) const
{
    if (this->HasQueryParam(key))
    {
        return this->query_params.at(key);
    }
    return "";
}

size_t HttpRequest::GetBodyLength() const
{
    if (this->HasHeader("Content-Length"))
    {
        try
        {
            return std::stoul(this->GetHeader("Content-Length"));
        }
        catch (const std::exception &e)
        {
            LOG(ERROR,
                "Invalid Content-Length value: " << this->GetHeader("Content-Length") << ", error: " << e.what());
            return 0;  // 无效的Content-Length值,返回0
        }
    }
    return 0;  // 没有Content-Length头部字段,返回0
}

bool HttpRequest::IsKeepAlive() const
{
    if (this->HasHeader("Connection"))
    {
        std::string connection_value = this->GetHeader("Connection");
        // HTTP/1.1默认是长连接,除非明确指定为"close"
        if (connection_value == "keep-alive")
        {
            return true;  // 明确指定为keep-alive,认为是长连接
        }
        else if (connection_value == "close")
        {
            return false;  // 明确指定为close,认为是短连接
        }
    }
    // 没有Connection头部字段,根据HTTP版本判断默认连接类型
    return this->version == "HTTP/1.1";  // HTTP/1.1默认是长连接,HTTP/1.0默认是短连接
}

void HttpRequest::Clear()
{
    this->method.clear();
    this->uri.clear();
    this->version.clear();
    this->headers.clear();
    this->query_params.clear();
    this->body.clear();
    this->_matchs = std::smatch();  // 重置正则表达式匹配结果
}
