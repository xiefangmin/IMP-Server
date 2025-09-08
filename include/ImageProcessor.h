#ifndef IMAGE_PROCESSOR_H
#define IMAGE_PROCESSOR_H

#include <vector>
#include <string>
#include <opencv2/opencv.hpp>
#include "YOLOv8Detector.h"

class ImageProcessor {
public:
    static bool process(const std::vector<char>& input_data,
                       std::vector<char>& output_data,
                       const std::string& filter_type,
                       std::string& output_content_type,
                       const std::string& blur_intensity = "",
                       const std::string& sharpen_intensity = "");
    
    // YOLOv8目标检测相关方法（使用YOLOv8Detector）
    static bool loadYOLOModel(const std::string& model_path, const std::string& config_path = "");
    static std::vector<YOLODetection> detectObjects(const cv::Mat& image);
    static cv::Mat drawDetections(const cv::Mat& image, const std::vector<YOLODetection>& detections);
    static bool processWithYOLO(const std::vector<char>& input_data,
                               std::vector<char>& output_data,
                               std::string& output_content_type);
    
    // YOLOv8图像分割相关方法
    static std::vector<YOLOSegmentation> detectSegmentations(const cv::Mat& image);
    static cv::Mat drawSegmentations(const cv::Mat& image, const std::vector<YOLOSegmentation>& segmentations, bool draw_boxes = false);
    static bool processWithYOLOSegmentation(const std::vector<char>& input_data,
                                           std::vector<char>& output_data,
                                           std::string& output_content_type);
    static bool processWithYOLOSegmentationWithBoxes(const std::vector<char>& input_data,
                                                    std::vector<char>& output_data,
                                                    std::string& output_content_type);
    
    // 获取检测器实例（延迟初始化）
    static YOLOv8Detector* getDetector();

private:
    // OpenCV滤镜效果方法
    static cv::Mat applySepiaFilter(const cv::Mat& image);
    static cv::Mat applyEmbossFilter(const cv::Mat& image);
    static cv::Mat applySharpenFilter(const cv::Mat& image, float intensity = 1.0f);
    static cv::Mat applyCartoonFilter(const cv::Mat& image);
    static cv::Mat applyOilPaintingFilter(const cv::Mat& image);

private:
    static YOLOv8Detector* yolo_detector;
};

#endif // IMAGE_PROCESSOR_H

