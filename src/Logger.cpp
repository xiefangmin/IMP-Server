#include "Logger.h"
#include <iostream>
#include <iomanip>
#include <ctime>
#include <algorithm>

Logger& Logger::getInstance() {
    static Logger instance;
    return instance;
}

void Logger::initialize(const std::string& level, bool enable_console, 
                       bool enable_file, const std::string& log_file) {
    setLevel(level);
    enable_console_ = enable_console;
    enable_file_ = enable_file;
    log_file_path_ = log_file;
    
    // 初始化文件流
    if (enable_file_) {
        file_stream_ = std::make_unique<std::ofstream>(log_file_path_, std::ios::app);
        if (!file_stream_->is_open()) {
            std::cerr << "❌ 无法打开日志文件: " << log_file_path_ << std::endl;
            enable_file_ = false;
        }
    }
    
    // 启动工作线程
    should_stop_ = false;
    worker_thread_ = std::thread(&Logger::workerThread, this);
    
    info(">>>日志系统初始化完成");
}

void Logger::log(LogLevel level, const std::string& message, 
                 const std::string& file, int line) {
    if (level < current_level_) {
        return;
    }
    
    // 格式化日志消息
    std::stringstream ss;
    ss << "[" << getCurrentTime() << "] "
       << "[" << levelToString(level) << "] ";
    
    if (!file.empty()) {
        // 提取文件名（去掉路径）
        size_t pos = file.find_last_of("/\\");
        std::string filename = (pos == std::string::npos) ? file : file.substr(pos + 1);
        ss << "[" << filename << ":" << line << "] ";
    }
    
    ss << message;
    
    std::string formatted_message = ss.str();
    
    // 同步写入控制台
    if (enable_console_) {
        writeToConsole(formatted_message);
    }
    
    // 异步写入文件
    if (enable_file_) {
        std::lock_guard<std::mutex> lock(queue_mutex_);
        log_queue_.push(formatted_message);
        queue_cv_.notify_one();
    }
}

void Logger::debug(const std::string& message, const std::string& file, int line) {
    log(LogLevel::DEBUG, message, file, line);
}

void Logger::info(const std::string& message, const std::string& file, int line) {
    log(LogLevel::INFO, message, file, line);
}

void Logger::error(const std::string& message, const std::string& file, int line) {
    log(LogLevel::ERROR, message, file, line);
}

void Logger::setLevel(LogLevel level) {
    current_level_ = level;
}

void Logger::setLevel(const std::string& level) {
    std::string upper_level = level;
    std::transform(upper_level.begin(), upper_level.end(), upper_level.begin(), ::toupper);
    
    if (upper_level == "DEBUG") {
        current_level_ = LogLevel::DEBUG;
    } else if (upper_level == "INFO") {
        current_level_ = LogLevel::INFO;
    } else if (upper_level == "ERROR") {
        current_level_ = LogLevel::ERROR;
    } else {
        std::cerr << "⚠️ 未知日志级别: " << level << "，使用默认级别 INFO" << std::endl;
        current_level_ = LogLevel::INFO;
    }
}

void Logger::shutdown() {
    should_stop_ = true;
    queue_cv_.notify_all();
    
    if (worker_thread_.joinable()) {
        worker_thread_.join();
    }
    
    if (file_stream_ && file_stream_->is_open()) {
        file_stream_->close();
    }
    
    info("日志系统已关闭");
}

Logger::~Logger() {
    shutdown();
}

std::string Logger::levelToString(LogLevel level) {
    switch (level) {
        case LogLevel::DEBUG: return "DEBUG";
        case LogLevel::INFO:  return "INFO ";
        case LogLevel::ERROR: return "ERROR";
        default: return "UNKNOWN";
    }
}

std::string Logger::getCurrentTime() {
    auto now = std::chrono::system_clock::now();
    auto time_t = std::chrono::system_clock::to_time_t(now);
    auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(
        now.time_since_epoch()) % 1000;
    
    std::stringstream ss;
    ss << std::put_time(std::localtime(&time_t), "%Y-%m-%d %H:%M:%S");
    ss << "." << std::setfill('0') << std::setw(3) << ms.count();
    
    return ss.str();
}

void Logger::writeToConsole(const std::string& formatted_message) {
    std::lock_guard<std::mutex> lock(console_mutex_);
    std::cout << formatted_message << std::endl;
}

void Logger::writeToFile(const std::string& formatted_message) {
    if (file_stream_ && file_stream_->is_open()) {
        std::lock_guard<std::mutex> lock(file_mutex_);
        *file_stream_ << formatted_message << std::endl;
        file_stream_->flush();
    }
}

void Logger::workerThread() {
    while (!should_stop_) {
        std::unique_lock<std::mutex> lock(queue_mutex_);
        
        // 等待日志消息或停止信号
        queue_cv_.wait(lock, [this] { return !log_queue_.empty() || should_stop_; });
        
        // 处理队列中的所有消息
        while (!log_queue_.empty()) {
            std::string message = log_queue_.front();
            log_queue_.pop();
            lock.unlock();
            
            writeToFile(message);
            
            lock.lock();
        }
    }
}

