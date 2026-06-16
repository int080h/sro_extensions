#pragma once
#include <string>
#include <vector>
#include <fstream>
#include <mutex>

enum class LogLevel {
    Info,
    Warning,
    Error
};

struct LogEntry {
    LogLevel level = LogLevel::Info;
    std::string message;
};

class Logger {
public:
    static Logger& Instance();

    void Info(const std::string& msg);
    void Warning(const std::string& msg);
    void Error(const std::string& msg);

    const std::vector<LogEntry>& Entries() const { return m_entries; }
    void Clear();
    std::string ExportText() const;

    void InstallCrashHandlers();

private:
    Logger();
    void Log(LogLevel level, const std::string& msg);

    std::vector<LogEntry> m_entries;
    std::ofstream m_file;
    std::mutex m_mutex;
};

inline void LogMsg(const std::string& msg) { Logger::Instance().Info(msg); }
inline void LogMsgW(const std::wstring& msg) { Logger::Instance().Info(std::string(msg.begin(), msg.end())); }
