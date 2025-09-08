# 🖼️ AI图像处理中心

一个基于C++和OpenCV的高性能图像处理Web服务器，集成了多种传统图像滤镜和AI深度学习功能，支持实时图像分割、目标检测和智能滤镜处理。

![License](https://img.shields.io/badge/license-MIT-blue.svg)
![C++](https://img.shields.io/badge/C%2B%2B-17-blue.svg)
![OpenCV](https://img.shields.io/badge/OpenCV-4.x-green.svg)
![Platform](https://img.shields.io/badge/platform-Linux%20%7C%20Windows-lightgrey.svg)
![AI](https://img.shields.io/badge/AI-YOLOv8-orange.svg)

## ✨ 核心功能

### 🎨 传统图像滤镜
- **基础滤镜**: 灰度转换、高斯模糊、Canny边缘检测
- **艺术滤镜**: 复古棕褐色、浮雕效果、卡通化、油画效果
- **智能控制**: 高斯模糊和锐化滤镜支持强度滑动条控制

### 🤖 目标检测与分割
- **YOLOv8目标检测**: 支持80种COCO数据集物体类别
- **YOLOv8图像分割**: 精确的像素级分割，支持多种分割模式
- **实时推理**: 基于ONNX格式的高效CPU推理


## 🎯 支持的图像处理效果

### 基础滤镜
| 滤镜类型 | 描述 | 输出格式 | 参数控制 |
|---------|------|----------|----------|
| `none` | 原图（无处理） | JPEG | - |
| `grayscale` | 灰度转换 | JPEG | - |
| `blur` | 高斯模糊 | JPEG | ✅ 强度滑动条 (3-51) |
| `canny` | Canny边缘检测 | PNG | - |
| `sharpen` | 锐化滤镜 | JPEG | ✅ 强度滑动条 (0.1-3.0) |

### 艺术滤镜
| 滤镜类型 | 描述 | 输出格式 | 技术实现 |
|---------|------|----------|----------|
| `sepia` | 复古棕褐色 | JPEG | 颜色矩阵变换 |
| `emboss` | 浮雕效果 | JPEG | 3D卷积核滤波 |
| `cartoon` | 卡通化效果 | JPEG | 双边滤波+边缘检测 |
| `oil_painting` | 油画效果 | JPEG | 双边滤波+对比度增强 |

### AI深度学习
| 功能类型 | 描述 | 输出格式 | 模型支持 |
|---------|------|----------|----------|
| `yolo_detect` | YOLOv8目标检测 | JPEG | yolov8n.onnx |
| `yolo_segment` | YOLOv8图像分割 | JPEG | yolov8x-seg.onnx |
| `yolo_segment_with_boxes` | 检测+分割组合 | JPEG | yolov8x-seg.onnx |

## 🛠️ 技术架构

### 后端技术栈
- **核心语言**: C++17
- **图像处理**: OpenCV 4.9.0
- **网络框架**: 自定义epoll服务器
- **并发处理**: 智能线程池
- **AI推理**: ONNX Runtime 
- **配置管理**: JSON配置文件支持
- **日志系统**: 多级别日志记录


### 系统架构
```
┌─────────────────┐    ┌─────────────────┐    ┌─────────────────┐
│   Web Browser   │    │   Mobile App    │    │   API Client    │
└─────────┬───────┘    └─────────┬───────┘    └─────────┬───────┘
          │                      │                      │
          └──────────────────────┼──────────────────────┘
                                 │ HTTP
                    ┌─────────────▼─────────────┐
                    │     Multi-Port Server    │
                    │    (8080-8099)           │
                    └─────────────┬─────────────┘
                                 │
                    ┌─────────────▼─────────────┐
                    │      Thread Pool         │
                    │   (Auto CPU Detection)   │
                    └─────────────┬─────────────┘
                                 │
                    ┌─────────────▼─────────────┐
                    │   Image Processor         │
                    │  ┌─────────────────────┐  │
                    │  │  Traditional Filters │  │
                    │  │  AI Deep Learning   │  │
                    │  │  Parameter Control  │  │
                    │  └─────────────────────┘  │
                    └───────────────────────────┘
```


## 🚀 快速开始

### 1. 环境准备

- **Ubuntu 22.04.5 LTS**
- **OpenCV 4.9.0**
- **CMake 3.16+**
- **C++**
- **YOLOv8 ONNX模型**


### 2. 克隆项目
```bash
git clone https://github.com/xiefangmin/IMP-Server.git
```

### 3. 下载AI模型
```bash
# 创建模型目录
mkdir -p models

# 下载YOLOv8目标检测模型
https://github.com/ultralytics/assets/releases/download/v8.3.0/yolov8n.pt
https://github.com/ultralytics/assets/releases/download/v8.3.0/yolov8s.pt
https://github.com/ultralytics/assets/releases/download/v8.3.0/yolov8m.pt
https://github.com/ultralytics/assets/releases/download/v8.3.0/yolov8l.pt
https://github.com/ultralytics/assets/releases/download/v8.3.0/yolov8x.pt

# 下载YOLOv8图像分割模型
https://github.com/ultralytics/assets/releases/download/v8.3.0/yolov8n-seg.pt
https://github.com/ultralytics/assets/releases/download/v8.3.0/yolov8s-seg.pt
https://github.com/ultralytics/assets/releases/download/v8.3.0/yolov8m-seg.pt
https://github.com/ultralytics/assets/releases/download/v8.3.0/yolov8l-seg.pt
https://github.com/ultralytics/assets/releases/download/v8.3.0/yolov8x-seg.pt

# 转为 onn
https://docs.ultralytics.com/zh/integrations/onnx/
```

### 4. 编译项目
```bash
mkdir build && cd build
cmake .. 
make -j$(nproc)


### 5. 配置服务器
编辑 `config.json` 文件来配置服务器参数


### 6. 运行服务器
```bash
# 使用默认配置文件 config.json
./image_server

# 使用自定义配置文件
./image_server -p /path/to/custom_config.json

```

### 7. 访问Web界面
打开浏览器访问: `http://192.168.25.130:9090` (根据配置文件中的IP和端口)

## 📖 使用指南


#### 图像处理接口
```http
POST /upload
Content-Type: multipart/form-data

参数:
- image: 图像文件 (支持JPG, PNG, BMP等格式)
- filter: 滤镜类型
- blur_intensity: 高斯模糊强度 (3-51, 仅blur滤镜)
- sharpen_intensity: 锐化强度 (0.1-3.0, 仅sharpen滤镜)
- uuid: 请求唯一标识符
```

#### 响应格式
```http
HTTP/1.1 200 OK
Content-Type: image/jpeg 或 image/png
X-UUID: 请求UUID
Content-Length: 图像数据大小

[处理后的图像数据]
```

#### API使用示例
```bash
# 基础滤镜
curl -X POST -F "image=@test.jpg" -F "filter=grayscale" http://localhost:8080/upload --output ./test_outimg.jpg


# 带参数控制
curl -X POST -F "image=@test.jpg" -F "filter=blur" -F "blur_intensity=25" http://localhost:8080/upload --output ./test_outimg.jpg

# yolov8 功能
curl -X POST -F "image=@test.jpg" -F "filter=yolo_detect" http://localhost:8080/upload --output ./test_outimg.jpg
curl -X POST -F "image=@test.jpg" -F "filter=yolo_segment" http://localhost:8080/upload --output ./test_outimg.jpg
```


## ⚙️ 配置文件管理

项目支持通过JSON配置文件进行灵活配置，无需重新编译即可修改参数。

### 配置文件结构

完整的 `config.json` 配置文件包含以下部分：

```json
{
  "server": {
    "ports": [9090, 9091],
    "thread_pool_size": 16,
    "max_connections": 1000,
    "ip_address": "192.168.25.130"
  },
  "yolo": {
    "model_path": "models/yolov8n.onnx",
    "segmentation_model_path": "models/yolov8x-seg.onnx",
    "confidence_threshold": 0.1,
    "nms_threshold": 0.5,
    "input_width": 640,
    "input_height": 640,
    "backend": "OPENCV",
    "target": "CPU"
  },
  "image_processing": {
    "max_image_size": 10485760,
    "supported_formats": ["jpg", "jpeg", "png", "bmp", "tiff"],
    "output_quality": 95
  },
  "logging": {
    "level": "INFO",
    "enable_console": false,
    "enable_file": true,
    "log_file": "server.log"
  }
}
```



```bash
# 使用默认配置文件
./image_server

# 使用自定义配置文件
./image_server -p custom_config.json

# 查看帮助信息
./image_server -h
```




#### 运行时问题
```bash
# 端口占用检查
netstat -tlnp | grep :9090
netstat -anop | grep :9090


# 配置文件检查
cat config.json | jq .  # 验证JSON格式
```

#### AI功能问题
- **模型加载失败**: 检查ONNX文件完整性和OpenCV版本
- **检测无结果**: 调整置信度阈值，检查图像质量
- **分割效果差**: 确保图像包含COCO数据集支持的物体
- **推理速度慢**: 考虑使用GPU版本OpenCV或优化模型





### 并发性能
- **单端口**: 1000+ QPS
- **多端口**: 5000+ QPS
- **内存使用**: 500MB (基础) + 100MB/端口




## 📚 相关资源

- [OpenCV官方文档](https://docs.opencv.org/)
- [YOLOv8官方文档](https://docs.ultralytics.com/)
- [ONNX Runtime文档](https://onnxruntime.ai/)

