#ifndef YOLOV8_ENGINE_H
#define YOLOV8_ENGINE_H

#include <vector>
#include <memory>
#include <opencv2/opencv.hpp>
#include "../rknn/include/rknn_api.h"
#include "../types/model_config.h"
#include "../rga/include/rga.h"
#include "../rga/include/im2d.h"

/**
 * @brief YOLOv8n RKNN推理引擎
 * 
 * 基于参考项目实现，专门用于YOLOv8n模型的RKNN推理
 * 支持RK3588 NPU硬件加速
 */
class YOLOv8Engine {
public:
    /**
     * @brief 构造函数
     */
    YOLOv8Engine();
    
    /**
     * @brief 析构函数
     */
    ~YOLOv8Engine();
    
    /**
     * @brief 初始化引擎
     * @param config 模型配置
     * @return 0成功，-1失败
     */
    int initialize(const ModelConfig& config);
    
    /**
     * @brief 执行推理
     * @param input_image 输入图像
     * @param results 输出结果
     * @return 0成功，-1失败
     */
    int inference(const cv::Mat& input_image, InferenceResultGroup& results);
    
    /**
     * @brief 释放资源
     */
    void release();
    
    /**
     * @brief 检查是否已初始化
     * @return true已初始化，false未初始化
     */
    bool isInitialized() const;

private:
    // RKNN上下文
    rknn_context ctx_;
    
    // 模型配置
    ModelConfig config_;
    
    // 模型输入输出属性
    rknn_tensor_attr input_attrs_[1];
    rknn_tensor_attr output_attrs_[3];
    
    // 内存管理
    rknn_tensor_mem* input_mems_[1];
    rknn_tensor_mem* output_mems_[3];
    
    // 量化参数
    std::vector<float> out_scales_;
    std::vector<int32_t> out_zps_;
    
    // 缩放参数
    float scale_w_;
    float scale_h_;
    
    // 初始化状态
    bool initialized_;
    
    // 输入输出数量
    uint32_t n_input_;
    uint32_t n_output_;
    
    /**
     * @brief 加载RKNN模型
     * @param model_path 模型路径
     * @return 0成功，-1失败
     */
    int loadModel(const std::string& model_path);
    
    /**
     * @brief 初始化输入输出张量
     * @return 0成功，-1失败
     */
    int initializeTensors();
    
    /**
     * @brief 预处理输入图像
     * @param input_image 输入图像
     * @param processed_data 处理后的数据
     * @return 0成功，-1失败
     */
    int preprocessImage(const cv::Mat& input_image, void** processed_data);
    
    /**
     * @brief 后处理推理结果
     * @param output_data 推理输出数据
     * @param results 处理后的结果
     * @return 0成功，-1失败
     */
    int postprocessResults(void** output_data, InferenceResultGroup& results);
    
    /**
     * @brief 执行NMS（非极大值抑制）
     * @param boxes 边界框
     * @param scores 置信度
     * @param class_ids 类别ID
     * @param conf_threshold 置信度阈值
     * @param nms_threshold NMS阈值
     * @param indices 保留的索引
     */
    void performNMS(const std::vector<cv::Rect2f>& boxes,
                    const std::vector<float>& scores,
                    const std::vector<int>& class_ids,
                    float conf_threshold,
                    float nms_threshold,
                    std::vector<int>& indices);
    
    /**
     * @brief 计算IoU（交并比）
     * @param box1 边界框1
     * @param box2 边界框2
     * @return IoU值
     */
    float calculateIoU(const cv::Rect2f& box1, const cv::Rect2f& box2);
};

#endif // YOLOV8_ENGINE_H
