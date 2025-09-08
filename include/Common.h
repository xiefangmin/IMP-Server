#pragma once

#include <cstdint>
#include <string>
#include <memory>
#include <functional>

namespace ImageServerDEF {

// 常量定义
constexpr int MAX_EVENTS = 1024;
constexpr int BUFFER_SIZE = 8192;
constexpr int THREAD_POOL_SIZE = 4;
constexpr int DEFAULT_PORT = 8080;
constexpr int BACKLOG = 128;

// 文件上传限制
constexpr size_t MAX_FILE_SIZE = 50 * 1024 * 1024;  // 50MB
constexpr size_t MAX_IMAGE_SIZE = 20 * 1024 * 1024; // 20MB (图像专用)

// 文件保存路径配置
constexpr const char* UPLOAD_DIR = "uploads";  // 上传文件保存目录
constexpr const char* PROCESSED_DIR = "processed";  // 处理后文件保存目录

// 事件类型
enum class EventType {
    READ,
    WRITE,
    ERROR,
    CLOSE
};

// 连接状态
enum class ConnectionState {
    CONNECTING,
    CONNECTED,
    READING,
    WRITING,
    CLOSING,
    CLOSED
};

// 前向声明
class Connection;
class EpollReactor;
class ThreadPool;
class ImageProcessor;

// 智能指针类型别名
using ConnectionPtr = std::shared_ptr<Connection>;
using ReactorPtr = std::unique_ptr<EpollReactor>;
using ThreadPoolPtr = std::unique_ptr<ThreadPool>;
using ImageProcessorPtr = std::unique_ptr<ImageProcessor>;

// 回调函数类型
using EventCallback = std::function<void(ConnectionPtr, EventType)>;
using TaskCallback = std::function<void()>;

} // namespace ImageServer

