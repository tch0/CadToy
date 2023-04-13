#pragma once
#include <iostream>
#include <cstdlib>
#include <string>
#include <source_location>
#include <format>

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

    Logger(std::ostream& os = std::cout);
    ~Logger();
    // set the lowest output log level, default to Info.
    void setLowestOutputLevel(LogLevel level);
    // block specific log level to output, affect higher levels than output log level, default to none.
    void blockLevel(LogLevel level);
    
    // log functions
    void trace(const std::string& str, const std::source_location& loc = std::source_location::current());
    void debug(const std::string& str, const std::source_location& loc = std::source_location::current());
    void info(const std::string& str, const std::source_location& loc = std::source_location::current());
    void warning(const std::string& str, const std::source_location& loc = std::source_location::current());
    void error(const std::string& str, const std::source_location& loc = std::source_location::current());
    void fatal(const std::string& str, const std::source_location& loc = std::source_location::current());
private:
    std::ostream& m_Out;
    bool m_bBlockTrace = false;
    bool m_bBlockDebug = false;
    bool m_bBlockInfo = false;
    bool m_bBlockWarning = false;
    bool m_bBlockError = false;
    bool m_bBlockFatal = false;
    LogLevel m_LowestLevel = Info;

    // help function
    // strip file path in file name, only keep file name itself
    static std::string stripFilePath(const char* file);
};

// a global logger to use
inline Logger* s_pGlobalLogger = nullptr;
Logger& globalLogger();
Logger& defaultLogger();
void setGlobalLogger(Logger* pLogger);

// assertion
inline void tchAssert(bool pred, const std::string& info = "", const std::source_location& loc = std::source_location::current())
{
    if (!pred)
    {
        globalLogger().fatal(std::format("Assertion failed: {}!", info), loc);
        std::exit(-1);
    }
}