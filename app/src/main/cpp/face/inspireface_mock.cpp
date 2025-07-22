/**
 * @file inspireface_mock.cpp
 * @brief InspireFace模拟实现
 * @author AI Assistant
 * @date 2025-07-22
 */

#include "inspireface_mock.h"
#include <android/log.h>
#include <random>
#include <cmath>

#define LOG_TAG "MockInspireFace"
#define LOGD(...) __android_log_print(ANDROID_LOG_DEBUG, LOG_TAG, __VA_ARGS__)
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR, LOG_TAG, __VA_ARGS__)
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO, LOG_TAG, __VA_ARGS__)

// 全局库初始化状态
static bool g_mockLibraryInitialized = false;

// ==================== MockInspireFaceSession 实现 ====================

MockInspireFaceSession::MockInspireFaceSession() : m_initialized(false) {
    LOGD("🔧 Mock: MockInspireFaceSession constructor");
}

MockInspireFaceSession::~MockInspireFaceSession() {
    release();
}

bool MockInspireFaceSession::initialize(AAssetManager* assetManager, const std::string& internalDataPath,
                                       bool enableFaceAttribute) {
    if (m_initialized) {
        LOGD("🔧 Mock: Session already initialized");
        return true;
    }

    LOGI("🔧 Mock: Initializing session with path: %s", internalDataPath.c_str());

    if (!g_mockLibraryInitialized) {
        LOGE("🔧 Mock: Library not initialized");
        return false;
    }

    m_dataPath = internalDataPath;
    
    // 尝试加载OpenCV人脸检测器（如果可用）
    try {
        // 注意：在实际部署中，需要将haarcascade_frontalface_alt.xml放到assets中
        std::string cascadePath = internalDataPath + "/haarcascade_frontalface_alt.xml";
        if (!m_faceCascade.load(cascadePath)) {
            LOGD("🔧 Mock: OpenCV cascade not available, will use simplified detection");
        } else {
            LOGD("🔧 Mock: OpenCV cascade loaded successfully");
        }
    } catch (const std::exception& e) {
        LOGD("🔧 Mock: Exception loading cascade: %s", e.what());
    }

    m_initialized = true;
    LOGI("🔧 Mock: ✅ Session initialized successfully");
    return true;
}

void MockInspireFaceSession::release() {
    if (m_initialized) {
        LOGD("🔧 Mock: Releasing session");
        m_initialized = false;
    }
}

// ==================== MockInspireFaceImageProcessor 实现 ====================

MockInspireFaceImageProcessor::MockInspireFaceImageProcessor() {
    LOGD("🔧 Mock: MockInspireFaceImageProcessor constructor");
}

MockInspireFaceImageProcessor::~MockInspireFaceImageProcessor() {
    LOGD("🔧 Mock: MockInspireFaceImageProcessor destructor");
}

bool MockInspireFaceImageProcessor::createImageStreamFromMat(const cv::Mat& image, void** imageStream) {
    if (image.empty()) {
        LOGE("🔧 Mock: Input image is empty");
        return false;
    }

    try {
        ImageStreamData* data = new ImageStreamData();
        data->image = image.clone();
        data->valid = true;
        *imageStream = data;
        
        LOGD("🔧 Mock: Created image stream: %dx%d", image.cols, image.rows);
        return true;
    } catch (const std::exception& e) {
        LOGE("🔧 Mock: Exception creating image stream: %s", e.what());
        return false;
    }
}

void MockInspireFaceImageProcessor::releaseImageStream(void* imageStream) {
    if (imageStream) {
        ImageStreamData* data = static_cast<ImageStreamData*>(imageStream);
        delete data;
        LOGD("🔧 Mock: Released image stream");
    }
}

// ==================== MockInspireFaceDetector 实现 ====================

MockInspireFaceDetector::MockInspireFaceDetector() : m_session(nullptr), m_initialized(false) {
    LOGD("🔧 Mock: MockInspireFaceDetector constructor");
}

MockInspireFaceDetector::~MockInspireFaceDetector() {
    LOGD("🔧 Mock: MockInspireFaceDetector destructor");
}

bool MockInspireFaceDetector::initialize(MockInspireFaceSession* session) {
    if (!session || !session->isInitialized()) {
        LOGE("🔧 Mock: Invalid session");
        return false;
    }

    m_session = session;
    m_initialized = true;
    
    LOGI("🔧 Mock: ✅ Detector initialized successfully");
    return true;
}

bool MockInspireFaceDetector::detectAndAnalyze(void* imageStream, 
                                              std::vector<MockFaceDetectionResult>& faceResults,
                                              std::vector<MockFaceAttributeResult>& attributeResults) {
    if (!m_initialized || !imageStream) {
        LOGE("🔧 Mock: Detector not initialized or invalid image stream");
        return false;
    }

    try {
        ImageStreamData* data = static_cast<ImageStreamData*>(imageStream);
        if (!data || !data->valid || data->image.empty()) {
            LOGE("🔧 Mock: Invalid image data");
            return false;
        }

        cv::Mat& image = data->image;
        faceResults.clear();
        attributeResults.clear();

        LOGD("🔧 Mock: Processing image: %dx%d", image.cols, image.rows);

        // 简化的人脸检测（使用图像中心区域作为模拟人脸）
        int centerX = image.cols / 2;
        int centerY = image.rows / 2;
        int faceSize = std::min(image.cols, image.rows) / 4;
        
        if (faceSize > 50) { // 最小人脸尺寸
            MockFaceDetectionResult faceResult;
            faceResult.faceRect = cv::Rect(centerX - faceSize/2, centerY - faceSize/2, faceSize, faceSize);
            faceResult.confidence = 0.85f; // 模拟置信度
            faceResult.faceId = 1;
            
            faceResults.push_back(faceResult);

            // 提取人脸区域进行属性分析
            cv::Mat faceImage = image(faceResult.faceRect);
            
            MockFaceAttributeResult attrResult;
            attrResult.gender = estimateGender(faceImage);
            attrResult.ageBracket = estimateAge(faceImage);
            attrResult.confidence = 0.75f;
            
            attributeResults.push_back(attrResult);
            
            LOGD("🔧 Mock: ✅ Detected 1 face, gender=%d, age=%d", 
                 attrResult.gender, attrResult.ageBracket);
        } else {
            LOGD("🔧 Mock: No faces detected (image too small)");
        }

        return true;
    } catch (const std::exception& e) {
        LOGE("🔧 Mock: Exception in detectAndAnalyze: %s", e.what());
        return false;
    }
}

int MockInspireFaceDetector::estimateGender(const cv::Mat& faceImage) {
    // 简化的性别估计：基于图像亮度分布
    cv::Scalar meanBrightness = cv::mean(faceImage);
    return (meanBrightness[0] > 120) ? 1 : 0; // 简单的阈值判断
}

int MockInspireFaceDetector::estimateAge(const cv::Mat& faceImage) {
    // 简化的年龄估计：基于图像纹理复杂度
    cv::Mat gray;
    if (faceImage.channels() > 1) {
        cv::cvtColor(faceImage, gray, cv::COLOR_BGR2GRAY);
    } else {
        gray = faceImage;
    }
    
    cv::Scalar stdDev;
    cv::meanStdDev(gray, cv::Scalar(), stdDev);
    
    // 根据标准差映射到年龄段
    double complexity = stdDev[0];
    if (complexity < 20) return 0;      // 0-9岁
    else if (complexity < 30) return 1; // 10-19岁
    else if (complexity < 40) return 2; // 20-29岁
    else if (complexity < 50) return 3; // 30-39岁
    else return 4; // 40+岁
}

// ==================== MockInspireFaceUtils 实现 ====================

namespace MockInspireFaceUtils {

bool initializeLibrary() {
    if (g_mockLibraryInitialized) {
        LOGD("🔧 Mock: Library already initialized");
        return true;
    }

    LOGI("🔧 Mock: Initializing mock InspireFace library");
    
    // 模拟库初始化过程
    g_mockLibraryInitialized = true;
    
    LOGI("🔧 Mock: ✅ Library initialized successfully");
    return true;
}

void releaseLibrary() {
    if (g_mockLibraryInitialized) {
        LOGI("🔧 Mock: Releasing mock InspireFace library");
        g_mockLibraryInitialized = false;
    }
}

} // namespace MockInspireFaceUtils
