#include <jni.h>
#include <string>
#include <android/native_window_jni.h> // ANativeWindow 用来渲染画面的 == Surface对象
#include <android/asset_manager.h>
#include <android/asset_manager_jni.h>
#include <unistd.h>
#include "log4c.h"
#include "ZLPlayer.h"
#include <jni.h>

JavaVM *vm = nullptr;
ANativeWindow *window = nullptr;
pthread_mutex_t windowMutex = PTHREAD_MUTEX_INITIALIZER;
AAssetManager *nativeAssetManager;

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
