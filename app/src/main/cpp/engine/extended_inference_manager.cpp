#include "extended_inference_manager.h"
#include "log4c.h"
#include <algorithm>

ExtendedInferenceManager::ExtendedInferenceManager()
    : m_initialized(false)
    , m_faceAnalysisEnabled(false)
    , m_statisticsEnabled(false) {
    
    // 创建核心推理管理器 (C++11兼容)
    m_inferenceManager.reset(new InferenceManager());
    
    // 初始化默认配置
    initializeDefaultConfigs();
    
    LOGI("ExtendedInferenceManager created");
}

ExtendedInferenceManager::~ExtendedInferenceManager() {
    release();
    LOGI("ExtendedInferenceManager destroyed");
}

bool ExtendedInferenceManager::initialize(const ModelConfig& yolov5_config,
                                         const ModelConfig* yolov8_config) {
    std::lock_guard<std::mutex> lock(m_mutex);

    if (m_initialized) {
        LOGW("ExtendedInferenceManager already initialized");
        return true;
    }

    LOGI("Initializing ExtendedInferenceManager");

    // 初始化核心推理管理器
    if (m_inferenceManager->initialize(yolov5_config, yolov8_config) != 0) {
        LOGE("Failed to initialize core InferenceManager");
        return false;
    }
    
    // 验证级联配置
    if (!validateCascadeConfig()) {
        LOGE("Invalid cascade configuration");
        return false;
    }
    
    m_initialized = true;
    LOGI("ExtendedInferenceManager initialized successfully");
    return true;
}

void ExtendedInferenceManager::release() {
    std::lock_guard<std::mutex> lock(m_mutex);
    
    if (!m_initialized) return;
    
    LOGI("Releasing ExtendedInferenceManager");
    
    // 释放扩展功能
    releaseFaceAnalysis();
    releaseStatistics();
    
    // 释放核心推理管理器
    if (m_inferenceManager) {
        m_inferenceManager->release();
    }
    
    m_initialized = false;
    LOGI("ExtendedInferenceManager released");
}

int ExtendedInferenceManager::inference(const cv::Mat& input_image, InferenceResultGroup& results) {
    // 保持完全向后兼容的原有接口
    if (!m_initialized || !m_inferenceManager) {
        LOGE("ExtendedInferenceManager not initialized");
        return -1;
    }
    
    return m_inferenceManager->inference(input_image, results);
}

int ExtendedInferenceManager::extendedInference(const cv::Mat& input_image, ExtendedInferenceResult& result) {
    std::lock_guard<std::mutex> lock(m_mutex);
    
    if (!m_initialized || !m_inferenceManager) {
        LOGE("ExtendedInferenceManager not initialized");
        return -1;
    }
    
    auto startTime = std::chrono::steady_clock::now();
    
    // 执行级联检测
    int ret = performCascadeDetection(input_image, result);
    
    auto endTime = std::chrono::steady_clock::now();
    result.performanceInfo.totalTime = 
        std::chrono::duration_cast<std::chrono::milliseconds>(endTime - startTime);
    
    // 更新性能监控
    updatePerformanceMonitor(ret == 0, result.performanceInfo.totalTime, 
                           result.performanceInfo.faceAnalysisTime);
    
    // 记录结果日志
    if (ret == 0) {
        logExtendedInferenceResult(result);
    }
    
    return ret;
}

int ExtendedInferenceManager::setCurrentModel(ModelType modelType) {
    if (!m_initialized || !m_inferenceManager) {
        LOGE("ExtendedInferenceManager not initialized");
        return -1;
    }

    return m_inferenceManager->setCurrentModel(modelType);
}

ModelType ExtendedInferenceManager::getCurrentModel() const {
    if (!m_initialized || !m_inferenceManager) {
        return ModelType::YOLOV5; // 默认返回YOLOv5
    }
    
    return m_inferenceManager->getCurrentModel();
}

bool ExtendedInferenceManager::isModelInitialized(ModelType type) const {
    if (!m_initialized || !m_inferenceManager) {
        return false;
    }

    return m_inferenceManager->isModelInitialized(type);
}

bool ExtendedInferenceManager::initializeFaceAnalysis(const std::string& modelPath) {
    std::lock_guard<std::mutex> lock(m_mutex);
    
    LOGI("Initializing face analysis with model: %s", modelPath.c_str());
    
    if (!m_faceAnalysisManager) {
        m_faceAnalysisManager.reset(new FaceAnalysisManager());
    }
    
    if (m_faceAnalysisManager->initialize(modelPath)) {
        m_faceAnalysisEnabled = true;
        LOGI("Face analysis initialized successfully");
        return true;
    } else {
        LOGE("Failed to initialize face analysis");
        return false;
    }
}

void ExtendedInferenceManager::releaseFaceAnalysis() {
    if (m_faceAnalysisManager) {
        m_faceAnalysisManager->release();
        m_faceAnalysisManager.reset();
    }
    m_faceAnalysisEnabled = false;
    LOGI("Face analysis released");
}

bool ExtendedInferenceManager::initializeInspireFace(AAssetManager* assetManager,
                                                    const std::string& internalDataPath) {
    std::lock_guard<std::mutex> lock(m_mutex);

    LOGI("Initializing InspireFace with internal path: %s", internalDataPath.c_str());

    if (!m_faceAnalysisManager) {
        m_faceAnalysisManager.reset(new FaceAnalysisManager());
    }

    if (m_faceAnalysisManager->initializeInspireFace(assetManager, internalDataPath)) {
        m_faceAnalysisEnabled = true;
        LOGI("InspireFace initialized successfully");
        return true;
    } else {
        LOGE("Failed to initialize InspireFace");
        return false;
    }
}

bool ExtendedInferenceManager::initializeStatistics() {
    std::lock_guard<std::mutex> lock(m_mutex);
    
    LOGI("Initializing statistics");
    
    if (!m_statisticsManager) {
        m_statisticsManager.reset(new StatisticsManager());
    }
    
    // 设置统计配置
    m_statisticsManager->setConfig(m_cascadeConfig.statisticsConfig);
    
    m_statisticsEnabled = true;
    LOGI("Statistics initialized successfully");
    return true;
}

void ExtendedInferenceManager::releaseStatistics() {
    if (m_statisticsManager) {
        m_statisticsManager.reset();
    }
    m_statisticsEnabled = false;
    LOGI("Statistics released");
}

void ExtendedInferenceManager::setCascadeConfig(const CascadeDetectionConfig& config) {
    std::lock_guard<std::mutex> lock(m_mutex);
    
    if (!config.isValid()) {
        LOGE("Invalid CascadeDetectionConfig provided");
        return;
    }
    
    m_cascadeConfig = config;
    
    // 更新子模块配置
    if (m_faceAnalysisManager) {
        m_faceAnalysisManager->setConfig(config.faceAnalysisConfig);
    }
    
    if (m_statisticsManager) {
        m_statisticsManager->setConfig(config.statisticsConfig);
    }
    
    LOGI("CascadeDetectionConfig updated");
}

CascadeDetectionConfig ExtendedInferenceManager::getCascadeConfig() const {
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_cascadeConfig;
}

bool ExtendedInferenceManager::isExtendedModeEnabled() const {
    return m_cascadeConfig.enableFaceAnalysis || m_cascadeConfig.enableStatistics;
}

ExtendedInferenceManager::PerformanceMonitor ExtendedInferenceManager::getPerformanceMonitor() const {
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_performanceMonitor;
}

void ExtendedInferenceManager::resetPerformanceMonitor() {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_performanceMonitor = PerformanceMonitor();
    LOGI("Performance monitor reset");
}

StatisticsData ExtendedInferenceManager::getCurrentStatistics() const {
    if (m_statisticsManager) {
        return m_statisticsManager->getCurrentStatistics();
    }
    return StatisticsData();
}

void ExtendedInferenceManager::resetStatistics() {
    if (m_statisticsManager) {
        m_statisticsManager->resetCurrentStatistics();
    }
}

std::string ExtendedInferenceManager::getStatisticsSummary() const {
    if (m_statisticsManager) {
        return m_statisticsManager->exportCurrentStatistics();
    }
    return "Statistics not available";
}

// ==================== 私有方法实现 ====================

int ExtendedInferenceManager::performCascadeDetection(const cv::Mat& input_image, 
                                                     ExtendedInferenceResult& result) {
    auto objectDetectionStart = std::chrono::steady_clock::now();
    
    // 1. 执行原有目标检测
    int ret = m_inferenceManager->inference(input_image, result.objectDetections);
    if (ret != 0) {
        LOGE("Object detection failed with code: %d", ret);
        return ret;
    }
    
    auto objectDetectionEnd = std::chrono::steady_clock::now();
    result.performanceInfo.objectDetectionTime = 
        std::chrono::duration_cast<std::chrono::milliseconds>(objectDetectionEnd - objectDetectionStart);
    
    // 2. 检查是否启用扩展功能
    if (!isExtendedModeEnabled()) {
        LOGD("Extended mode disabled, returning object detection results only");
        return 0;
    }
    
    // 3. 筛选人员检测结果
    auto personDetections = filterPersonDetections(result.objectDetections.results);
    result.performanceInfo.processedPersonCount = static_cast<int>(personDetections.size());
    
    if (personDetections.empty()) {
        LOGD("No person detections found, skipping face analysis");
        return 0;
    }
    
    LOGD("Found %zu person detections for face analysis", personDetections.size());
    
    // 4. 执行人脸分析
    if (m_cascadeConfig.enableFaceAnalysis && m_faceAnalysisEnabled && m_faceAnalysisManager) {
        auto faceAnalysisStart = std::chrono::steady_clock::now();
        
        bool faceAnalysisSuccess = m_faceAnalysisManager->analyzePersonRegions(
            input_image, personDetections, result.faceAnalysisResults);
        
        auto faceAnalysisEnd = std::chrono::steady_clock::now();
        result.performanceInfo.faceAnalysisTime = 
            std::chrono::duration_cast<std::chrono::milliseconds>(faceAnalysisEnd - faceAnalysisStart);
        
        if (!faceAnalysisSuccess) {
            LOGW("Face analysis failed, continuing with object detection only");
        } else {
            result.performanceInfo.detectedFaceCount = 0;
            for (const auto& faceResult : result.faceAnalysisResults) {
                result.performanceInfo.detectedFaceCount += static_cast<int>(faceResult.faces.size());
            }
        }
    }
    
    // 5. 更新统计数据
    if (m_cascadeConfig.enableStatistics && m_statisticsEnabled && m_statisticsManager) {
        m_statisticsManager->incrementFrameCount();
        
        if (!result.faceAnalysisResults.empty()) {
            m_statisticsManager->incrementAnalysisCount();
            m_statisticsManager->updateStatistics(result.faceAnalysisResults);
        }
        
        result.statistics = m_statisticsManager->getCurrentStatistics();
    }
    
    return 0;
}

std::vector<InferenceResult> ExtendedInferenceManager::filterPersonDetections(
    const std::vector<InferenceResult>& allDetections) const {
    
    std::vector<InferenceResult> personDetections;
    
    for (const auto& detection : allDetections) {
        if (detection.class_name == "person" && 
            detection.confidence >= m_cascadeConfig.personConfidenceThreshold) {
            
            // 检查人员区域大小
            float width = detection.x2 - detection.x1;
            float height = detection.y2 - detection.y1;
            
            if (width >= m_cascadeConfig.minPersonPixelSize && 
                height >= m_cascadeConfig.minPersonPixelSize) {
                personDetections.push_back(detection);
                
                // 限制最大人员数量
                if (personDetections.size() >= static_cast<size_t>(m_cascadeConfig.maxPersonsPerFrame)) {
                    break;
                }
            }
        }
    }
    
    return personDetections;
}

void ExtendedInferenceManager::updatePerformanceMonitor(bool success, 
                                                       std::chrono::milliseconds inferenceTime,
                                                       std::chrono::milliseconds faceAnalysisTime) {
    m_performanceMonitor.totalInferenceCount++;
    if (success) {
        m_performanceMonitor.successfulInferenceCount++;
    }
    m_performanceMonitor.totalInferenceTime += inferenceTime;
    m_performanceMonitor.totalFaceAnalysisTime += faceAnalysisTime;
    m_performanceMonitor.faceAnalysisCount++;
    m_performanceMonitor.lastInferenceTime = std::chrono::steady_clock::now();
}

void ExtendedInferenceManager::logExtendedInferenceResult(const ExtendedInferenceResult& result) const {
    LOGD("Extended inference result:");
    LOGD("  Object detections: %zu", result.objectDetections.results.size());
    LOGD("  Person count: %d", result.getPersonCount());
    LOGD("  Face analysis results: %zu", result.faceAnalysisResults.size());
    LOGD("  Valid faces: %d", result.getValidFaceCount());
    LOGD("  Performance: OD=%lld ms, FA=%lld ms, Total=%lld ms",
         result.performanceInfo.objectDetectionTime.count(),
         result.performanceInfo.faceAnalysisTime.count(),
         result.performanceInfo.totalTime.count());
    
    if (m_cascadeConfig.enableStatistics) {
        LOGD("  Statistics: %d persons, %d faces, %d male, %d female",
             result.statistics.totalPersonCount,
             result.statistics.totalFaceCount,
             result.statistics.maleCount,
             result.statistics.femaleCount);
    }
}

void ExtendedInferenceManager::logPerformanceStats() const {
    const auto& stats = m_performanceMonitor;
    LOGI("Extended Inference Performance Stats:");
    LOGI("  Total inferences: %d", stats.totalInferenceCount);
    LOGI("  Successful: %d (%.1f%%)", stats.successfulInferenceCount, stats.getSuccessRate());
    LOGI("  Average inference time: %.1f ms", stats.getAverageInferenceTime());
    LOGI("  Average face analysis time: %.1f ms", stats.getAverageFaceAnalysisTime());
    LOGI("  Face analysis count: %d", stats.faceAnalysisCount);
}

bool ExtendedInferenceManager::validateCascadeConfig() const {
    if (!m_cascadeConfig.isValid()) {
        LOGE("Invalid cascade configuration");
        return false;
    }
    
    if (m_cascadeConfig.enableFaceAnalysis && !m_cascadeConfig.faceAnalysisConfig.isValid()) {
        LOGE("Invalid face analysis configuration");
        return false;
    }
    
    if (m_cascadeConfig.enableStatistics && !m_cascadeConfig.statisticsConfig.isValid()) {
        LOGE("Invalid statistics configuration");
        return false;
    }
    
    return true;
}

void ExtendedInferenceManager::initializeDefaultConfigs() {
    // 初始化默认级联配置
    m_cascadeConfig.enableFaceAnalysis = false;
    m_cascadeConfig.enableStatistics = false;
    m_cascadeConfig.enablePersonTracking = false;
    m_cascadeConfig.personConfidenceThreshold = 0.5f;
    m_cascadeConfig.minPersonPixelSize = 50;
    m_cascadeConfig.maxPersonsPerFrame = 10;
    
    // 初始化默认人脸分析配置
    m_cascadeConfig.faceAnalysisConfig = FaceAnalysisConfig();
    
    // 初始化默认统计配置
    m_cascadeConfig.statisticsConfig = StatisticsConfig();
    
    LOGD("Default configurations initialized");
}

// ==================== 工具函数实现 ====================

namespace ExtendedInferenceUtils {

InferenceResultGroup convertToInferenceResultGroup(const std::vector<Detection>& detections) {
    InferenceResultGroup group;
    group.results.reserve(detections.size());
    
    for (const auto& detection : detections) {
        InferenceResult result;
        result.class_name = detection.className;
        result.confidence = detection.confidence;
        result.x1 = static_cast<float>(detection.box.x);
        result.y1 = static_cast<float>(detection.box.y);
        result.x2 = static_cast<float>(detection.box.x + detection.box.width);
        result.y2 = static_cast<float>(detection.box.y + detection.box.height);

        group.results.push_back(result);
    }
    
    return group;
}

std::vector<Detection> convertToDetections(const InferenceResultGroup& results) {
    std::vector<Detection> detections;
    detections.reserve(results.results.size());
    
    for (const auto& result : results.results) {
        Detection detection;
        detection.className = result.class_name;
        detection.confidence = result.confidence;
        detection.class_id = result.class_id;

        // 转换坐标格式：从x1,y1,x2,y2到cv::Rect
        detection.box = cv::Rect(
            static_cast<int>(result.x1),
            static_cast<int>(result.y1),
            static_cast<int>(result.x2 - result.x1),
            static_cast<int>(result.y2 - result.y1)
        );

        detections.push_back(detection);
    }
    
    return detections;
}

PerformanceReport generatePerformanceReport(
    const ExtendedInferenceManager::PerformanceMonitor& monitor,
    const StatisticsData& statistics) {
    
    PerformanceReport report;
    report.averageInferenceTime = monitor.getAverageInferenceTime();
    report.averageFaceAnalysisTime = monitor.getAverageFaceAnalysisTime();
    report.successRate = monitor.getSuccessRate();
    report.totalProcessedFrames = monitor.totalInferenceCount;
    report.totalDetectedPersons = statistics.totalPersonCount;
    report.totalDetectedFaces = statistics.totalFaceCount;
    
    std::stringstream ss;
    ss << "Performance Report:\n";
    ss << "  Processed frames: " << report.totalProcessedFrames << "\n";
    ss << "  Success rate: " << std::fixed << std::setprecision(1) << report.successRate << "%\n";
    ss << "  Avg inference time: " << std::fixed << std::setprecision(1) << report.averageInferenceTime << " ms\n";
    ss << "  Avg face analysis time: " << std::fixed << std::setprecision(1) << report.averageFaceAnalysisTime << " ms\n";
    ss << "  Total persons detected: " << report.totalDetectedPersons << "\n";
    ss << "  Total faces detected: " << report.totalDetectedFaces << "\n";
    
    report.summary = ss.str();
    return report;
}

bool validateExtendedConfig(const CascadeDetectionConfig& config) {
    return config.isValid();
}

CascadeDetectionConfig getDefaultCascadeConfig() {
    CascadeDetectionConfig config;
    config.enableFaceAnalysis = false;
    config.enableStatistics = false;
    config.enablePersonTracking = false;
    config.personConfidenceThreshold = 0.5f;
    config.minPersonPixelSize = 50;
    config.maxPersonsPerFrame = 10;
    config.faceAnalysisConfig = FaceAnalysisConfig();
    config.statisticsConfig = StatisticsConfig();
    return config;
}

cv::Mat drawExtendedResults(const cv::Mat& image, const ExtendedInferenceResult& result) {
    cv::Mat resultImage = image.clone();
    
    // 绘制目标检测结果
    for (const auto& detection : result.objectDetections.results) {
        cv::Rect rect(static_cast<int>(detection.x1), static_cast<int>(detection.y1),
                     static_cast<int>(detection.x2 - detection.x1),
                     static_cast<int>(detection.y2 - detection.y1));
        
        cv::Scalar color = (detection.class_name == "person") ? cv::Scalar(0, 255, 0) : cv::Scalar(255, 0, 0);
        cv::rectangle(resultImage, rect, color, 2);
        
        std::string label = detection.class_name + " " + std::to_string(static_cast<int>(detection.confidence * 100)) + "%";
        cv::putText(resultImage, label, cv::Point(rect.x, rect.y - 5),
                   cv::FONT_HERSHEY_SIMPLEX, 0.5, color, 1);
    }
    
    // 绘制人脸分析结果
    for (const auto& faceResult : result.faceAnalysisResults) {
        for (const auto& face : faceResult.faces) {
            cv::rectangle(resultImage, face.faceRect, cv::Scalar(255, 255, 0), 1);
            
            if (face.attributes.isValid()) {
                std::string text = face.attributes.getGenderString() + " " + 
                                 face.attributes.getAgeBracketString();
                cv::putText(resultImage, text, 
                           cv::Point(face.faceRect.x, face.faceRect.y - 5),
                           cv::FONT_HERSHEY_SIMPLEX, 0.4, cv::Scalar(255, 255, 0), 1);
            }
        }
    }
    
    return resultImage;
}

// 带类别过滤的绘制版本
cv::Mat drawExtendedResults(const cv::Mat& image, 
                           const ExtendedInferenceResult& result,
                           const std::set<std::string>& enabledClasses) {
    cv::Mat resultImage = image.clone();
    
    // 绘制目标检测结果（只绘制启用的类别）
    for (const auto& detection : result.objectDetections.results) {
        // 检查类别是否启用
        if (enabledClasses.find(detection.class_name) == enabledClasses.end()) {
            continue; // 跳过未启用的类别
        }
        
        cv::Rect rect(static_cast<int>(detection.x1), static_cast<int>(detection.y1),
                     static_cast<int>(detection.x2 - detection.x1),
                     static_cast<int>(detection.y2 - detection.y1));
        
        cv::Scalar color = (detection.class_name == "person") ? cv::Scalar(0, 255, 0) : cv::Scalar(255, 0, 0);
        cv::rectangle(resultImage, rect, color, 2);
        
        std::string label = detection.class_name + " " + std::to_string(static_cast<int>(detection.confidence * 100)) + "%";
        cv::putText(resultImage, label, cv::Point(rect.x, rect.y - 5),
                   cv::FONT_HERSHEY_SIMPLEX, 0.5, color, 1);
    }
    
    // 绘制人脸分析结果（只有当person类别启用时才绘制）
    if (enabledClasses.find("person") != enabledClasses.end()) {
        for (const auto& faceResult : result.faceAnalysisResults) {
            for (const auto& face : faceResult.faces) {
                cv::rectangle(resultImage, face.faceRect, cv::Scalar(255, 255, 0), 1);
                
                if (face.attributes.isValid()) {
                    std::string text = face.attributes.getGenderString() + " " + 
                                     face.attributes.getAgeBracketString();
                    cv::putText(resultImage, text, 
                               cv::Point(face.faceRect.x, face.faceRect.y - 5),
                               cv::FONT_HERSHEY_SIMPLEX, 0.4, cv::Scalar(255, 255, 0), 1);
                }
            }
        }
    }
    
    return resultImage;
}

std::string formatExtendedResults(const ExtendedInferenceResult& result) {
    std::stringstream ss;
    
    ss << "Extended Inference Results:\n";
    ss << "  Object detections: " << result.objectDetections.results.size() << "\n";
    ss << "  Person count: " << result.getPersonCount() << "\n";
    ss << "  Face analysis results: " << result.faceAnalysisResults.size() << "\n";
    ss << "  Valid faces: " << result.getValidFaceCount() << "\n";
    ss << "  Processing time: " << result.performanceInfo.totalTime.count() << " ms\n";
    
    if (result.statistics.totalPersonCount > 0) {
        ss << "  Statistics: " << result.statistics.maleCount << " male, " 
           << result.statistics.femaleCount << " female\n";
    }
    
    return ss.str();
}

} // namespace ExtendedInferenceUtils
