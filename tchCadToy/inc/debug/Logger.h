#pragma once
#include <iostream>
#include <fstream>
#include <cstdlib>
#include <string>
#include <source_location>
#include <format>
#include <mutex>
#include <memory>
#include <vector>
#include <chrono>
#include <thread>
#include <queue>
#include <functional>
#include <atomic>
#include <filesystem>

namespace tch {

class Logger
{
public:
    enum LogLevel
    {
        Trace,              // the most detailed informations, usually do not use, for detailed tracing.
        Debug,              // general debug output.
        Info,               // usual informations.
        Warning,            // warning, maybe something wrong but little problem, maybe not.
        Error,              // something is definitely wrong, the program can be continued.
        Fatal,              // something is definitely wrong, better to kill the program.
        FinalLevel = 100    // no meaning, for comparing, do not use.
    };

    struct LogConfig
    {
        LogLevel lowestLevel = Trace;
        bool coloredOutput = true;
        bool asyncLogging = false;
        size_t maxLogFileSize = 10 * 1024 * 1024; // 10MB
        int maxLogFiles = 5;
        std::string logFilePattern = "log_{}.txt";
    };

    Logger(std::ostream& os = std::cout, const LogConfig& config = LogConfig());
    Logger(const std::string& logFile, const LogConfig& config = LogConfig());
    ~Logger();
    
    // Configuration
    void setLowestOutputLevel(LogLevel level);
    void setColoredOutput(bool enabled);
    void setAsyncLogging(bool enabled);
    void setLogFileSizeLimit(size_t size);
    void setLogFileCountLimit(int count);
    void setLogFilePattern(const std::string& pattern);
    
    // Log level control
    void blockLevel(LogLevel level);
    void unblockLevel(LogLevel level);
    
    // Output stream management
    void addOutputStream(std::ostream& os);
    void removeOutputStream(std::ostream& os);
    
    // Log functions
    void trace(const std::string& str, const std::source_location& loc = std::source_location::current());
    void debug(const std::string& str, const std::source_location& loc = std::source_location::current());
    void info(const std::string& str, const std::source_location& loc = std::source_location::current());
    void warning(const std::string& str, const std::source_location& loc = std::source_location::current());
    void error(const std::string& str, const std::source_location& loc = std::source_location::current());
    void fatal(const std::string& str, const std::source_location& loc = std::source_location::current());

private:
    struct LogMessage
    {
        LogLevel level;
        std::string message;
        std::source_location location;
        std::chrono::system_clock::time_point timestamp;
    };

    std::mutex m_mutex; // for thread safety
    std::vector<std::reference_wrapper<std::ostream>> m_outputStreams;
    std::unique_ptr<std::ofstream> m_logFileStream;
    std::string m_logFilePath;
    
    // Configuration
    LogConfig m_config;
    
    // Log level blocking
    bool m_bBlockTrace = false;
    bool m_bBlockDebug = false;
    bool m_bBlockInfo = false;
    bool m_bBlockWarning = false;
    bool m_bBlockError = false;
    bool m_bBlockFatal = false;
    
    // Asynchronous logging
    std::atomic<bool> m_running{ true };
    std::unique_ptr<std::thread> m_logThread;
    std::queue<LogMessage> m_logQueue;
    std::condition_variable m_logCondition;
    
    // help functions
    static std::string stripFilePath(const char* file);
    std::string getLevelString(LogLevel level);
    std::string resetColor();
    std::string getTimestamp();
    
    // Log file management
    void checkLogFileSize();
    void rotateLogFiles();
    
    // Log thread function
    void logThreadFunc();
    
    // Log implementation
    void log(LogLevel level, const std::string& str, const std::source_location& loc);
    
    // Internal log processing
    void processLogMessage(const LogMessage& msg);
};

// Global logger functions

inline Logger* s_pGlobalLogger = nullptr;

Logger& globalLogger();
Logger& defaultLogger();
void setGlobalLogger(Logger* pLogger);

// Convenience macros

#define LOG_TRACE(...) ::tch::globalLogger().trace(std::format(__VA_ARGS__))
#define LOG_DEBUG(...) ::tch::globalLogger().debug(std::format(__VA_ARGS__))
#define LOG_INFO(...) ::tch::globalLogger().info(std::format(__VA_ARGS__))
#define LOG_WARNING(...) ::tch::globalLogger().warning(std::format(__VA_ARGS__))
#define LOG_ERROR(...) ::tch::globalLogger().error(std::format(__VA_ARGS__))
#define LOG_FATAL(...) ::tch::globalLogger().fatal(std::format(__VA_ARGS__))

// Assertion

inline void tchAssert(bool pred, const std::string& info = "", const std::source_location& loc = std::source_location::current())
{
    if (!pred)
    {
        globalLogger().fatal(std::format("Assertion failed: {}!", info), loc);
        std::exit(-1);
    }
}

} // namespace tch
