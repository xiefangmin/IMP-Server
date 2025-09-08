#include "ImageProcessor.h"
#include "ConfigManager.h"
#include <opencv2/opencv.hpp>
#include <fstream>
#include <iostream>

// 静态成员变量定义
// YOLOv8Detector ImageProcessor::yolo_detector = YOLOv8Detector();
// 静态成员变量定义 - 使用指针进行延迟初始化
YOLOv8Detector* ImageProcessor::yolo_detector = nullptr;
// 获取检测器实例，确保延迟初始化
YOLOv8Detector* ImageProcessor::getDetector() {
    if (yolo_detector == nullptr) {
        yolo_detector = new YOLOv8Detector();
        std::cout << "[ImageProcessor] 创建新的YOLOv8Detector实例" << std::endl;
    }
    return yolo_detector;
}


bool ImageProcessor::process(const std::vector<char>& input_data,
                           std::vector<char>& output_data,
                           const std::string& filter_type,
                           std::string& output_content_type,
                           const std::string& blur_intensity,
                           const std::string& sharpen_intensity) {
    if (input_data.empty()) {
        return false;
    }


    // 检查是否是YOLO目标检测请求
    if (filter_type == "yolo_detect") {
        return processWithYOLO(input_data, output_data, output_content_type);
    }    
    // 检查是否是YOLO图像分割请求
    if (filter_type == "yolo_segment") {
        return processWithYOLOSegmentation(input_data, output_data, output_content_type);
    }
    
    // 检查是否是YOLO目标检测+分割请求
    if (filter_type == "yolo_segment_with_boxes") {
        return processWithYOLOSegmentationWithBoxes(input_data, output_data, output_content_type);
    }

    // 1. 解码图像数据
    cv::Mat image = cv::imdecode(cv::Mat(input_data), cv::IMREAD_COLOR);
    if (image.empty()) {
        return false;
    }

    // 2. 应用滤镜
    cv::Mat processed_image;
    if (filter_type == "grayscale") {
        cv::cvtColor(image, processed_image, cv::COLOR_BGR2GRAY);
    } else if (filter_type == "blur") {
        // 解析模糊强度参数
        int blur_size = 15; // 默认值
        if (!blur_intensity.empty()) {
            try {
                blur_size = std::stoi(blur_intensity);
                // 确保模糊核大小为奇数且在合理范围内
                if (blur_size < 3) blur_size = 3;
                if (blur_size > 51) blur_size = 51;
                if (blur_size % 2 == 0) blur_size += 1; // 确保为奇数
            } catch (const std::exception& e) {
                std::cout << "警告：无法解析模糊强度参数，使用默认值15" << std::endl;
                blur_size = 15;
            }
        }
        // std::cout << "使用模糊核大小: " << blur_size << "x" << blur_size << std::endl;
        cv::GaussianBlur(image, processed_image, cv::Size(blur_size, blur_size), 0);
    } else if (filter_type == "canny") {
        cv::Mat gray;
        cv::cvtColor(image, gray, cv::COLOR_BGR2GRAY);
        cv::Canny(gray, processed_image, 100, 200);
    } else if (filter_type == "sepia") {
        // 复古棕褐色滤镜
        processed_image = applySepiaFilter(image);
    } else if (filter_type == "emboss") {
        // 浮雕效果
        processed_image = applyEmbossFilter(image);
    } else if (filter_type == "sharpen") {
        // 解析锐化强度参数
        float sharpen_factor = 1.0f; // 默认值
        if (!sharpen_intensity.empty()) {
            try {
                sharpen_factor = std::stof(sharpen_intensity);
                // 确保锐化因子在合理范围内
                if (sharpen_factor < 0.1f) sharpen_factor = 0.1f;
                if (sharpen_factor > 3.0f) sharpen_factor = 3.0f;
            } catch (const std::exception& e) {
                std::cout << "警告：无法解析锐化强度参数，使用默认值1.0" << std::endl;
                sharpen_factor = 1.0f;
            }
        }
        // std::cout << "使用锐化强度: " << sharpen_factor << std::endl;
        processed_image = applySharpenFilter(image, sharpen_factor);
    } else if (filter_type == "cartoon") {
        // 卡通化效果
        processed_image = applyCartoonFilter(image);
    } else if (filter_type == "oil_painting") {
        // 油画效果
        processed_image = applyOilPaintingFilter(image);
    } else {
        // 如果没有匹配的滤镜，则返回原图
        processed_image = image;
    }
    
    // 3. 编码图像为目标格式
    // 注意：Canny 边缘检测后是单通道灰度图，
    std::string ext = ".jpg";
    output_content_type = "image/jpeg";
    if (filter_type == "canny") {
        ext = ".png";
        output_content_type = "image/png";
    }

    return cv::imencode(ext, processed_image, reinterpret_cast<std::vector<uchar>&>(output_data));
}

bool ImageProcessor::loadYOLOModel(const std::string& model_path, const std::string& config_path) {
    std::cout << "[ImageProcessor] 加载YOLOv8模型: " << model_path << std::endl;
    
    // 获取配置管理器
    ConfigManager& config = ConfigManager::getInstance();
    
    // 如果未指定模型路径，从配置文件获取
    std::string actual_model_path = model_path.empty() ? config.getYOLOModelPath() : model_path;
    
    YOLOv8Detector* detector = getDetector();
    
    // 从配置文件获取YOLO参数
    float conf_threshold = config.getYOLOConfidenceThreshold();
    float nms_threshold = config.getYOLONMSThreshold();
    int input_width = config.getYOLOInputWidth();
    int input_height = config.getYOLOInputHeight();
    
    // 重新创建检测器实例以应用新配置
    if (yolo_detector != nullptr) {
        delete yolo_detector;
    }
    yolo_detector = new YOLOv8Detector(conf_threshold, nms_threshold, input_width, input_height);
    
    bool result = yolo_detector->loadModel(actual_model_path, config_path);
    std::cout << "[ImageProcessor] 模型加载结果: " << (result ? "成功" : "失败") << std::endl;
    return result;
}

std::vector<YOLODetection> ImageProcessor::detectObjects(const cv::Mat& image) {
    std::cout << "[ImageProcessor] 开始目标检测，图像尺寸: " << image.cols << "x" << image.rows << std::endl;
    // std::cout << "[ImageProcessor] 模型加载状态: " << (yolo_detector.isModelLoaded() ? "已加载" : "未加载") << std::endl;
    YOLOv8Detector* detector = getDetector();
    std::cout << "[ImageProcessor] 模型加载状态: " << (detector->isModelLoaded() ? "已加载" : "未加载") << std::endl;
    
    

    

        // YOLOv8Detector test_detector;
        // test_detector.loadModel("/home/xie/Desktop/cprj2/models/yolov8x.onnx");
        // auto detections = test_detector.detect(image);

    // auto detections = yolo_detector.detect(image);
    auto detections = detector->detect(image);
    std::cout << "[ImageProcessor] 检测到 " << detections.size() << " 个目标" << std::endl;
    return detections;
}

cv::Mat ImageProcessor::drawDetections(const cv::Mat& image, const std::vector<YOLODetection>& detections) {
    // return yolo_detector.drawDetections(image, detections);
    YOLOv8Detector* detector = getDetector();
    return detector->drawDetections(image, detections);
}

bool ImageProcessor::processWithYOLO(const std::vector<char>& input_data,
                                   std::vector<char>& output_data,
                                   std::string& output_content_type) {
    if (input_data.empty()) {
        return false;
    }
    
    // 解码图像
    cv::Mat image = cv::imdecode(cv::Mat(input_data), cv::IMREAD_COLOR);
    if (image.empty()) {
        return false;
    }

    std::cout << "[ImageProcessor] processWithYOLO - 图像解码成功: " << image.cols << "x" << image.rows << std::endl;
    
    // 获取检测器实例
    YOLOv8Detector* detector = getDetector();
    // 如果模型未加载，尝试加载默认模型
    if (!detector->isModelLoaded()) {
        std::cout << "[ImageProcessor] 模型未加载，尝试加载默认模型" << std::endl;
        // 使用配置管理器获取模型路径
        ConfigManager& config = ConfigManager::getInstance();
        std::string model_path = config.getYOLOModelPath();
        if (!detector->loadModel(model_path)) {
            std::cerr << "❌ 无法加载YOLOv8模型: " << model_path << std::endl;
            return false;
        }
    } else {
        std::cout << "[ImageProcessor] 模型已加载" << std::endl;
    }
    
    // 执行目标检测
    std::vector<YOLODetection> detections = detectObjects(image);
    
    // 绘制检测结果
    cv::Mat result_image = drawDetections(image, detections);
    
    // 编码结果图像
    output_content_type = "image/jpeg";
    return cv::imencode(".jpg", result_image, reinterpret_cast<std::vector<uchar>&>(output_data));
}

std::vector<YOLOSegmentation> ImageProcessor::detectSegmentations(const cv::Mat& image) {
    std::cout << "[ImageProcessor] 开始图像分割，图像尺寸: " << image.cols << "x" << image.rows << std::endl;
    YOLOv8Detector* detector = getDetector();
    std::cout << "[ImageProcessor] 模型加载状态: " << (detector->isModelLoaded() ? "已加载" : "未加载") << std::endl;
    
    auto segmentations = detector->detectSegmentation(image);
    std::cout << "[ImageProcessor] 检测到 " << segmentations.size() << " 个分割区域" << std::endl;
    return segmentations;
}

cv::Mat ImageProcessor::drawSegmentations(const cv::Mat& image, const std::vector<YOLOSegmentation>& segmentations, bool draw_boxes) {
    YOLOv8Detector* detector = getDetector();
    return detector->drawSegmentations(image, segmentations, draw_boxes);
}

bool ImageProcessor::processWithYOLOSegmentation(const std::vector<char>& input_data,
                                               std::vector<char>& output_data,
                                               std::string& output_content_type) {
    if (input_data.empty()) {
        return false;
    }
    
    // 解码图像
    cv::Mat image = cv::imdecode(cv::Mat(input_data), cv::IMREAD_COLOR);
    if (image.empty()) {
        return false;
    }
    
    std::cout << "[ImageProcessor] processWithYOLOSegmentation - 图像解码成功: " << image.cols << "x" << image.rows << std::endl;
    
    // 获取检测器实例
    YOLOv8Detector* detector = getDetector();
    // 如果模型未加载，尝试加载默认模型
    if (!detector->isModelLoaded()) {
        std::cout << "[ImageProcessor] 模型未加载，尝试加载默认模型" << std::endl;
        // 使用配置管理器获取分割模型路径
        ConfigManager& config = ConfigManager::getInstance();
        std::string model_path = config.getYOLOSegmentationModelPath();
        if (!detector->loadModel(model_path)) {
            std::cerr << "❌ 无法加载YOLOv8分割模型: " << model_path << std::endl;
            return false;
        }
    } else {
        std::cout << "[ImageProcessor] 模型已加载" << std::endl;
    }
    
    // 执行图像分割
    std::vector<YOLOSegmentation> segmentations = detectSegmentations(image);
    
    // 绘制分割掩码（默认不显示边界框）
    cv::Mat result_image = drawSegmentations(image, segmentations, false);
    
    // 编码结果图像
    output_content_type = "image/jpeg";
    return cv::imencode(".jpg", result_image, reinterpret_cast<std::vector<uchar>&>(output_data));
}

bool ImageProcessor::processWithYOLOSegmentationWithBoxes(const std::vector<char>& input_data,
                                                        std::vector<char>& output_data,
                                                        std::string& output_content_type) {
    if (input_data.empty()) {
        return false;
    }
    
    // 解码图像
    cv::Mat image = cv::imdecode(cv::Mat(input_data), cv::IMREAD_COLOR);
    if (image.empty()) {
        return false;
    }
    
    std::cout << "[ImageProcessor] processWithYOLOSegmentationWithBoxes - 图像解码成功: " << image.cols << "x" << image.rows << std::endl;
    
    // 获取检测器实例
    YOLOv8Detector* detector = getDetector();
    // 如果模型未加载，尝试加载默认模型
    if (!detector->isModelLoaded()) {
        std::cout << "[ImageProcessor] 模型未加载，尝试加载默认模型" << std::endl;
        // 使用配置管理器获取分割模型路径
        ConfigManager& config = ConfigManager::getInstance();
        std::string model_path = config.getYOLOSegmentationModelPath();
        if (!detector->loadModel(model_path)) {
            std::cerr << "❌ 无法加载YOLOv8分割模型: " << model_path << std::endl;
            return false;
        }
    } else {
        std::cout << "[ImageProcessor] 模型已加载" << std::endl;
    }
    
    // 执行图像分割
    std::vector<YOLOSegmentation> segmentations = detectSegmentations(image);
    
    // 绘制分割结果（显示边界框和标签）
    cv::Mat result_image = drawSegmentations(image, segmentations, true);
    
    // 编码结果图像
    output_content_type = "image/jpeg";
    return cv::imencode(".jpg", result_image, reinterpret_cast<std::vector<uchar>&>(output_data));
}

// OpenCV滤镜效果实现

cv::Mat ImageProcessor::applySepiaFilter(const cv::Mat& image) {
    cv::Mat result = image.clone();
    
    // 棕褐色滤镜矩阵
    cv::Mat sepia_matrix = (cv::Mat_<float>(3, 3) << 
        0.393, 0.769, 0.189,
        0.349, 0.686, 0.168,
        0.272, 0.534, 0.131);
    
    // 应用矩阵变换
    cv::transform(result, result, sepia_matrix);
    
    // 确保像素值在有效范围内
    cv::threshold(result, result, 255, 255, cv::THRESH_TRUNC);
    
    return result;
}

cv::Mat ImageProcessor::applyEmbossFilter(const cv::Mat& image) {
    cv::Mat gray;
    cv::cvtColor(image, gray, cv::COLOR_BGR2GRAY);
    
    // 浮雕卷积核
    cv::Mat kernel = (cv::Mat_<float>(3, 3) << 
        -2, -1, 0,
        -1, 1, 1,
        0, 1, 2);
    
    cv::Mat result;
    cv::filter2D(gray, result, -1, kernel);
    
    // 添加偏移量使结果为正
    result = result + 128;
    
    // 转换为3通道
    cv::cvtColor(result, result, cv::COLOR_GRAY2BGR);
    
    return result;
}

cv::Mat ImageProcessor::applySharpenFilter(const cv::Mat& image, float intensity) {
    cv::Mat result;
    
    // 基础锐化卷积核
    cv::Mat kernel = (cv::Mat_<float>(3, 3) << 
        0, -1, 0,
        -1, 5, -1,
        0, -1, 0);
    
    // 根据强度调整卷积核
    kernel.at<float>(1, 1) = 4 + intensity; // 中心值 = 4 + 强度
    kernel.at<float>(0, 1) = -intensity;    // 上下左右 = -强度
    kernel.at<float>(1, 0) = -intensity;
    kernel.at<float>(1, 2) = -intensity;
    kernel.at<float>(2, 1) = -intensity;
    
    cv::filter2D(image, result, -1, kernel);
    
    // 确保像素值在有效范围内
    cv::threshold(result, result, 255, 255, cv::THRESH_TRUNC);
    cv::threshold(result, result, 0, 0, cv::THRESH_TOZERO);
    
    return result;
}

cv::Mat ImageProcessor::applyCartoonFilter(const cv::Mat& image) {
    cv::Mat result;
    
    // 双边滤波减少噪声并保持边缘
    cv::Mat bilateral;
    cv::bilateralFilter(image, bilateral, 9, 75, 75);
    
    // 边缘检测
    cv::Mat gray, edges;
    cv::cvtColor(bilateral, gray, cv::COLOR_BGR2GRAY);
    cv::adaptiveThreshold(gray, edges, 255, cv::ADAPTIVE_THRESH_MEAN_C, cv::THRESH_BINARY, 9, 9);
    
    // 将边缘转换为3通道
    cv::Mat edges_color;
    cv::cvtColor(edges, edges_color, cv::COLOR_GRAY2BGR);
    
    // 反转边缘（黑色边缘）
    edges_color = 255 - edges_color;
    
    // 将双边滤波结果与边缘结合
    cv::bitwise_and(bilateral, edges_color, result);
    
    return result;
}

cv::Mat ImageProcessor::applyOilPaintingFilter(const cv::Mat& image) {
    cv::Mat result;
    
    // 使用双边滤波模拟油画效果
    cv::bilateralFilter(image, result, 15, 80, 80);
    
    // 增强对比度
    cv::Mat lab;
    cv::cvtColor(result, lab, cv::COLOR_BGR2Lab);
    
    std::vector<cv::Mat> lab_channels;
    cv::split(lab, lab_channels);
    
    // 增强L通道（亮度）
    lab_channels[0] = lab_channels[0] * 1.2;
    
    cv::merge(lab_channels, lab);
    cv::cvtColor(lab, result, cv::COLOR_Lab2BGR);
    
    // 确保像素值在有效范围内
    cv::threshold(result, result, 255, 255, cv::THRESH_TRUNC);
    
    return result;
}
