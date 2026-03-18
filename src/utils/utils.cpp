#include "utils/utils.h"

// 静态成员变量定义
std::unordered_map<int, std::string> Utils::status_messages = {{100, "Continue"},
                                                               {101, "Switching Protocol"},
                                                               {102, "Processing"},
                                                               {103, "Early Hints"},
                                                               {200, "OK"},
                                                               {201, "Created"},
                                                               {202, "Accepted"},
                                                               {203, "Non-Authoritative Information"},
                                                               {204, "No Content"},
                                                               {205, "Reset Content"},
                                                               {206, "Partial Content"},
                                                               {207, "Multi-Status"},
                                                               {208, "Already Reported"},
                                                               {226, "IM Used"},
                                                               {300, "Multiple Choice"},
                                                               {301, "Moved Permanently"},
                                                               {302, "Found"},
                                                               {303, "See Other"},
                                                               {304, "Not Modified"},
                                                               {305, "Use Proxy"},
                                                               {306, "unused"},
                                                               {307, "Temporary Redirect"},
                                                               {308, "Permanent Redirect"},
                                                               {400, "Bad Request"},
                                                               {401, "Unauthorized"},
                                                               {402, "Payment Required"},
                                                               {403, "Forbidden"},
                                                               {404, "Not Found"},
                                                               {405, "Method Not Allowed"},
                                                               {406, "Not Acceptable"},
                                                               {407, "Proxy Authentication Required"},
                                                               {408, "Request Timeout"},
                                                               {409, "Conflict"},
                                                               {410, "Gone"},
                                                               {411, "Length Required"},
                                                               {412, "Precondition Failed"},
                                                               {413, "Payload Too Large"},
                                                               {414, "URI Too Long"},
                                                               {415, "Unsupported Media Type"},
                                                               {416, "Range Not Satisfiable"},
                                                               {417, "Expectation Failed"},
                                                               {418, "I'm a teapot"},
                                                               {421, "Misdirected Request"},
                                                               {422, "Unprocessable Entity"},
                                                               {423, "Locked"},
                                                               {424, "Failed Dependency"},
                                                               {425, "Too Early"},
                                                               {426, "Upgrade Required"},
                                                               {428, "Precondition Required"},
                                                               {429, "Too Many Requests"},
                                                               {431, "Request Header Fields Too Large"},
                                                               {451, "Unavailable For Legal Reasons"},
                                                               {501, "Not Implemented"},
                                                               {502, "Bad Gateway"},
                                                               {503, "Service Unavailable"},
                                                               {504, "Gateway Timeout"},
                                                               {505, "HTTP Version Not Supported"},
                                                               {506, "Variant Also Negotiates"},
                                                               {507, "Insufficient Storage"},
                                                               {508, "Loop Detected"},
                                                               {510, "Not Extended"},
                                                               {511, "Network Authentication Required"}};

std::vector<std::string> Utils::Split(const std::string &str, const std::string &delimiter, bool ignore_empty)
{
    std::vector<std::string> result;
    size_t start = 0;
    size_t end = str.find(delimiter);
    while (end != std::string::npos)
    {
        result.push_back(str.substr(start, end - start));
        start = end + delimiter.length();
        end = str.find(delimiter, start);
    }
    result.push_back(str.substr(start));

    // 忽略空字符串
    if (ignore_empty)
    {
        result.erase(std::remove_if(result.begin(), result.end(), [](const std::string &s) { return s.empty(); }),
                     result.end());
    }
    return result;
}

bool Utils::GetFileContent(const std::string &file_name, Buffer *buffer)
{
    if (buffer == nullptr)
    {
        LOG(ERROR, "Buffer is null for file: " << file_name);
        return false;
    }

    std::ifstream file(file_name, std::ios::binary);
    // 文件描述符退出函数后销毁
    if (!file.is_open())
    {
        LOG(ERROR, "Failed to open file: " << file_name);
        return false;  // 文件打开失败
    }

    constexpr size_t kChunkSize = 8192;
    char chunk[kChunkSize];
    while (file)
    {
        file.read(chunk, static_cast<std::streamsize>(kChunkSize));
        std::streamsize bytes = file.gcount();
        if (bytes <= 0)
        {
            break;
        }

        if (!buffer->Write(chunk, static_cast<uint64_t>(bytes)))
        {
            LOG(ERROR, "Failed to write file content into buffer: " << file_name);
            return false;
        }
    }

    if (file.bad())
    {
        LOG(ERROR, "I/O error while reading file: " << file_name);
        return false;
    }

    return true;
}

bool Utils::WriteFileContent(const char *file, const std::string &content)
{
    // 覆盖写入文件内容,文件描述符退出函数后销毁,没有文件则失败
    std::ofstream f(file, std::ios::binary | std::ios::trunc);
    if (!f.is_open())
    {
        LOG(ERROR, "Failed to open file for writing: " << file);
        return false;
    }
    f.write(content.data(), content.size());
    if (!f.good())
    {
        LOG(ERROR, "Failed to write content to file: " << file);
        return false;
    }
    return true;
}

std::string Utils::UrlEncode(const std::string &str, bool encode_space_as_plus)
{
    std::string result;
    for (const char &ch : str)
    {
        if (isalnum(static_cast<unsigned char>(ch)) || ch == '-' || ch == '_' || ch == '.' || ch == '~')
        {
            // 不编码的字符
            result += ch;
        }
        else if (ch == ' ' && encode_space_as_plus)
        {
            // 空格编码为+
            result += '+';
        }
        else
        {
            // 其他字符需要编码,转化为%XX格式的十六进制表示,其中XX是字符的ASCII码的两位十六进制数
            char buf[4] = {0};
            snprintf(buf, sizeof(buf), "%%%02X", static_cast<unsigned char>(ch));
            result += buf;
        }
    }
    return result;
}

std::string Utils::UrlDecode(const std::string &str, bool decode_plus_as_space)
{
    std::string result;
    for (size_t i = 0; i < str.size(); ++i)
    {
        if (str[i] == '%' && i + 2 < str.size() && isxdigit(static_cast<unsigned char>(str[i + 1])) &&
            isxdigit(static_cast<unsigned char>(str[i + 2])))
        {
            // %XX格式的十六进制数,转化为对应的字符
            char hex[3] = {str[i + 1], str[i + 2], '\0'};
            result += static_cast<char>(strtol(hex, nullptr, 16));
            i += 2;  // 跳过已处理的%XX
        }
        else if (str[i] == '+' && decode_plus_as_space)
        {
            // +解码为空格
            result += ' ';
        }
        else
        {
            // 其他字符直接添加到结果中
            result += str[i];
        }
    }
    return result;
}

std::string Utils::GetStatusMessage(int status_code)
{
    auto it = status_messages.find(status_code);
    if (it != status_messages.end())
    {
        return it->second;
    }
    return "Unknown Status";
}