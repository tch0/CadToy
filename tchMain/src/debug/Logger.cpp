#include <algorithm>
#include <iomanip>
#include <format>

#include "debug/Logger.h"


Logger::Logger(std::ostream& os)
    : m_Out(os)
{
}

Logger::~Logger()
{
    if (this == s_pGlobalLogger)
    {
        s_pGlobalLogger = nullptr; // reset global logger to nullptr if current logger is global logger.
    }
}

// set the lowest output log level, default to Info.
void Logger::setLowestOutputLevel(LogLevel level)
{
    m_LowestLevel = level;
}

// block specific log level to output, affect higher levels than output log level, default to none.
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

// log functions
void Logger::trace(const std::string& str, const std::source_location& loc)
{
    if (!m_bBlockTrace && m_LowestLevel <= Trace)
    {
        m_Out << std::format("[ Trace   ][ {: >20} : {: >4} : {: <30} ]: {}\n", stripFilePath(loc.file_name()), loc.line(), loc.function_name(), str);
    }
}
void Logger::debug(const std::string& str, const std::source_location& loc)
{
    if (!m_bBlockDebug && m_LowestLevel <= Debug)
    {
        m_Out << std::format("[ Debug   ][ {: >20} : {: >4} : {: <30} ]: {}\n", stripFilePath(loc.file_name()), loc.line(), loc.function_name(), str);
    }
}
void Logger::info(const std::string& str, const std::source_location& loc)
{
    if (!m_bBlockInfo && m_LowestLevel <= Info)
    {
        m_Out << std::format("[ Info    ][ {: >20} : {: >4} : {: <30} ]: {}\n", stripFilePath(loc.file_name()), loc.line(), loc.function_name(), str);
    }
}
void Logger::warning(const std::string& str, const std::source_location& loc)
{
    if (!m_bBlockWarning && m_LowestLevel <= Warning)
    {
        m_Out << std::format("[ Warning ][ {: >20} : {: >4} : {: <30} ]: {}\n", stripFilePath(loc.file_name()), loc.line(), loc.function_name(), str);
    }
}
void Logger::error(const std::string& str, const std::source_location& loc)
{
    if (!m_bBlockError && m_LowestLevel <= Error)
    {
        m_Out << std::format("[ Error   ][ {: >20} : {: >4} : {: <30} ]: {}\n", stripFilePath(loc.file_name()), loc.line(), loc.function_name(), str);
    }
}
void Logger::fatal(const std::string& str, const std::source_location& loc)
{
    if (!m_bBlockFatal && m_LowestLevel <= Fatal)
    {
        m_Out << std::format("[ Fatal   ][ {: >20} : {: >4} : {: <30} ]: {}\n", stripFilePath(loc.file_name()), loc.line(), loc.function_name(), str);
    }
}

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

// a global logger to use
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