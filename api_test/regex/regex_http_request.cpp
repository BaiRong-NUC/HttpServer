// 利用正则表达式解析HTTP请求行
#include <iostream>
#include <regex>
#include <string>

int main(int argc, char const *argv[])
{
    std::string http = "GET /login?user=admin&password=123 HTTP/1.1\r\n";
    std::smatch matchs;
    std::regex re("([A-Z]+)\\s+([^\\s]+)\\s+HTTP/([0-9\\.]+)");
    if (std::regex_search(http, matchs, re))
    {
        std::cout << "Method: " << matchs[1] << std::endl;
        std::cout << "Path: " << matchs[2] << std::endl;
        std::cout << "Version: " << matchs[3] << std::endl;
    }
    return 0;
}
