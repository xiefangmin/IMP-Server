#include "Logger.h"
#include <iostream>
#include <thread>
#include <chrono>

int main() {
    std::cout << "=== Logger基本功能测试 ===" << std::endl;
    
    // 获取Logger实例
    Logger& logger = Logger::getInstance();
    
    // 初始化日志系统
    logger.initialize("DEBUG", false, true, "test.log");
    
    // 测试基本日志级别
    std::cout << "\n1. 基本日志级别测试:" << std::endl;
    LOG_DEBUG("调试信息 - 用于开发调试");
    LOG_INFO("信息日志 - 正常运行信息");
    LOG_ERROR("错误日志 - 错误和异常");
    
    // 测试日志级别过滤
    std::cout << "\n2. 日志级别过滤测试:" << std::endl;
    logger.setLevel("INFO");
    LOG_DEBUG("这条DEBUG信息不应该显示");
    LOG_INFO("这条INFO信息应该显示");
    LOG_ERROR("这条ERROR信息应该显示");
    
    // 测试多线程
    std::cout << "\n3. 多线程测试:" << std::endl;
    std::thread t1([]() {
        for (int i = 0; i < 3; ++i) {
            LOG_INFO("线程1: 消息 " + std::to_string(i));
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
        }
    });
    
    std::thread t2([]() {
        for (int i = 0; i < 3; ++i) {
            LOG_INFO("线程2: 消息 " + std::to_string(i));
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
        }
    });
    
    t1.join();
    t2.join();
    
    // 测试文件输出
    std::cout << "\n4. 文件输出测试:" << std::endl;
    LOG_INFO("这条信息会同时输出到控制台和test.log文件");
    
    // 等待文件写入
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    
    // 关闭日志系统
    logger.shutdown();
    
    std::cout << "\n=== 测试完成 ===" << std::endl;
    std::cout << "请检查 test.log 文件查看文件输出结果" << std::endl;
    
    return 0;
}
