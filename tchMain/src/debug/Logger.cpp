#include "debug/Logger.h"
#include <algorithm>
#include <iomanip>
#include <sstream>

namespace tch {

Logger::Logger(std::ostream& os, const LogConfig& config)
    : m_config(config)
    , m_bBlockTrace(false)
    , m_bBlockDebug(false)
    , m_bBlockInfo(false)
    , m_bBlockWarning(false)
    , m_bBlockError(false)
    , m_bBlockFatal(false)
{
    addOutputStream(os);
    
    if (m_config.asyncLogging) {
        m_logThread = std::make_unique<std::thread>(&Logger::logThreadFunc, this);
    }
}

Logger::Logger(const std::string& logFile, const LogConfig& config)
    : m_config(config)
    , m_logFilePath(logFile)
    , m_bBlockTrace(false)
    , m_bBlockDebug(false)
    , m_bBlockInfo(false)
    , m_bBlockWarning(false)
    , m_bBlockError(false)
    , m_bBlockFatal(false)
{
    m_config.coloredOutput = false; // Disable colored output for file
    
    m_logFileStream = std::make_unique<std::ofstream>(logFile, std::ios::out | std::ios::app);
    if (m_logFileStream->is_open()) {
        addOutputStream(*m_logFileStream);
    }
    
    if (m_config.asyncLogging) {
        m_logThread = std::make_unique<std::thread>(&Logger::logThreadFunc, this);
    }
}

Logger::~Logger()
{
    if (m_config.asyncLogging && m_logThread) {
        m_running = false;
        m_logCondition.notify_one();
        if (m_logThread->joinable()) {
            m_logThread->join();
        }
    }
    
    if (this == s_pGlobalLogger) {
        s_pGlobalLogger = nullptr;
    }
}

// Configuration
void Logger::setLowestOutputLevel(LogLevel level)
{
    m_config.lowestLevel = level;
}

void Logger::setColoredOutput(bool enabled)
{
    m_config.coloredOutput = enabled;
}

void Logger::setAsyncLogging(bool enabled)
{
    if (enabled == m_config.asyncLogging) {
        return;
    }
    
    if (enabled) {
        // Enable async logging
        m_config.asyncLogging = true;
        m_logThread = std::make_unique<std::thread>(&Logger::logThreadFunc, this);
    } else {
        // Disable async logging
        m_config.asyncLogging = false;
        if (m_logThread) {
            m_running = false;
            m_logCondition.notify_one();
            if (m_logThread->joinable()) {
                m_logThread->join();
            }
            m_logThread.reset();
        }
        // Process any remaining logs
        std::queue<LogMessage> tempQueue;
        {
            std::lock_guard<std::mutex> lock(m_mutex);
            tempQueue.swap(m_logQueue);
        }
        while (!tempQueue.empty()) {
            processLogMessage(tempQueue.front());
            tempQueue.pop();
        }
    }
}

void Logger::setLogFileSizeLimit(size_t size)
{
    m_config.maxLogFileSize = size;
}

void Logger::setLogFileCountLimit(int count)
{
    m_config.maxLogFiles = count;
}

void Logger::setLogFilePattern(const std::string& pattern)
{
    m_config.logFilePattern = pattern;
}

// Log level control
void Logger::blockLevel(LogLevel level)
{
    switch (level)
    {
    case Trace:
        m_bBlockTrace = true;
        break;
    case Debug:
        m_bBlockDebug = true;
        break;
    case Info:
        m_bBlockInfo = true;
        break;
    case Warning:
        m_bBlockWarning = true;
        break;
    case Error:
        m_bBlockError = true;
        break;
    case Fatal:
        m_bBlockFatal = true;
        break;
    default:
        break;
    }
}

void Logger::unblockLevel(LogLevel level)
{
    switch (level)
    {
    case Trace:
        m_bBlockTrace = false;
        break;
    case Debug:
        m_bBlockDebug = false;
        break;
    case Info:
        m_bBlockInfo = false;
        break;
    case Warning:
        m_bBlockWarning = false;
        break;
    case Error:
        m_bBlockError = false;
        break;
    case Fatal:
        m_bBlockFatal = false;
        break;
    default:
        break;
    }
}

// Output stream management
void Logger::addOutputStream(std::ostream& os)
{
    std::lock_guard<std::mutex> lock(m_mutex);
    m_outputStreams.emplace_back(os);
}

void Logger::removeOutputStream(std::ostream& os)
{
    std::lock_guard<std::mutex> lock(m_mutex);
    auto it = std::find_if(m_outputStreams.begin(), m_outputStreams.end(),
        [&os](const std::reference_wrapper<std::ostream>& ref) {
            return &ref.get() == &os;
        });
    if (it != m_outputStreams.end()) {
        m_outputStreams.erase(it);
    }
}

// strip file path in file name, only keep file name itself
std::string Logger::stripFilePath(const char* file)
{
    std::string str(file);
    for (auto rit = str.rbegin(); rit != str.rend(); rit++)
    {
        if (*rit == '/' || *rit == '\\')
        {
            return std::string(rit.base(), str.end());
        }
    }
    return str;
}

// get level string with color
std::string Logger::getLevelString(LogLevel level)
{
    if (!m_config.coloredOutput) {
        return "";
    }
    
    // Cross-platform color codes
    #ifdef _WIN32
    // Windows doesn't support ANSI color codes by default
    // For simplicity, we'll return empty string on Windows
    return "";
    #else
    // ANSI color codes for Unix-like systems
    switch (level) {
    case Trace:
        return "\033[36m"; // Cyan
    case Debug:
        return "\033[32m"; // Green
    case Info:
        return "\033[37m"; // White
    case Warning:
        return "\033[33m"; // Yellow
    case Error:
        return "\033[31m"; // Red
    case Fatal:
        return "\033[35m"; // Magenta
    default:
        return "";
    }
    #endif
}

// reset color
std::string Logger::resetColor()
{
    if (!m_config.coloredOutput) {
        return "";
    }
    
    #ifdef _WIN32
    return "";
    #else
    return "\033[0m";
    #endif
}

// get timestamp
std::string Logger::getTimestamp()
{
    auto now = std::chrono::system_clock::now();
    auto now_c = std::chrono::system_clock::to_time_t(now);
    auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch()) % 1000;
    
    std::stringstream ss;
    ss << std::put_time(std::localtime(&now_c), "%Y-%m-%d %H:%M:%S.") << std::setw(3) << std::setfill('0') << ms.count();
    return ss.str();
}

// Log file management
void Logger::checkLogFileSize()
{
    if (!m_logFileStream || !m_logFileStream->is_open()) {
        return;
    }
    
    m_logFileStream->seekp(0, std::ios::end);
    size_t size = m_logFileStream->tellp();
    
    if (size >= m_config.maxLogFileSize) {
        rotateLogFiles();
    }
}

void Logger::rotateLogFiles()
{
    if (!m_logFileStream || !m_logFileStream->is_open()) {
        return;
    }
    
    m_logFileStream->close();
    
    // Rotate log files
    for (int i = m_config.maxLogFiles - 1; i > 0; --i) {
        std::stringstream oldNameStream;
        oldNameStream << m_config.logFilePattern.substr(0, m_config.logFilePattern.find("{}")) << (i - 1) << m_config.logFilePattern.substr(m_config.logFilePattern.find("{}") + 2);
        std::string oldName = oldNameStream.str();
        
        std::stringstream newNameStream;
        newNameStream << m_config.logFilePattern.substr(0, m_config.logFilePattern.find("{}")) << i << m_config.logFilePattern.substr(m_config.logFilePattern.find("{}") + 2);
        std::string newName = newNameStream.str();
        
        try {
            std::filesystem::rename(oldName, newName);
        } catch (...) {
            // Ignore errors
        }
    }
    
    // Rename current log file to log_0.txt
    std::stringstream newNameStream;
    newNameStream << m_config.logFilePattern.substr(0, m_config.logFilePattern.find("{}")) << 0 << m_config.logFilePattern.substr(m_config.logFilePattern.find("{}") + 2);
    std::string newName = newNameStream.str();
    try {
        std::filesystem::rename(m_logFilePath, newName);
    } catch (...) {
        // Ignore errors
    }
    
    // Create new log file
    m_logFileStream = std::make_unique<std::ofstream>(m_logFilePath, std::ios::out | std::ios::trunc);
    if (m_logFileStream->is_open()) {
        // Remove old stream and add new one
        std::lock_guard<std::mutex> lock(m_mutex);
        auto it = std::find_if(m_outputStreams.begin(), m_outputStreams.end(),
            [this](const std::reference_wrapper<std::ostream>& ref) {
                return &ref.get() == m_logFileStream.get();
            });
        if (it != m_outputStreams.end()) {
            m_outputStreams.erase(it);
        }
        m_outputStreams.emplace_back(*m_logFileStream);
    }
}

// Log thread function
void Logger::logThreadFunc()
{
    while (m_running) {
        std::unique_lock<std::mutex> lock(m_mutex);
        m_logCondition.wait(lock, [this] { return !m_logQueue.empty() || !m_running; });
        
        if (!m_running && m_logQueue.empty()) {
            break;
        }
        
        if (!m_logQueue.empty()) {
            LogMessage msg = m_logQueue.front();
            m_logQueue.pop();
            lock.unlock();
            
            processLogMessage(msg);
        }
    }
}

// Log functions implementation
void Logger::trace(const std::string& str, const std::source_location& loc) {
    log(Trace, str, loc);
}

void Logger::debug(const std::string& str, const std::source_location& loc) {
    log(Debug, str, loc);
}

void Logger::info(const std::string& str, const std::source_location& loc) {
    log(Info, str, loc);
}

void Logger::warning(const std::string& str, const std::source_location& loc) {
    log(Warning, str, loc);
}

void Logger::error(const std::string& str, const std::source_location& loc) {
    log(Error, str, loc);
}

void Logger::fatal(const std::string& str, const std::source_location& loc) {
    log(Fatal, str, loc);
}

// Log implementation
void Logger::log(LogLevel level, const std::string& str, const std::source_location& loc)
{
    bool shouldLog = false;
    switch (level) {
    case Trace:
        shouldLog = !m_bBlockTrace && m_config.lowestLevel <= Trace;
        break;
    case Debug:
        shouldLog = !m_bBlockDebug && m_config.lowestLevel <= Debug;
        break;
    case Info:
        shouldLog = !m_bBlockInfo && m_config.lowestLevel <= Info;
        break;
    case Warning:
        shouldLog = !m_bBlockWarning && m_config.lowestLevel <= Warning;
        break;
    case Error:
        shouldLog = !m_bBlockError && m_config.lowestLevel <= Error;
        break;
    case Fatal:
        shouldLog = !m_bBlockFatal && m_config.lowestLevel <= Fatal;
        break;
    default:
        return;
    }
    
    if (!shouldLog) {
        return;
    }
    
    if (m_config.asyncLogging) {
        LogMessage msg;
        msg.level = level;
        msg.message = str;
        msg.location = loc;
        msg.timestamp = std::chrono::system_clock::now();
        
        { 
            std::lock_guard<std::mutex> lock(m_mutex);
            m_logQueue.push(msg);
        }
        m_logCondition.notify_one();
    } else {
        LogMessage msg;
        msg.level = level;
        msg.message = str;
        msg.location = loc;
        msg.timestamp = std::chrono::system_clock::now();
        processLogMessage(msg);
    }
}

// Internal log processing
void Logger::processLogMessage(const LogMessage& msg)
{
    std::lock_guard<std::mutex> lock(m_mutex);
    
    std::string levelStr = getLevelString(msg.level);
    std::string reset = resetColor();
    std::string timestamp = getTimestamp();
    
    std::string logStr = std::format("{}[ {: <8} ]{}[ {} ] [ {: >20} : {: >4} : {: <30} ]: {}\n", 
        levelStr, 
        msg.level == Trace ? "Trace" : 
        msg.level == Debug ? "Debug" : 
        msg.level == Info ? "Info" : 
        msg.level == Warning ? "Warning" : 
        msg.level == Error ? "Error" : "Fatal", 
        reset,
        timestamp,
        stripFilePath(msg.location.file_name()), 
        msg.location.line(), 
        msg.location.function_name(), 
        msg.message);
    
    for (auto& os : m_outputStreams) {
        os.get() << logStr;
        os.get().flush();
    }
    
    // Check log file size for rotation
    if (m_logFileStream && m_logFileStream->is_open()) {
        checkLogFileSize();
    }
}

// Global logger functions

Logger& globalLogger()
{
    if (s_pGlobalLogger)
    {
        return *s_pGlobalLogger;
    }
    return defaultLogger();
}

Logger& defaultLogger()
{
    static Logger coutLogger(std::cout);
    return coutLogger;
}

void setGlobalLogger(Logger* pLogger)
{
    s_pGlobalLogger = pLogger;
}

} // namespace tch
