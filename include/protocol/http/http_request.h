#pragma once
#include <utils/public.h>
#include <utils/utils.h>

/**
 * HTTP请求解析与管理，支持GET、POST等方法，解析请求行、头部和消息体。
 * 1. 存储HTTP请求信息
 * 2. HTTP格式:
 *      请求行: METHOD URI VERSION\r\n
 *      请求头部: Header-Name: Header-Value\r\n
 *      空行: \r\n
 *      正文: 可选,根据Content-Length或Transfer-Encoding确定长度
 * 3. 方法:
 *      - 提供查询字符串,头部字段的查询,获取,插入
 *      - 获取正文长度
 *      - 判断是否为长连接
 */

class HttpRequest
{
   private:
    std::smatch _matchs;  // 保存请求行的正则表达式匹配结果,用于提取请求方法、URI和HTTP版本等信息
   public:
    std::string method;                                         // 请求方法，如GET、POST等
    std::string uri;                                            // 请求URI资源路径,可能包含查询字符串
    std::string version;                                        // HTTP版本
    std::unordered_map<std::string, std::string> headers;       // 请求头部字段
    std::unordered_map<std::string, std::string> query_params;  // 查询字符串
    std::string body;                                           // 请求消息体
    bool ParseHttpRequest(const std::string &raw_request);      // 解析原始HTTP请求字符串
    
};