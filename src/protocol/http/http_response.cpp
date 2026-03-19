#include <protocol/http/http_response.h>

HttpResponse::HttpResponse(int status) : status_code(status), is_redirect(false) {}

void HttpResponse::Clear()
{
    this->status_code = 200;
    this->headers.clear();
    this->body.clear();
    this->is_redirect = false;
    this->redirect_location.clear();
    this->version.clear();
}

void HttpResponse::SetHeader(const std::string &key, const std::string &value) { this->headers[key] = value; }

bool HttpResponse::HasHeader(const std::string &key) const { return this->headers.find(key) != this->headers.end(); }

std::string HttpResponse::GetHeader(const std::string &key) const
{
    if (this->HasHeader(key))
    {
        return this->headers.at(key);
    }
    return "";
}

void HttpResponse::SetBody(const std::string &body, const std::string &content_type)
{
    this->body = body;
    this->SetHeader("Content-Type", content_type);
}

std::string HttpResponse::GetBody() const { return this->body; }

void HttpResponse::SetRedirect(const std::string &location, int status_code)
{
    this->is_redirect = true;
    this->redirect_location = location;
    this->status_code = status_code;
    this->SetHeader("Location", location);
}

bool HttpResponse::IsKeepAlive() const
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
