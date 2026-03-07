#include "../include/log.h"

std::string LogLevelToString(LogLevel level)
{
    switch (level)
    {
    case INFO:
        return "INFO";
    case WARNING:
        return "WARNING";
    case ERROR:
        return "ERROR";
    default:
        return "UNKNOWN";
    }
}

std::string LogLevelToString(std::string level)
{
    return level;
}

std::string GetRelativeFile(const char *file)
{
    const char *pos = strstr(file, "HttpServer/");
    if (pos)
    {
        return std::string("~/") + (pos + strlen("HttpServer/"));
    }
    return file;
}