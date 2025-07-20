#include "inspireface_wrapper.h"
#include "log4c.h"
#include <cstring>
#include <fstream>
#include <vector>

// 引入InspireFace C API
extern "C" {
#include "inspireface.h"
#include "intypedef.h"
}

// 全局库初始化状态
static bool g_inspireFaceLibraryInitialized = false;

// ==================== InspireFaceSession 实现 ====================

InspireFaceSession::InspireFaceSession() 
    : m_session(nullptr)
    , m_initialized(false) {
}

InspireFaceSession::~InspireFaceSession() {
    release();
}

bool InspireFaceSession::initialize(AAssetManager* assetManager, const std::string& internalDataPath,
                                   bool enableFaceAttribute) {
    if (m_initialized) {
        LOGW("InspireFaceSession already initialized");
        return true;
    }

    LOGI("Initializing InspireFaceSession with internal path: %s", internalDataPath.c_str());

    // 确保InspireFace库已初始化
    if (!g_inspireFaceLibraryInitialized) {
        LOGE("InspireFace library not initialized");
        return false;
    }

    // 初始化模型管理器
    m_modelManager.reset(new InspireFaceModelManager(assetManager, internalDataPath));
    if (!m_modelManager->initialize()) {
        LOGE("Failed to initialize model manager");
        return false;
    }

    m_modelPath = m_modelManager->getModelRootPath();

    // 创建会话参数
    HFSessionCustomParameter parameter;
    parameter.enable_recognition = enableFaceAttribute ? 1 : 0;
    parameter.enable_liveness = 0;
    parameter.enable_ir_liveness = 0;
    parameter.enable_mask_detect = 0;
    parameter.enable_face_quality = 0;
    parameter.enable_face_attribute = enableFaceAttribute ? 1 : 0;
    parameter.enable_interaction_liveness = 0;
    parameter.enable_detect_mode_landmark = 0;
    parameter.enable_face_pose = 0;
    parameter.enable_face_emotion = 0;

    // 创建InspireFace会话
    HFSession session = nullptr;
    HResult result = HFCreateInspireFaceSession(parameter, HF_DETECT_MODE_ALWAYS_DETECT,
                                               10, // maxDetectFaceNum
                                               320, // detectPixelLevel
                                               1, // trackByDetectModeFPS
                                               &session);

    if (result != HSUCCEED || !session) {
        LOGE("Failed to create InspireFace session, error code: %ld", result);
        return false;
    }

    m_session = session;
    m_initialized = true;

    LOGI("InspireFaceSession initialized successfully");
    return true;
}

void InspireFaceSession::release() {
    if (!m_initialized) return;

    LOGI("Releasing InspireFaceSession");

    if (m_session) {
        HFReleaseInspireFaceSession(static_cast<HFSession>(m_session));
        m_session = nullptr;
    }

    m_initialized = false;
    m_modelPath.clear();

    LOGI("InspireFaceSession released");
}

// ==================== InspireFaceImageProcessor 实现 ====================

InspireFaceImageProcessor::InspireFaceImageProcessor() 
    : m_imageBitmap(nullptr) {
}

InspireFaceImageProcessor::~InspireFaceImageProcessor() {
    if (m_imageBitmap) {
        // TODO: 释放图像位图
        m_imageBitmap = nullptr;
    }
}

bool InspireFaceImageProcessor::createImageStreamFromMat(const cv::Mat& image, void** imageStream) {
    if (image.empty() || !imageStream) {
        LOGE("Invalid parameters for createImageStreamFromMat");
        return false;
    }

    LOGD("Creating image stream from Mat: %dx%d, channels=%d",
         image.cols, image.rows, image.channels());

    // 检查图像格式
    if (image.channels() != 3 && image.channels() != 1) {
        LOGE("Only 3-channel BGR or 1-channel GRAY images are supported");
        return false;
    }

    // 创建图像位图数据结构
    HFImageBitmapData bitmapData;
    bitmapData.data = image.data;
    bitmapData.width = image.cols;
    bitmapData.height = image.rows;
    bitmapData.channels = image.channels(); // 使用真实的channels字段

    // 创建图像位图
    HFImageBitmap bitmap = nullptr;
    HResult result = HFCreateImageBitmap(&bitmapData, &bitmap);
    if (result != HSUCCEED || !bitmap) {
        LOGE("Failed to create image bitmap, error code: %ld", result);
        return false;
    }

    // 从图像位图创建图像流
    HFImageStream stream = nullptr;
    result = HFCreateImageStreamFromImageBitmap(bitmap, HF_CAMERA_ROTATION_0, &stream);

    // 释放临时的图像位图
    HFReleaseImageBitmap(bitmap);

    if (result != HSUCCEED || !stream) {
        LOGE("Failed to create image stream, error code: %ld", result);
        return false;
    }

    *imageStream = stream;

    LOGD("Image stream created successfully");
    return true;
}

void InspireFaceImageProcessor::releaseImageStream(void* imageStream) {
    if (!imageStream) return;

    LOGD("Releasing image stream");

    HFReleaseImageStream(static_cast<HFImageStream>(imageStream));
}

bool InspireFaceImageProcessor::createImageStreamFromROI(const cv::Mat& image, 
                                                        const cv::Rect& roi, 
                                                        void** imageStream) {
    if (image.empty() || !imageStream) {
        LOGE("Invalid parameters for createImageStreamFromROI");
        return false;
    }
    
    // 验证ROI边界
    cv::Rect validROI = roi & cv::Rect(0, 0, image.cols, image.rows);
    if (validROI.width <= 0 || validROI.height <= 0) {
        LOGE("Invalid ROI: [%d,%d,%d,%d]", roi.x, roi.y, roi.width, roi.height);
        return false;
    }
    
    LOGD("Creating image stream from ROI: [%d,%d,%d,%d]", 
         validROI.x, validROI.y, validROI.width, validROI.height);
    
    // 提取ROI区域
    cv::Mat roiImage = image(validROI);
    
    // 创建图像流
    return createImageStreamFromMat(roiImage, imageStream);
}

// ==================== InspireFaceDetector 实现 ====================

InspireFaceDetector::InspireFaceDetector() 
    : m_session(nullptr)
    , m_initialized(false) {
}

InspireFaceDetector::~InspireFaceDetector() {
    // 析构时不需要释放session，因为session由外部管理
}

bool InspireFaceDetector::initialize(InspireFaceSession* session) {
    if (!session || !session->isInitialized()) {
        LOGE("Invalid or uninitialized session");
        return false;
    }
    
    m_session = session;
    m_initialized = true;
    
    LOGI("InspireFaceDetector initialized successfully");
    return true;
}

bool InspireFaceDetector::detectFaces(void* imageStream,
                                     std::vector<FaceDetectionResult>& results) {
    if (!m_initialized || !imageStream) {
        LOGE("Detector not initialized or invalid image stream");
        return false;
    }

    results.clear();

    LOGD("Executing face detection");

    HFSession session = static_cast<HFSession>(m_session->getSessionHandle());
    HFImageStream stream = static_cast<HFImageStream>(imageStream);

    // 执行人脸跟踪检测
    HFMultipleFaceData multipleFaceData;
    HResult result = HFExecuteFaceTrack(session, stream, &multipleFaceData);

    if (result != HSUCCEED) {
        LOGE("Face detection failed, error code: %ld", result);
        return false;
    }

    if (multipleFaceData.detectedNum <= 0) {
        LOGD("No faces detected");
        return true;
    }

    // 转换检测结果
    if (!convertMultipleFaceData(&multipleFaceData, results)) {
        LOGE("Failed to convert face detection results");
        return false;
    }

    LOGD("Face detection completed: %zu faces detected", results.size());
    return true;
}

bool InspireFaceDetector::analyzeFaceAttributes(void* imageStream,
                                               const std::vector<FaceDetectionResult>& faceResults,
                                               std::vector<FaceAttributeResult>& attributeResults) {
    if (!m_initialized || !imageStream) {
        LOGE("Detector not initialized or invalid image stream");
        return false;
    }

    attributeResults.clear();

    if (faceResults.empty()) {
        LOGD("No faces to analyze attributes for");
        return true;
    }

    LOGD("Analyzing face attributes for %zu faces", faceResults.size());

    HFSession session = static_cast<HFSession>(m_session->getSessionHandle());
    HFImageStream stream = static_cast<HFImageStream>(imageStream);

    // 构建HFMultipleFaceData结构用于管道处理
    HFMultipleFaceData multipleFaceData;
    multipleFaceData.detectedNum = faceResults.size();

    // 分配内存用于存储人脸数据
    std::vector<HFaceRect> rects(faceResults.size());
    std::vector<HInt32> trackIds(faceResults.size());
    std::vector<HFloat> confidences(faceResults.size());

    for (size_t i = 0; i < faceResults.size(); ++i) {
        rects[i].x = faceResults[i].faceRect.x;
        rects[i].y = faceResults[i].faceRect.y;
        rects[i].width = faceResults[i].faceRect.width;
        rects[i].height = faceResults[i].faceRect.height;
        trackIds[i] = faceResults[i].trackId;
        confidences[i] = faceResults[i].confidence;
    }

    multipleFaceData.rects = rects.data();
    multipleFaceData.trackIds = trackIds.data();
    multipleFaceData.detConfidence = confidences.data();

    // 创建管道处理参数
    HFSessionCustomParameter parameter;
    parameter.enable_recognition = 0;
    parameter.enable_liveness = 0;
    parameter.enable_ir_liveness = 0;
    parameter.enable_mask_detect = 0;
    parameter.enable_face_quality = 0;
    parameter.enable_face_attribute = 1;
    parameter.enable_interaction_liveness = 0;
    parameter.enable_detect_mode_landmark = 0;
    parameter.enable_face_pose = 0;
    parameter.enable_face_emotion = 0;

    // 执行管道处理
    HResult result = HFMultipleFacePipelineProcess(session, stream, &multipleFaceData, parameter);
    if (result != HSUCCEED) {
        LOGE("Face pipeline process failed, error code: %ld", result);
        return false;
    }

    // 获取属性结果
    HFFaceAttributeResult attrResult;
    result = HFGetFaceAttributeResult(session, &attrResult);
    if (result != HSUCCEED) {
        LOGE("Failed to get face attribute result, error code: %ld", result);
        return false;
    }

    // 转换属性结果
    if (!convertAttributeResult(&attrResult, attributeResults)) {
        LOGE("Failed to convert attribute results");
        return false;
    }

    LOGD("Face attribute analysis completed for %zu faces", attributeResults.size());
    return true;
}

bool InspireFaceDetector::detectAndAnalyze(void* imageStream,
                                          std::vector<FaceDetectionResult>& faceResults,
                                          std::vector<FaceAttributeResult>& attributeResults) {
    // 先执行人脸检测
    if (!detectFaces(imageStream, faceResults)) {
        LOGE("Face detection failed");
        return false;
    }
    
    // 如果没有检测到人脸，直接返回成功
    if (faceResults.empty()) {
        attributeResults.clear();
        LOGD("No faces detected, skipping attribute analysis");
        return true;
    }
    
    // 执行属性分析
    if (!analyzeFaceAttributes(imageStream, faceResults, attributeResults)) {
        LOGE("Face attribute analysis failed");
        return false;
    }
    
    return true;
}

bool InspireFaceDetector::convertMultipleFaceData(void* multipleFaceData,
                                                  std::vector<FaceDetectionResult>& results) {
    if (!multipleFaceData) {
        LOGE("Invalid multiple face data");
        return false;
    }

    HFMultipleFaceData* faceData = static_cast<HFMultipleFaceData*>(multipleFaceData);

    HInt32 faceCount = faceData->detectedNum;
    LOGD("Converting %d faces from InspireFace data", faceCount);

    results.reserve(faceCount);

    for (HInt32 i = 0; i < faceCount; ++i) {
        FaceDetectionResult faceResult;

        // 转换人脸边界框
        if (faceData->rects && i < faceCount) {
            HFaceRect& rect = faceData->rects[i];
            faceResult.faceRect = cv::Rect(rect.x, rect.y, rect.width, rect.height);
        }

        // 转换置信度
        if (faceData->detConfidence && i < faceCount) {
            faceResult.confidence = faceData->detConfidence[i];
        } else {
            faceResult.confidence = 0.8f; // 默认置信度
        }

        // 转换跟踪ID
        if (faceData->trackIds && i < faceCount) {
            faceResult.trackId = faceData->trackIds[i];
        } else {
            faceResult.trackId = i; // 使用索引作为默认ID
        }

        // 转换人脸token
        if (faceData->tokens && i < faceCount) {
            faceResult.faceToken = &(faceData->tokens[i]);
        } else {
            faceResult.faceToken = nullptr;
        }

        results.push_back(faceResult);

        LOGD("Face %d: rect=[%d,%d,%d,%d], conf=%.2f, trackId=%d",
             i, faceResult.faceRect.x, faceResult.faceRect.y,
             faceResult.faceRect.width, faceResult.faceRect.height,
             faceResult.confidence, faceResult.trackId);
    }

    LOGD("Successfully converted %zu faces", results.size());
    return true;
}

bool InspireFaceDetector::convertAttributeResult(void* attributeData,
                                                std::vector<FaceAttributeResult>& results) {
    if (!attributeData) {
        LOGE("Invalid attribute data");
        return false;
    }

    HFFaceAttributeResult* attrResult = static_cast<HFFaceAttributeResult*>(attributeData);

    HInt32 faceCount = attrResult->num;
    LOGD("Converting attributes for %d faces", faceCount);

    results.clear();
    results.reserve(faceCount);

    for (HInt32 i = 0; i < faceCount; ++i) {
        FaceAttributeResult result;

        // 转换性别信息
        if (attrResult->gender && i < faceCount) {
            result.gender = attrResult->gender[i];
            result.genderConfidence = 0.9f; // InspireFace可能不提供置信度，使用默认值
        }

        // 转换年龄段信息
        if (attrResult->ageBracket && i < faceCount) {
            result.ageBracket = attrResult->ageBracket[i];
            result.ageConfidence = 0.85f; // 使用默认置信度
        }

        // 转换种族信息
        if (attrResult->race && i < faceCount) {
            result.race = attrResult->race[i];
            result.raceConfidence = 0.9f; // 使用默认置信度
        }

        results.push_back(result);

        LOGD("Face %d attributes: %s, %s, %s", i,
             result.getGenderString().c_str(),
             result.getAgeBracketString().c_str(),
             result.getRaceString().c_str());
    }

    LOGD("Successfully converted attributes for %zu faces", results.size());
    return true;
}

// ==================== InspireFaceUtils 实现 ====================

namespace InspireFaceUtils {

bool initializeLibrary() {
    if (g_inspireFaceLibraryInitialized) {
        LOGW("InspireFace library already initialized");
        return true;
    }

    LOGI("Initializing InspireFace library");

    // 初始化InspireFace库，使用模型目录作为资源路径
    // 注意：这个路径会在后面的initialize方法中被设置
    const char* resourcePath = "/data/data/com.wulala.myyolov5rtspthreadpool/files/inspireface";
    HResult result = HFLaunchInspireFace(resourcePath);
    if (result != HSUCCEED) {
        LOGE("Failed to initialize InspireFace library, error code: %ld", result);
        return false;
    }

    g_inspireFaceLibraryInitialized = true;
    LOGI("InspireFace library initialized successfully");
    return true;
}

void releaseLibrary() {
    if (!g_inspireFaceLibraryInitialized) {
        return;
    }

    LOGI("Releasing InspireFace library");

    // 释放InspireFace库
    HFTerminateInspireFace();
    g_inspireFaceLibraryInitialized = false;

    LOGI("InspireFace library released");
}

bool checkModelFiles(const std::string& modelPath) {
    LOGD("Checking model files at: %s", modelPath.c_str());

    // 检查配置文件
    std::string configFile = modelPath + "/__inspire__";
    std::ifstream config(configFile);
    if (!config.is_open()) {
        LOGE("Config file not found: %s", configFile.c_str());
        return false;
    }
    config.close();

    // 检查关键模型文件
    std::vector<std::string> requiredFiles = {
        "_00_scrfd_2_5g_bnkps_shape320x320_rk3588",  // 人脸检测
        "_03_r18_Glint360K_fixed_rk3588",            // 特征提取
        "_08_fairface_model_rk3588"                  // 属性分析
    };

    for (const auto& file : requiredFiles) {
        std::string fullPath = modelPath + "/" + file;
        std::ifstream modelFile(fullPath);
        if (!modelFile.is_open()) {
            LOGE("Required model file not found: %s", fullPath.c_str());
            return false;
        }
        modelFile.close();
    }

    LOGI("All required model files found");
    return true;
}

std::string getVersion() {
    // 获取InspireFace版本信息
    HFInspireFaceVersion version;
    HResult result = HFQueryInspireFaceVersion(&version);
    if (result == HSUCCEED) {
        char versionStr[64];
        snprintf(versionStr, sizeof(versionStr), "InspireFace v%d.%d.%d (RK3588)",
                version.major, version.minor, version.patch);
        return std::string(versionStr);
    }

    return "InspireFace v4.0 (RK3588)";
}

void setLogLevel(int level) {
    LOGD("Setting InspireFace log level to: %d", level);

    // 转换为InspireFace日志级别
    HFLogLevel hfLevel = HF_LOG_INFO; // 默认级别
    switch (level) {
        case 0: hfLevel = HF_LOG_NONE; break;
        case 1: hfLevel = HF_LOG_DEBUG; break;
        case 2: hfLevel = HF_LOG_INFO; break;
        case 3: hfLevel = HF_LOG_WARN; break;
        case 4: hfLevel = HF_LOG_ERROR; break;
        case 5: hfLevel = HF_LOG_FATAL; break;
        default: hfLevel = HF_LOG_INFO; break;
    }

    // 设置InspireFace日志级别
    HFSetLogLevel(hfLevel);
}

void convertRect(const cv::Rect& cvRect, void* hfRect) {
    if (!hfRect) return;

    HFaceRect* rect = static_cast<HFaceRect*>(hfRect);
    rect->x = cvRect.x;
    rect->y = cvRect.y;
    rect->width = cvRect.width;
    rect->height = cvRect.height;

    LOGD("Converted OpenCV Rect [%d,%d,%d,%d] to InspireFace Rect",
         cvRect.x, cvRect.y, cvRect.width, cvRect.height);
}

cv::Rect convertRect(const void* hfRect) {
    if (!hfRect) {
        return cv::Rect(0, 0, 0, 0);
    }

    const HFaceRect* rect = static_cast<const HFaceRect*>(hfRect);
    cv::Rect cvRect(rect->x, rect->y, rect->width, rect->height);

    LOGD("Converted InspireFace Rect [%d,%d,%d,%d] to OpenCV Rect",
         rect->x, rect->y, rect->width, rect->height);

    return cvRect;
}

bool checkResult(long result, const std::string& operation) {
    if (result == ISF_SUCCESS) {
        return true;
    }
    
    LOGE("InspireFace operation '%s' failed with code: %ld", operation.c_str(), result);
    return false;
}

} // namespace InspireFaceUtils
