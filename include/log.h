#include "./public.h"
#pragma once

// Log.h定义了一个简单的日志系统,使用宏LOG来输出日志信息,并包含日志级别和文件位置信息,时间信息.
enum LogLevel
{
    INFO,
    WARNING,
    ERROR
};
std::string LogLevelToString(LogLevel level);
std::string LogLevelToString(std::string level);
std::string GetRelativeFile(const char *file);
#define LOG(level, msg) std::cout << "[" << LogLevelToString(level) << " " << GetRelativeFile(__FILE__) << ":" << __LINE__ << "] " << msg << std::endl