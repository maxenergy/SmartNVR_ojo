#include "yolov8_postprocess.h"
#include "../include/logging.h"
#include <algorithm>
#include <cmath>

// COCO数据集类别名称（临时测试用）
static const char* yolov8_class_names[YOLOV8_OBJ_CLASS_NUM] = {
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

int yolov8_post_process(int8_t* input0, int8_t* input1, int8_t* input2,
                        int model_in_h, int model_in_w,
                        float conf_threshold, float nms_threshold,
                        float scale_w, float scale_h,
                        std::vector<int32_t>& qnt_zps,
                        std::vector<float>& qnt_scales,
                        yolov8_detect_result_group_t* group) {
    
    if (!group || qnt_zps.size() < 3 || qnt_scales.size() < 3) {
        LOGE("YOLOv8PostProcess: 无效参数");
        return -1;
    }
    
    // 初始化结果组
    memset(group, 0, sizeof(yolov8_detect_result_group_t));
    
    std::vector<cv::Rect2f> boxes;
    std::vector<float> scores;
    std::vector<int> class_ids;
    
    // 解析三个输出张量
    // YOLOv8n通常有3个输出：80x80, 40x40, 20x20
    int output_sizes[3] = {80*80, 40*40, 20*20};
    int8_t* outputs[3] = {input0, input1, input2};
    
    for (int i = 0; i < 3; i++) {
        parseYOLOv8Output(outputs[i], output_sizes[i],
                          model_in_w, model_in_h,
                          conf_threshold,
                          scale_w, scale_h,
                          qnt_zps[i], qnt_scales[i],
                          boxes, scores, class_ids);
    }
    
    // 执行NMS
    std::vector<int> indices;
    yolov8_nms(boxes, scores, conf_threshold, nms_threshold, indices);
    
    // 填充结果
    int count = 0;
    for (int idx : indices) {
        if (count >= YOLOV8_OBJ_NUMB_MAX_SIZE) break;
        
        yolov8_detect_result_t& result = group->results[count];
        result.class_id = class_ids[idx];
        result.confidence = scores[idx];
        
        // 转换边界框格式
        result.box.left = static_cast<int>(boxes[idx].x);
        result.box.top = static_cast<int>(boxes[idx].y);
        result.box.right = static_cast<int>(boxes[idx].x + boxes[idx].width);
        result.box.bottom = static_cast<int>(boxes[idx].y + boxes[idx].height);
        
        // 设置类别名称
        if (result.class_id >= 0 && result.class_id < YOLOV8_OBJ_CLASS_NUM) {
            strncpy(result.name, yolov8_class_names[result.class_id], YOLOV8_OBJ_NAME_MAX_SIZE - 1);
            result.name[YOLOV8_OBJ_NAME_MAX_SIZE - 1] = '\0';
        } else {
            snprintf(result.name, YOLOV8_OBJ_NAME_MAX_SIZE, "unknown_%d", result.class_id);
        }
        
        count++;
    }
    
    group->count = count;
    group->id = 0;
    
    LOGD("YOLOv8PostProcess: 检测到 %d 个目标", count);
    return count;
}

void convertYOLOv8ToUnifiedResults(const yolov8_detect_result_group_t& yolov8_group,
                                   InferenceResultGroup& unified_results) {
    unified_results.results.clear();
    unified_results.results.reserve(yolov8_group.count);
    
    for (int i = 0; i < yolov8_group.count; i++) {
        const yolov8_detect_result_t& yolo_result = yolov8_group.results[i];
        
        InferenceResult unified_result;
        unified_result.class_id = yolo_result.class_id;
        unified_result.confidence = yolo_result.confidence;
        unified_result.x1 = static_cast<float>(yolo_result.box.left);
        unified_result.y1 = static_cast<float>(yolo_result.box.top);
        unified_result.x2 = static_cast<float>(yolo_result.box.right);
        unified_result.y2 = static_cast<float>(yolo_result.box.bottom);
        unified_result.class_name = std::string(yolo_result.name);
        
        unified_results.results.push_back(unified_result);
    }
}

const char* getYOLOv8ClassName(int class_id) {
    if (class_id >= 0 && class_id < YOLOV8_OBJ_CLASS_NUM) {
        return yolov8_class_names[class_id];
    }
    return "unknown";
}

void deinitYOLOv8PostProcess() {
    // 清理资源（如果需要）
    LOGD("YOLOv8PostProcess: 反初始化完成");
}

void yolov8_nms(const std::vector<cv::Rect2f>& boxes,
                const std::vector<float>& scores,
                float score_threshold,
                float nms_threshold,
                std::vector<int>& indices) {
    
    indices.clear();
    
    // 创建索引数组并按置信度排序
    std::vector<std::pair<float, int>> score_index_pairs;
    for (size_t i = 0; i < scores.size(); i++) {
        if (scores[i] >= score_threshold) {
            score_index_pairs.push_back(std::make_pair(scores[i], i));
        }
    }
    
    // 按置信度降序排序
    std::sort(score_index_pairs.begin(), score_index_pairs.end(),
              [](const std::pair<float, int>& a, const std::pair<float, int>& b) {
                  return a.first > b.first;
              });
    
    std::vector<bool> suppressed(boxes.size(), false);
    
    for (const auto& pair : score_index_pairs) {
        int idx = pair.second;
        if (suppressed[idx]) continue;
        
        indices.push_back(idx);
        
        // 抑制重叠的边界框
        for (size_t j = 0; j < boxes.size(); j++) {
            if (suppressed[j] || j == idx) continue;
            
            float iou = yolov8_calculate_iou(boxes[idx], boxes[j]);
            if (iou > nms_threshold) {
                suppressed[j] = true;
            }
        }
    }
}

float yolov8_calculate_iou(const cv::Rect2f& box1, const cv::Rect2f& box2) {
    float x1 = std::max(box1.x, box2.x);
    float y1 = std::max(box1.y, box2.y);
    float x2 = std::min(box1.x + box1.width, box2.x + box2.width);
    float y2 = std::min(box1.y + box1.height, box2.y + box2.height);
    
    if (x2 <= x1 || y2 <= y1) {
        return 0.0f;
    }
    
    float intersection = (x2 - x1) * (y2 - y1);
    float area1 = box1.width * box1.height;
    float area2 = box2.width * box2.height;
    float union_area = area1 + area2 - intersection;
    
    return intersection / union_area;
}

void parseYOLOv8Output(int8_t* output_data, int output_size,
                       int model_in_w, int model_in_h,
                       float conf_threshold,
                       float scale_w, float scale_h,
                       int32_t qnt_zp, float qnt_scale,
                       std::vector<cv::Rect2f>& boxes,
                       std::vector<float>& scores,
                       std::vector<int>& class_ids) {

    // YOLOv8n输出格式：[batch, 84, grid_h, grid_w]
    // 84 = 4(bbox) + 80(classes)
    int grid_size = static_cast<int>(sqrt(output_size / YOLOV8_PROP_BOX_SIZE));

    for (int i = 0; i < output_size; i += YOLOV8_PROP_BOX_SIZE) {
        // 反量化边界框坐标
        float x = (static_cast<float>(output_data[i] - qnt_zp) * qnt_scale);
        float y = (static_cast<float>(output_data[i + 1] - qnt_zp) * qnt_scale);
        float w = (static_cast<float>(output_data[i + 2] - qnt_zp) * qnt_scale);
        float h = (static_cast<float>(output_data[i + 3] - qnt_zp) * qnt_scale);

        // 找到最大置信度的类别
        float max_conf = 0.0f;
        int max_class_id = -1;

        for (int c = 0; c < YOLOV8_OBJ_CLASS_NUM; c++) {
            float conf = (static_cast<float>(output_data[i + 4 + c] - qnt_zp) * qnt_scale);
            if (conf > max_conf) {
                max_conf = conf;
                max_class_id = c;
            }
        }

        // 应用置信度阈值
        if (max_conf >= conf_threshold) {
            // 转换坐标格式：中心点+宽高 -> 左上角+宽高
            float x1 = (x - w / 2.0f) * scale_w;
            float y1 = (y - h / 2.0f) * scale_h;
            float width = w * scale_w;
            float height = h * scale_h;

            // 边界检查
            x1 = std::max(0.0f, x1);
            y1 = std::max(0.0f, y1);

            boxes.push_back(cv::Rect2f(x1, y1, width, height));
            scores.push_back(max_conf);
            class_ids.push_back(max_class_id);
        }
    }
}
