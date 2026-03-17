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
    if (buffer == nullptr)
    {
        LOG(ERROR, "Buffer is null for file: " << file_name);
        return false;
    }

    std::ifstream file(file_name, std::ios::binary);
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
    file.close();
    if (file.bad())
    {
        LOG(ERROR, "I/O error while reading file: " << file_name);
        return false;
    }

    return true;
}