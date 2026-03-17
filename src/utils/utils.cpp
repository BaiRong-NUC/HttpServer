#include "utils/utils.h"

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
    std::ifstream file(file_name, std::ios::binary);
    if (!file.is_open())
    {
        LOG(ERROR, "Failed to open file: " << file_name);
        return false;  // 文件打开失败
    }

    buffer->Write(std::string(std::istreambuf_iterator<char>(file), std::istreambuf_iterator<char>()));
    file.close();
    return true;
}