#include <jni.h>
#include <android/log.h>
#include <opencv2/opencv.hpp>
#include <vector>
#include <string>
#include <memory>

#include "../engine/inference_manager.h"
#include "../types/model_config.h"
#include "../include/logging.h"
#include "../statistics/statistics_manager.h"
#include "../include/ZLPlayer.h"

static const char* YOLO_TAG = "RealYOLOInferenceJNI";

// 全局推理管理器实例
static std::unique_ptr<InferenceManager> g_inference_manager = nullptr;
static bool g_initialized = false;

// 🔧 新增：外部统计管理器访问
extern std::unique_ptr<StatisticsManager> g_globalStatisticsManager;
extern std::mutex g_managerMutex;

extern "C" {

/**
 * 初始化真实YOLO推理引擎
 */
JNIEXPORT jint JNICALL
Java_com_wulala_myyolov5rtspthreadpool_RealYOLOInference_initializeYOLO(
    JNIEnv *env, jclass clazz, jstring model_path) {
    
    LOGI("RealYOLOInference: 开始初始化YOLO推理引擎");
    
    if (g_initialized) {
        LOGW("RealYOLOInference: 引擎已经初始化");
        return 0;
    }
    
    try {
        // 获取模型路径
        const char* path_cstr = env->GetStringUTFChars(model_path, nullptr);
        std::string modelPath(path_cstr);
        env->ReleaseStringUTFChars(model_path, path_cstr);
        
        LOGI("RealYOLOInference: 模型路径: %s", modelPath.c_str());
        
        // 创建推理管理器
        g_inference_manager.reset(new InferenceManager());

        // 配置YOLOv5模型
        ModelConfig yolov5_config = ModelConfig::getYOLOv5Config();
        yolov5_config.model_path = modelPath;

        // 初始化推理管理器（只使用YOLOv5）
        if (g_inference_manager->initialize(yolov5_config, nullptr) != 0) {
            LOGE("RealYOLOInference: 推理管理器初始化失败");
            g_inference_manager.reset();
            return -1;
        }

        // 设置当前模型为YOLOv5
        if (g_inference_manager->setCurrentModel(ModelType::YOLOV5) != 0) {
            LOGE("RealYOLOInference: 设置YOLOv5模型失败");
            g_inference_manager.reset();
            return -2;
        }
        
        g_initialized = true;
        LOGI("RealYOLOInference: YOLO推理引擎初始化成功");
        return 0;
        
    } catch (const std::exception& e) {
        LOGE("RealYOLOInference: 初始化异常: %s", e.what());
        g_inference_manager.reset();
        return -3;
    }
}

/**
 * 执行真实YOLO推理
 */
JNIEXPORT jobjectArray JNICALL
Java_com_wulala_myyolov5rtspthreadpool_RealYOLOInference_performInference(
    JNIEnv *env, jclass clazz, jbyteArray image_data, jint width, jint height) {
    
    if (!g_initialized || !g_inference_manager) {
        LOGE("RealYOLOInference: 引擎未初始化");
        return nullptr;
    }
    
    try {
        // 获取图像数据
        jbyte* data = env->GetByteArrayElements(image_data, nullptr);
        jsize data_length = env->GetArrayLength(image_data);
        
        LOGD("RealYOLOInference: 开始推理，图像尺寸: %dx%d, 数据长度: %d", width, height, data_length);
        
        // 创建OpenCV Mat
        cv::Mat image;
        if (data_length == width * height * 3) {
            // RGB/BGR格式
            image = cv::Mat(height, width, CV_8UC3, data);
        } else if (data_length == width * height) {
            // 灰度格式
            image = cv::Mat(height, width, CV_8UC1, data);
            cv::cvtColor(image, image, cv::COLOR_GRAY2BGR);
        } else {
            LOGE("RealYOLOInference: 不支持的图像格式，数据长度: %d", data_length);
            env->ReleaseByteArrayElements(image_data, data, JNI_ABORT);
            return nullptr;
        }
        
        // 执行推理
        InferenceResultGroup results;
        auto start_time = std::chrono::high_resolution_clock::now();
        
        int ret = g_inference_manager->inference(image, results);
        
        auto end_time = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time);
        
        env->ReleaseByteArrayElements(image_data, data, JNI_ABORT);
        
        if (ret != 0) {
            LOGE("RealYOLOInference: 推理失败，错误码: %d", ret);
            return nullptr;
        }
        
        LOGD("RealYOLOInference: 推理完成，检测到 %zu 个目标，耗时: %lld ms", 
             results.results.size(), duration.count());
        
        // 创建Java结果数组
        jclass detection_class = env->FindClass("com/wulala/myyolov5rtspthreadpool/DetectionResult");
        if (!detection_class) {
            LOGE("RealYOLOInference: 找不到DetectionResult类");
            return nullptr;
        }
        
        jmethodID constructor = env->GetMethodID(detection_class, "<init>", 
                                               "(IFFFFFLjava/lang/String;)V");
        if (!constructor) {
            LOGE("RealYOLOInference: 找不到DetectionResult构造函数");
            return nullptr;
        }
        
        jobjectArray result_array = env->NewObjectArray(results.results.size(), detection_class, nullptr);
        
        for (size_t i = 0; i < results.results.size(); i++) {
            const InferenceResult& result = results.results[i];
            
            jstring class_name = env->NewStringUTF(result.class_name.c_str());
            
            jobject detection_obj = env->NewObject(detection_class, constructor,
                                                  result.class_id,
                                                  result.confidence,
                                                  result.x1, result.y1,
                                                  result.x2, result.y2,
                                                  class_name);
            
            env->SetObjectArrayElement(result_array, i, detection_obj);
            
            env->DeleteLocalRef(class_name);
            env->DeleteLocalRef(detection_obj);
        }
        
        env->DeleteLocalRef(detection_class);
        
        LOGD("RealYOLOInference: 成功返回 %zu 个检测结果", results.results.size());
        return result_array;
        
    } catch (const std::exception& e) {
        LOGE("RealYOLOInference: 推理异常: %s", e.what());
        return nullptr;
    }
}

/**
 * 获取引擎状态信息
 */
JNIEXPORT jstring JNICALL
Java_com_wulala_myyolov5rtspthreadpool_RealYOLOInference_getEngineStatus(
    JNIEnv *env, jclass clazz) {
    
    std::string status;
    
    if (g_initialized && g_inference_manager) {
        status = "YOLO推理引擎状态:\n";
        status += "- 引擎已初始化: ✅\n";
        status += "- 当前模型: YOLOv5\n";
        
        // 检查模型初始化状态
        bool yolov5_ready = g_inference_manager->isModelInitialized(ModelType::YOLOV5);
        status += "- YOLOv5模型: " + std::string(yolov5_ready ? "✅ 就绪" : "❌ 未就绪") + "\n";

        ModelType current_model = g_inference_manager->getCurrentModel();
        status += "- 活动模型: " + std::string(current_model == ModelType::YOLOV5 ? "YOLOv5" : "YOLOv8n") + "\n";
        
    } else {
        status = "YOLO推理引擎状态:\n";
        status += "- 引擎已初始化: ❌\n";
        status += "- 需要先调用initializeYOLO()方法\n";
    }
    
    return env->NewStringUTF(status.c_str());
}

/**
 * 释放推理引擎资源
 */
JNIEXPORT void JNICALL
Java_com_wulala_myyolov5rtspthreadpool_RealYOLOInference_releaseEngine(
    JNIEnv *env, jclass clazz) {
    
    LOGI("RealYOLOInference: 释放推理引擎资源");
    
    if (g_inference_manager) {
        g_inference_manager->release();
        g_inference_manager.reset();
    }
    
    g_initialized = false;
    LOGI("RealYOLOInference: 推理引擎资源已释放");
}

/**
 * 检查引擎是否已初始化
 */
JNIEXPORT jboolean JNICALL
Java_com_wulala_myyolov5rtspthreadpool_RealYOLOInference_isInitialized(
    JNIEnv *env, jclass clazz) {

    return g_initialized && g_inference_manager != nullptr;
}

/**
 * 🔧 新增：获取实时统计数据
 * 高效的JNI接口，一次性返回所有统计信息
 */
JNIEXPORT jobject JNICALL
Java_com_wulala_myyolov5rtspthreadpool_RealYOLOInference_getRealTimeStatistics(
    JNIEnv *env, jclass clazz) {

    try {
        // 获取统计数据
        StatisticsData currentStats;
        bool hasData = false;

        {
            std::lock_guard<std::mutex> lock(g_managerMutex);
            if (g_globalStatisticsManager) {
                currentStats = g_globalStatisticsManager->getCurrentStatistics();
                hasData = true;
            }
        }

        // 创建Java对象
        jclass resultClass = env->FindClass("com/wulala/myyolov5rtspthreadpool/BatchStatisticsResult");
        if (!resultClass) {
            LOGE("Failed to find BatchStatisticsResult class");
            return nullptr;
        }

        jmethodID constructor = env->GetMethodID(resultClass, "<init>", "()V");
        if (!constructor) {
            LOGE("Failed to find BatchStatisticsResult constructor");
            return nullptr;
        }

        jobject result = env->NewObject(resultClass, constructor);
        if (!result) {
            LOGE("Failed to create BatchStatisticsResult object");
            return nullptr;
        }

        // 设置字段
        jfieldID successField = env->GetFieldID(resultClass, "success", "Z");
        jfieldID personCountField = env->GetFieldID(resultClass, "personCount", "I");
        jfieldID maleCountField = env->GetFieldID(resultClass, "maleCount", "I");
        jfieldID femaleCountField = env->GetFieldID(resultClass, "femaleCount", "I");
        jfieldID totalFaceCountField = env->GetFieldID(resultClass, "totalFaceCount", "I");
        jfieldID ageBracketsField = env->GetFieldID(resultClass, "ageBrackets", "[I");

        if (hasData) {
            env->SetBooleanField(result, successField, JNI_TRUE);
            env->SetIntField(result, personCountField, currentStats.totalPersonCount);
            env->SetIntField(result, maleCountField, currentStats.maleCount);
            env->SetIntField(result, femaleCountField, currentStats.femaleCount);
            env->SetIntField(result, totalFaceCountField, currentStats.totalFaceCount);

            // 设置年龄分布数组
            jintArray ageArray = env->NewIntArray(9);
            if (ageArray) {
                jint ageData[9];
                for (int i = 0; i < 9; i++) {
                    ageData[i] = (i < currentStats.ageBracketCounts.size()) ?
                                currentStats.ageBracketCounts[i] : 0;
                }
                env->SetIntArrayRegion(ageArray, 0, 9, ageData);
                env->SetObjectField(result, ageBracketsField, ageArray);
                env->DeleteLocalRef(ageArray);
            }

            LOGD("RealYOLOInference: 返回统计数据 - 人员:%d, 男性:%d, 女性:%d, 人脸:%d",
                 currentStats.totalPersonCount, currentStats.maleCount,
                 currentStats.femaleCount, currentStats.totalFaceCount);
        } else {
            env->SetBooleanField(result, successField, JNI_FALSE);
            LOGD("RealYOLOInference: 统计管理器未初始化，返回空数据");
        }

        return result;

    } catch (const std::exception& e) {
        LOGE("RealYOLOInference: getRealTimeStatistics exception: %s", e.what());
        return nullptr;
    } catch (...) {
        LOGE("RealYOLOInference: getRealTimeStatistics unknown exception");
        return nullptr;
    }
}

/**
 * 🔧 新增：重置统计数据
 */
JNIEXPORT void JNICALL
Java_com_wulala_myyolov5rtspthreadpool_RealYOLOInference_resetStatistics(
    JNIEnv *env, jclass clazz) {

    try {
        std::lock_guard<std::mutex> lock(g_managerMutex);
        if (g_globalStatisticsManager) {
            // 重新创建统计管理器实现重置
            g_globalStatisticsManager.reset(new StatisticsManager());
            LOGD("RealYOLOInference: 统计数据已重置");
        }
    } catch (const std::exception& e) {
        LOGE("RealYOLOInference: resetStatistics exception: %s", e.what());
    }
}

} // extern "C"
