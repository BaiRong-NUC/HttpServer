#pragma once
#include <utils/public.h>
#include <utils/utils.h>

/**
 * HTTP请求解析与管理，支持GET、POST等方法，解析请求行、头部和消息体。
 */

class HttpRequest
{
   private:
    std::string method;                                    // 请求方法，如GET、POST等
    std::string uri;                                       // 请求URI
    std::string version;                                   // HTTP版本
    std::unordered_map<std::string, std::string> headers;  // 请求头部字段
    std::string body;                                      // 请求消息体
   private:
    bool ParseHttpRequest(const std::string &raw_request);  // 解析原始HTTP请求字符串
};