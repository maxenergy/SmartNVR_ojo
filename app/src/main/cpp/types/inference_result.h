#ifndef INFERENCE_RESULT_H
#define INFERENCE_RESULT_H

#include <string>
#include <vector>
#include <opencv2/opencv.hpp>

/**
 * @brief 推理结果结构
 * 
 * 用于表示YOLO模型的单个检测结果
 */
struct InferenceResult {
    std::string class_name;     // 类别名称
    float confidence;           // 置信度
    float x1, y1;              // 左上角坐标
    float x2, y2;              // 右下角坐标
    
    InferenceResult() 
        : class_name("")
        , confidence(0.0f)
        , x1(0.0f), y1(0.0f)
        , x2(0.0f), y2(0.0f) {
    }
    
    InferenceResult(const std::string& name, float conf, float x1, float y1, float x2, float y2)
        : class_name(name)
        , confidence(conf)
        , x1(x1), y1(y1)
        , x2(x2), y2(y2) {
    }
    
    /**
     * 获取边界框宽度
     */
    float getWidth() const {
        return x2 - x1;
    }
    
    /**
     * 获取边界框高度
     */
    float getHeight() const {
        return y2 - y1;
    }
    
    /**
     * 获取边界框面积
     */
    float getArea() const {
        return getWidth() * getHeight();
    }
    
    /**
     * 获取边界框中心点
     */
    cv::Point2f getCenter() const {
        return cv::Point2f((x1 + x2) / 2.0f, (y1 + y2) / 2.0f);
    }
    
    /**
     * 转换为cv::Rect
     */
    cv::Rect toRect() const {
        return cv::Rect(
            static_cast<int>(x1),
            static_cast<int>(y1),
            static_cast<int>(getWidth()),
            static_cast<int>(getHeight())
        );
    }
    
    /**
     * 检查是否为有效检测结果
     */
    bool isValid() const {
        return !class_name.empty() && 
               confidence > 0 && 
               getWidth() > 0 && 
               getHeight() > 0;
    }
};

/**
 * @brief 推理结果组
 * 
 * 用于表示一次推理的所有检测结果
 */
struct InferenceResultGroup {
    std::vector<InferenceResult> results;   // 检测结果列表
    
    /**
     * 获取指定类别的检测结果
     */
    std::vector<InferenceResult> getResultsByClass(const std::string& className) const {
        std::vector<InferenceResult> classResults;
        for (const auto& result : results) {
            if (result.class_name == className) {
                classResults.push_back(result);
            }
        }
        return classResults;
    }
    
    /**
     * 获取指定类别的检测数量
     */
    int getCountByClass(const std::string& className) const {
        int count = 0;
        for (const auto& result : results) {
            if (result.class_name == className) {
                count++;
            }
        }
        return count;
    }
    
    /**
     * 获取最高置信度的检测结果
     */
    InferenceResult getBestResult() const {
        if (results.empty()) return InferenceResult();
        
        auto maxIt = std::max_element(results.begin(), results.end(),
                                     [](const InferenceResult& a, const InferenceResult& b) {
                                         return a.confidence < b.confidence;
                                     });
        return *maxIt;
    }
    
    /**
     * 过滤低置信度结果
     */
    void filterByConfidence(float minConfidence) {
        results.erase(
            std::remove_if(results.begin(), results.end(),
                          [minConfidence](const InferenceResult& result) {
                              return result.confidence < minConfidence;
                          }),
            results.end()
        );
    }
    
    /**
     * 清空结果
     */
    void clear() {
        results.clear();
    }
    
    /**
     * 检查是否为空
     */
    bool empty() const {
        return results.empty();
    }
    
    /**
     * 获取结果数量
     */
    size_t size() const {
        return results.size();
    }
};

/**
 * @brief 推理统计信息
 */
struct InferenceStats {
    int totalDetections = 0;        // 总检测数量
    int validDetections = 0;        // 有效检测数量
    float averageConfidence = 0.0f; // 平均置信度
    float maxConfidence = 0.0f;     // 最高置信度
    float minConfidence = 1.0f;     // 最低置信度
    
    /**
     * 从推理结果组更新统计信息
     */
    void updateFromResults(const InferenceResultGroup& results) {
        totalDetections = static_cast<int>(results.size());
        validDetections = 0;
        
        if (totalDetections == 0) {
            averageConfidence = 0.0f;
            maxConfidence = 0.0f;
            minConfidence = 1.0f;
            return;
        }
        
        float totalConfidence = 0.0f;
        maxConfidence = 0.0f;
        minConfidence = 1.0f;
        
        for (const auto& result : results.results) {
            if (result.isValid()) {
                validDetections++;
                totalConfidence += result.confidence;
                maxConfidence = std::max(maxConfidence, result.confidence);
                minConfidence = std::min(minConfidence, result.confidence);
            }
        }
        
        averageConfidence = (validDetections > 0) ? totalConfidence / validDetections : 0.0f;
    }
    
    /**
     * 重置统计信息
     */
    void reset() {
        totalDetections = 0;
        validDetections = 0;
        averageConfidence = 0.0f;
        maxConfidence = 0.0f;
        minConfidence = 1.0f;
    }
};

// 工具函数
namespace InferenceResultUtils {
    
    /**
     * 将Detection结构转换为InferenceResult
     */
    InferenceResult convertFromDetection(const Detection& detection);
    
    /**
     * 将InferenceResult转换为Detection结构
     */
    Detection convertToDetection(const InferenceResult& result);
    
    /**
     * 计算两个边界框的IoU
     */
    float calculateIoU(const InferenceResult& a, const InferenceResult& b);
    
    /**
     * 非极大值抑制
     */
    std::vector<InferenceResult> applyNMS(const std::vector<InferenceResult>& results, 
                                         float iouThreshold = 0.5f);
    
    /**
     * 按置信度排序
     */
    void sortByConfidence(std::vector<InferenceResult>& results, bool descending = true);
    
    /**
     * 格式化输出推理结果
     */
    std::string formatResults(const InferenceResultGroup& results);
}

#endif // INFERENCE_RESULT_H
