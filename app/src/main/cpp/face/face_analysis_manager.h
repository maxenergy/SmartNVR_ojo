#ifndef FACE_ANALYSIS_MANAGER_H
#define FACE_ANALYSIS_MANAGER_H

#include <opencv2/opencv.hpp>
#include <vector>
#include <memory>
#include <mutex>
#include <string>
#include <chrono>
#include <jni.h>
#include <android/asset_manager.h>
#include "types/model_config.h"
#include "types/yolo_datatype.h"
#include "inspireface_wrapper.h"

// 前向声明
struct Detection;
class InspireFaceSession;
class InspireFaceImageProcessor;
class InspireFaceDetector;

// 人脸信息结构
struct FaceInfo {
    cv::Rect faceRect;              // 人脸边界框 (相对于原图)
    float confidence;               // 检测置信度
    
    // 属性信息
    struct Attributes {
        int gender;                 // 0: 女性, 1: 男性, -1: 未知
        float genderConfidence;
        
        int ageBracket;             // 年龄段 0-8, -1: 未知
        float ageConfidence;
        
        int race;                   // 种族 0-4, -1: 未知
        float raceConfidence;
        
        bool isValid() const {
            return gender >= 0 && ageBracket >= 0;
        }
        
        std::string getGenderString() const {
            switch (gender) {
                case 0: return "女性";
                case 1: return "男性";
                default: return "未知";
            }
        }
        
        std::string getAgeBracketString() const {
            const char* ageLabels[] = {
                "0-2岁", "3-9岁", "10-19岁", "20-29岁", "30-39岁",
                "40-49岁", "50-59岁", "60-69岁", "70岁以上"
            };
            if (ageBracket >= 0 && ageBracket < 9) {
                return ageLabels[ageBracket];
            }
            return "未知";
        }
        
        std::string getRaceString() const {
            const char* raceLabels[] = {
                "黑人", "亚洲人", "拉丁裔", "中东人", "白人"
            };
            if (race >= 0 && race < 5) {
                return raceLabels[race];
            }
            return "未知";
        }
    } attributes;
    
    // 关键点 (可选)
    std::vector<cv::Point2f> landmarks;
    
    FaceInfo() : confidence(0.0f) {
        attributes.gender = -1;
        attributes.genderConfidence = 0.0f;
        attributes.ageBracket = -1;
        attributes.ageConfidence = 0.0f;
        attributes.race = -1;
        attributes.raceConfidence = 0.0f;
    }
};

// 人脸分析结果
struct FaceAnalysisResult {
    int personId;                   // 关联的人员ID
    InferenceResult personDetection; // 人员检测结果
    std::vector<FaceInfo> faces;    // 检测到的人脸
    
    bool hasValidFaces() const {
        return !faces.empty() && 
               std::any_of(faces.begin(), faces.end(),
                          [](const FaceInfo& f) { return f.attributes.isValid(); });
    }
    
    FaceInfo getBestFace() const {
        if (faces.empty()) return FaceInfo();
        return *std::max_element(faces.begin(), faces.end(),
                                [](const FaceInfo& a, const FaceInfo& b) {
                                    return a.confidence < b.confidence;
                                });
    }
    
    int getValidFaceCount() const {
        return std::count_if(faces.begin(), faces.end(),
                            [](const FaceInfo& f) { return f.attributes.isValid(); });
    }
};

// 人脸分析配置
struct FaceAnalysisConfig {
    bool enableGenderDetection = true;
    bool enableAgeDetection = true;
    bool enableRaceDetection = false;
    
    float faceDetectionThreshold = 0.5f;
    int maxFacesPerPerson = 1;
    int minFacePixelSize = 30;
    
    // 性能配置
    int maxConcurrentAnalysis = 2;
    int analysisInterval = 3;           // 每N帧分析一次
    
    // ROI扩展配置
    float roiExpandRatio = 0.1f;        // ROI区域扩展比例
    
    bool isValid() const {
        return faceDetectionThreshold > 0.0f && 
               maxFacesPerPerson > 0 && 
               minFacePixelSize > 0;
    }
};

// 人脸分析管理器
class FaceAnalysisManager {
private:
    bool m_initialized;
    bool m_inspireFaceInitialized;
    mutable std::mutex m_mutex;
    
    FaceAnalysisConfig m_config;
    
    // 性能统计
    struct PerformanceStats {
        int totalAnalysisCount = 0;
        int successfulAnalysisCount = 0;
        std::chrono::milliseconds totalProcessingTime{0};
        std::chrono::steady_clock::time_point lastAnalysisTime;
        
        double getAverageProcessingTime() const {
            if (totalAnalysisCount == 0) return 0.0;
            return static_cast<double>(totalProcessingTime.count()) / totalAnalysisCount;
        }
        
        double getSuccessRate() const {
            if (totalAnalysisCount == 0) return 0.0;
            return static_cast<double>(successfulAnalysisCount) / totalAnalysisCount * 100.0;
        }
    } m_performanceStats;
    
    // 帧计数器 (用于控制分析频率)
    int m_frameCounter = 0;
    
    // JNI相关
    jobject m_javaInspireFaceInstance = nullptr;
    jmethodID m_createImageStreamMethod = nullptr;
    jmethodID m_executeFaceTrackMethod = nullptr;
    jmethodID m_pipelineProcessMethod = nullptr;
    jmethodID m_getFaceAttributeMethod = nullptr;
    jmethodID m_releaseImageStreamMethod = nullptr;

    // InspireFace组件
    std::unique_ptr<InspireFaceSession> m_inspireFaceSession;
    std::unique_ptr<InspireFaceImageProcessor> m_imageProcessor;
    std::unique_ptr<InspireFaceDetector> m_faceDetector;

public:
    FaceAnalysisManager();
    ~FaceAnalysisManager();
    
    // 初始化和释放
    bool initialize(const std::string& modelPath);
    bool initializeInspireFace(AAssetManager* assetManager, const std::string& internalDataPath);
    void release();
    
    // 核心功能
    bool analyzePersonRegions(const cv::Mat& image, 
                             const std::vector<InferenceResult>& personDetections,
                             std::vector<FaceAnalysisResult>& results);

    
    // 简化的人脸分析接口 (用于JNI调用)
    struct PersonDetection {
        float x1, y1, x2, y2;
        float confidence;
    };
    
    struct SimpleFaceAnalysisResult {
        bool success = false;
        std::string errorMessage;
        int faceCount = 0;
        int maleCount = 0;
        int femaleCount = 0;
        int ageGroups[9] = {0}; // 年龄组分布
        
        struct Face {
            float x1, y1, x2, y2;
            float confidence;
            int gender; // 0: 女性, 1: 男性
            int age;    // 年龄段 0-8
        };
        std::vector<Face> faces;
    };
    
    bool analyzeFaces(const cv::Mat& image, 
                     const std::vector<PersonDetection>& personDetections,
                     SimpleFaceAnalysisResult& result);
    
    // 配置管理
    void setConfig(const FaceAnalysisConfig& config);
    FaceAnalysisConfig getConfig() const;
    
    // 状态查询
    bool isInitialized() const { return m_initialized; }
    bool isInspireFaceReady() const { return m_inspireFaceInitialized; }
    
    // 性能统计
    PerformanceStats getPerformanceStats() const;
    void resetPerformanceStats();
    
    // 帧控制
    bool shouldAnalyzeCurrentFrame();
    void incrementFrameCounter() { m_frameCounter++; }

private:
    // 内部实现方法
    bool initializeInspireFace(const std::string& modelPath);
    bool initializeJNIMethods();
    void releaseJNIResources();
    
    bool analyzePersonROI(const cv::Mat& image,
                         const cv::Rect& personRect,
                         FaceAnalysisResult& result);
    
    cv::Rect expandROI(const cv::Rect& originalROI, 
                      const cv::Size& imageSize) const;
    
    bool convertMatToInspireFaceFormat(const cv::Mat& image, 
                                      jobject& imageStream);
    
    bool extractFaceAttributesFromJava(jobject faceAttributeResult,
                                      std::vector<FaceInfo>& faces);
    
    void updatePerformanceStats(bool success, 
                               std::chrono::milliseconds processingTime);
    
    // 日志和调试
    void logAnalysisResult(const FaceAnalysisResult& result) const;
    void logPerformanceStats() const;
};

// 工具函数
namespace FaceAnalysisUtils {
    // 人员检测结果筛选
    std::vector<InferenceResult> filterPersonDetections(
        const std::vector<InferenceResult>& allDetections,
        float confidenceThreshold = 0.5f,
        int minPixelSize = 50);
    
    // ROI有效性检查
    bool isValidPersonROI(const cv::Rect& roi, const cv::Size& imageSize);
    
    // 人脸区域坐标转换 (ROI坐标 -> 原图坐标)
    cv::Rect convertFaceRectToImageCoords(const cv::Rect& faceRect,
                                         const cv::Rect& personROI);
    
    // 性别年龄统计辅助函数
    std::pair<int, int> countGenderDistribution(
        const std::vector<FaceAnalysisResult>& results);
    
    std::vector<int> countAgeBracketDistribution(
        const std::vector<FaceAnalysisResult>& results);
    
    // 调试和可视化
    cv::Mat drawFaceAnalysisResults(const cv::Mat& image,
                                   const std::vector<FaceAnalysisResult>& results);
}

#endif // FACE_ANALYSIS_MANAGER_H
