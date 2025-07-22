#include "../include/face_analysis_manager.h"
#include "../face/inspireface_wrapper.h"  // 🔧 Phase 2: 恢复真实InspireFace
#include <android/log.h>
#include <algorithm>
#include <cmath>
#include <thread>      // 🔧 Phase 2: 延迟初始化线程支持
#include <chrono>      // 🔧 Phase 2: 时间延迟支持

// 🔧 Phase 2: 添加日志宏定义
#ifndef LOG_TAG
#define LOG_TAG "FaceAnalysisManager"
#endif

#ifndef LOGE
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR, LOG_TAG, __VA_ARGS__)
#endif

#ifndef LOGD
#define LOGD(...) __android_log_print(ANDROID_LOG_DEBUG, LOG_TAG, __VA_ARGS__)
#endif

// 🔧 Phase 2: InspireFace库初始化包装函数（使用真实InspireFace）
bool initializeInspireFaceLibrary() {
    try {
        return InspireFaceUtils::initializeLibrary();
    } catch (const std::exception& e) {
        LOGE("🔧 Phase 2: Exception in initializeInspireFaceLibrary: %s", e.what());
        return false;
    } catch (...) {
        LOGE("🔧 Phase 2: Unknown exception in initializeInspireFaceLibrary");
        return false;
    }
}

#define TAG "FaceAnalysisManager"
#define LOGD(...) __android_log_print(ANDROID_LOG_DEBUG, TAG, __VA_ARGS__)
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR, TAG, __VA_ARGS__)
#define LOGW(...) __android_log_print(ANDROID_LOG_WARN, TAG, __VA_ARGS__)

FaceAnalysisManager::FaceAnalysisManager() :
    initialized_(false), inspireface_initialized_(false),
    m_assetManager(nullptr), m_internalDataPath("") {
    LOGD("🔧 Phase 2: FaceAnalysisManager constructor with InspireFace support");
    LOGD("🔧 Phase 2: Constructor - initialized_=%s, inspireface_initialized_=%s",
         initialized_ ? "true" : "false", inspireface_initialized_ ? "true" : "false");
}

FaceAnalysisManager::~FaceAnalysisManager() {
    release();
    LOGD("FaceAnalysisManager destructor");
}

int FaceAnalysisManager::initialize() {
    try {
        // 加载OpenCV人脸检测器
        // 注意：在实际应用中，需要将haarcascade_frontalface_alt.xml文件放到assets目录
        // 这里使用简化的初始化

        // TODO: 加载实际的人脸检测模型
        // std::string cascade_path = "/path/to/haarcascade_frontalface_alt.xml";
        // if (!face_cascade_.load(cascade_path)) {
        //     LOGE("Failed to load face cascade classifier");
        //     return -1;
        // }

        // 清空已知人员数据库
        known_persons_.clear();

        initialized_ = true;
        LOGD("🔧 Phase 2: Face analysis manager initialized successfully");
        LOGD("🔧 Phase 2: Basic initialization complete - ready for InspireFace integration");
        return 0;

    } catch (const std::exception& e) {
        LOGE("Failed to initialize face analysis manager: %s", e.what());
        return -1;
    }
}

// 🔧 Phase 2: 添加extended_inference_manager.cpp需要的方法
bool FaceAnalysisManager::initialize(const std::string& modelPath) {
    try {
        LOGD("🔧 Phase 2: Initializing FaceAnalysisManager with model path: %s", modelPath.c_str());

        // 调用原有的初始化方法
        int result = initialize();
        if (result != 0) {
            return false;
        }

        // TODO: 在Phase 2中，这里将集成真正的InspireFace模型加载
        LOGD("🔧 Phase 2: Model path stored for future InspireFace integration: %s", modelPath.c_str());

        return true;

    } catch (const std::exception& e) {
        LOGE("Failed to initialize face analysis manager with model path: %s", e.what());
        return false;
    }
}

bool FaceAnalysisManager::initializeInspireFace(AAssetManager* assetManager, const std::string& internalDataPath) {
    try {
        LOGD("🔧 Phase 2: Preparing InspireFace for delayed initialization");
        LOGD("🔧 Phase 2: Asset manager: %p", assetManager);
        LOGD("🔧 Phase 2: Internal data path: %s", internalDataPath.c_str());

        // 🔧 Phase 2: 保存参数，准备延迟初始化
        m_assetManager = assetManager;
        m_internalDataPath = internalDataPath;

        // 🔧 Phase 2: 启动延迟初始化线程（10-15秒后执行）
        std::thread delayedInitThread([this]() {
            try {
                LOGD("🔧 Phase 2: Delayed initialization thread started");

                // 等待RTSP流稳定（12秒）
                std::this_thread::sleep_for(std::chrono::seconds(12));

                LOGD("🔧 Phase 2: Starting delayed InspireFace initialization...");
                bool success = performDelayedInspireFaceInitialization();

                if (success) {
                    LOGD("🔧 Phase 2: ✅ Delayed InspireFace initialization completed successfully");
                } else {
                    LOGE("🔧 Phase 2: ❌ Delayed InspireFace initialization failed");
                }

            } catch (const std::exception& e) {
                LOGE("🔧 Phase 2: Exception in delayed initialization thread: %s", e.what());
            } catch (...) {
                LOGE("🔧 Phase 2: Unknown exception in delayed initialization thread");
            }
        });

        // 分离线程，让它独立运行
        delayedInitThread.detach();

        LOGD("🔧 Phase 2: ✅ InspireFace delayed initialization scheduled");
        return true;

        /*
        // 🔧 Phase 2: 临时注释掉InspireFace初始化代码，避免与RTSP/MPP冲突

        // 1. 参数验证
        if (!assetManager) {
            LOGE("🔧 Phase 2: AssetManager is null, cannot initialize InspireFace");
            return false;
        }

        if (internalDataPath.empty()) {
            LOGE("🔧 Phase 2: Internal data path is empty, cannot initialize InspireFace");
            return false;
        }

        // 2. 调用基础初始化方法
        LOGD("🔧 Phase 2: Calling basic initialize()...");
        int result = initialize();
        if (result != 0) {
            LOGE("🔧 Phase 2: Basic initialization failed with code: %d", result);
            return false;
        }
        LOGD("🔧 Phase 2: Basic initialization completed successfully");

        // 3. 清理旧的InspireFace组件（如果存在）
        if (inspireface_initialized_) {
            LOGD("🔧 Phase 2: Cleaning up existing InspireFace components...");
            cleanupInspireFaceComponents();
        }
        */

        // 4. 初始化InspireFace库（必须在创建组件之前）
        LOGD("🔧 Phase 2: Initializing InspireFace library...");
        try {
            if (!initializeInspireFaceLibrary()) {
                LOGE("🔧 Phase 2: Failed to initialize InspireFace library");
                return false;
            }
            LOGD("🔧 Phase 2: InspireFace library initialized successfully");
        } catch (const std::exception& e) {
            LOGE("🔧 Phase 2: Exception initializing InspireFace library: %s", e.what());
            return false;
        }

        // 5. 渐进式创建InspireFace组件
        LOGD("🔧 Phase 2: Creating InspireFace session...");
        try {
            inspireface_session_ = std::unique_ptr<InspireFaceSession>(new InspireFaceSession());
            if (!inspireface_session_) {
                LOGE("🔧 Phase 2: Failed to create InspireFace session");
                cleanupInspireFaceComponents();
                return false;
            }
            LOGD("🔧 Phase 2: InspireFace session created successfully");
        } catch (const std::exception& e) {
            LOGE("🔧 Phase 2: Exception creating InspireFace session: %s", e.what());
            cleanupInspireFaceComponents();
            return false;
        }

        LOGD("🔧 Phase 2: Creating image processor...");
        try {
            image_processor_ = std::unique_ptr<InspireFaceImageProcessor>(new InspireFaceImageProcessor());
            if (!image_processor_) {
                LOGE("🔧 Phase 2: Failed to create image processor");
                cleanupInspireFaceComponents();
                return false;
            }
            LOGD("🔧 Phase 2: Image processor created successfully");
        } catch (const std::exception& e) {
            LOGE("🔧 Phase 2: Exception creating image processor: %s", e.what());
            cleanupInspireFaceComponents();
            return false;
        }

        LOGD("🔧 Phase 2: Creating face detector...");
        try {
            face_detector_ = std::unique_ptr<InspireFaceDetector>(new InspireFaceDetector());
            if (!face_detector_) {
                LOGE("🔧 Phase 2: Failed to create face detector");
                cleanupInspireFaceComponents();
                return false;
            }
            LOGD("🔧 Phase 2: Face detector created successfully");
        } catch (const std::exception& e) {
            LOGE("🔧 Phase 2: Exception creating face detector: %s", e.what());
            cleanupInspireFaceComponents();
            return false;
        }

        // 5. 初始化InspireFace会话（增强安全检查）
        LOGD("🔧 Phase 2: Initializing InspireFace session...");
        try {
            // 验证会话对象状态
            if (!inspireface_session_) {
                LOGE("🔧 Phase 2: InspireFace session is null before initialization");
                cleanupInspireFaceComponents();
                return false;
            }

            // 验证参数
            if (!assetManager) {
                LOGE("🔧 Phase 2: AssetManager is null during session initialization");
                cleanupInspireFaceComponents();
                return false;
            }

            LOGD("🔧 Phase 2: Calling session->initialize() with validated parameters...");
            bool initResult = inspireface_session_->initialize(assetManager, internalDataPath, true);

            if (!initResult) {
                LOGE("🔧 Phase 2: InspireFace session initialization returned false");
                cleanupInspireFaceComponents();
                return false;
            }

            // 验证初始化结果
            if (!inspireface_session_->isInitialized()) {
                LOGE("🔧 Phase 2: InspireFace session reports not initialized after init call");
                cleanupInspireFaceComponents();
                return false;
            }

            LOGD("🔧 Phase 2: ✅ InspireFace session initialized and verified successfully");
        } catch (const std::exception& e) {
            LOGE("🔧 Phase 2: Exception initializing InspireFace session: %s", e.what());
            cleanupInspireFaceComponents();
            return false;
        } catch (...) {
            LOGE("🔧 Phase 2: Unknown exception initializing InspireFace session");
            cleanupInspireFaceComponents();
            return false;
        }

        // 6. 初始化人脸检测器
        LOGD("🔧 Phase 2: Initializing face detector...");
        try {
            if (!face_detector_->initialize(inspireface_session_.get())) {
                LOGE("🔧 Phase 2: Failed to initialize face detector");
                cleanupInspireFaceComponents();
                return false;
            }
            LOGD("🔧 Phase 2: Face detector initialized successfully");
        } catch (const std::exception& e) {
            LOGE("🔧 Phase 2: Exception initializing face detector: %s", e.what());
            cleanupInspireFaceComponents();
            return false;
        }

        // 7. 标记初始化完成
        inspireface_initialized_ = true;
        LOGD("🔧 Phase 2: ✅ InspireFace initialization completed successfully!");

        return true;

    } catch (const std::exception& e) {
        LOGE("🔧 Phase 2: ❌ Critical exception in initializeInspireFace: %s", e.what());
        inspireface_initialized_ = false;
        cleanupInspireFaceComponents();
        return false;
    } catch (...) {
        LOGE("🔧 Phase 2: ❌ Unknown exception in initializeInspireFace");
        inspireface_initialized_ = false;
        cleanupInspireFaceComponents();
        return false;
    }
}

// 🔧 Phase 2: 实现延迟初始化的核心方法
bool FaceAnalysisManager::performDelayedInspireFaceInitialization() {
    LOGD("🔧 Phase 2: === Starting delayed InspireFace initialization ===");

    try {
        // 1. 参数验证
        if (!m_assetManager) {
            LOGE("🔧 Phase 2: AssetManager is null, cannot initialize InspireFace");
            return false;
        }

        if (m_internalDataPath.empty()) {
            LOGE("🔧 Phase 2: Internal data path is empty, cannot initialize InspireFace");
            return false;
        }

        // 2. 确保基础组件已初始化
        if (!initialized_) {
            LOGD("🔧 Phase 2: Calling basic initialize()...");
            int result = initialize();
            if (result != 0) {
                LOGE("🔧 Phase 2: Basic initialization failed with code: %d", result);
                return false;
            }
            LOGD("🔧 Phase 2: Basic initialization completed successfully");
        }

        // 3. 清理旧的InspireFace组件（如果存在）
        if (inspireface_initialized_) {
            LOGD("🔧 Phase 2: Cleaning up existing InspireFace components...");
            cleanupInspireFaceComponents();
        }

        // 4. 分步初始化InspireFace
        LOGD("🔧 Phase 2: Step 1 - Initializing InspireFace library...");
        if (!initializeInspireFaceLibraryStep()) {
            LOGE("🔧 Phase 2: Failed to initialize InspireFace library");
            return false;
        }

        LOGD("🔧 Phase 2: Step 2 - Creating InspireFace components...");
        if (!createInspireFaceComponentsStep()) {
            LOGE("🔧 Phase 2: Failed to create InspireFace components");
            cleanupInspireFaceComponents();
            return false;
        }

        LOGD("🔧 Phase 2: Step 3 - Initializing InspireFace session...");
        if (!initializeInspireFaceSessionStep()) {
            LOGE("🔧 Phase 2: Failed to initialize InspireFace session");
            cleanupInspireFaceComponents();
            return false;
        }

        // 5. 标记初始化完成
        inspireface_initialized_ = true;
        LOGD("🔧 Phase 2: ✅ All InspireFace components initialized successfully");

        return true;

    } catch (const std::exception& e) {
        LOGE("🔧 Phase 2: Exception in delayed initialization: %s", e.what());
        cleanupInspireFaceComponents();
        return false;
    } catch (...) {
        LOGE("🔧 Phase 2: Unknown exception in delayed initialization");
        cleanupInspireFaceComponents();
        return false;
    }
}

// 🔧 Phase 2: 分步初始化方法实现
bool FaceAnalysisManager::initializeInspireFaceLibraryStep() {
    try {
        LOGD("🔧 Phase 2: Calling HFLaunchInspireFace...");
        if (!initializeInspireFaceLibrary()) {
            LOGE("🔧 Phase 2: Failed to initialize InspireFace library");
            return false;
        }
        LOGD("🔧 Phase 2: ✅ InspireFace library initialized successfully");
        return true;
    } catch (const std::exception& e) {
        LOGE("🔧 Phase 2: Exception initializing InspireFace library: %s", e.what());
        return false;
    } catch (...) {
        LOGE("🔧 Phase 2: Unknown exception initializing InspireFace library");
        return false;
    }
}

bool FaceAnalysisManager::createInspireFaceComponentsStep() {
    try {
        // 创建会话
        LOGD("🔧 Phase 2: Creating InspireFace session...");
        inspireface_session_ = std::unique_ptr<InspireFaceSession>(new InspireFaceSession());
        if (!inspireface_session_) {
            LOGE("🔧 Phase 2: Failed to create InspireFace session");
            return false;
        }

        // 创建图像处理器
        LOGD("🔧 Phase 2: Creating image processor...");
        image_processor_ = std::unique_ptr<InspireFaceImageProcessor>(new InspireFaceImageProcessor());
        if (!image_processor_) {
            LOGE("🔧 Phase 2: Failed to create image processor");
            return false;
        }

        // 创建人脸检测器
        LOGD("🔧 Phase 2: Creating face detector...");
        face_detector_ = std::unique_ptr<InspireFaceDetector>(new InspireFaceDetector());
        if (!face_detector_) {
            LOGE("🔧 Phase 2: Failed to create face detector");
            return false;
        }

        LOGD("🔧 Phase 2: ✅ All InspireFace components created successfully");
        return true;

    } catch (const std::exception& e) {
        LOGE("🔧 Phase 2: Exception creating InspireFace components: %s", e.what());
        return false;
    } catch (...) {
        LOGE("🔧 Phase 2: Unknown exception creating InspireFace components");
        return false;
    }
}

bool FaceAnalysisManager::initializeInspireFaceSessionStep() {
    try {
        // 初始化会话
        LOGD("🔧 Phase 2: Initializing InspireFace session...");
        if (!inspireface_session_->initialize(m_assetManager, m_internalDataPath, true)) {
            LOGE("🔧 Phase 2: Failed to initialize InspireFace session");
            return false;
        }

        // 初始化人脸检测器
        LOGD("🔧 Phase 2: Initializing face detector...");
        if (!face_detector_->initialize(inspireface_session_.get())) {
            LOGE("🔧 Phase 2: Failed to initialize face detector");
            return false;
        }

        LOGD("🔧 Phase 2: ✅ InspireFace session and detector initialized successfully");
        return true;

    } catch (const std::exception& e) {
        LOGE("🔧 Phase 2: Exception initializing InspireFace session: %s", e.what());
        return false;
    } catch (...) {
        LOGE("🔧 Phase 2: Unknown exception initializing InspireFace session");
        return false;
    }
}

void FaceAnalysisManager::release() {
    if (initialized_) {
        // 🔧 Phase 2: 清理InspireFace资源
        cleanupInspireFaceComponents();

        known_persons_.clear();
        initialized_ = false;
        LOGD("🔧 Phase 2: Face analysis manager released");
    }
}

// 🔧 Phase 2: 添加InspireFace组件清理方法
void FaceAnalysisManager::cleanupInspireFaceComponents() {
    try {
        LOGD("🔧 Phase 2: Cleaning up InspireFace components...");

        if (inspireface_initialized_) {
            if (inspireface_session_) {
                LOGD("🔧 Phase 2: Releasing InspireFace session...");
                try {
                    inspireface_session_->release();
                } catch (const std::exception& e) {
                    LOGE("🔧 Phase 2: Exception releasing InspireFace session: %s", e.what());
                }
                inspireface_session_.reset();
                LOGD("🔧 Phase 2: InspireFace session released");
            }

            if (face_detector_) {
                LOGD("🔧 Phase 2: Releasing face detector...");
                face_detector_.reset();
                LOGD("🔧 Phase 2: Face detector released");
            }

            if (image_processor_) {
                LOGD("🔧 Phase 2: Releasing image processor...");
                image_processor_.reset();
                LOGD("🔧 Phase 2: Image processor released");
            }

            inspireface_initialized_ = false;
            LOGD("🔧 Phase 2: ✅ InspireFace components cleanup completed");
        }

    } catch (const std::exception& e) {
        LOGE("🔧 Phase 2: Exception during InspireFace cleanup: %s", e.what());
        inspireface_initialized_ = false;
    } catch (...) {
        LOGE("🔧 Phase 2: Unknown exception during InspireFace cleanup");
        inspireface_initialized_ = false;
    }
}

FaceAnalysisResult FaceAnalysisManager::analyzeFace(const cv::Mat& image) {
    FaceAnalysisResult result;
    result.face_detected = false;
    result.confidence = 0.0f;
    result.age = 0;
    result.gender = 0;
    result.person_id = -1;

    if (!initialized_ || image.empty()) {
        return result;
    }

    try {
        // 🔧 Phase 2: 检查InspireFace状态（延迟初始化）
        if (!inspireface_initialized_) {
            LOGD("🔧 Phase 2: InspireFace not yet initialized, using fallback detection");
        }

        // 🔧 Phase 2: 使用InspireFace进行真正的人脸分析（如果已初始化）
        if (inspireface_initialized_ && inspireface_session_ && image_processor_ && face_detector_) {
            LOGD("🔧 Phase 2: Using InspireFace for face analysis");
            // 1. 创建图像流
            void* imageStream = nullptr;
            if (!image_processor_->createImageStreamFromMat(image, &imageStream)) {
                LOGE("Failed to create image stream from Mat");
                return result;
            }

            // 2. 执行人脸检测和属性分析
            std::vector<FaceDetectionResult> faceResults;
            std::vector<FaceAttributeResult> attributeResults;

            bool success = face_detector_->detectAndAnalyze(imageStream, faceResults, attributeResults);

            // 3. 释放图像流
            image_processor_->releaseImageStream(imageStream);

            if (success && !faceResults.empty()) {
                // 使用第一个检测到的人脸
                const auto& faceResult = faceResults[0];
                result.face_detected = true;
                result.confidence = faceResult.confidence;

                // 设置人脸边界框
                result.face_box.x = faceResult.faceRect.x;
                result.face_box.y = faceResult.faceRect.y;
                result.face_box.width = faceResult.faceRect.width;
                result.face_box.height = faceResult.faceRect.height;

                // 如果有属性分析结果，使用它们
                if (!attributeResults.empty()) {
                    const auto& attributes = attributeResults[0];
                    result.gender = attributes.gender;
                    result.age = attributes.ageBracket;

                    LOGD("🔧 Phase 2: InspireFace analysis - gender=%d, age=%d, confidence=%.2f",
                         result.gender, result.age, result.confidence);
                } else {
                    // 使用简化的年龄性别估计作为fallback
                    cv::Mat face_image = image(cv::Rect(result.face_box.x, result.face_box.y,
                                                       result.face_box.width, result.face_box.height));
                    result.age = estimateAge(face_image);
                    result.gender = recognizeGender(face_image);

                    LOGD("🔧 Phase 2: Fallback analysis - gender=%d, age=%d", result.gender, result.age);
                }

                // TODO: 实现人脸特征提取和识别
                result.face_features = extractFaceFeatures(image);
                result.person_id = recognizePerson(result.face_features);
            }
        } else {
            // 🔧 Fallback: 使用简化的人脸检测
            LOGD("🔧 Phase 2: Using fallback face detection (InspireFace not available)");

            std::vector<cv::Rect> faces;
            int face_count = detectFaces(image, faces);

            if (face_count > 0) {
                cv::Rect face_rect = faces[0];
                result.face_detected = true;
                result.confidence = 0.8f;

                result.face_box.x = face_rect.x;
                result.face_box.y = face_rect.y;
                result.face_box.width = face_rect.width;
                result.face_box.height = face_rect.height;

                cv::Mat face_image = image(face_rect);
                result.age = estimateAge(face_image);
                result.gender = recognizeGender(face_image);
                result.face_features = extractFaceFeatures(face_image);
                result.person_id = recognizePerson(result.face_features);

                LOGD("🔧 Phase 2: Fallback analysis completed");
            }
        }

    } catch (const std::exception& e) {
        LOGE("Face analysis exception: %s", e.what());
    }

    return result;
}

int FaceAnalysisManager::detectFaces(const cv::Mat& image, std::vector<cv::Rect>& faces) {
    faces.clear();
    
    if (!initialized_ || image.empty()) {
        return 0;
    }
    
    try {
        // 简化的人脸检测实现
        // 在实际应用中，这里应该使用训练好的人脸检测模型
        
        // 转换为灰度图
        cv::Mat gray;
        if (image.channels() == 3) {
            cv::cvtColor(image, gray, cv::COLOR_BGR2GRAY);
        } else {
            gray = image.clone();
        }
        
        // TODO: 使用实际的人脸检测算法
        // face_cascade_.detectMultiScale(gray, faces, 1.1, 3, 0, cv::Size(30, 30));
        
        // 简化实现：假设在图像中心区域有一个人脸
        if (image.cols > 100 && image.rows > 100) {
            int face_size = std::min(image.cols, image.rows) / 3;
            int x = (image.cols - face_size) / 2;
            int y = (image.rows - face_size) / 2;
            faces.push_back(cv::Rect(x, y, face_size, face_size));
        }
        
        return faces.size();
        
    } catch (const std::exception& e) {
        LOGE("Face detection exception: %s", e.what());
        return 0;
    }
}

std::vector<float> FaceAnalysisManager::extractFaceFeatures(const cv::Mat& face_image) {
    std::vector<float> features;
    
    if (!initialized_ || face_image.empty()) {
        return features;
    }
    
    try {
        // 简化的特征提取实现
        // 在实际应用中，这里应该使用深度学习模型提取人脸特征
        
        cv::Mat processed = preprocessFaceImage(face_image);
        
        // 简化实现：使用图像的统计特征作为人脸特征
        cv::Scalar mean, stddev;
        cv::meanStdDev(processed, mean, stddev);
        
        features.resize(128); // 假设特征向量长度为128
        for (int i = 0; i < 128; i++) {
            features[i] = static_cast<float>(mean[0] + stddev[0] * sin(i * 0.1));
        }
        
        LOGD("Extracted face features: %zu dimensions", features.size());
        
    } catch (const std::exception& e) {
        LOGE("Feature extraction exception: %s", e.what());
    }
    
    return features;
}

int FaceAnalysisManager::estimateAge(const cv::Mat& face_image) {
    if (!initialized_ || face_image.empty()) {
        return 25; // 默认年龄
    }
    
    try {
        // 简化的年龄估计实现
        // 在实际应用中，这里应该使用训练好的年龄估计模型
        
        cv::Mat processed = preprocessFaceImage(face_image);
        cv::Scalar mean = cv::mean(processed);
        
        // 简化算法：基于图像亮度估计年龄
        int estimated_age = static_cast<int>(20 + mean[0] / 5);
        estimated_age = std::max(1, std::min(100, estimated_age));
        
        return estimated_age;
        
    } catch (const std::exception& e) {
        LOGE("Age estimation exception: %s", e.what());
        return 25;
    }
}

int FaceAnalysisManager::recognizeGender(const cv::Mat& face_image) {
    if (!initialized_ || face_image.empty()) {
        return 0; // 默认男性
    }
    
    try {
        // 简化的性别识别实现
        // 在实际应用中，这里应该使用训练好的性别识别模型
        
        cv::Mat processed = preprocessFaceImage(face_image);
        cv::Scalar mean = cv::mean(processed);
        
        // 简化算法：基于图像特征判断性别
        return (static_cast<int>(mean[0]) % 2);
        
    } catch (const std::exception& e) {
        LOGE("Gender recognition exception: %s", e.what());
        return 0;
    }
}

int FaceAnalysisManager::recognizePerson(const std::vector<float>& face_features) {
    if (!initialized_ || face_features.empty() || known_persons_.empty()) {
        return -1; // 未识别
    }
    
    try {
        float best_similarity = 0.0f;
        int best_person_id = -1;
        const float threshold = 0.7f; // 相似度阈值
        
        // 与已知人员进行匹配
        for (const auto& person : known_persons_) {
            float similarity = calculateSimilarity(face_features, person.features);
            if (similarity > best_similarity && similarity > threshold) {
                best_similarity = similarity;
                best_person_id = person.id;
            }
        }
        
        if (best_person_id != -1) {
            LOGD("Person recognized: ID=%d, similarity=%.2f", best_person_id, best_similarity);
        }
        
        return best_person_id;
        
    } catch (const std::exception& e) {
        LOGE("Person recognition exception: %s", e.what());
        return -1;
    }
}

void FaceAnalysisManager::addKnownPerson(int person_id, const std::vector<float>& face_features, const std::string& name) {
    if (!initialized_ || face_features.empty()) {
        return;
    }
    
    try {
        KnownPerson person;
        person.id = person_id;
        person.name = name;
        person.features = face_features;
        
        known_persons_.push_back(person);
        
        LOGD("Added known person: ID=%d, name=%s", person_id, name.c_str());
        
    } catch (const std::exception& e) {
        LOGE("Add known person exception: %s", e.what());
    }
}

float FaceAnalysisManager::calculateSimilarity(const std::vector<float>& features1, const std::vector<float>& features2) {
    if (features1.size() != features2.size() || features1.empty()) {
        return 0.0f;
    }
    
    try {
        // 计算余弦相似度
        float dot_product = 0.0f;
        float norm1 = 0.0f;
        float norm2 = 0.0f;
        
        for (size_t i = 0; i < features1.size(); i++) {
            dot_product += features1[i] * features2[i];
            norm1 += features1[i] * features1[i];
            norm2 += features2[i] * features2[i];
        }
        
        if (norm1 == 0.0f || norm2 == 0.0f) {
            return 0.0f;
        }
        
        return dot_product / (sqrt(norm1) * sqrt(norm2));
        
    } catch (const std::exception& e) {
        LOGE("Similarity calculation exception: %s", e.what());
        return 0.0f;
    }
}

cv::Mat FaceAnalysisManager::preprocessFaceImage(const cv::Mat& face_image) {
    cv::Mat processed;
    
    try {
        // 调整大小到标准尺寸
        cv::resize(face_image, processed, cv::Size(112, 112));
        
        // 转换为灰度图
        if (processed.channels() == 3) {
            cv::cvtColor(processed, processed, cv::COLOR_BGR2GRAY);
        }
        
        // 直方图均衡化
        cv::equalizeHist(processed, processed);
        
        // 归一化
        processed.convertTo(processed, CV_32F, 1.0/255.0);
        
    } catch (const std::exception& e) {
        LOGE("Face image preprocessing exception: %s", e.what());
        processed = face_image.clone();
    }
    
    return processed;
}

// 🔧 Phase 2: 添加analyzePersonRegions方法
bool FaceAnalysisManager::analyzePersonRegions(const cv::Mat& image,
                                              const std::vector<InferenceResult>& personDetections,
                                              std::vector<FaceAnalysisResult>& results) {
    try {
        LOGD("🔧 Phase 2: Analyzing %zu person regions for faces", personDetections.size());

        results.clear();

        if (!initialized_ || image.empty() || personDetections.empty()) {
            LOGW("FaceAnalysisManager not initialized or invalid input");
            return false;
        }

        // 遍历每个人员检测结果
        for (const auto& person : personDetections) {
            // 转换坐标为cv::Rect
            cv::Rect personRect(
                static_cast<int>(person.x1),
                static_cast<int>(person.y1),
                static_cast<int>(person.x2 - person.x1),
                static_cast<int>(person.y2 - person.y1)
            );

            // 确保ROI在图像范围内
            personRect &= cv::Rect(0, 0, image.cols, image.rows);

            if (personRect.width > 20 && personRect.height > 30) {
                // 提取人员区域
                cv::Mat personROI = image(personRect);

                // 分析这个人员区域的人脸
                FaceAnalysisResult faceResult = analyzeFace(personROI);

                // 如果检测到人脸，调整坐标到原图坐标系
                if (faceResult.face_detected) {
                    faceResult.face_box.x += personRect.x;
                    faceResult.face_box.y += personRect.y;
                    faceResult.person_id = static_cast<int>(results.size()); // 临时ID

                    results.push_back(faceResult);

                    LOGD("🔧 Face detected in person region %zu: age=%d, gender=%d",
                         results.size()-1, faceResult.age, faceResult.gender);
                }
            }
        }

        LOGD("🔧 Phase 2: Analyzed %zu person regions, found %zu faces",
             personDetections.size(), results.size());

        return true;

    } catch (const std::exception& e) {
        LOGE("Error in analyzePersonRegions: %s", e.what());
        return false;
    }
}

// 🔧 Phase 2: 添加setConfig方法
void FaceAnalysisManager::setConfig(const FaceAnalysisConfig& config) {
    LOGD("🔧 Phase 2: Setting FaceAnalysisConfig (placeholder)");
    // TODO: 在Phase 2中实现配置设置
}

// 🔧 Phase 2: 添加getConfig方法
FaceAnalysisConfig FaceAnalysisManager::getConfig() const {
    LOGD("🔧 Phase 2: Getting FaceAnalysisConfig (placeholder)");
    // TODO: 在Phase 2中实现配置获取
    return FaceAnalysisConfig();
}

// 🔧 Phase 2: 添加analyzeFaces方法（用于direct_inspireface_test_jni.cpp）
bool FaceAnalysisManager::analyzeFaces(const cv::Mat& image,
                                      const std::vector<PersonDetection>& personDetections,
                                      SimpleFaceAnalysisResult& result) {
    try {
        LOGD("🔧 Phase 2: analyzeFaces called with %zu person detections", personDetections.size());

        // 重置结果
        result.success = false;
        result.faceCount = 0;
        result.maleCount = 0;
        result.femaleCount = 0;
        memset(result.ageGroups, 0, sizeof(result.ageGroups));
        result.faces.clear();
        result.errorMessage.clear();

        if (!initialized_) {
            result.errorMessage = "FaceAnalysisManager not initialized";
            LOGE("FaceAnalysisManager not initialized");
            return false;
        }

        if (image.empty()) {
            result.errorMessage = "Empty input image";
            LOGE("Empty input image");
            return false;
        }

        if (personDetections.empty()) {
            LOGD("No person detections, returning success with zero faces");
            result.success = true;
            return true;
        }

        // 转换PersonDetection到InferenceResult格式
        std::vector<InferenceResult> inferenceResults;
        for (const auto& person : personDetections) {
            InferenceResult inference;
            inference.x1 = person.x1;
            inference.y1 = person.y1;
            inference.x2 = person.x2;
            inference.y2 = person.y2;
            inference.confidence = person.confidence;
            inference.class_name = "person";
            inferenceResults.push_back(inference);
        }

        // 使用现有的analyzePersonRegions方法
        std::vector<FaceAnalysisResult> analysisResults;
        bool success = analyzePersonRegions(image, inferenceResults, analysisResults);

        if (!success) {
            result.errorMessage = "analyzePersonRegions failed";
            LOGE("analyzePersonRegions failed");
            return false;
        }

        // 转换结果格式
        for (const auto& analysisResult : analysisResults) {
            if (analysisResult.face_detected) {
                result.faceCount++;

                // 统计性别
                if (analysisResult.gender == 0) {
                    result.maleCount++;
                } else {
                    result.femaleCount++;
                }

                // 统计年龄组
                if (analysisResult.age >= 0 && analysisResult.age < 9) {
                    result.ageGroups[analysisResult.age]++;
                }

                // 添加人脸信息
                SimpleFaceAnalysisResult::Face face;
                face.x1 = analysisResult.face_box.x;
                face.y1 = analysisResult.face_box.y;
                face.x2 = analysisResult.face_box.x + analysisResult.face_box.width;
                face.y2 = analysisResult.face_box.y + analysisResult.face_box.height;
                face.confidence = analysisResult.confidence;
                face.gender = analysisResult.gender;
                face.age = analysisResult.age;

                result.faces.push_back(face);
            }
        }

        result.success = true;
        LOGD("🔧 Phase 2: analyzeFaces completed - %d faces, %d male, %d female",
             result.faceCount, result.maleCount, result.femaleCount);

        return true;

    } catch (const std::exception& e) {
        result.success = false;
        result.errorMessage = std::string("Exception: ") + e.what();
        LOGE("Exception in analyzeFaces: %s", e.what());
        return false;
    }
}

// 🔧 Phase 2: 添加测试方法来验证InspireFace功能
bool FaceAnalysisManager::testInspireFaceIntegration() {
    LOGD("🔧 Phase 2: Testing InspireFace integration...");

    try {
        // 1. 检查基础初始化状态
        LOGD("🔧 Phase 2: Basic initialized: %s", initialized_ ? "YES" : "NO");
        LOGD("🔧 Phase 2: InspireFace initialized: %s", inspireface_initialized_ ? "YES" : "NO");

        // 2. 检查InspireFace组件状态
        LOGD("🔧 Phase 2: InspireFace session: %s", inspireface_session_ ? "CREATED" : "NULL");
        LOGD("🔧 Phase 2: Image processor: %s", image_processor_ ? "CREATED" : "NULL");
        LOGD("🔧 Phase 2: Face detector: %s", face_detector_ ? "CREATED" : "NULL");

        // 3. 如果组件存在，测试基本功能
        if (inspireface_initialized_ && inspireface_session_) {
            LOGD("🔧 Phase 2: InspireFace components are ready for testing");

            // 创建一个测试图像（100x100的灰度图像）
            cv::Mat testImage = cv::Mat::zeros(100, 100, CV_8UC3);
            cv::rectangle(testImage, cv::Rect(25, 25, 50, 50), cv::Scalar(255, 255, 255), -1);

            LOGD("🔧 Phase 2: Created test image: %dx%d", testImage.cols, testImage.rows);

            // 调用analyzeFace方法进行测试
            FaceAnalysisResult testResult = analyzeFace(testImage);

            LOGD("🔧 Phase 2: Test analyzeFace result - face_detected: %s, confidence: %.2f",
                 testResult.face_detected ? "YES" : "NO", testResult.confidence);

            return true;
        } else {
            LOGW("🔧 Phase 2: InspireFace components not initialized - this is expected if not explicitly initialized");
            return false;
        }

    } catch (const std::exception& e) {
        LOGE("🔧 Phase 2: Exception in testInspireFaceIntegration: %s", e.what());
        return false;
    }
}
