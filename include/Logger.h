#ifndef LOGGER_H
#define LOGGER_H

#include <string>
#include <fstream>
#include <mutex>
#include <memory>
#include <queue>
#include <thread>
#include <condition_variable>
#include <atomic>
#include <chrono>
#include <sstream>

enum class LogLevel {
    DEBUG = 0,
    INFO = 1,
    ERROR = 2
};

class Logger {
public:
    // 单例模式
    static Logger& getInstance();
    
    // 初始化日志系统
    void initialize(const std::string& level = "INFO", 
                   bool enable_console = true, 
                   bool enable_file = false, 
                   const std::string& log_file = "server.log");
    
    // 日志输出接口
    void log(LogLevel level, const std::string& message, 
             const std::string& file = "", int line = 0);
    
    // 便捷方法
    void debug(const std::string& message, const std::string& file = "", int line = 0);
    void info(const std::string& message, const std::string& file = "", int line = 0);
    void error(const std::string& message, const std::string& file = "", int line = 0);
    
    
    // 设置日志级别
    void setLevel(LogLevel level);
    void setLevel(const std::string& level);
    
    // 关闭日志系统
    void shutdown();
    
    // 禁用拷贝构造和赋值
    Logger(const Logger&) = delete;
    Logger& operator=(const Logger&) = delete;

private:
    Logger() = default;
    ~Logger();
    
    // 内部方法
    std::string levelToString(LogLevel level);
    std::string getCurrentTime();
    void writeToConsole(const std::string& formatted_message);
    void writeToFile(const std::string& formatted_message);
    void workerThread();
    
    // 成员变量
    LogLevel current_level_;
    bool enable_console_;
    bool enable_file_;
    std::string log_file_path_;
    
    std::unique_ptr<std::ofstream> file_stream_;
    std::mutex console_mutex_;
    std::mutex file_mutex_;
    
    // 异步写入相关
    std::queue<std::string> log_queue_;
    std::mutex queue_mutex_;
    std::condition_variable queue_cv_;
    std::atomic<bool> should_stop_;
    std::thread worker_thread_;
};

// 宏定义，方便使用
#define LOG_DEBUG(msg) Logger::getInstance().debug(msg, __FILE__, __LINE__)
#define LOG_INFO(msg) Logger::getInstance().info(msg, __FILE__, __LINE__)
#define LOG_ERROR(msg) Logger::getInstance().error(msg, __FILE__, __LINE__)

#endif // LOGGER_H
