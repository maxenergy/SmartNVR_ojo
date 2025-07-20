#include <jni.h>
#include <android/asset_manager_jni.h>
#include <string>
#include <vector>
#include "log4c.h"

// 引入InspireFace C API
extern "C" {
#include "inspireface.h"
#include "intypedef.h"
}

extern "C" {

/**
 * 测试实际的人脸检测功能
 */
JNIEXPORT jint JNICALL
Java_com_wulala_myyolov5rtspthreadpool_FaceDetectionTest_testFaceDetection(
    JNIEnv* env, jobject thiz, jobject asset_manager, jstring internal_data_path) {
    
    LOGI("=== 开始人脸检测功能测试 ===");
    
    // 获取AssetManager
    AAssetManager* assetManager = AAssetManager_fromJava(env, asset_manager);
    if (!assetManager) {
        LOGE("Failed to get AssetManager");
        return -1;
    }
    
    // 获取内部数据路径
    const char* dataPath = env->GetStringUTFChars(internal_data_path, nullptr);
    if (!dataPath) {
        LOGE("Failed to get internal data path");
        return -2;
    }
    
    LOGI("Internal data path: %s", dataPath);
    
    try {
        // 1. 初始化InspireFace库
        LOGI("Step 1: Initializing InspireFace library...");
        std::string archivePath = std::string(dataPath) + "/Gundam_RK3588";
        LOGI("Using archive path: %s", archivePath.c_str());
        
        HResult result = HFLaunchInspireFace(archivePath.c_str());
        if (result != HSUCCEED) {
            LOGE("Failed to initialize InspireFace library, error code: %ld", result);
            env->ReleaseStringUTFChars(internal_data_path, dataPath);
            return -3;
        }
        LOGI("✅ InspireFace library initialized successfully");
        
        // 2. 创建InspireFace会话
        LOGI("Step 2: Creating InspireFace session...");
        HFSession session = nullptr;
        
        // 初始化会话参数
        HFSessionCustomParameter parameter = {0};
        parameter.enable_recognition = 0;        // 禁用人脸识别
        parameter.enable_liveness = 0;           // 禁用活体检测
        parameter.enable_ir_liveness = 0;        // 禁用红外活体检测
        parameter.enable_mask_detect = 0;        // 禁用口罩检测
        parameter.enable_face_quality = 0;       // 禁用人脸质量检测
        parameter.enable_face_attribute = 1;     // 启用人脸属性分析
        parameter.enable_interaction_liveness = 0; // 禁用交互式活体检测
        parameter.enable_detect_mode_landmark = 1; // 启用关键点检测
        parameter.enable_face_pose = 0;          // 禁用人脸姿态估计
        
        HResult sessionResult = HFCreateInspireFaceSession(parameter,
                                                          HF_DETECT_MODE_ALWAYS_DETECT, 
                                                          -1,   // maxDetectFaceNum: -1表示默认
                                                          320,  // 像素级别：320
                                                          -1,   // trackByDetectModeFPS: -1表示默认
                                                          &session);
        if (sessionResult != HSUCCEED || session == nullptr) {
            LOGE("Failed to create InspireFace session, error code: %ld", sessionResult);
            env->ReleaseStringUTFChars(internal_data_path, dataPath);
            return -4;
        }
        LOGI("✅ InspireFace session created successfully");
        
        // 3. 创建测试图像（简单的320x320灰色图像）
        LOGI("Step 3: Creating test image...");
        int width = 320;
        int height = 320;
        int channels = 3;
        std::vector<uint8_t> imageData(width * height * channels, 128); // 灰色图像

        // 创建HFImageData结构体
        HFImageData imgData = {0};
        imgData.data = imageData.data();
        imgData.width = width;
        imgData.height = height;
        imgData.format = HF_STREAM_BGR;
        imgData.rotation = HF_CAMERA_ROTATION_0;

        // 使用正确的API创建HFImageStream
        HFImageStream imageStream = nullptr;
        HResult streamResult = HFCreateImageStream(&imgData, &imageStream);
        if (streamResult != HSUCCEED || imageStream == nullptr) {
            LOGE("Failed to create image stream, error code: %ld", streamResult);
            HFReleaseInspireFaceSession(session);
            env->ReleaseStringUTFChars(internal_data_path, dataPath);
            return -5;
        }

        LOGI("✅ Test image stream created: %dx%d, %d channels", width, height, channels);

        // 4. 执行人脸检测
        LOGI("Step 4: Performing face detection...");
        HFMultipleFaceData faceData = {0};
        HResult detectResult = HFExecuteFaceTrack(session, imageStream, &faceData);
        
        if (detectResult == HSUCCEED) {
            LOGI("✅ Face detection executed successfully");
            LOGI("Detected faces count: %d", faceData.detectedNum);

            if (faceData.detectedNum > 0) {
                LOGI("Face detection results:");
                for (int i = 0; i < faceData.detectedNum && i < 10; i++) {
                    LOGI("  Face %d: x1=%d, y1=%d, x2=%d, y2=%d",
                         i, faceData.rects[i].x, faceData.rects[i].y,
                         faceData.rects[i].x + faceData.rects[i].width,
                         faceData.rects[i].y + faceData.rects[i].height);
                }
            } else {
                LOGI("No faces detected in test image (expected for blank image)");
            }
        } else {
            LOGE("Face detection failed, error code: %ld", detectResult);
            HFReleaseImageStream(imageStream);
            HFReleaseInspireFaceSession(session);
            env->ReleaseStringUTFChars(internal_data_path, dataPath);
            return -6;
        }
        
        // 5. 测试人脸属性分析（如果检测到人脸）
        if (faceData.detectedNum > 0) {
            LOGI("Step 5: Testing face attribute analysis...");

            // 使用正确的API获取人脸属性结果
            HFFaceAttributeResult attributeResult = {0};
            HResult attrResult = HFGetFaceAttributeResult(session, &attributeResult);

            if (attrResult == HSUCCEED) {
                LOGI("✅ Face attribute analysis completed");
                LOGI("Attribute results for %d faces:", attributeResult.num);
                if (attributeResult.num > 0) {
                    LOGI("  Race: %d (0:Black, 1:Asian, 2:Latino, 3:Middle Eastern, 4:White)", attributeResult.race[0]);
                    LOGI("  Gender: %d (0:Female, 1:Male)", attributeResult.gender[0]);
                    LOGI("  Age Bracket: %d (0:0-2, 1:3-9, 2:10-19, 3:20-29, 4:30-39, 5:40-49, 6:50-59, 7:60-69, 8:70+)", attributeResult.ageBracket[0]);
                }
            } else {
                LOGE("Face attribute analysis failed, error code: %ld", attrResult);
            }
        } else {
            LOGI("Step 5: Skipping attribute analysis (no faces detected)");
        }
        
        // 6. 清理资源
        LOGI("Step 6: Cleaning up resources...");
        HFReleaseImageStream(imageStream);
        HFReleaseInspireFaceSession(session);
        LOGI("✅ Resources cleaned up successfully");
        
        LOGI("=== 人脸检测功能测试完成 - 所有步骤成功 ===");
        
    } catch (const std::exception& e) {
        LOGE("Exception during face detection test: %s", e.what());
        env->ReleaseStringUTFChars(internal_data_path, dataPath);
        return -6;
    } catch (...) {
        LOGE("Unknown exception during face detection test");
        env->ReleaseStringUTFChars(internal_data_path, dataPath);
        return -7;
    }
    
    // 释放字符串资源
    env->ReleaseStringUTFChars(internal_data_path, dataPath);
    
    LOGI("✅ Face detection test completed successfully");
    return 0;
}

/**
 * 获取人脸检测能力信息
 */
JNIEXPORT jstring JNICALL
Java_com_wulala_myyolov5rtspthreadpool_FaceDetectionTest_getFaceDetectionCapabilities(
    JNIEnv* env, jobject thiz) {
    
    LOGI("Getting face detection capabilities...");
    
    try {
        // 构建能力信息字符串
        std::string capabilities = "Face Detection Capabilities:\n";
        capabilities += "- Detection Models: SCRFD (160x160, 320x320, 640x640)\n";
        capabilities += "- Landmark Detection: 106 points\n";
        capabilities += "- Face Attributes: Age, Gender, Race\n";
        capabilities += "- Face Quality: Pose, Blur, Illumination\n";
        capabilities += "- Supported Formats: BGR, RGB\n";
        capabilities += "- Max Faces: Configurable\n";
        capabilities += "- Platform: RK3588 RKNN\n";
        capabilities += "- Performance: Real-time\n";
        
        LOGI("Face detection capabilities: %s", capabilities.c_str());
        
        return env->NewStringUTF(capabilities.c_str());
        
    } catch (const std::exception& e) {
        LOGE("Exception getting capabilities: %s", e.what());
        std::string error = "Error: " + std::string(e.what());
        return env->NewStringUTF(error.c_str());
    } catch (...) {
        LOGE("Unknown exception getting capabilities");
        return env->NewStringUTF("Error: Unknown exception");
    }
}

} // extern "C"
