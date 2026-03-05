#include <iostream>
#include <regex>
#include <string>

int main(int argc, char const *argv[])
{
    // std::string http = "GET /login?user=admin&password=123 HTTP/1.1\n";
    // std::string http = "GET /login?user=admin&password=123 HTTP/1.1";
    // std::string http = "GET   /login?user=admin&password=123 HTTP/1.1\r\n\r\n";
    // std::string http = "GET /hello/index.html HTTP/1.1\r\n";
    std::string http = "GET /hello/index.html HTTP/1.1\n";
    std::smatch matchs;
    /**
     * 正则表达式解析
        1. ([A-Z]+): 匹配请求方法，例如 GET。
            捕获组 1：matchs[1]。
        2. \\s+: 匹配一个或多个空白字符。
        3. (/[^\\s\\?]+): 匹配路径部分，以 / 开头，直到遇到空白字符或 ? 为止。
        捕获组 2：matchs[2]。
        4. (\\?([^\\s]+))?: 匹配查询参数部分（以 ? 开头），并将其设为可选项（通过 ? 实现）。
            捕获组 3：整个查询参数部分（包括 ?）。
            捕获组 4：查询参数的具体内容（不包括 ?）。
        5. \\s+HTTP/([0-9\\.]+): 匹配 HTTP/ 后的版本号。
            捕获组 5：matchs[5]。
     */
    std::regex re("([A-Z]+)\\s+(/[^\\s\\?]+)(\\?([^\\s]+))?\\s+HTTP/([0-9\\.]+)");
    if (std::regex_search(http, matchs, re))
    {
        std::cout << "Method: " << matchs[1] << std::endl; // GET
        std::cout << "Path: " << matchs[2] << std::endl;   // /hello/index.html
        if (matchs[4].matched)
        {
            std::cout << "Query: " << matchs[4] << std::endl; // 查询参数（如果存在）
        }
        std::cout << "Version: " << matchs[5] << std::endl;
    }
    else
    {
        std::cout << "No match found." << std::endl;
    }
    return 0;
}