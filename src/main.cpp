#include "Server.h"
#include "ConfigManager.h"
#include "Logger.h"
#include <iostream>
#include <cstdlib>   // for exit

#ifdef _WIN32
    #include <getopt.h>  // Windows需要单独安装getopt
#else
    #include <unistd.h>  // Linux/Unix系统
#endif

using namespace std;



int main(int argc, char* argv[]) {
    // 加载配置文件
    ConfigManager& config = ConfigManager::getInstance();
    std::string config_path = "config.json";
    
    // 使用getopt解析命令行参数
    int opt;
    while ((opt = getopt(argc, argv, "p:h")) != -1) {
        switch (opt) {
            case 'p':
                config_path = optarg;
                break;
            case 'h':
                std::cout << "用法: " << argv[0] << " [-p <配置文件路径>] [-h]" << std::endl;
                std::cout << "选项:" << std::endl;
                std::cout << "  -p <路径>    指定配置文件路径 (默认: config.json)" << std::endl;
                std::cout << "  -h          显示此帮助信息" << std::endl;
                return 0;
            case '?':
                std::cerr << "使用 -h 查看帮助信息" << std::endl;
                return 1;
            default:
                return 1;
        }
    }
    
    if (!config.loadConfig(config_path)) {
        std::cerr << "❌ 配置文件加载失败，使用默认配置" << std::endl;
    }
    
    // 初始化日志系统
    Logger& logger = Logger::getInstance();
    logger.initialize(
        config.getLogLevel(),
        config.isConsoleLogEnabled(),
        config.isFileLogEnabled(),
        config.getLogFile()
    );
    
    LOG_INFO("服务器启动中...");
    
    // 从配置文件获取端口列表和ip地址
    std::vector<int> ports = config.getServerPorts();
    // const char * addr= config.getServerIP().data(); 临时对象的指针,内容被释放，悬空指针
    std::string addr = config.getServerIP();  // 保存完整的string对象
    // std::cout<<"this->_addr: "<<addr<<endl;// 输出 this->_addr: 1
    // printf("%s\n", addr);// 输出 this->_addr: 192.168.25.130

    std::string ports_str = "服务器IP:"+ addr +", 将监听以下端口: ";
    for (size_t i = 0; i < ports.size(); ++i) {
        ports_str += std::to_string(ports[i]);
        if (i < ports.size() - 1) ports_str += ", ";
    }
    LOG_INFO(ports_str);

    // 从配置文件获取线程池大小
    unsigned int num_threads = config.getThreadPoolSize();
    
    // 如果配置文件中的线程数为0，则使用硬件检测
    if (num_threads == 0) {
        num_threads = std::thread::hardware_concurrency();
        if (num_threads == 0) {
            num_threads = 4; // 如果无法检测，则默认为4
        }
 
        num_threads = std::max(num_threads * 2, 4u);  // 最少4个线程
    }
    
    LOG_INFO("系统检测到 " + std::to_string(std::thread::hardware_concurrency()) + " 个CPU核心");
    LOG_INFO("配置线程池大小: " + std::to_string(num_threads) + " 个线程");

    try {
        // 创建并启动服务器
        Server server(addr.data(),ports, num_threads);

        
        LOG_INFO("服务器正在 " + std::to_string(ports.size()) + " 个端口上启动，使用 " + std::to_string(num_threads) + " 个工作线程...");
        
        std::string server_ip = config.getServerIP();
        LOG_INFO("请在浏览器中打开以下任一地址:");
        for (int port : ports) {
            LOG_INFO("  http://" + server_ip + ":" + std::to_string(port));
        }
        
        server.run();
    } catch (const std::runtime_error& e) {
        LOG_ERROR("服务器启动失败: " + std::string(e.what()));
        return 1;
    }

    return 0;
}
