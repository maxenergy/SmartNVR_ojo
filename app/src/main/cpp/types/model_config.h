#ifndef MODEL_CONFIG_H
#define MODEL_CONFIG_H

#include <string>
#include <vector>

/**
 * @brief 模型类型枚举
 */
enum class ModelType {
    YOLOV5 = 0,
    YOLOV8N = 1
};

/**
 * @brief 模型配置结构体
 */
struct ModelConfig {
    ModelType type;
    std::string model_path;
    int input_width;
    int input_height;
    int input_channels;
    float conf_threshold;
    float nms_threshold;
    int num_classes;
    
    // YOLOv5默认配置
    static ModelConfig getYOLOv5Config() {
        return {
            .type = ModelType::YOLOV5,
            .model_path = "/data/data/com.wulala.myyolov5rtspthreadpool/files/yolov5s.rknn",
            .input_width = 640,
            .input_height = 640,
            .input_channels = 3,
            .conf_threshold = 0.5f,
            .nms_threshold = 0.6f,
            .num_classes = 80
        };
    }
    
    // YOLOv8n默认配置（临时使用YOLOv5模型测试代码路径）
    static ModelConfig getYOLOv8nConfig() {
        return {
            .type = ModelType::YOLOV8N,
            .model_path = "/data/data/com.wulala.myyolov5rtspthreadpool/files/yolov5s.rknn",  // 临时使用YOLOv5模型
            .input_width = 640,
            .input_height = 640,
            .input_channels = 3,
            .conf_threshold = 0.5f,
            .nms_threshold = 0.6f,
            .num_classes = 80  // 临时使用80类别
        };
    }
};

/**
 * @brief 推理结果结构体（统一格式）
 */
struct InferenceResult {
    int class_id;
    float confidence;
    float x1, y1, x2, y2;  // 边界框坐标
    std::string class_name;
};

/**
 * @brief 推理结果组
 */
struct InferenceResultGroup {
    std::vector<InferenceResult> results;
    int frame_id;
    long timestamp;
    ModelType model_type;
};

#endif // MODEL_CONFIG_H
