
#include "protocol/http/http_context.h"
#include "utils/buffer.h"
#include <cassert>
#include <iostream>
using namespace std;

void test_HttpContext_ParseRequest()
{
    // 构造一个简单的 HTTP GET 请求
    // std::string req = "GET /index.html?user=abc&id=123 HTTP/1.1\r\nHost: localhost\r\n\r\n";
    std::string req = "GET /index.html?user=abc&id=123 HTTP/1.1\r\nHost: localhost\r\n\r\n";
    Buffer buf;
    buf.Write(req);
    HttpContext ctx;
    bool ok = ctx.ParseRequest(buf);
    assert(ok);
    assert(ctx.GetAcceptStatus() == HttpAcceptStatus::ACCEPTING_REQUEST_LINE ||
           ctx.GetAcceptStatus() == HttpAcceptStatus::ACCEPTING_HEADERS);
    HttpRequest &request = ctx.GetRequest();
    assert(request.method == "GET");
    assert(request.uri == "/index.html");
    assert(request.version == "1.1");
    assert(request.GetQueryParam("user") == "abc");
    assert(request.GetQueryParam("id") == "123");
    std::cout << "test_HttpContext_ParseRequest 通过!" << std::endl;
}

void test_HttpContext_Getters()
{
    HttpContext ctx;
    assert(ctx.GetResponseStatus() == 200);
    assert(ctx.GetAcceptStatus() == HttpAcceptStatus::ACCEPTING_REQUEST_LINE);
    HttpRequest &req = ctx.GetRequest();
    HttpResponse &resp = ctx.GetResponse();
    resp.status_code = 404;
    assert(ctx.GetResponse().status_code == 404);
    std::cout << "test_HttpContext_Getters 通过!" << std::endl;
}

void test_HttpContext_ParseRequest_NoQuery()
{
    // 构造一个不带查询字符串的 HTTP GET 请求
    std::string req = "GET /index.html HTTP/1.1\r\nHost: localhost\r\n\r\n";
    Buffer buf;
    buf.Write(req);
    HttpContext ctx;
    bool ok = ctx.ParseRequest(buf);
    assert(ok);
    assert(ctx.GetAcceptStatus() == HttpAcceptStatus::ACCEPTING_REQUEST_LINE ||
           ctx.GetAcceptStatus() == HttpAcceptStatus::ACCEPTING_HEADERS);
    HttpRequest &request = ctx.GetRequest();
    assert(request.method == "GET");
    assert(request.uri == "/index.html");
    assert(request.version == "1.1");
    assert(request.GetQueryParam("user").empty());
    assert(request.GetQueryParam("id").empty());
    std::cout << "test_HttpContext_ParseRequest_NoQuery 通过!" << std::endl;
} 

int main()
{
    test_HttpContext_ParseRequest();
    test_HttpContext_ParseRequest_NoQuery();
    test_HttpContext_Getters();
    std::cout << "所有 HttpContext 测试通过!" << std::endl;
    return 0;
}
