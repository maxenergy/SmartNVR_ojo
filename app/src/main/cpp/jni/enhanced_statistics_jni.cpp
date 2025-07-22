/**
 * @file enhanced_statistics_jni.cpp
 * @brief Phase 1: 增强统计功能的JNI接口实现
 * @author AI Assistant
 * @date 2025-07-22
 */

#include <jni.h>
#include <android/log.h>
#include <string>
#include <map>
#include "../include/statistics_manager.h"

#define TAG "EnhancedStatisticsJNI"
#define LOGD(...) __android_log_print(ANDROID_LOG_DEBUG, TAG, __VA_ARGS__)
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR, TAG, __VA_ARGS__)

// 🔧 Phase 1: 完整的getCurrentStatistics JNI实现
extern "C" JNIEXPORT jobject JNICALL
Java_com_wulala_myyolov5rtspthreadpool_DirectInspireFaceTest_getCurrentStatistics(
    JNIEnv *env, jclass clazz) {
    
    try {
        LOGD("🔧 JNI: 开始获取当前统计数据");
        
        // 获取所有摄像头的统计数据
        auto all_stats = g_stats_collector.getAllStats();
        
        // 创建BatchStatisticsResult对象
        jclass resultClass = env->FindClass("com/wulala/myyolov5rtspthreadpool/BatchStatisticsResult");
        if (!resultClass) {
            LOGE("❌ 找不到BatchStatisticsResult类");
            return nullptr;
        }
        
        jmethodID constructor = env->GetMethodID(resultClass, "<init>", "()V");
        if (!constructor) {
            LOGE("❌ 找不到BatchStatisticsResult构造函数");
            return nullptr;
        }
        
        jobject result = env->NewObject(resultClass, constructor);
        if (!result) {
            LOGE("❌ 创建BatchStatisticsResult对象失败");
            return nullptr;
        }
        
        // 计算汇总统计数据
        int total_person_count = 0;
        int total_face_count = 0;
        int male_count = 0;
        int female_count = 0;
        double total_processing_time = 0.0;
        int total_analysis_count = 0;
        
        for (const auto& pair : all_stats) {
            const auto& stats = pair.second;
            total_person_count += stats.current_person_count;
            total_processing_time += stats.avg_detection_time;
            total_analysis_count += stats.frames_processed;
        }
        
        double avg_processing_time = total_analysis_count > 0 ? total_processing_time / all_stats.size() : 0.0;
        double success_rate = total_analysis_count > 0 ? 100.0 : 0.0;
        
        // 设置基础字段
        jfieldID successField = env->GetFieldID(resultClass, "success", "Z");
        jfieldID personCountField = env->GetFieldID(resultClass, "personCount", "I");
        jfieldID totalFaceCountField = env->GetFieldID(resultClass, "totalFaceCount", "I");
        jfieldID maleCountField = env->GetFieldID(resultClass, "maleCount", "I");
        jfieldID femaleCountField = env->GetFieldID(resultClass, "femaleCount", "I");
        jfieldID avgProcessingTimeField = env->GetFieldID(resultClass, "averageProcessingTime", "D");
        jfieldID totalAnalysisCountField = env->GetFieldID(resultClass, "totalAnalysisCount", "I");
        jfieldID successRateField = env->GetFieldID(resultClass, "successRate", "D");
        jfieldID errorMessageField = env->GetFieldID(resultClass, "errorMessage", "Ljava/lang/String;");
        
        if (successField && personCountField && totalFaceCountField && maleCountField && 
            femaleCountField && avgProcessingTimeField && totalAnalysisCountField && 
            successRateField && errorMessageField) {
            
            env->SetBooleanField(result, successField, JNI_TRUE);
            env->SetIntField(result, personCountField, total_person_count);
            env->SetIntField(result, totalFaceCountField, total_face_count);
            env->SetIntField(result, maleCountField, male_count);
            env->SetIntField(result, femaleCountField, female_count);
            env->SetDoubleField(result, avgProcessingTimeField, avg_processing_time);
            env->SetIntField(result, totalAnalysisCountField, total_analysis_count);
            env->SetDoubleField(result, successRateField, success_rate);
            
            jstring emptyMessage = env->NewStringUTF("");
            env->SetObjectField(result, errorMessageField, emptyMessage);
            env->DeleteLocalRef(emptyMessage);
            
            LOGD("✅ JNI: 成功返回统计数据 - %d人员, %d分析次数, %.1fms平均处理时间", 
                 total_person_count, total_analysis_count, avg_processing_time);
        } else {
            LOGE("❌ 获取BatchStatisticsResult字段失败");
        }
        
        env->DeleteLocalRef(resultClass);
        return result;
        
    } catch (const std::exception& e) {
        LOGE("❌ JNI getCurrentStatistics异常: %s", e.what());
        return nullptr;
    } catch (...) {
        LOGE("❌ JNI getCurrentStatistics未知异常");
        return nullptr;
    }
}

// 🔧 Phase 1: 获取指定摄像头统计数据的JNI接口
extern "C" JNIEXPORT jobject JNICALL
Java_com_wulala_myyolov5rtspthreadpool_DirectInspireFaceTest_getCameraStatistics(
    JNIEnv *env, jclass clazz, jint camera_id) {
    
    try {
        LOGD("🔧 JNI: 获取Camera %d统计数据", camera_id);
        
        auto stats = g_stats_collector.getCameraStats(camera_id);
        
        // 创建简化的统计结果对象
        jclass resultClass = env->FindClass("com/wulala/myyolov5rtspthreadpool/BatchStatisticsResult");
        if (!resultClass) {
            LOGE("❌ 找不到BatchStatisticsResult类");
            return nullptr;
        }
        
        jmethodID constructor = env->GetMethodID(resultClass, "<init>", "()V");
        jobject result = env->NewObject(resultClass, constructor);
        
        // 设置字段
        jfieldID successField = env->GetFieldID(resultClass, "success", "Z");
        jfieldID personCountField = env->GetFieldID(resultClass, "personCount", "I");
        jfieldID totalAnalysisCountField = env->GetFieldID(resultClass, "totalAnalysisCount", "I");
        jfieldID avgProcessingTimeField = env->GetFieldID(resultClass, "averageProcessingTime", "D");
        
        env->SetBooleanField(result, successField, JNI_TRUE);
        env->SetIntField(result, personCountField, stats.current_person_count);
        env->SetIntField(result, totalAnalysisCountField, stats.frames_processed);
        env->SetDoubleField(result, avgProcessingTimeField, stats.avg_detection_time);
        
        LOGD("✅ JNI: Camera %d统计 - %d人员, %d帧处理, %.1fms平均时间", 
             camera_id, stats.current_person_count, stats.frames_processed, stats.avg_detection_time);
        
        env->DeleteLocalRef(resultClass);
        return result;
        
    } catch (const std::exception& e) {
        LOGE("❌ JNI getCameraStatistics异常: %s", e.what());
        return nullptr;
    }
}

// 🔧 Phase 1: 重置统计数据的JNI接口
extern "C" JNIEXPORT void JNICALL
Java_com_wulala_myyolov5rtspthreadpool_DirectInspireFaceTest_resetStatistics(
    JNIEnv *env, jclass clazz) {
    
    try {
        LOGD("🔧 JNI: 重置统计数据");
        g_stats_collector.resetStats();
        LOGD("✅ JNI: 统计数据重置完成");
    } catch (const std::exception& e) {
        LOGE("❌ JNI resetStatistics异常: %s", e.what());
    }
}

// 🔧 Phase 1: 获取性能指标的JNI接口
extern "C" JNIEXPORT jdoubleArray JNICALL
Java_com_wulala_myyolov5rtspthreadpool_DirectInspireFaceTest_getPerformanceMetrics(
    JNIEnv *env, jclass clazz, jint camera_id) {
    
    try {
        LOGD("🔧 JNI: 获取Camera %d性能指标", camera_id);
        
        auto stats = g_stats_collector.getCameraStats(camera_id);
        
        // 创建double数组返回性能指标
        jdoubleArray result = env->NewDoubleArray(4);
        if (!result) {
            LOGE("❌ 创建性能指标数组失败");
            return nullptr;
        }
        
        jdouble metrics[4] = {
            stats.avg_detection_time,    // 平均检测时间
            stats.avg_tracking_time,     // 平均跟踪时间
            static_cast<double>(stats.frames_processed),  // 处理帧数
            static_cast<double>(stats.frames_skipped)     // 跳过帧数
        };
        
        env->SetDoubleArrayRegion(result, 0, 4, metrics);
        
        LOGD("✅ JNI: Camera %d性能指标 - 检测%.1fms, 跟踪%.1fms, 处理%d帧", 
             camera_id, stats.avg_detection_time, stats.avg_tracking_time, stats.frames_processed);
        
        return result;
        
    } catch (const std::exception& e) {
        LOGE("❌ JNI getPerformanceMetrics异常: %s", e.what());
        return nullptr;
    }
}
