#pragma once

#include <iostream>
#include <string>
using std::cout;
using std::endl;

#include "UnCopyable.h"
#include "Timestamp.h"

#define LOG_INFO(logmsgFormat, ...)                              \
    do                                                           \
    {                                                            \
        Logger &logger = Logger::instance();                     \
        logger.setLogLevel(INFO);                                \
        char buf[1024] = {0};                                    \
        snprintf(buf, sizeof(buf), logmsgFormat, ##__VA_ARGS__); \
        logger.log(buf);                                         \
    } while (0)

#define LOG_ERROR(logmsgFormat, ...)                             \
    do                                                           \
    {                                                            \
        Logger &logger = Logger::instance();                     \
        logger.setLogLevel(ERROR);                               \
        char buf[1024] = {0};                                    \
        snprintf(buf, sizeof(buf), logmsgFormat, ##__VA_ARGS__); \
        logger.log(buf);                                         \
    } while (0)

#define LOG_FATAL(logmsgFormat, ...)                             \
    do                                                           \
    {                                                            \
        Logger &logger = Logger::instance();                     \
        logger.setLogLevel(FATAL);                               \
        char buf[1024] = {0};                                    \
        snprintf(buf, sizeof(buf), logmsgFormat, ##__VA_ARGS__); \
        logger.log(buf);                                         \
        exit(-1);                                                \
    } while (0)

#ifdef MUDEBUG // 使用条件编译让Debug信息只在Debug模式下打印
#define LOG_DEBUG(logmsgFormat, ...)                             \
    do                                                           \
    {                                                            \
        Logger &logger = Logger::instance();                     \
        logger.setLogLevel(DEBUG);                               \
        char buf[1024] = {0};                                    \
        snprintf(buf, sizeof(buf), logmsgFormat, ##__VA_ARGS__); \
        logger.log(buf);                                         \
    } while (0)
#else
#define LOG_DEBUG(logmsgFormat, ...)
#endif

// INFO  ERROR FATAL DEBUG
enum LogLevel
{
    INFO,
    ERROR,
    FATAL,
    DEBUG
};

class Logger : UnCopyable
{
public:
    inline static Logger &instance()
    {
        static Logger logger;
        return logger;
    }
    inline void setLogLevel(int level) { _logLevel = level; }
    inline void log(std::string msg)
    {
        switch (_logLevel)
        {
        case INFO:
            cout << "[INFO ";
            break;
        case ERROR:
            cout << "[ERROR ";
            break;
        case FATAL:
            cout << "[FATAL ";
            break;
        case DEBUG:
            cout << "[DEBUG ";
            break;
        default:
            cout << "[UNKNOWN ";
        }

        // 打印时间和msg
        cout << Timestamp::now().toString() << "]" << msg << endl;
    }

private:
    int _logLevel;
    Logger() {};
};