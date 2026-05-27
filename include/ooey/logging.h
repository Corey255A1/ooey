#ifndef OOEY_LOGGING_H
#define OOEY_LOGGING_H

#include <iostream>
#include <string>
#include <sstream>

#ifdef OOEY_LOGGING

namespace ooey {

enum class LogLevel {
    Debug,
    Info,
    Warning,
    Error
};

class Logger {
public:
    static Logger& getInstance() {
        static Logger instance;
        return instance;
    }

    void setLogLevel(LogLevel level) { m_logLevel = level; }
    LogLevel getLogLevel() const { return m_logLevel; }

    void log(LogLevel level, const std::string& category, const std::string& message) {
        if (static_cast<int>(level) >= static_cast<int>(m_logLevel)) {
            std::string levelStr;
            switch (level) {
                case LogLevel::Debug:   levelStr = "DEBUG"; break;
                case LogLevel::Info:    levelStr = "INFO"; break;
                case LogLevel::Warning: levelStr = "WARNING"; break;
                case LogLevel::Error:   levelStr = "ERROR"; break;
            }
            std::cout << "[" << levelStr << "][" << category << "] " << message << std::endl;
        }
    }

private:
    Logger() : m_logLevel(LogLevel::Info) {}
    LogLevel m_logLevel;
};

} // namespace ooey

// Logging macros - active when OOEY_LOGGING is defined
#define OOEY_LOG_DEBUG(category, msg) \
    do { \
        std::ostringstream oss; \
        oss << msg; \
        ooey::Logger::getInstance().log(ooey::LogLevel::Debug, category, oss.str()); \
    } while (0)

#define OOEY_LOG_INFO(category, msg) \
    do { \
        std::ostringstream oss; \
        oss << msg; \
        ooey::Logger::getInstance().log(ooey::LogLevel::Info, category, oss.str()); \
    } while (0)

#define OOEY_LOG_WARNING(category, msg) \
    do { \
        std::ostringstream oss; \
        oss << msg; \
        ooey::Logger::getInstance().log(ooey::LogLevel::Warning, category, oss.str()); \
    } while (0)

#define OOEY_LOG_ERROR(category, msg) \
    do { \
        std::ostringstream oss; \
        oss << msg; \
        ooey::Logger::getInstance().log(ooey::LogLevel::Error, category, oss.str()); \
    } while (0)

#else

// No-op implementations when OOEY_LOGGING is not defined
#define OOEY_LOG_DEBUG(category, msg) do { (void)(category); (void)(msg); } while (0)
#define OOEY_LOG_INFO(category, msg) do { (void)(category); (void)(msg); } while (0)
#define OOEY_LOG_WARNING(category, msg) do { (void)(category); (void)(msg); } while (0)
#define OOEY_LOG_ERROR(category, msg) do { (void)(category); (void)(msg); } while (0)

#endif // OOEY_LOGGING

#endif // OOEY_LOGGING_H
