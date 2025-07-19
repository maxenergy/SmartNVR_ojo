#include <jni.h>
#include <android/bitmap.h>
#include <opencv2/opencv.hpp>
#include "engine/extended_inference_manager.h"
#include "log4c.h"

// JNI方法名映射
extern "C" {

// 全局ExtendedInferenceManager实例
static ExtendedInferenceManager* g_extendedInferenceManager = nullptr;

/**
 * 初始化扩展推理管理器
 */
JNIEXPORT jint JNICALL
Java_com_wulala_myyolov5rtspthreadpool_ExtendedInferenceJNI_initializeExtendedInference(
    JNIEnv* env, jobject thiz, jstring yolov5_model_path, jstring yolov8_model_path) {
    
    LOGI("Initializing ExtendedInferenceManager");
    
    if (g_extendedInferenceManager != nullptr) {
        LOGW("ExtendedInferenceManager already initialized");
        return 0;
    }
    
    // 创建ExtendedInferenceManager实例
    g_extendedInferenceManager = new ExtendedInferenceManager();
    
    // 获取模型路径
    const char* yolov5_path = env->GetStringUTFChars(yolov5_model_path, nullptr);
    const char* yolov8_path = yolov8_model_path ? env->GetStringUTFChars(yolov8_model_path, nullptr) : nullptr;
    
    // 创建模型配置
    ModelConfig yolov5_config;
    // TODO: 设置YOLOv5配置参数
    
    ModelConfig* yolov8_config_ptr = nullptr;
    ModelConfig yolov8_config;
    if (yolov8_path) {
        // TODO: 设置YOLOv8配置参数
        yolov8_config_ptr = &yolov8_config;
    }
    
    // 初始化管理器
    bool success = g_extendedInferenceManager->initialize(yolov5_config, yolov8_config_ptr);
    
    // 释放字符串资源
    env->ReleaseStringUTFChars(yolov5_model_path, yolov5_path);
    if (yolov8_path) {
        env->ReleaseStringUTFChars(yolov8_model_path, yolov8_path);
    }
    
    if (!success) {
        LOGE("Failed to initialize ExtendedInferenceManager");
        delete g_extendedInferenceManager;
        g_extendedInferenceManager = nullptr;
        return -1;
    }
    
    LOGI("ExtendedInferenceManager initialized successfully");
    return 0;
}

/**
 * 初始化人脸分析功能
 */
JNIEXPORT jint JNICALL
Java_com_wulala_myyolov5rtspthreadpool_ExtendedInferenceJNI_initializeFaceAnalysis(
    JNIEnv* env, jobject thiz, jstring model_path) {
    
    if (!g_extendedInferenceManager) {
        LOGE("ExtendedInferenceManager not initialized");
        return -1;
    }
    
    const char* path = env->GetStringUTFChars(model_path, nullptr);
    bool success = g_extendedInferenceManager->initializeFaceAnalysis(std::string(path));
    env->ReleaseStringUTFChars(model_path, path);
    
    return success ? 0 : -1;
}

/**
 * 初始化统计功能
 */
JNIEXPORT jint JNICALL
Java_com_wulala_myyolov5rtspthreadpool_ExtendedInferenceJNI_initializeStatistics(
    JNIEnv* env, jobject thiz) {
    
    if (!g_extendedInferenceManager) {
        LOGE("ExtendedInferenceManager not initialized");
        return -1;
    }
    
    bool success = g_extendedInferenceManager->initializeStatistics();
    return success ? 0 : -1;
}

/**
 * 设置当前模型
 */
JNIEXPORT jint JNICALL
Java_com_wulala_myyolov5rtspthreadpool_ExtendedInferenceJNI_setCurrentModel(
    JNIEnv* env, jobject thiz, jint model_type) {
    
    if (!g_extendedInferenceManager) {
        LOGE("ExtendedInferenceManager not initialized");
        return -1;
    }
    
    ModelType type = (model_type == 0) ? ModelType::YOLOV5 : ModelType::YOLOV8N;
    return g_extendedInferenceManager->setCurrentModel(type);
}

/**
 * 获取当前模型类型
 */
JNIEXPORT jint JNICALL
Java_com_wulala_myyolov5rtspthreadpool_ExtendedInferenceJNI_getCurrentModel(
    JNIEnv* env, jobject thiz) {
    
    if (!g_extendedInferenceManager) {
        LOGE("ExtendedInferenceManager not initialized");
        return -1;
    }
    
    ModelType type = g_extendedInferenceManager->getCurrentModel();
    return (type == ModelType::YOLOV5) ? 0 : 1;
}

} // extern "C"

/**
 * 辅助函数：将Android Bitmap转换为OpenCV Mat
 */
cv::Mat bitmapToMat(JNIEnv* env, jobject bitmap) {
    AndroidBitmapInfo info;
    void* pixels;
    
    if (AndroidBitmap_getInfo(env, bitmap, &info) < 0) {
        LOGE("Failed to get bitmap info");
        return cv::Mat();
    }
    
    if (AndroidBitmap_lockPixels(env, bitmap, &pixels) < 0) {
        LOGE("Failed to lock bitmap pixels");
        return cv::Mat();
    }
    
    cv::Mat mat;
    if (info.format == ANDROID_BITMAP_FORMAT_RGBA_8888) {
        mat = cv::Mat(info.height, info.width, CV_8UC4, pixels);
        cv::cvtColor(mat, mat, cv::COLOR_RGBA2BGR);
    } else if (info.format == ANDROID_BITMAP_FORMAT_RGB_565) {
        mat = cv::Mat(info.height, info.width, CV_8UC2, pixels);
        cv::cvtColor(mat, mat, cv::COLOR_BGR5652BGR);
    } else {
        LOGE("Unsupported bitmap format: %d", info.format);
        AndroidBitmap_unlockPixels(env, bitmap);
        return cv::Mat();
    }
    
    // 创建副本，因为原始数据会被unlock
    cv::Mat result = mat.clone();
    
    AndroidBitmap_unlockPixels(env, bitmap);
    return result;
}

/**
 * 辅助函数：创建Java Detection对象
 */
jobject createDetectionObject(JNIEnv* env, const InferenceResult& result) {
    // 获取Detection类
    jclass detectionClass = env->FindClass("com/wulala/myyolov5rtspthreadpool/entities/Detection");
    if (!detectionClass) {
        LOGE("Failed to find Detection class");
        return nullptr;
    }
    
    // 获取构造函数
    jmethodID constructor = env->GetMethodID(detectionClass, "<init>", 
        "(Ljava/lang/String;FFFFFF)V");
    if (!constructor) {
        LOGE("Failed to find Detection constructor");
        return nullptr;
    }
    
    // 创建类名字符串
    jstring className = env->NewStringUTF(result.class_name.c_str());
    
    // 创建Detection对象
    jobject detection = env->NewObject(detectionClass, constructor,
        className, result.confidence, result.x1, result.y1, result.x2, result.y2);
    
    env->DeleteLocalRef(className);
    env->DeleteLocalRef(detectionClass);
    
    return detection;
}

extern "C" {

/**
 * 执行扩展推理（包含人脸分析和统计）
 */
JNIEXPORT jobject JNICALL
Java_com_wulala_myyolov5rtspthreadpool_ExtendedInferenceJNI_extendedInference(
    JNIEnv* env, jobject thiz, jobject bitmap) {
    
    if (!g_extendedInferenceManager) {
        LOGE("ExtendedInferenceManager not initialized");
        return nullptr;
    }
    
    // 转换Bitmap到Mat
    cv::Mat image = bitmapToMat(env, bitmap);
    if (image.empty()) {
        LOGE("Failed to convert bitmap to Mat");
        return nullptr;
    }
    
    // 执行扩展推理
    ExtendedInferenceResult result;
    int ret = g_extendedInferenceManager->extendedInference(image, result);
    
    if (ret != 0) {
        LOGE("Extended inference failed with code: %d", ret);
        return nullptr;
    }
    
    // TODO: 创建并返回Java结果对象
    // 这里需要创建一个包含所有结果的Java对象
    
    LOGD("Extended inference completed successfully");
    return nullptr; // 暂时返回null，后续实现完整的结果转换
}

/**
 * 执行标准推理（仅目标检测）
 */
JNIEXPORT jobjectArray JNICALL
Java_com_wulala_myyolov5rtspthreadpool_ExtendedInferenceJNI_standardInference(
    JNIEnv* env, jobject thiz, jobject bitmap) {
    
    if (!g_extendedInferenceManager) {
        LOGE("ExtendedInferenceManager not initialized");
        return nullptr;
    }
    
    // 转换Bitmap到Mat
    cv::Mat image = bitmapToMat(env, bitmap);
    if (image.empty()) {
        LOGE("Failed to convert bitmap to Mat");
        return nullptr;
    }
    
    // 执行标准推理
    InferenceResultGroup results;
    int ret = g_extendedInferenceManager->inference(image, results);
    
    if (ret != 0) {
        LOGE("Standard inference failed with code: %d", ret);
        return nullptr;
    }
    
    // 创建Detection数组
    jclass detectionClass = env->FindClass("com/wulala/myyolov5rtspthreadpool/entities/Detection");
    jobjectArray detectionArray = env->NewObjectArray(results.results.size(), detectionClass, nullptr);
    
    for (size_t i = 0; i < results.results.size(); ++i) {
        jobject detection = createDetectionObject(env, results.results[i]);
        if (detection) {
            env->SetObjectArrayElement(detectionArray, i, detection);
            env->DeleteLocalRef(detection);
        }
    }
    
    env->DeleteLocalRef(detectionClass);
    return detectionArray;
}

/**
 * 获取当前统计数据
 */
JNIEXPORT jstring JNICALL
Java_com_wulala_myyolov5rtspthreadpool_ExtendedInferenceJNI_getCurrentStatistics(
    JNIEnv* env, jobject thiz) {
    
    if (!g_extendedInferenceManager) {
        LOGE("ExtendedInferenceManager not initialized");
        return nullptr;
    }
    
    std::string stats = g_extendedInferenceManager->getStatisticsSummary();
    return env->NewStringUTF(stats.c_str());
}

/**
 * 重置统计数据
 */
JNIEXPORT void JNICALL
Java_com_wulala_myyolov5rtspthreadpool_ExtendedInferenceJNI_resetStatistics(
    JNIEnv* env, jobject thiz) {
    
    if (g_extendedInferenceManager) {
        g_extendedInferenceManager->resetStatistics();
    }
}

/**
 * 释放扩展推理管理器
 */
JNIEXPORT void JNICALL
Java_com_wulala_myyolov5rtspthreadpool_ExtendedInferenceJNI_releaseExtendedInference(
    JNIEnv* env, jobject thiz) {
    
    LOGI("Releasing ExtendedInferenceManager");
    
    if (g_extendedInferenceManager) {
        g_extendedInferenceManager->release();
        delete g_extendedInferenceManager;
        g_extendedInferenceManager = nullptr;
    }
    
    LOGI("ExtendedInferenceManager released");
}

} // extern "C"
