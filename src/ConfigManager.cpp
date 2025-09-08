#include "ConfigManager.h"
#include <fstream>
#include <iostream>
#include <stdexcept>

bool ConfigManager::loadConfig(const std::string& config_path) {
    try {
        std::ifstream file(config_path);
        if (!file.is_open()) {
            std::cerr << "❌ 无法打开配置文件: " << config_path << std::endl;
            return false;
        }
        
        file >> config_;
        file.close();
        
        config_loaded_ = true;
        std::cout << "✅ 配置文件加载成功: " << config_path << std::endl;
        return true;
        
    } catch (const std::exception& e) {
        std::cerr << "❌ 配置文件解析失败: " << e.what() << std::endl;
        config_loaded_ = false;
        return false;
    }
}

bool ConfigManager::reloadConfig(const std::string& config_path) {
    config_loaded_ = false;
    return loadConfig(config_path);
}

// 服务器配置方法
std::vector<int> ConfigManager::getServerPorts() const {
    if (!config_loaded_) return {8080, 8081, 8082, 8083, 8084};
    
    try {
        std::vector<int> ports;
        for (const auto& port : config_["server"]["ports"]) {
            ports.push_back(port.get<int>());
        }
        return ports;
    } catch (const std::exception& e) {
        std::cerr << "⚠️ 读取端口配置失败，使用默认端口: " << e.what() << std::endl;
        return {8080, 8081, 8082, 8083, 8084};
    }
}

int ConfigManager::getThreadPoolSize() const {
    if (!config_loaded_) return 16;
    
    try {
        return config_["server"]["thread_pool_size"].get<int>();
    } catch (const std::exception& e) {
        std::cerr << "⚠️ 读取线程池大小配置失败，使用默认值: " << e.what() << std::endl;
        return 16;
    }
}

int ConfigManager::getMaxConnections() const {
    if (!config_loaded_) return 1000;
    
    try {
        return config_["server"]["max_connections"].get<int>();
    } catch (const std::exception& e) {
        std::cerr << "⚠️ 读取最大连接数配置失败，使用默认值: " << e.what() << std::endl;
        return 1000;
    }
}

std::string ConfigManager::getServerIP() const {
    if (!config_loaded_) return "127.0.0.1";
    
    try {
        return config_["server"]["ip_address"].get<std::string>();
    } catch (const std::exception& e) {
        std::cerr << "⚠️ 读取服务器IP配置失败，使用默认值: " << e.what() << std::endl;
        return "127.0.0.1";
    }
}

// YOLO配置方法
std::string ConfigManager::getYOLOModelPath() const {
    if (!config_loaded_) return "models/yolov8n.onnx";
    
    try {
        return config_["yolo"]["model_path"].get<std::string>();
    } catch (const std::exception& e) {
        std::cerr << "⚠️ 读取YOLO模型路径配置失败，使用默认值: " << e.what() << std::endl;
        return "models/yolov8n.onnx";
    }
}

std::string ConfigManager::getYOLOSegmentationModelPath() const {
    if (!config_loaded_) return "models/yolov8x-seg.onnx";
    
    try {
        return config_["yolo"]["segmentation_model_path"].get<std::string>();
    } catch (const std::exception& e) {
        std::cerr << "⚠️ 读取YOLO分割模型路径配置失败，使用默认值: " << e.what() << std::endl;
        return "models/yolov8x-seg.onnx";
    }
}

float ConfigManager::getYOLOConfidenceThreshold() const {
    if (!config_loaded_) return 0.1f;
    
    try {
        return config_["yolo"]["confidence_threshold"].get<float>();
    } catch (const std::exception& e) {
        std::cerr << "⚠️ 读取置信度阈值配置失败，使用默认值: " << e.what() << std::endl;
        return 0.1f;
    }
}

float ConfigManager::getYOLONMSThreshold() const {
    if (!config_loaded_) return 0.5f;
    
    try {
        return config_["yolo"]["nms_threshold"].get<float>();
    } catch (const std::exception& e) {
        std::cerr << "⚠️ 读取NMS阈值配置失败，使用默认值: " << e.what() << std::endl;
        return 0.5f;
    }
}

int ConfigManager::getYOLOInputWidth() const {
    if (!config_loaded_) return 640;
    
    try {
        return config_["yolo"]["input_width"].get<int>();
    } catch (const std::exception& e) {
        std::cerr << "⚠️ 读取输入宽度配置失败，使用默认值: " << e.what() << std::endl;
        return 640;
    }
}

int ConfigManager::getYOLOInputHeight() const {
    if (!config_loaded_) return 640;
    
    try {
        return config_["yolo"]["input_height"].get<int>();
    } catch (const std::exception& e) {
        std::cerr << "⚠️ 读取输入高度配置失败，使用默认值: " << e.what() << std::endl;
        return 640;
    }
}

std::string ConfigManager::getYOLOBackend() const {
    if (!config_loaded_) return "OPENCV";
    
    try {
        return config_["yolo"]["backend"].get<std::string>();
    } catch (const std::exception& e) {
        std::cerr << "⚠️ 读取后端配置失败，使用默认值: " << e.what() << std::endl;
        return "OPENCV";
    }
}

std::string ConfigManager::getYOLOTarget() const {
    if (!config_loaded_) return "CPU";
    
    try {
        return config_["yolo"]["target"].get<std::string>();
    } catch (const std::exception& e) {
        std::cerr << "⚠️ 读取目标设备配置失败，使用默认值: " << e.what() << std::endl;
        return "CPU";
    }
}

// 图像处理配置方法
int ConfigManager::getMaxImageSize() const {
    if (!config_loaded_) return 10485760; // 10MB
    
    try {
        return config_["image_processing"]["max_image_size"].get<int>();
    } catch (const std::exception& e) {
        std::cerr << "⚠️ 读取最大图像大小配置失败，使用默认值: " << e.what() << std::endl;
        return 10485760;
    }
}

std::vector<std::string> ConfigManager::getSupportedFormats() const {
    if (!config_loaded_) return {"jpg", "jpeg", "png", "bmp", "tiff"};
    
    try {
        std::vector<std::string> formats;
        for (const auto& format : config_["image_processing"]["supported_formats"]) {
            formats.push_back(format.get<std::string>());
        }
        return formats;
    } catch (const std::exception& e) {
        std::cerr << "⚠️ 读取支持格式配置失败，使用默认值: " << e.what() << std::endl;
        return {"jpg", "jpeg", "png", "bmp", "tiff"};
    }
}

int ConfigManager::getOutputQuality() const {
    if (!config_loaded_) return 95;
    
    try {
        return config_["image_processing"]["output_quality"].get<int>();
    } catch (const std::exception& e) {
        std::cerr << "⚠️ 读取输出质量配置失败，使用默认值: " << e.what() << std::endl;
        return 95;
    }
}

// 日志配置方法
std::string ConfigManager::getLogLevel() const {
    if (!config_loaded_) return "INFO";
    
    try {
        return config_["logging"]["level"].get<std::string>();
    } catch (const std::exception& e) {
        std::cerr << "⚠️ 读取日志级别配置失败，使用默认值: " << e.what() << std::endl;
        return "INFO";
    }
}

bool ConfigManager::isConsoleLogEnabled() const {
    if (!config_loaded_) return true;
    
    try {
        return config_["logging"]["enable_console"].get<bool>();
    } catch (const std::exception& e) {
        std::cerr << "⚠️ 读取控制台日志配置失败，使用默认值: " << e.what() << std::endl;
        return true;
    }
}

bool ConfigManager::isFileLogEnabled() const {
    if (!config_loaded_) return false;
    
    try {
        return config_["logging"]["enable_file"].get<bool>();
    } catch (const std::exception& e) {
        std::cerr << "⚠️ 读取文件日志配置失败，使用默认值: " << e.what() << std::endl;
        return false;
    }
}

std::string ConfigManager::getLogFile() const {
    if (!config_loaded_) return "server.log";
    
    try {
        return config_["logging"]["log_file"].get<std::string>();
    } catch (const std::exception& e) {
        std::cerr << "⚠️ 读取日志文件配置失败，使用默认值: " << e.what() << std::endl;
        return "server.log";
    }
}
