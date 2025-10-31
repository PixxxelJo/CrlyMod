#include <chrono>
#include <ctime>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <mutex>
#include <sstream>
#include <string>
#include <thread>
#include <atomic>
#include <regex>

#ifdef _WIN32
#include <windows.h>
#endif

class Logger {
public:
    enum class Level { Debug = 0, Info, Warn, Error, Off };

    static Logger& instance() {
        static Logger logger;
        return logger;
    }

    void init(Level minLevel = Level::Debug, const std::string& filePath = "", bool colors = true) {
        std::lock_guard<std::mutex> lk(mutex_);
        minLevel_.store(static_cast<int>(minLevel));
        if (!filePath.empty()) {
            file_.open(filePath, std::ios::out | std::ios::app);
        }
        colorsEnabled_ = colors && enableAnsiOnWindows();
    }

    void setLevel(Level L) { minLevel_.store(static_cast<int>(L)); }

    template<typename... Args>
    void log(Level lvl, Args&&... args) {
        if (static_cast<int>(lvl) < minLevel_.load()) return;
        std::ostringstream oss;
        formatHeader(oss, lvl);
        (oss << ... << std::forward<Args>(args));
        const std::string line = oss.str();

        std::lock_guard<std::mutex> lk(mutex_);
        if (colorsEnabled_) std::cout << levelColor(lvl);
        std::cout << line << "\n";
        if (colorsEnabled_) std::cout << colorReset();
        std::cout.flush();

        if (file_.is_open()) {
            file_ << stripAnsi(line) << "\n";
            file_.flush();
        }
    }

    template<typename... Args> void debug(Args&&... a) { log(Level::Debug, std::forward<Args>(a)...); }
    template<typename... Args> void info(Args&&... a)  { log(Level::Info,  std::forward<Args>(a)...); }
    template<typename... Args> void warn(Args&&... a)  { log(Level::Warn,  std::forward<Args>(a)...); }
    template<typename... Args> void error(Args&&... a) { log(Level::Error, std::forward<Args>(a)...); }

private:
    Logger() { minLevel_.store(static_cast<int>(Level::Debug)); colorsEnabled_ = true; }
    ~Logger() { if (file_.is_open()) file_.close(); }

    void formatHeader(std::ostringstream& oss, Level lvl) {
        using namespace std::chrono;
        auto now = system_clock::now();
        auto ms = duration_cast<milliseconds>(now.time_since_epoch()) % 1000;
        std::time_t t = system_clock::to_time_t(now);
        std::tm tm;
#ifdef _WIN32
        localtime_s(&tm, &t);
#else
        localtime_r(&t, &tm);
#endif
        oss << std::put_time(&tm, "%Y-%m-%d %H:%M:%S");
        oss << '.' << std::setfill('0') << std::setw(3) << ms.count();
        auto tid = std::hash<std::thread::id>{}(std::this_thread::get_id());
        oss << " [" << tid << "] ";
        oss << "[" << levelToString(lvl) << "] ";
    }

    const char* levelToString(Level lvl) {
        switch (lvl) {
            case Level::Debug: return "DEBUG";
            case Level::Info:  return "INFO";
            case Level::Warn:  return "WARN";
            case Level::Error: return "ERROR";
            default:           return "UNKNOWN";
        }
    }

    const char* levelColor(Level lvl) {
        switch (lvl) {
            case Level::Debug: return "\x1b[36m";
            case Level::Info:  return "\x1b[32m";
            case Level::Warn:  return "\x1b[33m";
            case Level::Error: return "\x1b[31m";
            default:           return "";
        }
    }

    const char* colorReset() { return "\x1b[0m"; }

    std::string stripAnsi(const std::string& s) {
        static const std::regex re("\x1b\\[[0-9;]*m");
        return std::regex_replace(s, re, "");
    }

    bool enableAnsiOnWindows() {
#ifdef _WIN32
        HANDLE hOut = GetStdHandle(STD_OUTPUT_HANDLE);
        if (hOut == INVALID_HANDLE_VALUE) return false;
        DWORD dwMode = 0;
        if (!GetConsoleMode(hOut, &dwMode)) return false;
        dwMode |= ENABLE_VIRTUAL_TERMINAL_PROCESSING;
        if (!SetConsoleMode(hOut, dwMode)) return false;
        return true;
#else
        return true;
#endif
    }

    std::mutex mutex_;
    std::ofstream file_;
    std::atomic<int> minLevel_{0};
    bool colorsEnabled_{true};
};
