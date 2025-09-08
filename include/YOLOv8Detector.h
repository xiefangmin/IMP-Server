#ifndef YOLOV8_DETECTOR_H
#define YOLOV8_DETECTOR_H

#include <opencv2/opencv.hpp>
#include <opencv2/dnn.hpp>
#include <vector>
#include <string>

/**
 * @brief YOLOv8目标检测结果结构体
 */
struct YOLODetection {
    int class_id;           ///< 类别ID
    float confidence;       ///< 置信度
    cv::Rect box;          ///< 边界框
    std::string class_name; ///< 类别名称
};

/**
 * @brief YOLOv8图像分割结果结构体
 */
struct YOLOSegmentation {
    int class_id;           ///< 类别ID
    float confidence;       ///< 置信度
    cv::Rect box;          ///< 边界框
    cv::Mat mask;          ///< 分割掩码
    std::string class_name; ///< 类别名称
};

/**
 * @brief YOLOv8目标检测器类
 * 
 * 该类封装了YOLOv8模型的加载、推理和结果处理功能
 */
class YOLOv8Detector {
private:
    cv::dnn::Net yolo_net;                    ///< YOLOv8网络模型
    std::vector<std::string> class_names;     ///< 类别名称列表
    bool model_loaded;                        ///< 模型是否已加载
    float confidence_threshold;               ///< 置信度阈值
    float nms_threshold;                      ///< NMS阈值
    int net_width;                           ///< 网络输入宽度
    int net_height;                          ///< 网络输入高度
    
    // COCO数据集的80个类别名称
    static const std::vector<std::string> COCO_CLASSES;

public:
    /**
     * @brief 构造函数
     * @param conf_threshold 置信度阈值，默认0.5
     * @param nms_thresh NMS阈值，默认0.4
     * @param width 网络输入宽度，默认640
     * @param height 网络输入高度，默认640
     */
    YOLOv8Detector(float conf_threshold = 0.1f, 
                   float nms_thresh = 0.5f,
                   int width = 640, 
                   int height = 640);
    
    /**
     * @brief 析构函数
     */
    ~YOLOv8Detector() = default;
    
    /**
     * @brief 加载YOLOv8模型
     * @param model_path ONNX模型文件路径
     * @param config_path 配置文件路径（可选，YOLOv8通常不需要）
     * @return 是否加载成功
     */
    bool loadModel(const std::string& model_path, const std::string& config_path = "");
    
    /**
     * @brief 执行目标检测
     * @param image 输入图像
     * @return 检测结果列表
     */
    std::vector<YOLODetection> detect(const cv::Mat& image);
    
    /**
     * @brief 执行图像分割
     * @param image 输入图像
     * @return 分割结果列表
     */
    std::vector<YOLOSegmentation> detectSegmentation(const cv::Mat& image);
    
    /**
     * @brief 绘制检测结果
     * @param image 原始图像
     * @param detections 检测结果列表
     * @return 绘制了检测结果的图像
     */
    cv::Mat drawDetections(const cv::Mat& image, const std::vector<YOLODetection>& detections);
    
    /**
     * @brief 绘制分割结果
     * @param image 原始图像
     * @param segmentations 分割结果列表
     * @param draw_boxes 是否绘制边界框和标签，默认true
     * @return 绘制了分割结果的图像
     */
    cv::Mat drawSegmentations(const cv::Mat& image, const std::vector<YOLOSegmentation>& segmentations, bool draw_boxes = true);
    
    /**
     * @brief 检查模型是否已加载
     * @return 模型是否已加载
     */
    bool isModelLoaded() const;
    
    /**
     * @brief 获取类别名称列表
     * @return 类别名称列表
     */
    const std::vector<std::string>& getClassNames() const;
    
    /**
     * @brief 设置置信度阈值
     * @param threshold 新的置信度阈值
     */
    void setConfidenceThreshold(float threshold);
    
    /**
     * @brief 设置NMS阈值
     * @param threshold 新的NMS阈值
     */
    void setNMSThreshold(float threshold);
    
    /**
     * @brief 获取置信度阈值
     * @return 当前置信度阈值
     */
    float getConfidenceThreshold() const;
    
    /**
     * @brief 获取NMS阈值
     * @return 当前NMS阈值
     */
    float getNMSThreshold() const;
    
    /**
     * @brief 获取网络输入尺寸
     * @return 网络输入尺寸 (width, height)
     */
    cv::Size getInputSize() const;

private:
    /**
     * @brief 解析YOLOv8输出
     * @param outputs 网络输出
     * @param image_size 原始图像尺寸
     * @return 检测结果列表
     */
    std::vector<YOLODetection> parseOutputs(const std::vector<cv::Mat>& outputs, 
                                           const cv::Size& image_size);
    
    /**
     * @brief 解析YOLOv8分割输出
     * @param outputs 网络输出
     * @param image_size 原始图像尺寸
     * @return 分割结果列表
     */
    std::vector<YOLOSegmentation> parseSegmentationOutputs(const std::vector<cv::Mat>& outputs, 
                                                          const cv::Size& image_size);
    
    /**
     * @brief 应用非极大值抑制
     * @param boxes 边界框列表
     * @param confidences 置信度列表
     * @param class_ids 类别ID列表
     * @return 保留的检测结果索引
     */
    std::vector<int> applyNMS(const std::vector<cv::Rect>& boxes,
                             const std::vector<float>& confidences,
                             const std::vector<int>& class_ids);
    
    /**
     * @brief 处理分割掩码
     * @param mask_protos 掩码原型
     * @param mask_coeffs 掩码系数列表
     * @param boxes 边界框列表
     * @param image_size 原始图像尺寸
     * @return 处理后的掩码列表
     */
    std::vector<cv::Mat> processMasks(const cv::Mat& mask_protos,
                                     const std::vector<cv::Mat>& mask_coeffs,
                                     const std::vector<cv::Rect>& boxes,
                                     const cv::Size& image_size);
};

#endif // YOLOV8_DETECTOR_H
