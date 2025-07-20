#ifndef EXTENDED_INFERENCE_MANAGER_H
#define EXTENDED_INFERENCE_MANAGER_H

#include "inference_manager.h"
#include "../face/face_analysis_manager.h"
#include "../statistics/statistics_manager.h"
#include <chrono>
#include <android/asset_manager.h>

// 级联检测配置
struct CascadeDetectionConfig {
    bool enableFaceAnalysis = false;
    bool enableStatistics = false;
    bool enablePersonTracking = false;
    
    // 人员检测阈值
    float personConfidenceThreshold = 0.5f;
    int minPersonPixelSize = 50;
    int maxPersonsPerFrame = 10;
    
    // 人脸分析配置
    FaceAnalysisConfig faceAnalysisConfig;
    
    // 统计配置
    StatisticsConfig statisticsConfig;
    
    bool isValid() const {
        return personConfidenceThreshold > 0.0f && 
               minPersonPixelSize > 0 &&
               maxPersonsPerFrame > 0;
    }
};

// 扩展推理结果
struct ExtendedInferenceResult {
    InferenceResultGroup objectDetections;              // 原有目标检测结果
    std::vector<FaceAnalysisResult> faceAnalysisResults; // 人脸分析结果
    StatisticsData statistics;                          // 统计数据
    
    // 性能信息
    struct PerformanceInfo {
        std::chrono::milliseconds objectDetectionTime{0};
        std::chrono::milliseconds faceAnalysisTime{0};
        std::chrono::milliseconds totalTime{0};
        int processedPersonCount = 0;
        int detectedFaceCount = 0;
    } performanceInfo;
    
    bool hasPersonDetections() const {
        return std::any_of(objectDetections.results.begin(),
                          objectDetections.results.end(),
                          [](const InferenceResult& r) {
                              return r.class_name == "person";
                          });
    }
    
    int getPersonCount() const {
        return std::count_if(objectDetections.results.begin(),
                            objectDetections.results.end(),
                            [](const InferenceResult& r) {
                                return r.class_name == "person";
                            });
    }
    
    int getValidFaceCount() const {
        int count = 0;
        for (const auto& result : faceAnalysisResults) {
            count += result.getValidFaceCount();
        }
        return count;
    }
};

/**
 * @brief 扩展推理管理器
 * 
 * 在原有InferenceManager基础上扩展人脸分析和统计功能
 * 实现目标检测 → 人员筛选 → 人脸识别 → 属性分析的级联流程
 */
class ExtendedInferenceManager {
public:
    // 性能监控结构
    struct PerformanceMonitor {
        int totalInferenceCount = 0;
        int successfulInferenceCount = 0;
        int faceAnalysisCount = 0;
        std::chrono::milliseconds totalInferenceTime{0};
        std::chrono::milliseconds totalFaceAnalysisTime{0};
        std::chrono::steady_clock::time_point lastInferenceTime;
        
        double getAverageInferenceTime() const {
            if (totalInferenceCount == 0) return 0.0;
            return static_cast<double>(totalInferenceTime.count()) / totalInferenceCount;
        }
        
        double getAverageFaceAnalysisTime() const {
            if (faceAnalysisCount == 0) return 0.0;
            return static_cast<double>(totalFaceAnalysisTime.count()) / faceAnalysisCount;
        }
        
        double getSuccessRate() const {
            if (totalInferenceCount == 0) return 0.0;
            return static_cast<double>(successfulInferenceCount) / totalInferenceCount * 100.0;
        }
    };

private:
    // 核心推理管理器
    std::unique_ptr<InferenceManager> m_inferenceManager;

    // 人脸分析和统计模块
    std::unique_ptr<FaceAnalysisManager> m_faceAnalysisManager;
    std::unique_ptr<StatisticsManager> m_statisticsManager;

    // 级联检测配置
    CascadeDetectionConfig m_cascadeConfig;

    // 初始化状态
    bool m_initialized;
    bool m_faceAnalysisEnabled;
    bool m_statisticsEnabled;

    mutable std::mutex m_mutex;

    // 性能监控实例
    PerformanceMonitor m_performanceMonitor;

public:
    ExtendedInferenceManager();
    ~ExtendedInferenceManager();
    
    // 初始化和释放
    bool initialize(const ModelConfig& yolov5_config, 
                   const ModelConfig* yolov8_config = nullptr);
    void release();
    
    // 原有推理接口 (保持完全兼容性)
    int inference(const cv::Mat& input_image, InferenceResultGroup& results);
    
    // 扩展推理接口 (新功能)
    int extendedInference(const cv::Mat& input_image, ExtendedInferenceResult& result);
    
    // 模型管理 (委托给InferenceManager)
    int setCurrentModel(ModelType modelType);
    ModelType getCurrentModel() const;
    bool isModelInitialized(ModelType type) const;
    
    // 人脸分析管理
    bool initializeFaceAnalysis(const std::string& modelPath);
    bool initializeInspireFace(AAssetManager* assetManager, const std::string& internalDataPath);
    void releaseFaceAnalysis();
    bool isFaceAnalysisEnabled() const { return m_faceAnalysisEnabled; }
    FaceAnalysisManager* getFaceAnalysisManager() { return m_faceAnalysisManager.get(); }
    
    // 统计管理
    bool initializeStatistics();
    void releaseStatistics();
    bool isStatisticsEnabled() const { return m_statisticsEnabled; }
    StatisticsManager* getStatisticsManager() { return m_statisticsManager.get(); }
    
    // 配置管理
    void setCascadeConfig(const CascadeDetectionConfig& config);
    CascadeDetectionConfig getCascadeConfig() const;
    
    // 状态查询
    bool isInitialized() const { return m_initialized; }
    bool isExtendedModeEnabled() const;
    
    // 性能监控
    PerformanceMonitor getPerformanceMonitor() const;
    void resetPerformanceMonitor();
    
    // 便捷方法
    StatisticsData getCurrentStatistics() const;
    void resetStatistics();
    std::string getStatisticsSummary() const;

private:
    // 级联检测实现
    int performCascadeDetection(const cv::Mat& input_image, ExtendedInferenceResult& result);
    
    // 人员检测筛选
    std::vector<InferenceResult> filterPersonDetections(
        const std::vector<InferenceResult>& allDetections) const;
    
    // 性能监控更新
    void updatePerformanceMonitor(bool success, 
                                 std::chrono::milliseconds inferenceTime,
                                 std::chrono::milliseconds faceAnalysisTime);
    
    // 日志和调试
    void logExtendedInferenceResult(const ExtendedInferenceResult& result) const;
    void logPerformanceStats() const;
    
    // 内部工具方法
    bool validateCascadeConfig() const;
    void initializeDefaultConfigs();
};

// 工具函数
namespace ExtendedInferenceUtils {
    // 结果转换
    InferenceResultGroup convertToInferenceResultGroup(const std::vector<Detection>& detections);
    std::vector<Detection> convertToDetections(const InferenceResultGroup& results);
    
    // 性能分析
    struct PerformanceReport {
        double averageInferenceTime;
        double averageFaceAnalysisTime;
        double successRate;
        int totalProcessedFrames;
        int totalDetectedPersons;
        int totalDetectedFaces;
        std::string summary;
    };
    
    PerformanceReport generatePerformanceReport(
        const ExtendedInferenceManager::PerformanceMonitor& monitor,
        const StatisticsData& statistics);
    
    // 配置验证
    bool validateExtendedConfig(const CascadeDetectionConfig& config);
    CascadeDetectionConfig getDefaultCascadeConfig();
    
    // 调试和可视化
    cv::Mat drawExtendedResults(const cv::Mat& image, 
                               const ExtendedInferenceResult& result);
    
    std::string formatExtendedResults(const ExtendedInferenceResult& result);
}

#endif // EXTENDED_INFERENCE_MANAGER_H
