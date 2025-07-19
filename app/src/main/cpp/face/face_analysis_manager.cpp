#include "face_analysis_manager.h"
#include "inspireface_wrapper.h"
#include "log4c.h"
#include <chrono>

FaceAnalysisManager::FaceAnalysisManager()
    : m_initialized(false)
    , m_inspireFaceInitialized(false)
    , m_frameCounter(0)
    , m_javaInspireFaceInstance(nullptr)
    , m_createImageStreamMethod(nullptr)
    , m_executeFaceTrackMethod(nullptr)
    , m_pipelineProcessMethod(nullptr)
    , m_getFaceAttributeMethod(nullptr)
    , m_releaseImageStreamMethod(nullptr) {

    // 创建InspireFace组件
    m_inspireFaceSession.reset(new InspireFaceSession());
    m_imageProcessor.reset(new InspireFaceImageProcessor());
    m_faceDetector.reset(new InspireFaceDetector());

    LOGI("FaceAnalysisManager created");
}

FaceAnalysisManager::~FaceAnalysisManager() {
    release();
    LOGI("FaceAnalysisManager destroyed");
}

bool FaceAnalysisManager::initialize(const std::string& modelPath) {
    std::lock_guard<std::mutex> lock(m_mutex);
    
    if (m_initialized) {
        LOGW("FaceAnalysisManager already initialized");
        return true;
    }
    
    LOGI("Initializing FaceAnalysisManager with model: %s", modelPath.c_str());
    
    // 初始化默认配置
    m_config = FaceAnalysisConfig();
    
    // 重置性能统计
    m_performanceStats = PerformanceStats();
    
    // 初始化InspireFace (暂时标记为成功，实际实现需要JNI调用)
    m_inspireFaceInitialized = initializeInspireFace(modelPath);
    if (!m_inspireFaceInitialized) {
        LOGE("Failed to initialize InspireFace");
        return false;
    }
    
    m_initialized = true;
    LOGI("FaceAnalysisManager initialized successfully");
    return true;
}

void FaceAnalysisManager::release() {
    std::lock_guard<std::mutex> lock(m_mutex);
    
    if (!m_initialized) return;
    
    LOGI("Releasing FaceAnalysisManager");
    
    // 释放JNI资源
    releaseJNIResources();
    
    // 重置状态
    m_initialized = false;
    m_inspireFaceInitialized = false;
    m_frameCounter = 0;
    
    LOGI("FaceAnalysisManager released");
}

bool FaceAnalysisManager::analyzePersonRegions(const cv::Mat& image,
                                              const std::vector<InferenceResult>& personDetections,
                                              std::vector<FaceAnalysisResult>& results) {
    std::lock_guard<std::mutex> lock(m_mutex);
    
    if (!m_initialized || !m_inspireFaceInitialized) {
        LOGW("FaceAnalysisManager not initialized");
        return false;
    }
    
    // 检查是否应该分析当前帧
    if (!shouldAnalyzeCurrentFrame()) {
        return true; // 跳过分析但返回成功
    }
    
    auto startTime = std::chrono::steady_clock::now();
    
    results.clear();
    results.reserve(personDetections.size());
    
    LOGD("Analyzing %zu person regions", personDetections.size());
    
    bool overallSuccess = true;
    int successCount = 0;
    
    for (size_t i = 0; i < personDetections.size() && i < static_cast<size_t>(m_config.maxConcurrentAnalysis); ++i) {
        const auto& person = personDetections[i];
        
        // 验证人员检测结果
        if (person.class_name != "person") continue;
        
        // 计算人员区域
        cv::Rect personRect(
            static_cast<int>(person.x1),
            static_cast<int>(person.y1),
            static_cast<int>(person.x2 - person.x1),
            static_cast<int>(person.y2 - person.y1)
        );
        
        // 扩展ROI区域
        personRect = expandROI(personRect, image.size());
        
        // 边界检查
        personRect &= cv::Rect(0, 0, image.cols, image.rows);
        if (personRect.width < m_config.minFacePixelSize || 
            personRect.height < m_config.minFacePixelSize) {
            LOGD("Person region too small, skipping");
            continue;
        }
        
        // 创建人脸分析结果
        FaceAnalysisResult faceResult;
        faceResult.personId = static_cast<int>(i);
        faceResult.personDetection = person;
        
        // 执行人脸分析
        if (analyzePersonROI(image, personRect, faceResult)) {
            results.push_back(std::move(faceResult));
            successCount++;
        } else {
            overallSuccess = false;
            LOGW("Failed to analyze person region %zu", i);
        }
    }
    
    auto endTime = std::chrono::steady_clock::now();
    auto processingTime = std::chrono::duration_cast<std::chrono::milliseconds>(endTime - startTime);
    
    // 更新性能统计
    updatePerformanceStats(overallSuccess, processingTime);
    
    LOGD("Face analysis completed: %d/%zu successful, %lld ms", 
         successCount, personDetections.size(), processingTime.count());
    
    // 记录分析结果
    if (!results.empty()) {
        logAnalysisResult(results[0]); // 记录第一个结果作为示例
    }
    
    return overallSuccess;
}

void FaceAnalysisManager::setConfig(const FaceAnalysisConfig& config) {
    std::lock_guard<std::mutex> lock(m_mutex);
    
    if (!config.isValid()) {
        LOGE("Invalid FaceAnalysisConfig provided");
        return;
    }
    
    m_config = config;
    LOGI("FaceAnalysisConfig updated");
}

FaceAnalysisConfig FaceAnalysisManager::getConfig() const {
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_config;
}

FaceAnalysisManager::PerformanceStats FaceAnalysisManager::getPerformanceStats() const {
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_performanceStats;
}

void FaceAnalysisManager::resetPerformanceStats() {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_performanceStats = PerformanceStats();
    LOGI("Performance stats reset");
}

bool FaceAnalysisManager::shouldAnalyzeCurrentFrame() {
    m_frameCounter++;
    return (m_frameCounter % m_config.analysisInterval) == 0;
}

// ==================== 私有方法实现 ====================

bool FaceAnalysisManager::initializeInspireFace(const std::string& modelPath) {
    LOGI("Initializing InspireFace with model: %s", modelPath.c_str());

    // 初始化InspireFace库
    if (!InspireFaceUtils::initializeLibrary()) {
        LOGE("Failed to initialize InspireFace library");
        return false;
    }

    // 检查模型文件
    if (!InspireFaceUtils::checkModelFiles(modelPath)) {
        LOGE("Model files not found at: %s", modelPath.c_str());
        return false;
    }

    // 初始化会话
    if (!m_inspireFaceSession->initialize(modelPath, true)) {
        LOGE("Failed to initialize InspireFace session");
        return false;
    }

    // 初始化检测器
    if (!m_faceDetector->initialize(m_inspireFaceSession.get())) {
        LOGE("Failed to initialize face detector");
        return false;
    }

    LOGI("InspireFace initialized successfully");
    return true;
}

bool FaceAnalysisManager::initializeJNIMethods() {
    // TODO: 初始化JNI方法ID
    // 获取InspireFace相关的方法ID并缓存
    LOGD("JNI methods initialization placeholder");
    return true;
}

void FaceAnalysisManager::releaseJNIResources() {
    // TODO: 释放JNI资源
    // 释放全局引用和其他JNI资源
    LOGD("JNI resources release placeholder");
    
    m_javaInspireFaceInstance = nullptr;
    m_createImageStreamMethod = nullptr;
    m_executeFaceTrackMethod = nullptr;
    m_pipelineProcessMethod = nullptr;
    m_getFaceAttributeMethod = nullptr;
    m_releaseImageStreamMethod = nullptr;
}

bool FaceAnalysisManager::analyzePersonROI(const cv::Mat& image,
                                          const cv::Rect& personRect,
                                          FaceAnalysisResult& result) {
    LOGD("Analyzing person ROI: [%d,%d,%d,%d]",
         personRect.x, personRect.y, personRect.width, personRect.height);

    // 提取人员区域
    cv::Mat personROI = image(personRect);

    // 1. 创建图像流
    void* imageStream = nullptr;
    if (!m_imageProcessor->createImageStreamFromMat(personROI, &imageStream)) {
        LOGE("Failed to create image stream from person ROI");
        return false;
    }

    // 2. 执行人脸检测和属性分析
    std::vector<FaceDetectionResult> faceResults;
    std::vector<FaceAttributeResult> attributeResults;

    bool success = m_faceDetector->detectAndAnalyze(imageStream, faceResults, attributeResults);

    // 3. 释放图像流
    m_imageProcessor->releaseImageStream(imageStream);

    if (!success) {
        LOGE("Face detection and analysis failed");
        return false;
    }

    // 4. 转换结果格式
    result.faces.clear();
    for (size_t i = 0; i < faceResults.size() && i < attributeResults.size(); ++i) {
        FaceInfo faceInfo;

        // 转换人脸位置（相对于原图像的坐标）
        faceInfo.faceRect = cv::Rect(
            personRect.x + faceResults[i].faceRect.x,
            personRect.y + faceResults[i].faceRect.y,
            faceResults[i].faceRect.width,
            faceResults[i].faceRect.height
        );
        faceInfo.confidence = faceResults[i].confidence;

        // 转换属性信息
        faceInfo.attributes.gender = attributeResults[i].gender;
        faceInfo.attributes.genderConfidence = attributeResults[i].genderConfidence;
        faceInfo.attributes.ageBracket = attributeResults[i].ageBracket;
        faceInfo.attributes.ageConfidence = attributeResults[i].ageConfidence;
        faceInfo.attributes.race = attributeResults[i].race;
        faceInfo.attributes.raceConfidence = attributeResults[i].raceConfidence;

        result.faces.push_back(faceInfo);

        LOGD("Face %zu: %s, %s, conf=%.2f", i,
             attributeResults[i].getGenderString().c_str(),
             attributeResults[i].getAgeBracketString().c_str(),
             faceInfo.confidence);
    }

    LOGD("Person ROI analysis completed with %zu faces", result.faces.size());
    return true;
}

cv::Rect FaceAnalysisManager::expandROI(const cv::Rect& originalROI, 
                                       const cv::Size& imageSize) const {
    float expandRatio = m_config.roiExpandRatio;
    
    int expandX = static_cast<int>(originalROI.width * expandRatio);
    int expandY = static_cast<int>(originalROI.height * expandRatio);
    
    cv::Rect expandedROI(
        originalROI.x - expandX,
        originalROI.y - expandY,
        originalROI.width + 2 * expandX,
        originalROI.height + 2 * expandY
    );
    
    // 确保在图像边界内
    expandedROI &= cv::Rect(0, 0, imageSize.width, imageSize.height);
    
    return expandedROI;
}

bool FaceAnalysisManager::convertMatToInspireFaceFormat(const cv::Mat& image, 
                                                       jobject& imageStream) {
    // TODO: 实现cv::Mat到InspireFace格式的转换
    LOGD("Converting cv::Mat to InspireFace format placeholder");
    return true;
}

bool FaceAnalysisManager::extractFaceAttributesFromJava(jobject faceAttributeResult,
                                                       std::vector<FaceInfo>& faces) {
    // TODO: 从Java对象提取人脸属性信息
    LOGD("Extracting face attributes from Java placeholder");
    return true;
}

void FaceAnalysisManager::updatePerformanceStats(bool success, 
                                                 std::chrono::milliseconds processingTime) {
    m_performanceStats.totalAnalysisCount++;
    if (success) {
        m_performanceStats.successfulAnalysisCount++;
    }
    m_performanceStats.totalProcessingTime += processingTime;
    m_performanceStats.lastAnalysisTime = std::chrono::steady_clock::now();
}

void FaceAnalysisManager::logAnalysisResult(const FaceAnalysisResult& result) const {
    LOGD("Face analysis result for person %d:", result.personId);
    LOGD("  Person detection: [%.1f,%.1f,%.1f,%.1f] conf=%.2f", 
         result.personDetection.x1, result.personDetection.y1,
         result.personDetection.x2, result.personDetection.y2,
         result.personDetection.confidence);
    
    for (size_t i = 0; i < result.faces.size(); ++i) {
        const auto& face = result.faces[i];
        LOGD("  Face %zu: [%d,%d,%d,%d] conf=%.2f", i,
             face.faceRect.x, face.faceRect.y, 
             face.faceRect.width, face.faceRect.height,
             face.confidence);
        
        if (face.attributes.isValid()) {
            LOGD("    Gender: %s (%.2f)", face.attributes.getGenderString().c_str(),
                 face.attributes.genderConfidence);
            LOGD("    Age: %s (%.2f)", face.attributes.getAgeBracketString().c_str(),
                 face.attributes.ageConfidence);
        }
    }
}

void FaceAnalysisManager::logPerformanceStats() const {
    const auto& stats = m_performanceStats;
    LOGI("Face Analysis Performance Stats:");
    LOGI("  Total analyses: %d", stats.totalAnalysisCount);
    LOGI("  Successful: %d (%.1f%%)", stats.successfulAnalysisCount,
         stats.getSuccessRate());
    LOGI("  Average time: %.1f ms", stats.getAverageProcessingTime());
}

// ==================== 工具函数实现 ====================

namespace FaceAnalysisUtils {

std::vector<InferenceResult> filterPersonDetections(
    const std::vector<InferenceResult>& allDetections,
    float confidenceThreshold,
    int minPixelSize) {
    
    std::vector<InferenceResult> personDetections;
    
    for (const auto& detection : allDetections) {
        if (detection.class_name == "person" && 
            detection.confidence >= confidenceThreshold) {
            
            float width = detection.x2 - detection.x1;
            float height = detection.y2 - detection.y1;
            
            if (width >= minPixelSize && height >= minPixelSize) {
                personDetections.push_back(detection);
            }
        }
    }
    
    return personDetections;
}

bool isValidPersonROI(const cv::Rect& roi, const cv::Size& imageSize) {
    return roi.x >= 0 && roi.y >= 0 && 
           roi.x + roi.width <= imageSize.width &&
           roi.y + roi.height <= imageSize.height &&
           roi.width > 0 && roi.height > 0;
}

cv::Rect convertFaceRectToImageCoords(const cv::Rect& faceRect,
                                     const cv::Rect& personROI) {
    return cv::Rect(
        personROI.x + faceRect.x,
        personROI.y + faceRect.y,
        faceRect.width,
        faceRect.height
    );
}

std::pair<int, int> countGenderDistribution(
    const std::vector<FaceAnalysisResult>& results) {
    
    int maleCount = 0, femaleCount = 0;
    
    for (const auto& result : results) {
        for (const auto& face : result.faces) {
            if (face.attributes.gender == 1) maleCount++;
            else if (face.attributes.gender == 0) femaleCount++;
        }
    }
    
    return {maleCount, femaleCount};
}

std::vector<int> countAgeBracketDistribution(
    const std::vector<FaceAnalysisResult>& results) {
    
    std::vector<int> ageCounts(9, 0);
    
    for (const auto& result : results) {
        for (const auto& face : result.faces) {
            if (face.attributes.ageBracket >= 0 && face.attributes.ageBracket < 9) {
                ageCounts[face.attributes.ageBracket]++;
            }
        }
    }
    
    return ageCounts;
}

cv::Mat drawFaceAnalysisResults(const cv::Mat& image,
                               const std::vector<FaceAnalysisResult>& results) {
    cv::Mat resultImage = image.clone();
    
    for (const auto& result : results) {
        // 绘制人员边界框
        cv::Rect personRect(
            static_cast<int>(result.personDetection.x1),
            static_cast<int>(result.personDetection.y1),
            static_cast<int>(result.personDetection.x2 - result.personDetection.x1),
            static_cast<int>(result.personDetection.y2 - result.personDetection.y1)
        );
        cv::rectangle(resultImage, personRect, cv::Scalar(0, 255, 0), 2);
        
        // 绘制人脸信息
        for (const auto& face : result.faces) {
            cv::Rect faceRect = convertFaceRectToImageCoords(face.faceRect, personRect);
            cv::rectangle(resultImage, faceRect, cv::Scalar(255, 0, 0), 1);
            
            if (face.attributes.isValid()) {
                std::string text = face.attributes.getGenderString() + " " + 
                                 face.attributes.getAgeBracketString();
                cv::putText(resultImage, text, 
                           cv::Point(faceRect.x, faceRect.y - 5),
                           cv::FONT_HERSHEY_SIMPLEX, 0.5, cv::Scalar(255, 255, 0), 1);
            }
        }
    }
    
    return resultImage;
}

} // namespace FaceAnalysisUtils
