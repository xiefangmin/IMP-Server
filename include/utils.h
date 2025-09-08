#ifndef UTILS_H
#define UTILS_H

#include <string>
#include <fcntl.h>
#include <unistd.h>
#include <fstream>
#include <sstream>
#include <stdexcept>
#include <chrono>
#include <iomanip>
#include <ctime>
#include <vector>
#include <cstring>

#ifdef OPENSSL_DISABLED
// OpenSSL不可用时的备用实现
#else
#include <openssl/md5.h>
#endif



/**
 * 设置文件描述符为非阻塞模式
 * @param fd 文件描述符
 * @return 成功返回0，失败返回-1
 */
inline int set_non_blocking(int fd) {
    int flags = fcntl(fd, F_GETFL, 0);
    if (flags == -1) {
        return -1;
    }
    return fcntl(fd, F_SETFL, flags | O_NONBLOCK);
}

/**
 * 加载文件内容到字符串
 * @param filepath 文件路径
 * @return 文件内容字符串，如果文件不存在或读取失败返回空字符串
 */
inline std::string load_file(const std::string& filepath) {
    std::ifstream file(filepath);
    if (!file.is_open()) {
        return "";
    }
    
    std::stringstream buffer;
    buffer << file.rdbuf();
    return buffer.str();
}

/**
 * 检查文件是否存在
 * @param filepath 文件路径
 * @return 文件存在返回true，否则返回false
 */
inline bool file_exists(const std::string& filepath) {
    return access(filepath.c_str(), F_OK) == 0;
}

/**
 * 获取文件大小
 * @param filepath 文件路径
 * @return 文件大小（字节），文件不存在返回-1
 */
inline long get_file_size(const std::string& filepath) {
    std::ifstream file(filepath, std::ios::binary | std::ios::ate);
    if (!file.is_open()) {
        return -1;
    }
    return file.tellg();
}

/**
 * 获取文件的MIME类型
 * @param filepath 文件路径
 * @return MIME类型字符串
 */
inline std::string get_mime_type(const std::string& filepath) {
    size_t dot_pos = filepath.find_last_of('.');
    if (dot_pos == std::string::npos) {
        return "text/plain";
    }
    
    std::string extension = filepath.substr(dot_pos + 1);
    
    if (extension == "html" || extension == "htm") {
        return "text/html";
    } else if (extension == "css") {
        return "text/css";
    } else if (extension == "js") {
        return "application/javascript";
    } else if (extension == "jpg" || extension == "jpeg") {
        return "image/jpeg";
    } else if (extension == "png") {
        return "image/png";
    } else if (extension == "gif") {
        return "image/gif";
    } else if (extension == "ico") {
        return "image/x-icon";
    } else if (extension == "json") {
        return "application/json";
    } else {
        return "text/plain";
    }
}

/**
 * URL解码
 * @param encoded 编码后的字符串
 * @return 解码后的字符串
 */
inline std::string url_decode(const std::string& encoded) {
    std::string result;
    for (size_t i = 0; i < encoded.length(); ++i) {
        if (encoded[i] == '%' && i + 2 < encoded.length()) {
            int value;
            std::istringstream iss(encoded.substr(i + 1, 2));
            iss >> std::hex >> value;
            result += static_cast<char>(value);
            i += 2;
        } else if (encoded[i] == '+') {
            result += ' ';
        } else {
            result += encoded[i];
        }
    }
    return result;
}

/**
 * 安全的字符串到整数转换
 * @param str 字符串
 * @param default_value 默认值
 * @return 转换后的整数
 */
inline int safe_stoi(const std::string& str, int default_value = 0) {
    try {
        return std::stoi(str);
    } catch (const std::exception&) {
        return default_value;
    }
}

/**
 * 根据图片数据判断图片后缀
 * @param image_data 图片数据
 * @return 图片后缀（如"jpg", "png", "gif", "bmp", "webp"），无法识别返回"unknown"
 */
inline std::string get_image_extension(const std::vector<char>& image_data) {
    if (image_data.size() < 8) {
        return "unknown";
    }
    
    // 检查JPEG格式 (FF D8 FF)
    if (image_data.size() >= 3 && 
        static_cast<unsigned char>(image_data[0]) == 0xFF && 
        static_cast<unsigned char>(image_data[1]) == 0xD8 && 
        static_cast<unsigned char>(image_data[2]) == 0xFF) {
        return "jpg";
    }
    
    // 检查PNG格式 (89 50 4E 47 0D 0A 1A 0A)
    if (image_data.size() >= 8 && 
        static_cast<unsigned char>(image_data[0]) == 0x89 && 
        static_cast<unsigned char>(image_data[1]) == 0x50 && 
        static_cast<unsigned char>(image_data[2]) == 0x4E && 
        static_cast<unsigned char>(image_data[3]) == 0x47 && 
        static_cast<unsigned char>(image_data[4]) == 0x0D && 
        static_cast<unsigned char>(image_data[5]) == 0x0A && 
        static_cast<unsigned char>(image_data[6]) == 0x1A && 
        static_cast<unsigned char>(image_data[7]) == 0x0A) {
        return "png";
    }
    
    // 检查GIF格式 (47 49 46 38 37 61 或 47 49 46 38 39 61)
    if (image_data.size() >= 6 && 
        static_cast<unsigned char>(image_data[0]) == 0x47 && 
        static_cast<unsigned char>(image_data[1]) == 0x49 && 
        static_cast<unsigned char>(image_data[2]) == 0x46 && 
        static_cast<unsigned char>(image_data[3]) == 0x38 && 
        (static_cast<unsigned char>(image_data[4]) == 0x37 || 
         static_cast<unsigned char>(image_data[4]) == 0x39) && 
        static_cast<unsigned char>(image_data[5]) == 0x61) {
        return "gif";
    }
    
    // 检查BMP格式 (42 4D)
    if (image_data.size() >= 2 && 
        static_cast<unsigned char>(image_data[0]) == 0x42 && 
        static_cast<unsigned char>(image_data[1]) == 0x4D) {
        return "bmp";
    }
    
    // 检查WebP格式 (52 49 46 46 ... 57 45 42 50)
    if (image_data.size() >= 12 && 
        static_cast<unsigned char>(image_data[0]) == 0x52 && 
        static_cast<unsigned char>(image_data[1]) == 0x49 && 
        static_cast<unsigned char>(image_data[2]) == 0x46 && 
        static_cast<unsigned char>(image_data[3]) == 0x46 && 
        static_cast<unsigned char>(image_data[8]) == 0x57 && 
        static_cast<unsigned char>(image_data[9]) == 0x45 && 
        static_cast<unsigned char>(image_data[10]) == 0x42 && 
        static_cast<unsigned char>(image_data[11]) == 0x50) {
        return "webp";
    }
    
    // 检查TIFF格式 (49 49 2A 00 或 4D 4D 00 2A)
    if (image_data.size() >= 4 && 
        ((static_cast<unsigned char>(image_data[0]) == 0x49 && 
          static_cast<unsigned char>(image_data[1]) == 0x49 && 
          static_cast<unsigned char>(image_data[2]) == 0x2A && 
          static_cast<unsigned char>(image_data[3]) == 0x00) ||
         (static_cast<unsigned char>(image_data[0]) == 0x4D && 
          static_cast<unsigned char>(image_data[1]) == 0x4D && 
          static_cast<unsigned char>(image_data[2]) == 0x00 && 
          static_cast<unsigned char>(image_data[3]) == 0x2A))) {
        return "tiff";
    }
    
    return "unknown";
}

/**
 * 根据图片数据获取MIME类型
 * @param image_data 图片数据
 * @return MIME类型字符串
 */
inline std::string get_image_mime_type(const std::vector<char>& image_data) {
    std::string extension = get_image_extension(image_data);
    
    if (extension == "jpg") {
        return "image/jpeg";
    } else if (extension == "png") {
        return "image/png";
    } else if (extension == "gif") {
        return "image/gif";
    } else if (extension == "bmp") {
        return "image/bmp";
    } else if (extension == "webp") {
        return "image/webp";
    } else if (extension == "tiff") {
        return "image/tiff";
    } else {
        return "application/octet-stream";
    }
}

/**
 * 保存图片数据到根目录
 * @param image_data 图片数据
 * @return 保存的文件名，失败返回空字符串
 */
inline std::string save_image(const std::vector<char>& image_data) {
    if (image_data.empty()) {
        return "";
    }
    
    // 生成唯一的文件名
    auto now = std::chrono::system_clock::now();
    auto time_t = std::chrono::system_clock::to_time_t(now);
    auto milliseconds = std::chrono::duration_cast<std::chrono::milliseconds>(
        now.time_since_epoch()) % 1000;
    
    std::stringstream ss;
    ss << "uploaded_image_" << std::put_time(std::localtime(&time_t), "%Y%m%d_%H%M%S");
    ss << "_" << std::setfill('0') << std::setw(3) << milliseconds.count();
    
    // 根据图片数据判断后缀
    std::string extension = get_image_extension(image_data);
    if (extension != "unknown") {
        ss << "." << extension;
    } else {
        ss << ".jpg"; // 默认后缀
    }
    
    std::string filename = ss.str();
    
    // 保存文件到根目录
    std::ofstream file(filename, std::ios::binary);
    if (!file.is_open()) {
        return "";
    }
    
    file.write(image_data.data(), image_data.size());
    file.close();
    
    return filename;
}

/**
 * 计算MD5值
 * @param image_data 图片数据
 * @return MD5哈希值的十六进制字符串
 */
inline std::string calculate_md5(const std::vector<char>& image_data) {
    if (image_data.empty()) {
        return "";
    }
    
#ifdef OPENSSL_DISABLED
    // OpenSSL不可用时的简单哈希实现（不是真正的MD5，但可以避免编译错误）
    std::stringstream ss;
    ss << std::hex << std::setfill('0');
    
    uint32_t hash = 0;
    for (char c : image_data) {
        hash = ((hash << 5) + hash) + static_cast<unsigned char>(c);
    }
    
    ss << std::setw(8) << hash;
    return ss.str();
#else
    unsigned char digest[MD5_DIGEST_LENGTH];
    MD5_CTX ctx;
    MD5_Init(&ctx);
    MD5_Update(&ctx, image_data.data(), image_data.size());
    MD5_Final(digest, &ctx);
    
    // 转换为十六进制字符串
    std::stringstream ss;
    ss << std::hex << std::setfill('0');
    for (int i = 0; i < MD5_DIGEST_LENGTH; i++) {
        ss << std::setw(2) << static_cast<int>(digest[i]);
    }
    
    return ss.str();
#endif
}

// } // namespace ImageServer

#endif // UTILS_H
