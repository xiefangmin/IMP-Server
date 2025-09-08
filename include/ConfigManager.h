#ifndef CONFIG_MANAGER_H
#define CONFIG_MANAGER_H

#include <string>
#include <vector>
#include <memory>
#include <nlohmann/json.hpp>

/**
 * @brief 配置管理器类
 * 
 * 负责读取和管理JSON配置文件中的所有参数
 */
class ConfigManager {
private:
    nlohmann::json config_;
    bool config_loaded_;
    
    ConfigManager() : config_loaded_(false) {}
    
public:
    // 单例模式
    static ConfigManager& getInstance() {
        static ConfigManager instance;
        return instance;
    }
    
    // 禁用拷贝构造和赋值
    ConfigManager(const ConfigManager&) = delete;
    ConfigManager& operator=(const ConfigManager&) = delete;
    
    /**
     * @brief 加载配置文件
     * @param config_path 配置文件路径
     * @return 是否加载成功
     */
    bool loadConfig(const std::string& config_path = "config.json");
    
    /**
     * @brief 检查配置是否已加载
     * @return 配置是否已加载
     */
    bool isConfigLoaded() const { return config_loaded_; }
    
    // 服务器配置
    std::vector<int> getServerPorts() const;
    int getThreadPoolSize() const;
    int getMaxConnections() const;
    std::string getServerIP() const;
    
    // YOLO配置
    std::string getYOLOModelPath() const;
    std::string getYOLOSegmentationModelPath() const;
    float getYOLOConfidenceThreshold() const;
    float getYOLONMSThreshold() const;
    int getYOLOInputWidth() const;
    int getYOLOInputHeight() const;
    std::string getYOLOBackend() const;
    std::string getYOLOTarget() const;
    
    // 图像处理配置
    int getMaxImageSize() const;
    std::vector<std::string> getSupportedFormats() const;
    int getOutputQuality() const;
    
    // 日志配置
    std::string getLogLevel() const;
    bool isConsoleLogEnabled() const;
    bool isFileLogEnabled() const;
    std::string getLogFile() const;
    
    /**
     * @brief 获取原始JSON配置对象
     * @return JSON配置对象
     */
    const nlohmann::json& getRawConfig() const { return config_; }
    
    /**
     * @brief 重新加载配置文件
     * @param config_path 配置文件路径
     * @return 是否重新加载成功
     */
    bool reloadConfig(const std::string& config_path = "config.json");
};

#endif // CONFIG_MANAGER_H
