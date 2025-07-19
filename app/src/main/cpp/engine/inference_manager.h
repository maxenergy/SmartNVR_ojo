#ifndef INFERENCE_MANAGER_H
#define INFERENCE_MANAGER_H

#include <memory>
#include <mutex>
#include "../types/model_config.h"
#include "../types/yolo_datatype.h"
#include "rknn_engine.h"

// 前向声明
class YOLOv8Engine;

/**
 * @brief 统一推理管理器
 * 
 * 负责管理YOLOv5和YOLOv8n两种模型的推理，提供统一的接口
 * 确保向后兼容性，同时支持新的YOLOv8n功能
 */
class InferenceManager {
public:
    /**
     * @brief 构造函数
     */
    InferenceManager();
    
    /**
     * @brief 析构函数
     */
    ~InferenceManager();
    
    /**
     * @brief 初始化推理管理器
     * @param yolov5_config YOLOv5配置
     * @param yolov8_config YOLOv8n配置（可选）
     * @return 0成功，-1失败
     */
    int initialize(const ModelConfig& yolov5_config, 
                   const ModelConfig* yolov8_config = nullptr);
    
    /**
     * @brief 设置当前使用的模型类型
     * @param type 模型类型
     * @return 0成功，-1失败
     */
    int setCurrentModel(ModelType type);
    
    /**
     * @brief 获取当前模型类型
     * @return 当前模型类型
     */
    ModelType getCurrentModel() const;
    
    /**
     * @brief 执行推理（统一接口）
     * @param input_data 输入数据
     * @param results 输出结果
     * @return 0成功，-1失败
     */
    int inference(const cv::Mat& input_data, InferenceResultGroup& results);
    
    /**
     * @brief YOLOv5推理（保持向后兼容）
     * @param input_data 输入数据
     * @param results YOLOv5结果格式
     * @return 0成功，-1失败
     */
    int yolov5_inference(const cv::Mat& input_data, std::vector<Detection>& results);
    
    /**
     * @brief YOLOv8n推理
     * @param input_data 输入数据
     * @param results 推理结果
     * @return 0成功，-1失败
     */
    int yolov8_inference(const cv::Mat& input_data, InferenceResultGroup& results);
    
    /**
     * @brief 检查模型是否已初始化
     * @param type 模型类型
     * @return true已初始化，false未初始化
     */
    bool isModelInitialized(ModelType type) const;
    
    /**
     * @brief 释放资源
     */
    void release();

private:
    // YOLOv5引擎（现有）
    std::unique_ptr<RKEngine> yolov5_engine_;
    
    // YOLOv8n引擎（新增）
    std::unique_ptr<YOLOv8Engine> yolov8_engine_;
    
    // 当前使用的模型类型
    ModelType current_model_;
    
    // 模型配置
    ModelConfig yolov5_config_;
    ModelConfig yolov8_config_;
    
    // 初始化状态
    bool yolov5_initialized_;
    bool yolov8_initialized_;
    
    // 线程安全
    mutable std::mutex mutex_;
    
    /**
     * @brief 初始化YOLOv5引擎
     * @param config 配置
     * @return 0成功，-1失败
     */
    int initializeYOLOv5(const ModelConfig& config);
    
    /**
     * @brief 初始化YOLOv8n引擎
     * @param config 配置
     * @return 0成功，-1失败
     */
    int initializeYOLOv8(const ModelConfig& config);
};

#endif // INFERENCE_MANAGER_H
