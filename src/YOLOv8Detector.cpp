#include "YOLOv8Detector.h"
#include "ConfigManager.h"
#include <opencv2/opencv.hpp>
#include <opencv2/dnn.hpp>
#include <iostream>
#include <fstream>
#include <algorithm>
#include <Logger.h>

// COCO数据集的80个类别名称
const std::vector<std::string> YOLOv8Detector::COCO_CLASSES = {
    "person", "bicycle", "car", "motorcycle", "airplane", "bus", "train", "truck", "boat", "traffic light",
    "fire hydrant", "stop sign", "parking meter", "bench", "bird", "cat", "dog", "horse", "sheep", "cow",
    "elephant", "bear", "zebra", "giraffe", "backpack", "umbrella", "handbag", "tie", "suitcase", "frisbee",
    "skis", "snowboard", "sports ball", "kite", "baseball bat", "baseball glove", "skateboard", "surfboard",
    "tennis racket", "bottle", "wine glass", "cup", "fork", "knife", "spoon", "bowl", "banana", "apple",
    "sandwich", "orange", "broccoli", "carrot", "hot dog", "pizza", "donut", "cake", "chair", "couch",
    "potted plant", "bed", "dining table", "toilet", "tv", "laptop", "mouse", "remote", "keyboard", "cell phone",
    "microwave", "oven", "toaster", "sink", "refrigerator", "book", "clock", "vase", "scissors", "teddy bear",
    "hair drier", "toothbrush"
};

YOLOv8Detector::YOLOv8Detector(float conf_threshold, float nms_thresh, int width, int height)
    : model_loaded(false)
    , confidence_threshold(conf_threshold)
    , nms_threshold(nms_thresh)
    , net_width(width)
    , net_height(height)
    , class_names(COCO_CLASSES) {
}

bool YOLOv8Detector::loadModel(const std::string& model_path, const std::string& config_path) {
    try {
        // std::cout << "正在加载YOLOv8模型: " << model_path << std::endl;
        LOG_INFO("正在加载YOLOv8模型: "+config_path);

        
        // 检查文件是否存在
        std::ifstream file(model_path);
        if (!file.good()) {
            std::cerr << "❌ 模型文件不存在: " << model_path << std::endl;
            return false;
        }
        file.close();
        
        // 加载ONNX格式的YOLOv8模型
        yolo_net = cv::dnn::readNetFromONNX(model_path);
        
        // 设置后端和目标设备
        ConfigManager& config = ConfigManager::getInstance();
        std::string backend_str = config.getYOLOBackend();
        std::string target_str = config.getYOLOTarget();
        
        // 根据配置设置后端
        if (backend_str == "OPENCV") {
            yolo_net.setPreferableBackend(cv::dnn::DNN_BACKEND_OPENCV);
        } else if (backend_str == "INFERENCE_ENGINE") {
            yolo_net.setPreferableBackend(cv::dnn::DNN_BACKEND_INFERENCE_ENGINE);
        } else {
            yolo_net.setPreferableBackend(cv::dnn::DNN_BACKEND_OPENCV);
        }
        
        // 根据配置设置目标设备
        if (target_str == "CPU") {
            yolo_net.setPreferableTarget(cv::dnn::DNN_TARGET_CPU);
        } else if (target_str == "OPENCL") {
            yolo_net.setPreferableTarget(cv::dnn::DNN_TARGET_OPENCL);
        } else if (target_str == "OPENCL_FP16") {
            yolo_net.setPreferableTarget(cv::dnn::DNN_TARGET_OPENCL_FP16);
        } else {
            yolo_net.setPreferableTarget(cv::dnn::DNN_TARGET_CPU);
        }
        
        model_loaded = true;
        // std::cout << "✅ YOLOv8模型加载成功!" << std::endl;
        // std::cout << "模型路径: " << model_path << std::endl;
        // std::cout << "输入尺寸: " << net_width << "x" << net_height << std::endl;
        LOG_INFO("✅ YOLOv8模型加载成功!  模型路径: " + model_path + "输入尺寸:" +std::to_string(net_width));

        
        return true;
    } catch (const cv::Exception& e) {
        std::cerr << "❌ YOLOv8模型加载失败: " << e.what() << std::endl;
        model_loaded = false;
        return false;
    }
}

std::vector<YOLODetection> YOLOv8Detector::detect(const cv::Mat& image) {
    std::vector<YOLODetection> detections;
    
    if (!model_loaded) {
        std::cerr << "❌ YOLOv8模型未加载" << std::endl;
        return detections;
    }
    
    try {
        // 准备输入图像
        cv::Mat blob;
        cv::dnn::blobFromImage(image, blob, 1.0/255.0, cv::Size(net_width, net_height), cv::Scalar(0,0,0), true, false);
        
        // 设置输入
        yolo_net.setInput(blob);
        
        // 前向传播
        std::vector<cv::Mat> outputs;
        yolo_net.forward(outputs, yolo_net.getUnconnectedOutLayersNames());
        
        // 解析输出
        detections = parseOutputs(outputs, image.size());
        
    } catch (const cv::Exception& e) {
        std::cerr << "❌ 目标检测失败: " << e.what() << std::endl;
    }
    
    return detections;
}

std::vector<YOLOSegmentation> YOLOv8Detector::detectSegmentation(const cv::Mat& image) {
    std::vector<YOLOSegmentation> segmentations;
    
    if (!model_loaded) {
        std::cerr << "❌ YOLOv8模型未加载" << std::endl;
        return segmentations;
    }
    
    try {
        // 准备输入图像
        cv::Mat blob;
        cv::dnn::blobFromImage(image, blob, 1.0/255.0, cv::Size(net_width, net_height), cv::Scalar(0,0,0), true, false);
        
        // 设置输入
        yolo_net.setInput(blob);
        
        // 前向传播
        std::vector<cv::Mat> outputs;
        yolo_net.forward(outputs, yolo_net.getUnconnectedOutLayersNames());
        
        // 解析分割输出
        segmentations = parseSegmentationOutputs(outputs, image.size());
        
    } catch (const cv::Exception& e) {
        std::cerr << "❌ 图像分割失败: " << e.what() << std::endl;
    }
    
    return segmentations;
}

std::vector<YOLODetection> YOLOv8Detector::parseOutputs(const std::vector<cv::Mat>& outputs, const cv::Size& image_size) {
    std::vector<YOLODetection> detections;
    
    if (outputs.empty()) {
        return detections;
    }

	// 打印输出信息用于调试
	// std::cout << "输出层数: " << outputs.size() << std::endl;
	// std::cout << "输出维度: " << outputs[0].dims << std::endl;
	// std::cout << "输出形状: ";
	for (int i = 0; i < outputs[0].dims; ++i) {
		std::cout << outputs[0].size[i] << " ";
	}
	std::cout << std::endl;


    // YOLOv8输出格式: (batchSize, 84, 8400) 或 (batchSize, 85, 8400)
    // 84 = 4个坐标 + 80个类别概率
    // 85 = 4个坐标 + 1个置信度 + 80个类别概率
    int rows = outputs[0].size[2];        // 8400个检测框
    int dimensions = outputs[0].size[1];   // 84或85个特征
    
    // 重塑输出: (dimensions, rows) -> (rows, dimensions)
    cv::Mat output_processed = outputs[0].clone();
    output_processed = output_processed.reshape(1, dimensions);
    cv::transpose(output_processed, output_processed);
    
    // 计算缩放因子
    float x_factor = (float)image_size.width / net_width;
    float y_factor = (float)image_size.height / net_height;
    
    std::vector<int> class_ids;
    std::vector<float> confidences;
    std::vector<cv::Rect> boxes;
    
    float* data = (float*)output_processed.data;
    // std::cout << "开始处理 " << rows << " 个检测框..." << std::endl;
    // std::cout << "置信度阈值: " << confidence_threshold << std::endl;

    // 调试：统计置信度分布
    int low_conf_count = 0;
    int high_conf_count = 0;
    float max_conf_found = 0.0f;
    
    for (int i = 0; i < rows; ++i) {
        // 创建类别概率矩阵 (跳过前4个坐标值)
        cv::Mat scores(1, class_names.size(), CV_32FC1, data + 4);
        cv::Point class_id;
        double max_class_score;
        
        cv::minMaxLoc(scores, 0, &max_class_score, 0, &class_id);
        
        if (max_class_score > confidence_threshold) {
            confidences.push_back(max_class_score);
            class_ids.push_back(class_id.x);
            
            // 获取边界框坐标 (归一化坐标)
            float x = data[0];
            float y = data[1];
            float w = data[2];
            float h = data[3];
            
            // 转换为像素坐标
            int left = int((x - 0.5 * w) * x_factor);
            int top = int((y - 0.5 * h) * y_factor);
            int width = int(w * x_factor);
            int height = int(h * y_factor);
            
            boxes.push_back(cv::Rect(left, top, width, height));
        }
        
        data += dimensions;
    }
    
    // 应用非极大值抑制
    std::vector<int> nms_result = applyNMS(boxes, confidences, class_ids);
    
    // 创建最终检测结果
    for (unsigned long i = 0; i < nms_result.size(); ++i) {
        int idx = nms_result[i];
        YOLODetection detection;
        detection.class_id = class_ids[idx];
        detection.confidence = confidences[idx];
        detection.box = boxes[idx];
        detection.class_name = class_names[class_ids[idx]];
        detections.push_back(detection);
    }
    
    return detections;
}

std::vector<YOLOSegmentation> YOLOv8Detector::parseSegmentationOutputs(const std::vector<cv::Mat>& outputs, const cv::Size& image_size) {
    std::vector<YOLOSegmentation> segmentations;
    
    if (outputs.size() < 2) {
        std::cerr << "❌ 分割模型输出层数不足" << std::endl;
        return segmentations;
    }
    
    // YOLOv8分割输出格式: 
    // outputs[0]: (batchSize, 84, 8400) - 检测结果
    // outputs[1]: (batchSize, 32, 160, 160) - 掩码原型
    
    cv::Mat detection_output = outputs[0];  // 检测结果
    cv::Mat mask_protos = outputs[1];       // 掩码原型
    
    // 解析检测结果（与检测模型相同）
    int rows = detection_output.size[2];        // 8400个检测框
    int dimensions = detection_output.size[1];  // 84个特征
    
    // 重塑输出: (dimensions, rows) -> (rows, dimensions)
    cv::Mat output_processed = detection_output.clone();
    output_processed = output_processed.reshape(1, dimensions);
    cv::transpose(output_processed, output_processed);
    
    // 计算缩放因子
    float x_factor = (float)image_size.width / net_width;
    float y_factor = (float)image_size.height / net_height;
    
    std::vector<int> class_ids;
    std::vector<float> confidences;
    std::vector<cv::Rect> boxes;
    std::vector<cv::Mat> mask_coeffs;
    
    float* data = (float*)output_processed.data;
    
    for (int i = 0; i < rows; ++i) {
        // 创建类别概率矩阵 (跳过前4个坐标值)
        cv::Mat scores(1, class_names.size(), CV_32FC1, data + 4);
        cv::Point class_id;
        double max_class_score;
        
        cv::minMaxLoc(scores, 0, &max_class_score, 0, &class_id);
        
        if (max_class_score > confidence_threshold) {
            confidences.push_back(max_class_score);
            class_ids.push_back(class_id.x);
            
            // 获取边界框坐标 (归一化坐标)
            float x = data[0];
            float y = data[1];
            float w = data[2];
            float h = data[3];
            
            // 转换为像素坐标
            int left = int((x - 0.5 * w) * x_factor);
            int top = int((y - 0.5 * h) * y_factor);
            int width = int(w * x_factor);
            int height = int(h * y_factor);
            
            boxes.push_back(cv::Rect(left, top, width, height));
            
            // 提取掩码系数 (前32个值)
            cv::Mat coeffs(1, 32, CV_32FC1, data + 4 + class_names.size());
            mask_coeffs.push_back(coeffs.clone());
        }
        
        data += dimensions;
    }
    
    // 应用非极大值抑制
    std::vector<int> nms_result = applyNMS(boxes, confidences, class_ids);
    
    // 根据NMS结果过滤掩码系数和边界框
    std::vector<cv::Mat> filtered_mask_coeffs;
    std::vector<cv::Rect> filtered_boxes;
    
    for (int idx : nms_result) {
        filtered_mask_coeffs.push_back(mask_coeffs[idx]);
        filtered_boxes.push_back(boxes[idx]);
    }
    
    // 处理掩码
    std::vector<cv::Mat> masks = processMasks(mask_protos, filtered_mask_coeffs, filtered_boxes, image_size);
    
    // 创建最终分割结果
    for (unsigned long i = 0; i < nms_result.size(); ++i) {
        int idx = nms_result[i];
        YOLOSegmentation segmentation;
        segmentation.class_id = class_ids[idx];
        segmentation.confidence = confidences[idx];
        segmentation.box = boxes[idx];
        segmentation.mask = masks[i];  // 使用处理后的掩码
        segmentation.class_name = class_names[class_ids[idx]];
        segmentations.push_back(segmentation);
    }
    
    return segmentations;
}

std::vector<int> YOLOv8Detector::applyNMS(const std::vector<cv::Rect>& boxes,
                                         const std::vector<float>& confidences,
                                         const std::vector<int>& class_ids) {
    std::vector<int> nms_result;
    cv::dnn::NMSBoxes(boxes, confidences, confidence_threshold, nms_threshold, nms_result);
    return nms_result;
}

std::vector<cv::Mat> YOLOv8Detector::processMasks(const cv::Mat& mask_protos,
                                                 const std::vector<cv::Mat>& mask_coeffs,
                                                 const std::vector<cv::Rect>& boxes,
                                                 const cv::Size& image_size) {
    std::vector<cv::Mat> masks;
    
    if (mask_protos.empty() || mask_coeffs.empty()) {
        return masks;
    }
    
    // 掩码原型尺寸: (32, 160, 160)
    int mask_h = mask_protos.size[2];  // 160
    int mask_w = mask_protos.size[3];  // 160
    int mask_channels = mask_protos.size[1];  // 32
    
    // 重塑掩码原型: (32, 160, 160) -> (32, 160*160)
    cv::Mat mask_protos_reshaped = mask_protos.reshape(1, mask_channels);
    
    for (size_t i = 0; i < boxes.size() && i < mask_coeffs.size(); ++i) {
        // 计算掩码系数与掩码原型的乘积
        cv::Mat coeffs = mask_coeffs[i];  // 直接使用vector中的Mat
        cv::Mat mask = coeffs * mask_protos_reshaped;  // (1, 160*160)
        
        // 重塑为2D掩码
        mask = mask.reshape(1, mask_h);  // (160, 160)
        
        // 应用Sigmoid激活函数
        cv::Mat mask_sigmoid;
        cv::exp(-mask, mask_sigmoid);
        mask_sigmoid = 1.0 / (1.0 + mask_sigmoid);
        
        // 调整掩码尺寸到原始图像尺寸
        cv::Mat mask_resized;
        cv::resize(mask_sigmoid, mask_resized, image_size);
        
        // 应用边界框裁剪
        cv::Rect box = boxes[i];
        cv::Mat mask_cropped = mask_resized(box);
        
        // 二值化掩码
        cv::Mat mask_binary;
        cv::threshold(mask_cropped, mask_binary, 0.5, 255.0, cv::THRESH_BINARY);
        
        // 确保掩码是CV_8UC1类型
        mask_binary.convertTo(mask_binary, CV_8UC1);
        
        masks.push_back(mask_binary);
    }
    
    return masks;
}

cv::Mat YOLOv8Detector::drawDetections(const cv::Mat& image, const std::vector<YOLODetection>& detections) {
    cv::Mat result = image.clone();
    
    for (const auto& detection : detections) {
        // 绘制边界框
        cv::rectangle(result, detection.box, cv::Scalar(0, 255, 0), 2);
        
        // 准备标签文本
        std::string label = detection.class_name + " (" + 
                           std::to_string(detection.confidence).substr(0, 4) + ")";
        
        // 计算文本大小
        int baseline;
        cv::Size text_size = cv::getTextSize(label, cv::FONT_HERSHEY_SIMPLEX, 0.5, 1, &baseline);
        
        // 绘制标签背景
        cv::Point label_origin(detection.box.x, detection.box.y - 10);
        if (label_origin.y < 0) label_origin.y = detection.box.y + text_size.height + 10;
        
        cv::rectangle(result, 
                     cv::Point(label_origin.x, label_origin.y - text_size.height - baseline),
                     cv::Point(label_origin.x + text_size.width, label_origin.y + baseline),
                     cv::Scalar(0, 255, 0), -1);
        
        // 绘制标签文本
        cv::putText(result, label, label_origin, cv::FONT_HERSHEY_SIMPLEX, 0.5, cv::Scalar(0, 0, 0), 1);
    }
    
    return result;
}

cv::Mat YOLOv8Detector::drawSegmentations(const cv::Mat& image, const std::vector<YOLOSegmentation>& segmentations, bool draw_boxes) {
    cv::Mat result = image.clone();
    
    // 生成随机颜色
    std::vector<cv::Scalar> colors;
    cv::RNG rng(12345);
    for (int i = 0; i < 80; ++i) {
        colors.push_back(cv::Scalar(rng.uniform(0, 255), rng.uniform(0, 255), rng.uniform(0, 255)));
    }
    
    for (const auto& segmentation : segmentations) {
        // 获取颜色
        cv::Scalar color = colors[segmentation.class_id % colors.size()];
        
        // 绘制掩码
        cv::Mat mask_colored;
        cv::cvtColor(segmentation.mask, mask_colored, cv::COLOR_GRAY2BGR);
        
        // 确保掩码是CV_8UC3类型
        mask_colored.convertTo(mask_colored, CV_8UC3);
        
        // 应用颜色到掩码
        cv::Mat colored_mask = cv::Mat::zeros(mask_colored.size(), CV_8UC3);
        for (int y = 0; y < mask_colored.rows; ++y) {
            for (int x = 0; x < mask_colored.cols; ++x) {
                if (mask_colored.at<cv::Vec3b>(y, x)[0] > 0) {  // 如果掩码像素不为0
                    colored_mask.at<cv::Vec3b>(y, x) = cv::Vec3b(
                        static_cast<uchar>(color[0]),
                        static_cast<uchar>(color[1]),
                        static_cast<uchar>(color[2])
                    );
                }
            }
        }
        
        // 只在掩码区域应用颜色叠加，背景区域保持原样
        cv::Mat roi = result(segmentation.box);
        for (int y = 0; y < roi.rows && y < colored_mask.rows; ++y) {
            for (int x = 0; x < roi.cols && x < colored_mask.cols; ++x) {
                // 只有当掩码像素不为0时才应用颜色叠加
                if (colored_mask.at<cv::Vec3b>(y, x)[0] > 0 || 
                    colored_mask.at<cv::Vec3b>(y, x)[1] > 0 || 
                    colored_mask.at<cv::Vec3b>(y, x)[2] > 0) {
                    // 应用颜色叠加
                    cv::Vec3b original = roi.at<cv::Vec3b>(y, x);
                    cv::Vec3b mask_color = colored_mask.at<cv::Vec3b>(y, x);
                    
                    roi.at<cv::Vec3b>(y, x) = cv::Vec3b(
                        static_cast<uchar>(original[0] * 0.5 + mask_color[0] * 0.5),
                        static_cast<uchar>(original[1] * 0.5 + mask_color[1] * 0.5),
                        static_cast<uchar>(original[2] * 0.5 + mask_color[2] * 0.5)
                    );
                }
                // 背景区域（掩码为0）保持原样，不做任何处理
            }
        }
        
        // 根据参数决定是否绘制边界框和标签
        if (draw_boxes) {
            // 绘制边界框
            cv::rectangle(result, segmentation.box, color, 2);
            
            // 准备标签文本
            std::string label = segmentation.class_name + " (" + 
                               std::to_string(segmentation.confidence).substr(0, 4) + ")";
            
            // 计算文本大小
            int baseline;
            cv::Size text_size = cv::getTextSize(label, cv::FONT_HERSHEY_SIMPLEX, 0.5, 1, &baseline);
            
            // 绘制标签背景
            cv::Point label_origin(segmentation.box.x, segmentation.box.y - 10);
            if (label_origin.y < 0) label_origin.y = segmentation.box.y + text_size.height + 10;
            
            cv::rectangle(result, 
                         cv::Point(label_origin.x, label_origin.y - text_size.height - baseline),
                         cv::Point(label_origin.x + text_size.width, label_origin.y + baseline),
                         color, -1);
            
            // 绘制标签文本
            cv::putText(result, label, label_origin, cv::FONT_HERSHEY_SIMPLEX, 0.5, cv::Scalar(255, 255, 255), 1);
        }
    }
    
    return result;
}

bool YOLOv8Detector::isModelLoaded() const {
    return model_loaded;
}

const std::vector<std::string>& YOLOv8Detector::getClassNames() const {
    return class_names;
}

void YOLOv8Detector::setConfidenceThreshold(float threshold) {
    confidence_threshold = threshold;
}

void YOLOv8Detector::setNMSThreshold(float threshold) {
    nms_threshold = threshold;
}

float YOLOv8Detector::getConfidenceThreshold() const {
    return confidence_threshold;
}

float YOLOv8Detector::getNMSThreshold() const {
    return nms_threshold;
}

cv::Size YOLOv8Detector::getInputSize() const {
    return cv::Size(net_width, net_height);
}
