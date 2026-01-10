#pragma once

#include <string>
#include <iostream>
#include <cstdio>
#include <vector>
#include <cassert>
#include <unistd.h>
#include <chrono>
#include <sstream>
#include <iomanip>
#include <string.h>
#include <fstream>
#include <mutex>

inline std::string logFileName;
inline std::ofstream ofs;
inline std::mutex logMutex;

namespace {

std::vector<std::string> logLevelStr{
    "[ TRACE]",
    "[ DEBUG]",
    "[  INFO]",
    "[  WARN]",
    "[ ERROR]",
    "[ FATAL]"
};

inline std::string timestamp() {
    auto now = std::chrono::system_clock::now();
    std::time_t time = std::chrono::system_clock::to_time_t(now);
    
    auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch()) % 1000;

    std::stringstream ss;
    ss << std::put_time(std::gmtime(&time), "%Y%m%d %H:%M:%S") << "." 
       << std::setfill('0') << std::setw(3) << ms.count();

    return ss.str();
}

} //namespace anonymous

enum class LOG_LEVEL : unsigned {
    LOG_LEVEL_TRACE = 0,
    LOG_LEVEL_DEBUG,
    LOG_LEVEL_INFO,
    LOG_LEVEL_WARN,
    LOG_LEVEL_ERROR,
    LOG_LEVEL_FATAL
};

#ifndef NDEBUG
static LOG_LEVEL logLevel = LOG_LEVEL::LOG_LEVEL_DEBUG;
#else
static LOG_LEVEL logLevel = LOG_LEVEL::LOG_LEVEL_INFO;
#endif

namespace internal {

// 简单的格式化函数，支持基本格式
template<typename... Args>
inline void logSys(const std::string& file,
            int line,
            int to_abort,
            const char* fmt,
            Args... args)
{
    std::lock_guard<std::mutex> lock(logMutex);
    
    std::stringstream ss;
    ss << timestamp();
    ss << " [";
    ss << std::setfill(' ') << std::setw(5) << getpid();
    ss << "] ";

    const char* prefix = to_abort ? "[ SYSFA] " : "[SYSERR] ";
    ss << prefix;
    
    // 使用 snprintf 进行格式化
    char buffer[4096];
    snprintf(buffer, sizeof(buffer), fmt, args...);
    ss << buffer;

    const char* filename = strrchr(file.c_str(), '/');
    filename = filename ? filename + 1 : file.c_str();

    std::string output = ss.str() + ": " + strerror(errno) + " - " + filename + ":" + std::to_string(line) + "\n";

    if (logFileName.empty() || logFileName == "stdout") {
        std::cout << output << std::flush;
    }
    else {
        ofs << output << std::flush;
    }
    
    if (to_abort) {
        abort();
    }
}

template<typename... Args>
inline void logBase(const std::string& file,
            int line,
            LOG_LEVEL level,
            int to_abort,
            const char* fmt,
            Args... args)
{
    if (static_cast<unsigned>(level) < static_cast<unsigned>(logLevel)) {
        return;
    }
    
    std::lock_guard<std::mutex> lock(logMutex);
    
    std::ostringstream ss;
    ss << timestamp();
    ss << " [";
    ss << std::setfill(' ') << std::setw(5) << getpid();
    ss << "]";
    ss << " " << logLevelStr[static_cast<unsigned>(level)] << " ";
    
    // 使用 snprintf 进行格式化
    char buffer[4096];
    snprintf(buffer, sizeof(buffer), fmt, args...);
    ss << buffer;

    const char* filename = strrchr(file.c_str(), '/');
    filename = filename ? filename + 1 : file.c_str();

    std::string output = ss.str() + " - " + filename + ":" + std::to_string(line) + "\n";

    if (logFileName.empty() || logFileName == "stdout") {
        std::cout << output << std::flush;
    }
    else {
        ofs << output << std::flush;
    }

    if (to_abort) {
        abort();
    }
}

} //namespace internal

// 对外接口 - 使用宏定义
#define TRACE(fmt, ...) internal::logBase(__FILE__, __LINE__, LOG_LEVEL::LOG_LEVEL_TRACE, 0, fmt, ##__VA_ARGS__);

#define DEBUG(fmt, ...) internal::logBase(__FILE__, __LINE__, LOG_LEVEL::LOG_LEVEL_DEBUG, 0, fmt, ##__VA_ARGS__);

#define INFO(fmt, ...)  internal::logBase(__FILE__, __LINE__, LOG_LEVEL::LOG_LEVEL_INFO, 0, fmt, ##__VA_ARGS__);

#define WARN(fmt, ...)  internal::logBase(__FILE__, __LINE__, LOG_LEVEL::LOG_LEVEL_WARN, 0, fmt, ##__VA_ARGS__);

#define ERROR(fmt, ...) internal::logBase(__FILE__, __LINE__, LOG_LEVEL::LOG_LEVEL_ERROR, 0, fmt, ##__VA_ARGS__);

#define FATAL(fmt, ...) internal::logBase(__FILE__, __LINE__, LOG_LEVEL::LOG_LEVEL_FATAL, 1, fmt, ##__VA_ARGS__);

#define SYSERR(fmt, ...) internal::logSys(__FILE__, __LINE__, 0, fmt, ##__VA_ARGS__);

#define SYSFATAL(fmt, ...) internal::logSys(__FILE__, __LINE__, 1, fmt, ##__VA_ARGS__);

inline void setLogLevel(LOG_LEVEL rhs) { logLevel = rhs; }

inline void setLogFile(const std::string& fileName) {
    std::lock_guard<std::mutex> lock(logMutex);
    //关闭logFileName
    if (logFileName.size() > 0 && logFileName != "stdout") {
        ofs.close();
    }
    if (fileName != "stdout") {
        ofs.open(fileName, std::ios_base::out | std::ios_base::app);
    }
    logFileName = fileName;
}

