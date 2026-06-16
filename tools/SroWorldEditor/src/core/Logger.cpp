#include "core/Logger.h"
#include <windows.h>
#include <ctime>
#include <sstream>

Logger& Logger::Instance() {
    static Logger instance;
    return instance;
}

Logger::Logger() {
    m_file.open("sroworldeditor.log", std::ios::out | std::ios::app);
}

void Logger::Log(LogLevel level, const std::string& msg) {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_entries.push_back({level, msg});
    if (m_entries.size() > 5000) {
        m_entries.erase(m_entries.begin(), m_entries.begin() + 1000);
    }
    if (m_file.is_open()) {
        const char* tag = "INFO";
        if (level == LogLevel::Warning) tag = "WARN";
        if (level == LogLevel::Error) tag = "ERROR";
        m_file << "[" << tag << "] " << msg << std::endl;
    }
}

void Logger::Info(const std::string& msg) { Log(LogLevel::Info, msg); }
void Logger::Warning(const std::string& msg) { Log(LogLevel::Warning, msg); }
void Logger::Error(const std::string& msg) { Log(LogLevel::Error, msg); }

void Logger::Clear() {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_entries.clear();
}

std::string Logger::ExportText() const {
    std::ostringstream oss;
    for (const auto& e : m_entries) {
        switch (e.level) {
        case LogLevel::Info: oss << "[INFO] "; break;
        case LogLevel::Warning: oss << "[WARN] "; break;
        case LogLevel::Error: oss << "[ERROR] "; break;
        }
        oss << e.message << '\n';
    }
    return oss.str();
}

static LONG WINAPI CrashHandler(EXCEPTION_POINTERS*) {
    Logger::Instance().Error("Unhandled exception — see sroworldeditor.log");
    return EXCEPTION_EXECUTE_HANDLER;
}

void Logger::InstallCrashHandlers() {
    SetUnhandledExceptionFilter(CrashHandler);
}
