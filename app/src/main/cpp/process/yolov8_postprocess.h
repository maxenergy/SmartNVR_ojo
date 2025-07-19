#ifndef YOLOV8_POSTPROCESS_H
#define YOLOV8_POSTPROCESS_H

#include <vector>
#include <cstdint>
#include <opencv2/opencv.hpp>
#include "../types/model_config.h"

/**
 * @brief YOLOv8n后处理模块
 * 
 * 基于参考项目的后处理实现，适配YOLOv8n模型的输出格式
 * 支持置信度过滤、NMS等后处理操作
 */

// YOLOv8n相关常量（临时使用80类别测试）
#define YOLOV8_OBJ_NAME_MAX_SIZE 16
#define YOLOV8_OBJ_NUMB_MAX_SIZE 64
#define YOLOV8_OBJ_CLASS_NUM     80  // 临时使用80类别测试代码路径
#define YOLOV8_NMS_THRESH        0.6f
#define YOLOV8_BOX_THRESH        0.5f
#define YOLOV8_PROP_BOX_SIZE     (4 + YOLOV8_OBJ_CLASS_NUM)  // YOLOv8格式：x,y,w,h + classes

/**
 * @brief YOLOv8n边界框结构
 */
typedef struct _YOLOV8_BOX_RECT {
    int left;
    int right;
    int top;
    int bottom;
} YOLOV8_BOX_RECT;

/**
 * @brief YOLOv8n检测结果
 */
typedef struct _yolov8_detect_result_t {
    char name[YOLOV8_OBJ_NAME_MAX_SIZE];
    int class_id;
    YOLOV8_BOX_RECT box;
    float confidence;
} yolov8_detect_result_t;

/**
 * @brief YOLOv8n检测结果组
 */
typedef struct _yolov8_detect_result_group_t {
    int id;
    int count;
    yolov8_detect_result_t results[YOLOV8_OBJ_NUMB_MAX_SIZE];
} yolov8_detect_result_group_t;

/**
 * @brief YOLOv8n后处理主函数
 * 
 * @param input0 第一个输出张量数据
 * @param input1 第二个输出张量数据  
 * @param input2 第三个输出张量数据
 * @param model_in_h 模型输入高度
 * @param model_in_w 模型输入宽度
 * @param conf_threshold 置信度阈值
 * @param nms_threshold NMS阈值
 * @param scale_w 宽度缩放比例
 * @param scale_h 高度缩放比例
 * @param qnt_zps 量化零点
 * @param qnt_scales 量化缩放因子
 * @param group 输出结果组
 * @return 检测到的目标数量
 */
int yolov8_post_process(int8_t* input0, int8_t* input1, int8_t* input2,
                        int model_in_h, int model_in_w,
                        float conf_threshold, float nms_threshold,
                        float scale_w, float scale_h,
                        std::vector<int32_t>& qnt_zps,
                        std::vector<float>& qnt_scales,
                        yolov8_detect_result_group_t* group);

/**
 * @brief 转换YOLOv8结果到统一格式
 * 
 * @param yolov8_group YOLOv8检测结果组
 * @param unified_results 统一格式结果
 */
void convertYOLOv8ToUnifiedResults(const yolov8_detect_result_group_t& yolov8_group,
                                   InferenceResultGroup& unified_results);

/**
 * @brief 获取类别名称
 * 
 * @param class_id 类别ID
 * @return 类别名称
 */
const char* getYOLOv8ClassName(int class_id);

/**
 * @brief 反初始化后处理模块
 */
void deinitYOLOv8PostProcess();

/**
 * @brief YOLOv8n特定的NMS实现
 * 
 * @param boxes 边界框数组
 * @param scores 置信度数组
 * @param score_threshold 置信度阈值
 * @param nms_threshold NMS阈值
 * @param indices 保留的索引
 */
void yolov8_nms(const std::vector<cv::Rect2f>& boxes,
                const std::vector<float>& scores,
                float score_threshold,
                float nms_threshold,
                std::vector<int>& indices);

/**
 * @brief 计算两个边界框的IoU
 * 
 * @param box1 边界框1
 * @param box2 边界框2
 * @return IoU值
 */
float yolov8_calculate_iou(const cv::Rect2f& box1, const cv::Rect2f& box2);

/**
 * @brief 解析YOLOv8n输出张量
 * 
 * @param output_data 输出张量数据
 * @param output_size 输出张量大小
 * @param model_in_w 模型输入宽度
 * @param model_in_h 模型输入高度
 * @param conf_threshold 置信度阈值
 * @param scale_w 宽度缩放
 * @param scale_h 高度缩放
 * @param qnt_zp 量化零点
 * @param qnt_scale 量化缩放
 * @param boxes 输出边界框
 * @param scores 输出置信度
 * @param class_ids 输出类别ID
 */
void parseYOLOv8Output(int8_t* output_data, int output_size,
                       int model_in_w, int model_in_h,
                       float conf_threshold,
                       float scale_w, float scale_h,
                       int32_t qnt_zp, float qnt_scale,
                       std::vector<cv::Rect2f>& boxes,
                       std::vector<float>& scores,
                       std::vector<int>& class_ids);

#endif // YOLOV8_POSTPROCESS_H
