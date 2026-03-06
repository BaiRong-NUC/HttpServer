#include "./public.h"
#pragma once
inline std::string GetRelativeFile(const char *file)
{
    const char *pos = strstr(file, "HttpServer/");
    if (pos)
    {
        return std::string("~/") + (pos + strlen("HttpServer/"));
    }
    return file;
}
#define LOG(level, msg) std::cout << "[" << level << " " << GetRelativeFile(__FILE__) << ":" << __LINE__ << "] " << msg << std::endl