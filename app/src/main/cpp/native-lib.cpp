#include <jni.h>
#include <string>
#include <android/native_window_jni.h> // ANativeWindow 用来渲染画面的 == Surface对象
#include <android/asset_manager.h>
#include <android/asset_manager_jni.h>
#include <unistd.h>
#include <vector>
#include <map>
#include <vector>
#include <string>
#include <chrono>
#include "log4c.h"
#include "ZLPlayer.h"
#include <jni.h>

#define MAX_CAMERAS 16

JavaVM *vm = nullptr;
ANativeWindow *window = nullptr;  // 保留单摄像头兼容性
std::vector<ANativeWindow*> cameraWindows(MAX_CAMERAS, nullptr);  // 多摄像头窗口
std::map<int, ZLPlayer*> cameraPlayers;  // 每个摄像头对应的ZLPlayer实例
std::vector<std::string> rtspUrls(MAX_CAMERAS);  // 存储每个摄像头的RTSP URL
pthread_mutex_t windowMutex = PTHREAD_MUTEX_INITIALIZER;
AAssetManager *nativeAssetManager;

// 多摄像头轮播相关变量
int currentPlayingCamera = 0;
int totalCameras = 0;
std::chrono::steady_clock::time_point lastSwitchTime;

jint JNI_OnLoad(JavaVM *vm, void *args) {
    ::vm = vm;
    return JNI_VERSION_1_6;
}

void JNI_OnUnload(JavaVM *vm, void *args) {
    ::vm = nullptr;
}

void get_file_content(char *data, int *dataLen, const char *fileName) {

    // 获取文件内容
    if (nativeAssetManager == nullptr) {
        LOGE("AAssetManager is null");
    }
    LOGD("Opening fileName :%s", fileName);
    //打开指定文件
    AAsset *asset = AAssetManager_open(nativeAssetManager, fileName, AASSET_MODE_BUFFER);
    //获取文件长度
    *dataLen = AAsset_getLength(asset);
    LOGD("File size :%d", *dataLen);

    char *buf = new char[*dataLen];
    memset(buf, 0x00, *dataLen);
    //读取文件内容
    AAsset_read(asset, buf, *dataLen);

    memcpy(data, buf, *dataLen);

    // 道德底线，释放内存
    delete[] buf;

}

extern "C"
JNIEXPORT jlong
JNICALL
Java_com_wulala_myyolov5rtspthreadpool_MainActivity_prepareNative(JNIEnv *env, jobject thiz) {

    char *data = static_cast<char *>(malloc(1024 * 1024 * 50));
    int dataLen;
    // yolov5s_quant.rknn
    get_file_content(data, &dataLen, "yolov5s_quant.rknn");

    auto *zlPlayer = new ZLPlayer(data, dataLen);

    // zlPlayer->setModelFile(data, dataLen);
    free(data);
    return reinterpret_cast<jlong>(zlPlayer);
}


extern "C"
JNIEXPORT void JNICALL
Java_com_wulala_myyolov5rtspthreadpool_MainActivity_setNativeAssetManager(
        JNIEnv *env, jobject instance,
        jobject assetManager) {

    nativeAssetManager = AAssetManager_fromJava(env, assetManager);
    if (nativeAssetManager == nullptr) {
        LOGE("AAssetManager == null");
    }

    LOGD("AAssetManager been set");
}

extern "C"
JNIEXPORT void JNICALL
Java_com_wulala_myyolov5rtspthreadpool_MainActivity_setRtspUrl(JNIEnv *env, jobject thiz, jlong native_player_obj, jstring rtsp_url) {
    if (native_player_obj == 0) {
        LOGE("Native player object is null");
        return;
    }
    
    auto *zlPlayer = reinterpret_cast<ZLPlayer *>(native_player_obj);
    
    if (rtsp_url == nullptr) {
        LOGE("RTSP URL is null");
        return;
    }
    
    const char *url_str = env->GetStringUTFChars(rtsp_url, nullptr);
    if (url_str == nullptr) {
        LOGE("Failed to get RTSP URL string");
        return;
    }
    
    LOGD("Setting RTSP URL: %s", url_str);
    zlPlayer->setRtspUrl(url_str);
    
    env->ReleaseStringUTFChars(rtsp_url, url_str);
}

extern "C"
JNIEXPORT void JNICALL
Java_com_wulala_myyolov5rtspthreadpool_MainActivity_startRtspStream(JNIEnv *env, jobject thiz, jlong native_player_obj) {
    if (native_player_obj == 0) {
        LOGE("Native player object is null");
        return;
    }
    
    auto *zlPlayer = reinterpret_cast<ZLPlayer *>(native_player_obj);
    LOGD("Starting RTSP stream from JNI");
    zlPlayer->startRtspStream();
}

extern "C"
JNIEXPORT void JNICALL
Java_com_wulala_myyolov5rtspthreadpool_MainActivity_setNativeSurface(JNIEnv *env, jobject thiz, jobject surface) {
    pthread_mutex_lock(&windowMutex);
    
    // Release the previous window if any
    if (window) {
        ANativeWindow_release(window);
        window = nullptr;
    }
    
    // Get new window from Surface
    if (surface) {
        window = ANativeWindow_fromSurface(env, surface);
        LOGD("Native window set successfully");
    } else {
        LOGD("Native window released");
    }
    
    pthread_mutex_unlock(&windowMutex);
}

// 多摄像头支持方法

extern "C"
JNIEXPORT void JNICALL
Java_com_wulala_myyolov5rtspthreadpool_MainActivity_setCameraCount(JNIEnv *env, jobject thiz, jlong native_player_obj, jint count) {
    if (native_player_obj == 0) {
        LOGE("Native player object is null");
        return;
    }
    
    LOGD("Setting camera count to: %d", count);
    
    // 清理现有的摄像头实例（除了主实例）
    for (auto& pair : cameraPlayers) {
        if (pair.second && pair.first > 0) {  // 保留主实例(index 0)
            LOGD("Cleaning up ZLPlayer instance for camera %d", pair.first);
            // 先停止RTSP流，再删除实例
            pair.second->stopRtspStream();
            delete pair.second;
        }
    }
    cameraPlayers.clear();

    // 获取主ZLPlayer实例
    auto *mainPlayer = reinterpret_cast<ZLPlayer *>(native_player_obj);
    cameraPlayers[0] = mainPlayer;  // 主实例用于第一个摄像头

    // 为每个额外的摄像头创建独立的ZLPlayer实例
    for (int i = 1; i < count && i < MAX_CAMERAS; i++) {
        try {
            // 先创建空的ZLPlayer实例
            ZLPlayer* newPlayer = new ZLPlayer(nullptr, 0);

            // 从主实例获取模型数据并初始化
            char* modelData = mainPlayer->getModelData();
            int modelSize = mainPlayer->getModelSize();

            if (modelData != nullptr && modelSize > 0) {
                newPlayer->initializeModelData(modelData, modelSize);
                LOGD("Camera %d created independent ZLPlayer instance with model data", i);
            } else {
                LOGW("Camera %d created ZLPlayer instance without model data", i);
            }

            cameraPlayers[i] = newPlayer;
        } catch (const std::exception& e) {
            LOGE("Failed to create ZLPlayer instance for camera %d: %s", i, e.what());
            cameraPlayers[i] = nullptr;
        } catch (...) {
            LOGE("Failed to create ZLPlayer instance for camera %d: unknown error", i);
            cameraPlayers[i] = nullptr;
        }
    }
}

extern "C"
JNIEXPORT void JNICALL
Java_com_wulala_myyolov5rtspthreadpool_MainActivity_setNativeSurfaceForCamera(JNIEnv *env, jobject thiz, jlong native_player_obj, jint camera_index, jobject surface) {
    if (camera_index < 0 || camera_index >= MAX_CAMERAS) {
        LOGE("Invalid camera index: %d", camera_index);
        return;
    }
    
    ANativeWindow *newWindow = nullptr;
    ANativeWindow *oldWindow = nullptr;
    
    // 第一步：处理Surface转换，不持有锁
    if (surface) {
        newWindow = ANativeWindow_fromSurface(env, surface);
        LOGD("Native window created for camera %d successfully", camera_index);
    }
    
    // 第二步：在锁保护下更新窗口数组
    pthread_mutex_lock(&windowMutex);
    
    // 保存旧窗口用于后续释放
    oldWindow = cameraWindows[camera_index];
    cameraWindows[camera_index] = newWindow;
    
    // 暂时将第一个摄像头的窗口设置为全局窗口，以兼容现有代码
    if (camera_index == 0) {
        // Release the previous global window if any
        if (window && window != newWindow) {
            ANativeWindow_release(window);
        }
        window = newWindow;
        // 增加引用计数，避免双重释放
        if (window) {
            ANativeWindow_acquire(window);
        }
    }
    
    pthread_mutex_unlock(&windowMutex);
    
    // 第三步：在锁外部设置ZLPlayer专用窗口（避免死锁）
    if (surface) {
        auto it = cameraPlayers.find(camera_index);
        if (it != cameraPlayers.end() && it->second) {
            it->second->setNativeWindow(newWindow);
            LOGD("Dedicated window set for ZLPlayer camera %d", camera_index);
        }
    } else {
        LOGD("Native window released for camera %d", camera_index);
        
        // 清除对应ZLPlayer的专用窗口
        auto it = cameraPlayers.find(camera_index);
        if (it != cameraPlayers.end() && it->second) {
            it->second->setNativeWindow(nullptr);
        }
        
        // 如果是第一个摄像头被释放，也释放全局窗口
        if (camera_index == 0) {
            pthread_mutex_lock(&windowMutex);
            if (window) {
                ANativeWindow_release(window);
                window = nullptr;
            }
            pthread_mutex_unlock(&windowMutex);
        }
    }
    
    // 第四步：释放旧窗口
    if (oldWindow) {
        ANativeWindow_release(oldWindow);
    }
}

extern "C"
JNIEXPORT void JNICALL
Java_com_wulala_myyolov5rtspthreadpool_MainActivity_setRtspUrlForCamera(JNIEnv *env, jobject thiz, jlong native_player_obj, jint camera_index, jstring rtsp_url) {
    if (camera_index < 0 || camera_index >= MAX_CAMERAS) {
        LOGE("Invalid camera index: %d", camera_index);
        return;
    }
    
    if (rtsp_url == nullptr) {
        LOGE("RTSP URL is null for camera %d", camera_index);
        return;
    }
    
    const char *url_str = env->GetStringUTFChars(rtsp_url, nullptr);
    if (url_str == nullptr) {
        LOGE("Failed to get RTSP URL string for camera %d", camera_index);
        return;
    }
    
    LOGD("Setting RTSP URL for camera %d: %s", camera_index, url_str);

    // 存储RTSP URL到数组中
    rtspUrls[camera_index] = std::string(url_str);
    LOGD("RTSP URL stored for camera %d: %s", camera_index, url_str);
    
    env->ReleaseStringUTFChars(rtsp_url, url_str);
}

extern "C"
JNIEXPORT void JNICALL
Java_com_wulala_myyolov5rtspthreadpool_MainActivity_startAllRtspStreams(JNIEnv *env, jobject thiz, jlong native_player_obj, jint camera_count) {
    LOGD("Starting all independent RTSP streams, camera count: %d", camera_count);

    totalCameras = camera_count;

    // 启动所有摄像头的独立RTSP流
    for (int i = 0; i < camera_count && i < MAX_CAMERAS; i++) {
        auto it = cameraPlayers.find(i);
        if (it != cameraPlayers.end() && it->second) {
            if (!rtspUrls[i].empty()) {
                LOGD("Starting RTSP stream for camera %d: %s", i, rtspUrls[i].c_str());
                it->second->setRtspUrl(rtspUrls[i].c_str());
                it->second->startRtspStream();
                LOGD("Successfully started RTSP stream for camera %d", i);
            } else {
                LOGE("No RTSP URL found for camera %d", i);
            }
        } else {
            LOGE("ZLPlayer instance not found for camera %d", i);
        }
    }

    LOGD("All RTSP streams startup completed");
}

// 多摄像头同时显示模式，不再需要轮播功能
// 保留switchCamera接口用于兼容性，但功能改为重启所有流
extern "C"
JNIEXPORT void JNICALL
Java_com_wulala_myyolov5rtspthreadpool_MainActivity_switchCamera(JNIEnv *env, jobject thiz) {
    LOGD("Manual camera switch requested - restarting all streams");

    // 重启所有摄像头流
    for (int i = 0; i < totalCameras && i < MAX_CAMERAS; i++) {
        auto it = cameraPlayers.find(i);
        if (it != cameraPlayers.end() && it->second && !rtspUrls[i].empty()) {
            LOGD("Restarting RTSP stream for camera %d", i);
            it->second->setRtspUrl(rtspUrls[i].c_str());
            it->second->startRtspStream();
        }
    }
}
