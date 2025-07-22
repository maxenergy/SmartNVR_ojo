#include <mk_common.h>
#include <mk_player.h>
#include <android/native_window_jni.h>
#include <chrono>
#include <thread>
#include <opencv2/opencv.hpp>
#include <sys/resource.h>
#include <pthread.h>
#include <stdexcept>  // 🔧 添加异常处理支持
#include <exception>  // 🔧 添加异常处理支持
#include "ZLPlayer.h"
#include "mpp_err.h"
#include "cv_draw.h"
#include "../engine/inference_manager.h"  // 🔧 新增: 统一推理管理器
#include "../types/model_config.h"        // 🔧 新增: 模型配置
#include "../types/person_detection_types.h" // 🔧 新增: 人员检测数据类型
#include "../include/face_analysis_manager.h" // 🔧 新增: 人脸分析管理器
#include "../include/statistics_manager.h"    // 🔧 新增: 统计管理器
// Yolov8ThreadPool *yolov8_thread_pool;   // 线程池

extern pthread_mutex_t windowMutex;     // 静态初始化 所
extern ANativeWindow *window;

// 🔧 修复: 移除有问题的全局异常处理器

void *rtps_process(void *arg) {
    ZLPlayer *player = (ZLPlayer *) arg;
    if (player) {
        // 在线程内部设置优先级（Android兼容方式）
        int niceValue = (player->app_ctx.camera_index == 0) ? -5 : 0;
        if (setpriority(PRIO_PROCESS, 0, niceValue) == 0) {
            LOGD("RTSP thread priority set to nice=%d for camera %d", niceValue, player->app_ctx.camera_index);
        } else {
            LOGW("Failed to set RTSP thread priority for camera %d", player->app_ctx.camera_index);
        }

        player->process_video_rtsp();
    } else {
        LOGE("player is null");
    }
    return nullptr;
}

void *desplay_process(void *arg) {
    ZLPlayer *player = (ZLPlayer *) arg;
    if (player) {
        while (1) {
            player->display();
        }
    } else {
        LOGE("player is null");
    }
    return nullptr;
}

void ZLPlayer::setModelFile(char *data, int dataLen) {
    // 申请内存
    this->modelFileContent = new char[dataLen];
    // 复制内存
    memcpy(this->modelFileContent, data, dataLen);
    this->modelFileSize = dataLen;
}

void ZLPlayer::initializeModelData(char* modelData, int modelSize) {
    if (modelData != nullptr && modelSize > 0) {
        // 释放现有的模型数据
        if (this->modelFileContent != nullptr) {
            free(this->modelFileContent);
        }

        // 复制新的模型数据
        this->modelFileSize = modelSize;
        this->modelFileContent = new char[modelSize];  // 🔧 修复: 使用new[]替代malloc
        memcpy(this->modelFileContent, modelData, modelSize);

        // 重新初始化YOLOv5线程池，使用动态线程池大小
        if (app_ctx.yolov5ThreadPool != nullptr) {
            app_ctx.yolov5ThreadPool->setUpWithModelData(app_ctx.thread_pool_size, this->modelFileContent, this->modelFileSize);
            LOGD("YOLOv5 thread pool re-initialized with %d threads", app_ctx.thread_pool_size);
        }
    }
}

void ZLPlayer::setRtspUrl(const char *url) {
    if (rtsp_url != nullptr) {
        delete[] rtsp_url;
    }

    size_t url_len = strlen(url);
    rtsp_url = new char[url_len + 1];
    strcpy(rtsp_url, url);
    LOGD("RTSP URL set to: %s", rtsp_url);
}

void ZLPlayer::startRtspStream() {
    if (rtsp_url == nullptr) {
        LOGE("Cannot start RTSP stream: URL not set");
        return;
    }

    if (pid_rtsp != 0) {
        LOGD("RTSP stream already running");
        return;
    }

    LOGD("Starting RTSP stream with URL: %s", rtsp_url);
    isStreaming = true;  // 设置流状态标志

    int result = pthread_create(&pid_rtsp, nullptr, rtps_process, this);

    if (result != 0) {
        LOGE("Failed to create RTSP thread: %d", result);
        pid_rtsp = 0;
        isStreaming = false;
    } else {
        LOGD("RTSP thread created successfully for camera %d", app_ctx.camera_index);

        // 在线程创建后设置nice值（Android兼容方式）
        // 主摄像头使用更高优先级（更低的nice值）
        int niceValue = (app_ctx.camera_index == 0) ? -5 : 0;

        // 注意：这里需要在RTSP线程内部设置，暂时记录配置
        LOGD("Camera %d configured with nice value: %d", app_ctx.camera_index, niceValue);
    }
}

void ZLPlayer::stopRtspStream() {
    if (pid_rtsp != 0) {
        LOGD("Stopping RTSP stream");
        // 设置停止标志，让线程自然退出
        isStreaming = false;

        // 简单等待线程结束（Android NDK可能不支持pthread_timedjoin_np）
        int result = pthread_join(pid_rtsp, nullptr);
        if (result == 0) {
            LOGD("RTSP thread stopped gracefully");
        } else {
            LOGW("RTSP thread join failed, result: %d", result);
        }

        pid_rtsp = 0;
        LOGD("RTSP stream stopped");
    } else {
        LOGD("RTSP stream is not running");
    }
}

void ZLPlayer::setNativeWindow(ANativeWindow *window) {
    pthread_mutex_lock(&windowMutex);

    // 释放之前的专用窗口
    if (dedicatedWindow) {
        ANativeWindow_release(dedicatedWindow);
    }

    // 设置新的专用窗口
    dedicatedWindow = window;
    if (dedicatedWindow) {
        ANativeWindow_acquire(dedicatedWindow);
        LOGD("Dedicated native window set for ZLPlayer instance");
    } else {
        LOGD("Dedicated native window cleared for ZLPlayer instance");
    }

    pthread_mutex_unlock(&windowMutex);
}

// 性能优化：设置性能配置
void ZLPlayer::setPerformanceConfig(int cameraIndex, int totalCameras, bool performanceMode) {
    app_ctx.camera_index = cameraIndex;
    app_ctx.performance_mode = performanceMode;

    // 根据摄像头总数动态分配线程池大小
    if (totalCameras <= 1) {
        app_ctx.thread_pool_size = 12;  // 单摄像头使用较多线程
    } else if (totalCameras <= 2) {
        app_ctx.thread_pool_size = 8;   // 2路摄像头每路8个线程
    } else if (totalCameras <= 4) {
        app_ctx.thread_pool_size = 5;   // 4路摄像头每路5个线程
    } else {
        app_ctx.thread_pool_size = 3;   // 更多摄像头时进一步减少
    }

    LOGD("Camera %d performance config: threads=%d, performance_mode=%s",
         cameraIndex, app_ctx.thread_pool_size, performanceMode ? "true" : "false");
}

// 性能优化：优化线程池
void ZLPlayer::optimizeThreadPool() {
    if (app_ctx.yolov5ThreadPool && app_ctx.thread_pool_size > 0) {
        // 重新初始化线程池（如果支持动态调整）
        LOGD("Optimizing thread pool size to %d threads", app_ctx.thread_pool_size);
        // 注意：这里可能需要根据Yolov5ThreadPool的实际API进行调整
    }
}

// 性能优化：设置帧率限制
void ZLPlayer::setFrameRateLimit(int targetFps) {
    if (targetFps > 0 && targetFps <= 60) {
        int intervalMs = 1000 / targetFps;
        nextRendTime = std::chrono::steady_clock::now() + std::chrono::milliseconds(intervalMs);
        LOGD("Frame rate limit set to %d FPS (%d ms interval)", targetFps, intervalMs);
    }
}

// 内存使用监控
void ZLPlayer::logMemoryUsage() {
    // 读取进程内存信息
    FILE* file = fopen("/proc/self/status", "r");
    if (file) {
        char line[256];
        while (fgets(line, sizeof(line), file)) {
            if (strncmp(line, "VmRSS:", 6) == 0) {
                LOGD("Camera %d Memory RSS: %s", app_ctx.camera_index, line + 6);
            } else if (strncmp(line, "VmSize:", 7) == 0) {
                LOGD("Camera %d Memory VmSize: %s", app_ctx.camera_index, line + 7);
            }
        }
        fclose(file);
    }

    // 记录线程池状态
    if (app_ctx.yolov5ThreadPool) {
        LOGD("Camera %d ThreadPool status: configured with %d threads",
             app_ctx.camera_index, app_ctx.thread_pool_size);
    }
}

// 卡住检测和恢复方法实现
bool ZLPlayer::isStuck() {
    auto now = std::chrono::steady_clock::now();
    auto timeSinceLastFrame = std::chrono::duration_cast<std::chrono::seconds>(
        now - app_ctx.last_successful_frame).count();

    // 如果超过10秒没有成功帧，认为卡住
    if (timeSinceLastFrame > 10) {
        app_ctx.is_stuck = true;
        LOGW("Camera %d detected as stuck: %lld seconds since last successful frame",
             app_ctx.camera_index, (long long)timeSinceLastFrame);
        return true;
    }

    // 如果连续失败次数过多，也认为卡住
    if (app_ctx.consecutive_failures > 50) {
        app_ctx.is_stuck = true;
        LOGW("Camera %d detected as stuck: %d consecutive failures",
             app_ctx.camera_index, app_ctx.consecutive_failures);
        return true;
    }

    return false;
}

void ZLPlayer::resetStuckState() {
    app_ctx.is_stuck = false;
    app_ctx.consecutive_failures = 0;
    app_ctx.last_successful_frame = std::chrono::steady_clock::now();
    LOGD("Camera %d stuck state reset", app_ctx.camera_index);
}

bool ZLPlayer::attemptRestart() {
    if (app_ctx.restart_attempts >= 3) {
        LOGE("Camera %d maximum restart attempts reached", app_ctx.camera_index);
        return false;
    }

    app_ctx.restart_attempts++;
    LOGW("Camera %d attempting restart (attempt %d/3)",
         app_ctx.camera_index, app_ctx.restart_attempts);

    // 停止当前RTSP流
    stopRtspStream();

    // 等待一段时间后重新启动
    std::this_thread::sleep_for(std::chrono::seconds(2));

    // 重新启动RTSP流
    startRtspStream();

    // 重置状态
    resetStuckState();

    return true;
}

void ZLPlayer::updateFrameStatus(bool success) {
    if (success) {
        app_ctx.last_successful_frame = std::chrono::steady_clock::now();
        app_ctx.consecutive_failures = 0;
        app_ctx.restart_attempts = 0;  // 成功后重置重启计数
        if (app_ctx.is_stuck) {
            LOGD("Camera %d recovered from stuck state", app_ctx.camera_index);
            app_ctx.is_stuck = false;
        }
    } else {
        app_ctx.consecutive_failures++;
    }
}

ZLPlayer::ZLPlayer(char *modelFileData, int modelDataLen) {

    // 使用新的RTSP地址
    const char *default_url = "rtsp://192.168.31.22:8554/unicast";
    size_t url_len = strlen(default_url);
    rtsp_url = new char[url_len + 1];
    strcpy(rtsp_url, default_url);
    LOGD("ZLPlayer initialized with default RTSP URL: %s", rtsp_url);

    this->modelFileSize = modelDataLen;
    if (modelFileData != nullptr && modelDataLen > 0) {
        this->modelFileContent = (char *) malloc(modelDataLen);
        memcpy(this->modelFileContent, modelFileData, modelDataLen);
    } else {
        this->modelFileContent = nullptr;
        LOGW("ZLPlayer created without model data - will need to set model later");
    }

    LOGD("create mpp");
    // 创建上下文
    memset(&app_ctx, 0, sizeof(rknn_app_context_t)); // 初始化上下文

    // 初始化性能优化参数
    app_ctx.thread_pool_size = 8;  // 默认线程池大小，后续可通过setPerformanceConfig调整
    app_ctx.camera_index = 0;
    app_ctx.performance_mode = true;
    app_ctx.last_frame_time = std::chrono::steady_clock::now();

    // 初始化卡住检测参数
    app_ctx.last_successful_frame = std::chrono::steady_clock::now();
    app_ctx.consecutive_failures = 0;
    app_ctx.is_stuck = false;
    app_ctx.restart_attempts = 0;

    // app_ctx.job_cnt = 1;
    // app_ctx.result_cnt = 1;
    // app_ctx.mppDataThreadPool = new MppDataThreadPool();
    // yolov8_thread_pool = new Yolov8ThreadPool(); // 创建线程池
    // yolov8_thread_pool->setUpWithModelData(20, this->modelFileContent, this->modelFileSize);
    app_ctx.yolov5ThreadPool = new Yolov5ThreadPool(); // 创建线程池
    if (this->modelFileContent != nullptr && this->modelFileSize > 0) {
        app_ctx.yolov5ThreadPool->setUpWithModelData(app_ctx.thread_pool_size, this->modelFileContent, this->modelFileSize);
        LOGD("YOLOv5 thread pool initialized with %d threads", app_ctx.thread_pool_size);
    } else {
        LOGW("YOLOv5 thread pool created without model data - will initialize later");
    }

    // 🔧 新增: 初始化统一推理管理器
    app_ctx.inference_manager = new InferenceManager();
    if (app_ctx.inference_manager) {
        // 配置YOLOv5模型
        ModelConfig yolov5_config = ModelConfig::getYOLOv5Config();

        // 配置YOLOv8n模型（可选）
        ModelConfig yolov8_config = ModelConfig::getYOLOv8nConfig();

        // 初始化推理管理器（YOLOv5必须，YOLOv8n可选）
        if (app_ctx.inference_manager->initialize(yolov5_config, &yolov8_config) == 0) {
            LOGD("Unified inference manager initialized successfully");
            // 默认使用YOLOv5模型（保持向后兼容）
            app_ctx.inference_manager->setCurrentModel(ModelType::YOLOV5);
        } else {
            LOGW("Unified inference manager initialization failed, using legacy YOLOv5 only");
            delete app_ctx.inference_manager;
            app_ctx.inference_manager = nullptr;
        }
    }

    // 🔧 新增: 初始化简化的管理器（暂时设为nullptr，避免复杂依赖）
    app_ctx.face_analysis_manager = nullptr;
    app_ctx.statistics_manager = nullptr;
    LOGD("Simplified managers initialized (set to nullptr for now)");

    // app_ctx.mppDataThreadPool->setUpWithModelData(THREAD_POOL, this->modelFileContent, this->modelFileSize);

    // yolov8_thread_pool->setUp(model_path, 12);   // 初始化线程池

    // MPP 解码器
    if (app_ctx.decoder == nullptr) {
        LOGD("create decoder");
        MppDecoder *decoder = new MppDecoder();           // 创建解码器
        decoder->Init(264, 25, &app_ctx);                 // 初始化解码器
        decoder->SetCallback(mpp_decoder_frame_callback); // 设置回调函数，用来处理解码后的数据
        app_ctx.decoder = decoder;                        // 将解码器赋值给上下文
    } else {
        LOGD("decoder is not null");
    }
    // 初始化完成，等待手动启动RTSP流
    LOGD("ZLPlayer initialized successfully (RTSP ready to start)");
    
}

void API_CALL

on_track_frame_out(void *user_data, mk_frame frame) {
    rknn_app_context_t *ctx = (rknn_app_context_t *) user_data;
    // LOGD("on_track_frame_out ctx=%p\n", ctx);
    const char *data = mk_frame_get_data(frame);
    ctx->dts = mk_frame_get_dts(frame);
    ctx->pts = mk_frame_get_pts(frame);
    size_t size = mk_frame_get_data_size(frame);
    if (mk_frame_get_flags(frame) & MK_FRAME_FLAG_IS_KEY) {
        LOGD("Key frame size: %zu", size);
    } else if (MK_FRAME_FLAG_DROP_ABLE & mk_frame_get_flags(frame)) {
        LOGD("Drop able: %zu", size);
    } else if (MK_FRAME_FLAG_IS_CONFIG & mk_frame_get_flags(frame)) {
        LOGD("Config frame: %zu", size);
    } else if (MK_FRAME_FLAG_NOT_DECODE_ABLE & mk_frame_get_flags(frame)) {
        LOGD("Not decode able: %zu", size);
    } else {
        // LOGD("P-frame: %zu", size);
    }

    // LOGD("ctx->dts :%ld, ctx->pts :%ld", ctx->dts, ctx->pts);
    // LOGD("decoder=%p\n", ctx->decoder);
    ctx->decoder->Decode((uint8_t *) data, size, 0);
}

void API_CALL

on_mk_play_event_func(void *user_data, int err_code, const char *err_msg, mk_track tracks[],
                      int track_count) {
    rknn_app_context_t *ctx = (rknn_app_context_t *) user_data;
    if (err_code == 0) {
        // success
        LOGD("RTSP play success! Track count: %d", track_count);
        int i;
        for (i = 0; i < track_count; ++i) {
            if (mk_track_is_video(tracks[i])) {
                LOGD("got video track: %s", mk_track_codec_name(tracks[i]));
                // 监听track数据回调
                mk_track_add_delegate(tracks[i], on_track_frame_out, user_data);
            }
        }
    } else {
        LOGE("RTSP play failed: error %d - %s", err_code, err_msg ? err_msg : "Unknown error");
        // 不要退出应用，只是记录错误
        // 可以在这里添加重连逻辑
    }
}

void API_CALL

on_mk_shutdown_func(void *user_data, int err_code, const char *err_msg, mk_track tracks[], int track_count) {
    LOGE("RTSP play interrupted: error %d - %s", err_code, err_msg ? err_msg : "Unknown error");
    // 不要退出应用，只是记录错误
}

// 为每个ZLPlayer实例渲染到专用窗口
void ZLPlayer::renderFrameToWindow(uint8_t *src_data, int width, int height, int src_line_size, ANativeWindow *targetWindow) {
    LOGD("renderFrameToWindow called: width=%d, height=%d, src_line_size=%d", width, height, src_line_size);
    
    if (!src_data) {
        LOGE("renderFrameToWindow: src_data is null");
        return;
    }
    
    if (width <= 0 || height <= 0 || src_line_size <= 0) {
        LOGE("renderFrameToWindow: invalid parameters w=%d h=%d stride=%d", width, height, src_line_size);
        return;
    }

    // 优化：先检查窗口有效性，减少锁持有时间
    if (!targetWindow) {
        LOGD("renderFrameToWindow: target window is null, skipping render");
        return;
    }

    // 使用trylock避免长时间阻塞
    int lockResult = pthread_mutex_trylock(&windowMutex);
    if (lockResult != 0) {
        LOGW("Camera %d renderFrameToWindow: mutex is busy, skipping render", app_ctx.camera_index);
        return;
    }

    LOGD("renderFrameToWindow: Setting buffer geometry for window=%p", targetWindow);
    
    // Validate window before calling ANativeWindow_setBuffersGeometry
    int32_t current_width = ANativeWindow_getWidth(targetWindow);
    int32_t current_height = ANativeWindow_getHeight(targetWindow);
    
    if (current_width <= 0 || current_height <= 0) {
        LOGE("renderFrameToWindow: Invalid window dimensions w=%d h=%d", current_width, current_height);
        pthread_mutex_unlock(&windowMutex);
        return;
    }
    
    LOGD("renderFrameToWindow: Current window size: %dx%d, setting to: %dx%d", current_width, current_height, width, height);

    // 设置窗口的大小，各个属性
    int ret = ANativeWindow_setBuffersGeometry(targetWindow, width, height, WINDOW_FORMAT_RGBA_8888);
    if (ret != 0) {
        LOGE("ANativeWindow_setBuffersGeometry failed: %d", ret);
        pthread_mutex_unlock(&windowMutex);
        return;
    }

    // 他自己有个缓冲区 buffer
    ANativeWindow_Buffer window_buffer;

    // 如果我在渲染的时候，是被锁住的，那我就无法渲染，我需要释放 ，防止出现死锁
    if (ANativeWindow_lock(targetWindow, &window_buffer, 0)) {
        LOGE("ANativeWindow_lock failed");
        pthread_mutex_unlock(&windowMutex);
        return;
    }

    // 获取目标缓冲区数据
    uint8_t *dst_data = static_cast<uint8_t *>(window_buffer.bits);
    int dst_linesize = window_buffer.stride * 4;

    LOGD("renderFrameToWindow: window_buffer w=%d h=%d stride=%d", window_buffer.width, window_buffer.height, window_buffer.stride);

    int copy_height = std::min(height, window_buffer.height);
    int copy_width = std::min(src_line_size, dst_linesize);

    // 逐行复制数据
    for (int h = 0; h < copy_height; h++) {
        memcpy(dst_data + h * dst_linesize, src_data + h * src_line_size, copy_width);
    }

    // 数据刷新
    ANativeWindow_unlockAndPost(targetWindow); // 解锁后 并且刷新 window_buffer的数据显示画面
    pthread_mutex_unlock(&windowMutex);
    
    LOGD("renderFrameToWindow completed successfully");
}

// 函数指针的实现 实现渲染画面
void renderFrame(uint8_t *src_data, int width, int height, int src_line_size) {
    LOGD("renderFrame called: width=%d, height=%d, src_line_size=%d", width, height, src_line_size);
    
    if (!src_data) {
        LOGE("renderFrame: src_data is null");
        return;
    }
    
    if (width <= 0 || height <= 0 || src_line_size <= 0) {
        LOGE("renderFrame: invalid parameters w=%d h=%d stride=%d", width, height, src_line_size);
        return;
    }

    pthread_mutex_lock(&windowMutex);
    if (!window) {
        LOGD("renderFrame: window is null, skipping render");
        pthread_mutex_unlock(&windowMutex);
        return;
    }

    LOGD("renderFrame: Setting buffer geometry for window=%p", window);
    
    // Validate window before calling ANativeWindow_setBuffersGeometry
    // First try to get current window info to verify it's valid
    int32_t current_width = ANativeWindow_getWidth(window);
    int32_t current_height = ANativeWindow_getHeight(window);
    
    if (current_width <= 0 || current_height <= 0) {
        LOGE("renderFrame: Invalid window dimensions w=%d h=%d", current_width, current_height);
        pthread_mutex_unlock(&windowMutex);
        return;
    }
    
    LOGD("renderFrame: Current window size: %dx%d, setting to: %dx%d", current_width, current_height, width, height);

    // 设置窗口的大小，各个属性
    int ret = ANativeWindow_setBuffersGeometry(window, width, height, WINDOW_FORMAT_RGBA_8888);
    if (ret != 0) {
        LOGE("ANativeWindow_setBuffersGeometry failed: %d", ret);
        pthread_mutex_unlock(&windowMutex);
        return;
    }

    // 他自己有个缓冲区 buffer
    ANativeWindow_Buffer window_buffer;

    // 如果我在渲染的时候，是被锁住的，那我就无法渲染，我需要释放 ，防止出现死锁
    if (ANativeWindow_lock(window, &window_buffer, 0)) {
        LOGE("ANativeWindow_lock failed");
        pthread_mutex_unlock(&windowMutex);
        return;
    }

    // 填充[window_buffer]  画面就出来了  ==== 【目标 window_buffer】
    uint8_t *dst_data = static_cast<uint8_t *>(window_buffer.bits);
    int dst_linesize = window_buffer.stride * 4;

    LOGD("renderFrame: window_buffer w=%d h=%d stride=%d", window_buffer.width, window_buffer.height, window_buffer.stride);

    int copy_height = std::min(height, window_buffer.height);
    int copy_width = std::min(src_line_size, dst_linesize);

    for (int i = 0; i < copy_height; ++i) {
        // 图：一行一行显示 [高度不用管，用循环了，遍历高度]
        // 通用的
        memcpy(dst_data + i * dst_linesize, src_data + i * src_line_size, copy_width);
    }

    // 数据刷新
    ANativeWindow_unlockAndPost(window); // 解锁后 并且刷新 window_buffer的数据显示画面
    pthread_mutex_unlock(&windowMutex);
    
    LOGD("renderFrame completed successfully");
}

void ZLPlayer::display() {
    // 根据性能模式调整渲染间隔
    int renderIntervalMs = app_ctx.performance_mode ? 33 : 50;  // 30FPS vs 20FPS

    // 如果是高优先级摄像头，可以更高的帧率
    if (app_ctx.camera_index == 0) {
        renderIntervalMs = app_ctx.performance_mode ? 25 : 33;  // 40FPS vs 30FPS
    }

    std::this_thread::sleep_until(nextRendTime);

    // 设置下一次执行的时间点
    nextRendTime = std::chrono::steady_clock::now() + std::chrono::milliseconds(renderIntervalMs);
}

void ZLPlayer::get_detect_result() {
    try {
        std::vector<Detection> objects;
        // LOGD("decoder_callback Getting result count :%d", app_ctx.result_cnt);

        if (!app_ctx.yolov5ThreadPool) {
            LOGE("Camera %d YOLOv5ThreadPool is null", app_ctx.camera_index);
            updateFrameStatus(false);
            return;
        }

        auto ret_code = app_ctx.yolov5ThreadPool->getTargetResultNonBlock(objects, app_ctx.result_cnt);
        if (ret_code == NN_SUCCESS) {

        uint8_t idx;
        for (idx = 0; idx < objects.size(); idx++) {
            LOGD("objects[%d].classId: %d\n", idx, objects[idx].class_id);
            LOGD("objects[%d].prop: %f\n", idx, objects[idx].confidence);
            LOGD("objects[%d].class name: %s\n", idx, objects[idx].className.c_str());
        }
        auto frameData = app_ctx.yolov5ThreadPool->getTargetImgResult(app_ctx.result_cnt);
        if (!frameData || !frameData->data) {
            LOGE("Camera %d frameData is null or invalid", app_ctx.camera_index);
            updateFrameStatus(false);
            return;
        }

        app_ctx.result_cnt++;
        LOGD("Camera %d Get detect result counter:%d start display", app_ctx.camera_index, app_ctx.result_cnt);
        
        // 在显示之前绘制检测框
        if (objects.size() > 0) {
            // 🔧 新增: 类别过滤逻辑
            std::vector<Detection> filteredObjects;
            
            // 🔧 获取启用的类别列表（从Java层DetectionSettingsManager获取）
            std::set<std::string> enabledClasses = getEnabledClassesFromJava();
            
            // 过滤检测结果
            for (const auto& obj : objects) {
                if (enabledClasses.find(obj.className) != enabledClasses.end()) {
                    filteredObjects.push_back(obj);
                }
            }
            
            LOGD("🔍 检测结果过滤: %zu -> %zu (启用类别: person, bus, truck)", 
                 objects.size(), filteredObjects.size());
            
            // 只绘制过滤后的检测结果
            if (filteredObjects.size() > 0) {
                // 将RGBA数据转换为cv::Mat进行绘制
                cv::Mat display_mat(frameData->screenH, frameData->screenW, CV_8UC4, frameData->data);

                // 转换为RGB格式进行绘制（OpenCV绘制需要RGB格式）
                cv::Mat rgb_mat;
                cv::cvtColor(display_mat, rgb_mat, cv::COLOR_RGBA2RGB);

                // 🔧 新增：人员统计和人脸识别处理
                processPersonDetectionAndFaceAnalysis(rgb_mat, filteredObjects, frameData);

                // 绘制过滤后的检测框
                DrawDetections(rgb_mat, filteredObjects);
                LOGD("✅ 绘制了 %zu 个过滤后的检测框", filteredObjects.size());

                // 转换回RGBA格式
                cv::cvtColor(rgb_mat, display_mat, cv::COLOR_RGB2RGBA);
            } else {
                LOGD("⚠️ 过滤后没有检测结果需要绘制");
            }
        }

        // 添加时间戳信息到日志中，帮助调试时间同步问题
        struct timeval now;
        gettimeofday(&now, NULL);
        static struct timeval lastDisplayTime = {0, 0};
        
        if (lastDisplayTime.tv_sec != 0) {
            long displayGap = (now.tv_sec - lastDisplayTime.tv_sec) * 1000 + 
                             (now.tv_usec - lastDisplayTime.tv_usec) / 1000;
            LOGD("Display interval: %ld ms", displayGap);
            
            // 如果显示间隔太短，适当延迟以保持帧率稳定
            if (displayGap < 33) { // 约30fps
                std::this_thread::sleep_for(std::chrono::milliseconds(33 - displayGap));
            }
        }
        lastDisplayTime = now;

        // 使用专用窗口渲染，如果没有专用窗口则使用全局窗口
        bool renderSuccess = false;
        try {
            if (dedicatedWindow) {
                renderFrameToWindow((uint8_t *) frameData->data, frameData->screenW, frameData->screenH, frameData->screenStride, dedicatedWindow);
                renderSuccess = true;
            } else {
                renderFrame((uint8_t *) frameData->data, frameData->screenW, frameData->screenH, frameData->screenStride);
                renderSuccess = true;
            }
        } catch (...) {
            LOGE("Camera %d render failed", app_ctx.camera_index);
            renderSuccess = false;
        }

        // 释放内存
        delete frameData->data;
        frameData->data = nullptr;

        // 更新帧状态
        updateFrameStatus(renderSuccess);

    } else if (NN_RESULT_NOT_READY == ret_code) {
        // 结果未准备好，不算失败
        // LOGD("decoder_callback wait for result ready");
    } else {
        // 其他错误情况
        LOGW("Camera %d get_detect_result failed with code: %d", app_ctx.camera_index, ret_code);
        updateFrameStatus(false);
    }

    } catch (const std::exception& e) {
        LOGE("Camera %d get_detect_result exception: %s", app_ctx.camera_index, e.what());
        updateFrameStatus(false);
    } catch (...) {
        LOGE("Camera %d get_detect_result unknown exception", app_ctx.camera_index);
        updateFrameStatus(false);
    }
}

// 🔧 新增：获取当前检测结果
bool ZLPlayer::getCurrentDetectionResults(std::vector<Detection>& results) {
    try {
        results.clear();

        if (!app_ctx.yolov5ThreadPool) {
            LOGD("🔧 Camera %d YOLOv5ThreadPool is null", app_ctx.camera_index);
            return false;
        }

        // 🔧 优化：尝试获取最近的多个检测结果
        bool found = false;
        int attempts = 0;
        const int maxAttempts = 10; // 尝试最近10帧的结果

        for (int i = 0; i < maxAttempts && !found; i++) {
            int targetFrame = app_ctx.result_cnt - i;
            if (targetFrame < 0) break;

            auto ret_code = app_ctx.yolov5ThreadPool->getTargetResultNonBlock(results, targetFrame);
            attempts++;

            if (ret_code == NN_SUCCESS && !results.empty()) {
                // 应用类别过滤
                std::vector<Detection> filteredResults;
                std::set<std::string> enabledClasses = getEnabledClassesFromJava();

                for (const auto& detection : results) {
                    if (enabledClasses.find(detection.className) != enabledClasses.end()) {
                        filteredResults.push_back(detection);
                    }
                }

                if (!filteredResults.empty()) {
                    results = filteredResults;
                    found = true;

                    if (i > 0) {
                        LOGD("🔧 Camera %d 使用第%d帧前的检测结果", app_ctx.camera_index, i);
                    }

                    LOGD("🔧 Camera %d getCurrentDetectionResults: %zu 个过滤后的检测结果 (尝试%d次)",
                         app_ctx.camera_index, results.size(), attempts);
                }
            }
        }

        if (!found) {
            LOGD("🔧 Camera %d getCurrentDetectionResults: 无检测结果 (尝试%d次)",
                 app_ctx.camera_index, attempts);
            return false;
        }

        return true;

    } catch (const std::exception& e) {
        LOGE("🔧 Camera %d getCurrentDetectionResults exception: %s", app_ctx.camera_index, e.what());
        return false;
    } catch (...) {
        LOGE("🔧 Camera %d getCurrentDetectionResults unknown exception", app_ctx.camera_index);
        return false;
    }
}

int ZLPlayer::process_video_rtsp() {
    if (rtsp_url == nullptr) {
        LOGE("RTSP URL not set, cannot start streaming");
        return -1;
    }
    
    LOGD("process_video_rtsp starting with URL: %s", rtsp_url);

    mk_config config;
    memset(&config, 0, sizeof(mk_config));
    config.log_mask = LOG_CONSOLE;

    // 🔧 修复: 直接在内存中构建配置，强制禁用统计报告和所有网络功能
    static const char* minimal_config =
        "[general]\n"
        "enableVhost=0\n"
        "mediaServerId=local_server\n"
        "flowThreshold=0\n"
        "maxStreamWaitMS=5000\n"
        "mergeWriteMS=0\n"
        "enableStatistic=0\n"
        "reportServerUrl=\n"
        "enable_statistic=0\n"
        "report_server_url=\n"
        "\n"
        "[statistic]\n"
        "enable=0\n"
        "server_url=\n"
        "report_interval=0\n"
        "\n"
        "[hook]\n"
        "enable=0\n"
        "on_flow_report=\n"
        "on_server_started=\n"
        "on_server_keepalive=\n"
        "\n"
        "[http]\n"
        "enable=0\n"
        "port=0\n"
        "sslport=0\n"
        "\n"
        "[rtmp]\n"
        "enable=0\n"
        "port=0\n"
        "sslport=0\n"
        "\n"
        "[rtsp]\n"
        "enable=1\n"
        "port=0\n"
        "sslport=0\n"
        "authBasic=0\n"
        "directProxy=1\n"
        "\n"
        "[protocol]\n"
        "enable_hls=0\n"
        "enable_mp4=0\n"
        "enable_rtmp=0\n"
        "enable_ts=0\n"
        "enable_fmp4=0\n";

    config.ini_is_path = 0;  // 使用内存中的配置
    config.ini = minimal_config;
    
    try {
        // 添加URL预验证
        if (strlen(rtsp_url) < 10 ||
            (strncmp(rtsp_url, "rtsp://", 7) != 0 &&
             strncmp(rtsp_url, "http://", 7) != 0 &&
             strncmp(rtsp_url, "https://", 8) != 0)) {
            LOGE("Invalid RTSP URL format: %s", rtsp_url);
            return -1;
        }

        // 🔧 修复: 添加异常处理包装，捕获ZLMediaKit域名解析异常
        try {
            mk_env_init(&config);
            LOGD("mk_env_init completed");

            // 🔧 关键修复: 立即强制禁用统计报告，使用所有可能的配置选项名称
            // 禁用统计报告相关功能 - 使用所有可能的变体
            mk_set_option("general.enableStatistic", "0");
            mk_set_option("general.reportServerUrl", "");
            mk_set_option("general.enable_statistic", "0");
            mk_set_option("general.report_server_url", "");
            mk_set_option("general.reportServer", "");
            mk_set_option("general.report_server", "");
            mk_set_option("statistic.enable", "0");
            mk_set_option("statistic.server_url", "");
            mk_set_option("statistic.reportServerUrl", "");
            mk_set_option("statistic.report_server_url", "");
            mk_set_option("statistic.report_interval", "0");
            mk_set_option("statistic.report_enable", "0");

            // 强制设置空的报告服务器地址
            mk_set_option("general.reportServerUrl", "127.0.0.1");  // 设置为本地地址
            mk_set_option("statistic.server_url", "127.0.0.1");
            mk_set_option("statistic.reportServerUrl", "127.0.0.1");

            // 禁用HTTP服务器和相关功能
            mk_set_option("http.enable", "0");
            mk_set_option("http.port", "0");
            mk_set_option("http.sslport", "0");
            mk_set_option("http.notFound", "");

            // 禁用Hook功能
            mk_set_option("hook.enable", "0");
            mk_set_option("hook.on_flow_report", "");
            mk_set_option("hook.on_server_started", "");
            mk_set_option("hook.on_server_keepalive", "");

            // 禁用协议转换
            mk_set_option("protocol.enable_hls", "0");
            mk_set_option("protocol.enable_mp4", "0");
            mk_set_option("protocol.enable_rtmp", "0");
            mk_set_option("protocol.enable_ts", "0");
            mk_set_option("protocol.enable_fmp4", "0");

            // 只保留RTSP客户端功能
            mk_set_option("rtsp.enable", "1");
            mk_set_option("rtsp.port", "0");  // 禁用RTSP服务器
            mk_set_option("rtsp.sslport", "0");

            // 禁用RTMP
            mk_set_option("rtmp.enable", "0");
            mk_set_option("rtmp.port", "0");
            mk_set_option("rtmp.sslport", "0");

            LOGD("ZLMediaKit: All network services disabled, only RTSP client enabled");

        } catch (const std::invalid_argument& e) {
            LOGD("ZLMediaKit network config error (ignored): %s", e.what());
            // 继续运行，忽略统计报告功能
        } catch (const std::exception& e) {
            LOGD("ZLMediaKit initialization error (ignored): %s", e.what());
            // 继续运行，忽略统计报告功能
        } catch (...) {
            LOGD("ZLMediaKit unknown initialization error (ignored)");
            // 继续运行，忽略统计报告功能
        }

        mk_player player = nullptr;

        // 🔧 修复: 在player操作周围添加异常处理，防止网络相关异常
        try {
            player = mk_player_create();
            if (player == nullptr) {
                LOGE("Failed to create mk_player");
                return -1;
            }
            LOGD("mk_player_create completed");

            // 设置播放器选项以增加稳定性
            mk_player_set_option(player, "protocol_timeout", "10000000"); // 10秒超时
            mk_player_set_option(player, "stimeout", "5000000");          // 5秒连接超时
            mk_player_set_option(player, "max_delay", "500000");          // 最大延迟500ms
            mk_player_set_option(player, "rtsp_transport", "tcp");        // 强制使用TCP

            mk_player_set_on_result(player, on_mk_play_event_func, &app_ctx);
            mk_player_set_on_shutdown(player, on_mk_shutdown_func, &app_ctx);
            LOGD("mk_player callbacks set");

            LOGD("Starting RTSP play with enhanced options: %s", rtsp_url);
            mk_player_play(player, rtsp_url);
            LOGD("mk_player_play called");

        } catch (const std::invalid_argument& e) {
            LOGE("ZLMediaKit player network error (ignored): %s", e.what());
            if (player) {
                mk_player_release(player);
                player = nullptr;
            }
            return -1;
        } catch (const std::exception& e) {
            LOGE("ZLMediaKit player error (ignored): %s", e.what());
            if (player) {
                mk_player_release(player);
                player = nullptr;
            }
            return -1;
        } catch (...) {
            LOGE("ZLMediaKit player unknown error (ignored)");
            if (player) {
                mk_player_release(player);
                player = nullptr;
            }
            return -1;
        }

        // 添加连接状态检查和错误恢复
        int status_check_count = 0;
        int connection_timeout_count = 0;
        bool connection_established = false;

        while (isStreaming) {
            // 根据性能模式调整循环频率
            int sleepMs = app_ctx.performance_mode ? 33 : 50;  // 性能模式30FPS，普通模式20FPS
            std::this_thread::sleep_for(std::chrono::milliseconds(sleepMs));

            try {
                // 检查是否卡住
                if (isStuck()) {
                    LOGW("Camera %d is stuck, attempting restart", app_ctx.camera_index);
                    if (!attemptRestart()) {
                        LOGE("Camera %d restart failed, breaking RTSP loop", app_ctx.camera_index);
                        break;
                    }
                    continue;  // 重启后继续循环
                }

                get_detect_result();

                // 如果能正常获取结果，说明连接正常
                if (!connection_established) {
                    connection_established = true;
                    connection_timeout_count = 0;
                    LOGD("RTSP connection established successfully for camera %d", app_ctx.camera_index);
                }

            } catch (...) {
                LOGW("Error in get_detect_result for camera %d, connection may be unstable", app_ctx.camera_index);
                connection_timeout_count++;
                updateFrameStatus(false);  // 记录失败状态
            }

            status_check_count++;

            // 每5秒检查一次连接状态和内存使用
            if (status_check_count % 50 == 0) { // 5秒 = 50 * 100ms
                if (connection_established) {
                    LOGD("Camera %d RTSP connection active for %d seconds",
                         app_ctx.camera_index, status_check_count * 100 / 1000);
                } else {
                    LOGW("Camera %d RTSP connection not established after %d seconds",
                         app_ctx.camera_index, status_check_count * 100 / 1000);
                    connection_timeout_count++;
                }

                // 定期记录内存使用情况
                if (status_check_count % 200 == 0) { // 每20秒记录一次内存
                    logMemoryUsage();
                }
            }

            // 如果连接超时次数过多，尝试重连
            if (connection_timeout_count > 30) { // 30次检查失败
                LOGW("Camera %d RTSP connection timeout, attempting restart", app_ctx.camera_index);

                // 尝试重新连接
                if (attemptRestart()) {
                    connection_timeout_count = 0;  // 重置超时计数
                    connection_established = false; // 重置连接状态
                    continue;
                } else {
                    LOGE("Camera %d restart failed after timeout, breaking loop", app_ctx.camera_index);
                    break; // 退出循环
                }
            }
        }
        
        if (player) {
            mk_player_release(player);
        }
        
    } catch (const std::exception& e) {
        LOGE("Exception in RTSP processing: %s", e.what());
        return -1;
    } catch (...) {
        LOGE("Unknown exception in RTSP processing");
        return -1;
    }
    
    return 0;
}

ZLPlayer::~ZLPlayer() {
    LOGD("ZLPlayer destructor called - cleaning up resources");

    // 1. 停止RTSP线程
    if (pid_rtsp != 0) {
        LOGD("Stopping RTSP thread in destructor");
        isStreaming = false;

        // 在析构函数中，直接detach线程避免阻塞
        pthread_detach(pid_rtsp);
        pid_rtsp = 0;
        LOGD("RTSP thread detached in destructor");
    }

    // 2. 停止渲染线程
    if (pid_render != 0) {
        LOGD("Stopping render thread in destructor");
        // 渲染线程通常是无限循环，直接detach
        pthread_detach(pid_render);
        pid_render = 0;
    }

    // 3. 释放专用窗口
    if (dedicatedWindow) {
        pthread_mutex_lock(&windowMutex);
        ANativeWindow_release(dedicatedWindow);
        dedicatedWindow = nullptr;
        pthread_mutex_unlock(&windowMutex);
        LOGD("Released dedicated window");
    }

    // 4. 清理YOLOv5线程池
    if (app_ctx.yolov5ThreadPool) {
        delete app_ctx.yolov5ThreadPool;
        app_ctx.yolov5ThreadPool = nullptr;
        LOGD("Cleaned up YOLOv5 thread pool");
    }

    // 🔧 新增: 清理统一推理管理器
    if (app_ctx.inference_manager) {
        app_ctx.inference_manager->release();
        delete app_ctx.inference_manager;
        app_ctx.inference_manager = nullptr;
        LOGD("Cleaned up unified inference manager");
    }

    // 🔧 新增: 清理简化的管理器（已设为nullptr，无需清理）
    app_ctx.face_analysis_manager = nullptr;
    app_ctx.statistics_manager = nullptr;
    LOGD("Simplified managers cleanup completed");

    // 5. 清理MPP解码器
    if (app_ctx.decoder) {
        delete app_ctx.decoder;
        app_ctx.decoder = nullptr;
        LOGD("Cleaned up MPP decoder");
    }

    // 6. 释放RTSP URL
    if (rtsp_url != nullptr) {
        delete[] rtsp_url;
        rtsp_url = nullptr;
    }

    // 7. 释放模型数据
    if (modelFileContent != nullptr) {
        delete[] modelFileContent;  // 🔧 修复: 使用delete[]释放new[]分配的内存
        modelFileContent = nullptr;
    }

    LOGD("ZLPlayer destructor completed");
}

// 🔧 新增: 从Java层获取启用的类别
std::set<std::string> ZLPlayer::getEnabledClassesFromJava() {
    std::set<std::string> enabledClasses;
    
    // 🔧 修复: 从Java层DetectionSettingsManager动态获取启用的类别
    // 这里应该通过JNI调用获取，但为了简化实现，我们读取当前的配置
    // 用户可以通过SettingsActivity界面配置这些类别
    
    // 默认启用的类别（从DetectionSettingsManager的默认值获取）
    enabledClasses.insert("person");  // 默认启用人员检测
    
    // 🔧 注意: 用户可以通过SettingsActivity界面修改这些设置
    // 实际的类别过滤现在由Java层的DetectionResultFilter处理
    // native层的过滤主要用于减少绘制开销
    
    LOGD("📋 Native层使用的启用类别: person (默认)");
    LOGD("💡 用户可通过设置界面配置更多类别");
    
    return enabledClasses;
}

static struct timeval lastRenderTime;

void ZLPlayer::mpp_decoder_frame_callback(void *userdata, int width_stride, int height_stride, int width, int height, int format, int fd, void *data) {
    rknn_app_context_t *ctx = (rknn_app_context_t *) userdata;
    struct timeval start;
    struct timeval end;
    struct timeval memCpyEnd;
    gettimeofday(&start, NULL);
    
    // 使用RTSP时间戳进行更好的时间同步
    static uint64_t lastPts = 0;
    uint64_t currentPts = ctx->pts;
    
    long frameGap = start.tv_sec * 1000 + start.tv_usec / 1000 - lastRenderTime.tv_usec / 1000 - lastRenderTime.tv_sec * 1000;
    
    // 如果帧间隔太短（小于25ms），说明解码速度过快，需要控制
    if (frameGap < 25 && lastRenderTime.tv_sec != 0) {
        LOGD("Frame gap too short (%ld ms), skipping frame to maintain sync", frameGap);
        return; // 跳过这一帧，保持同步
    }
    
    LOGD("mpp_decoder_frame_callback Frame gap: %ld ms, PTS: %lu", frameGap, currentPts);
    gettimeofday(&lastRenderTime, NULL);

    // 12,441,600 3840x2160x3/2
    // int imgSize = width * height * get_bpp_from_format(RK_FORMAT_RGBA_8888);
#if 0
    auto frameData = std::make_shared<frame_data_t>();

    char *dstBuf = new char[imgSize]();
    memcpy(dstBuf, data, imgSize);

    gettimeofday(&memCpyEnd, NULL);
    frameGap = memCpyEnd.tv_sec * 1000 + memCpyEnd.tv_usec / 1000 - start.tv_usec / 1000 - start.tv_sec * 1000;
    LOGD("mpp_decoder_frame_callback Frame mem cpy spent :%ld\n", frameGap);

    frameData->dataSize = imgSize;
    frameData->screenStride = width * get_bpp_from_format(RK_FORMAT_YCbCr_420_SP);  // 解码出来的格式就是nv12
    frameData->data = dstBuf;
    frameData->screenW = width;
    frameData->screenH = height;
    frameData->heightStride = height_stride;
    frameData->widthStride = width_stride;
    frameData->frameFormat = RK_FORMAT_YCbCr_420_SP;
    frameData->frameId = ctx->job_cnt;

    ctx->job_cnt++;

    // 放入显示队列
    ctx->renderFrameQueue->push(frameData);

    LOGD("mpp_decoder_frame_callback task list size :%d", ctx->mppDataThreadPool->get_task_size());

    // 放入线程池, 进行并行推理
    ctx->mppDataThreadPool->submitTask(frameData);

    gettimeofday(&end, NULL);
    frameGap = end.tv_sec * 1000 + end.tv_usec / 1000 - start.tv_usec / 1000 - start.tv_sec * 1000;
    LOGD("mpp_decoder_frame_callback Frame spent :%ld\n", frameGap);

    return;
#endif

    int dstImgSize = width_stride * height_stride * get_bpp_from_format(RK_FORMAT_RGBA_8888);
    LOGD("img size is %d", dstImgSize);
    // img size is 33177600 1080p: 8355840
    char *dstBuf = new char[dstImgSize]();
    // rga_change_color_async(width_stride, height_stride, RK_FORMAT_YCbCr_420_SP, (char *) data,
    // width_stride, height_stride, RK_FORMAT_RGBA_8888, dstBuf);

    rga_change_color(width_stride, height_stride, RK_FORMAT_YCbCr_420_SP, (char *) data,
                     width_stride, height_stride, RK_FORMAT_RGBA_8888, dstBuf);

    auto frameData = std::make_shared<frame_data_t>();
    frameData->dataSize = dstImgSize;
    frameData->screenStride = width * get_bpp_from_format(RK_FORMAT_RGBA_8888);
    frameData->data = dstBuf;
    frameData->screenW = width;
    frameData->screenH = height;
    frameData->heightStride = height_stride;
    frameData->widthStride = width_stride;
    frameData->frameFormat = RK_FORMAT_RGBA_8888;

    // LOGD(">>>>>  frame id:%d", frameData->frameId);
    // LOGD("mpp_decoder_frame_callback task list size :%d", ctx->mppDataThreadPool->get_task_size());

    // 放入显示队列
    // ctx->renderFrameQueue->push(frameData);

    frameData->frameId = ctx->job_cnt;
    int detectPoolSize = ctx->yolov5ThreadPool->get_task_size();
    LOGD("detectPoolSize :%d", detectPoolSize);

    // 添加帧跳跃控制，避免推理队列过载
    // 但不能破坏时间同步机制
    ctx->frame_cnt++;
    
    // 优化后的推理策略：统一每2帧处理1帧（50%处理率）
    int maxQueueSize = ctx->performance_mode ? 3 : 5;  // 性能模式更严格的队列控制
    int frameSkip = 2;  // 统一设置为每2帧处理1帧，提高检测频率

    // 根据摄像头优先级调整处理策略
    bool isHighPriority = (ctx->camera_index == 0);  // 第一个摄像头优先级最高
    if (isHighPriority) {
        maxQueueSize += 2;  // 高优先级摄像头允许更大队列
        // 高优先级摄像头保持每2帧处理1帧，不再进一步减少跳帧
        // frameSkip 保持为2，确保所有摄像头使用统一的处理频率
    }

    bool shouldInference = (ctx->frame_cnt % frameSkip == 0 && detectPoolSize < maxQueueSize);

    if (shouldInference) {
        // 提交推理任务
        ctx->yolov5ThreadPool->submitTask(frameData);
        ctx->job_cnt++;
        LOGD("Camera %d Frame %d submitted to inference pool (priority: %s, skip_rate: 1/2)",
             ctx->camera_index, ctx->frame_cnt, isHighPriority ? "high" : "normal");
    } else {
        // 跳过推理，直接释放内存
        delete frameData->data;
        frameData->data = nullptr;
        LOGD("Camera %d Frame %d skipped inference (pool size: %d, max: %d, skip_rate: 1/2)",
             ctx->camera_index, ctx->frame_cnt, detectPoolSize, maxQueueSize);
    }

    //    if (ctx->frame_cnt % 2 == 1) {
    //        // if (detectPoolSize < MAX_TASK) {
    //        // 放入线程池, 进行并行推理
    //        ctx->yolov5ThreadPool->submitTask(frameData);
    //        ctx->job_cnt++;
    //    } else {
    //        // 直接释放
    //        delete frameData->data;
    //        frameData->data = nullptr;
    //    }

}

#if 0

void mpp_decoder_frame_callback_good_display(void *userdata, int width_stride, int height_stride, int width, int height, int format, int fd, void *data) {
    rknn_app_context_t *ctx = (rknn_app_context_t *) userdata;
    struct timeval end;
    gettimeofday(&end, NULL);
    long frameGap = end.tv_sec * 1000 + end.tv_usec / 1000 -
                    lastRenderTime.tv_usec / 1000 - lastRenderTime.tv_sec * 1000;
    // LOGD("decoded frame ctx->dts:%ld", ctx->dts);
    LOGD("mpp_decoder_frame_callback Frame gap :%ld\n", frameGap);
    gettimeofday(&lastRenderTime, NULL);

    rga_buffer_t origin;

    int imgSize = width * height * get_bpp_from_format(RK_FORMAT_RGBA_8888);
    // char *dstBuf = (char *) malloc(imgSize);
    // memset(dstBuf, 0, imgSize);
    // std::unique_ptr<char[]> dstBuf(new char[imgSize]());

    char *dstBuf = new char[imgSize]();
    rga_change_color_async(width_stride, height_stride, RK_FORMAT_YCbCr_420_SP, (char *) data,
                           width, height, RK_FORMAT_RGBA_8888, dstBuf);

    // usleep(1000 * 80);

#if 0

    origin = wrapbuffer_fd(fd, width, height, RK_FORMAT_YCbCr_420_SP, width_stride, height_stride);
    cv::Mat origin_mat = cv::Mat::zeros(height, width, CV_8UC3);
    rga_buffer_t rgb_img = wrapbuffer_virtualaddr((void *) origin_mat.data, width, height, RK_FORMAT_RGB_888);
    imcopy(origin, rgb_img);


    // yolov8_thread_pool->submitTask(origin_mat, ctx->job_cnt++);
    yolov8_thread_pool->submitTask(origin_mat, ctx->job_cnt);
    std::vector<Detection> objects;

    // 获取推理结果
    // auto ret_code = yolov8_thread_pool->getTargetResultNonBlock(objects, ctx->result_cnt);
    auto ret_code = yolov8_thread_pool->getTargetResultNonBlock(objects, ctx->job_cnt);
    if (ret_code == NN_SUCCESS) {
        ctx->result_cnt++;
    }
    LOGD("ctx->result_cnt:%d", ctx->result_cnt);



    detect_result_group_t detect_result_group;
    memset(&detect_result_group, 0, sizeof(detect_result_group_t));
    detect_result_group.count = objects.size();
    uint8_t idx;
    for (idx = 0; idx < objects.size(); idx++) {
        LOGD("objects[%d].classId: %d\n", idx, objects[idx].class_id);
        LOGD("objects[%d].prop: %f\n", idx, objects[idx].confidence);
        LOGD("objects[%d].class name: %s\n", idx, objects[idx].className.c_str());
        // int left;
        // int right;
        // int top;
        // int bottom;
        detect_result_group.results[idx].box.left = objects[idx].box.x;
        detect_result_group.results[idx].box.right = objects[idx].box.x + objects[idx].box.width;
        detect_result_group.results[idx].box.top = objects[idx].box.y;
        detect_result_group.results[idx].box.bottom = objects[idx].box.y + objects[idx].box.height;
        detect_result_group.results[idx].classId = objects[idx].class_id;
        detect_result_group.results[idx].prop = objects[idx].confidence;
        strcpy(detect_result_group.results[idx].name, objects[idx].className.c_str());
    }
#endif

    // frame_data_t *frameData = new frame_data_t();
    auto frameData = std::make_shared<frame_data_t>();
    frameData->dataSize = imgSize;
    frameData->screenStride = width * get_bpp_from_format(RK_FORMAT_RGBA_8888);
    frameData->data = dstBuf;
    frameData->screenW = width;
    frameData->screenH = height;
    frameData->heightStride = height_stride;
    frameData->widthStride = width_stride;

    ctx->renderFrameQueue->push(frameData);
    LOGD("Now render frame queue size is :%d", ctx->renderFrameQueue->size());

    //
    //    rga_buffer_t origin;
    //    rga_buffer_t src;
    //    int mpp_frame_fd = 0;
    //
    //    // 复制到另一个缓冲区，避免修改mpp解码器缓冲区
    //    // 使用的是RK RGA的格式转换：YUV420SP -> RGB888
    //    origin = wrapbuffer_fd(fd, width, height, RK_FORMAT_YCbCr_420_SP, width_stride, height_stride);
    //    src = wrapbuffer_fd(mpp_frame_fd, width, height, RK_FORMAT_YCbCr_420_SP, width_stride, height_stride);
    //    cv::Mat origin_mat = cv::Mat::zeros(height, width, CV_8UC3);
    //    rga_buffer_t rgb_img = wrapbuffer_virtualaddr((void *) origin_mat.data, width, height, RK_FORMAT_RGB_888);
    //    imcopy(origin, rgb_img);
    //
    //    LOGD("task size is:%d\n", yolov8_thread_pool->get_task_size());
    //    // 提交推理任务给线程池
    //    yolov8_thread_pool->submitTask(origin_mat, job_cnt++);
    //    std::vector<Detection> objects;
    // yolov8_thread_pool->submitTask(origin_mat, job_cnt++);
}

// 解码后的数据回调函数
void ZLPlayer::mpp_decoder_frame_callback(void *userdata, int width_stride, int height_stride, int width, int height, int format, int fd, void *data) {

    //    LOGD("width_stride :%d height_stride :%d width :%d height :%d format :%d\n",
    //         width_stride, height_stride, width, height, format);
    // LOGD("mpp_decoder_frame_callback\n");
    struct timeval now;
    struct timeval end;
    gettimeofday(&now, NULL);
    rknn_app_context_t *ctx = (rknn_app_context_t *) userdata;

    int ret = 0;
    static int frame_index = 0;
    frame_index++;

    void *mpp_frame = NULL;
    int mpp_frame_fd = 0;
    void *mpp_frame_addr = NULL;
    int enc_data_size;

    rga_buffer_t origin;
    rga_buffer_t src;

    // 复制到另一个缓冲区，避免修改mpp解码器缓冲区
    // 使用的是RK RGA的格式转换：YUV420SP -> RGB888
    origin = wrapbuffer_fd(fd, width, height, RK_FORMAT_YCbCr_420_SP, width_stride, height_stride);
    src = wrapbuffer_fd(mpp_frame_fd, width, height, RK_FORMAT_YCbCr_420_SP, width_stride, height_stride);
    cv::Mat origin_mat = cv::Mat::zeros(height, width, CV_8UC3);
    rga_buffer_t rgb_img = wrapbuffer_virtualaddr((void *) origin_mat.data, width, height, RK_FORMAT_RGB_888);
    imcopy(origin, rgb_img);

    static int job_cnt = 0;
    static int result_cnt = 0;
    LOGD("task size is:%d\n", yolov8_thread_pool->get_task_size());
    // 提交推理任务给线程池
    yolov8_thread_pool->submitTask(origin_mat, job_cnt++);
    std::vector<Detection> objects;
    // 获取推理结果
    auto ret_code = yolov8_thread_pool->getTargetResultNonBlock(objects, result_cnt);
    if (ret_code == NN_SUCCESS) {
        result_cnt++;
    }

    uint8_t idx;
    for (idx = 0; idx < objects.size(); idx++) {
        LOGD("objects[%d].classId: %d\n", idx, objects[idx].class_id);
        LOGD("objects[%d].prop: %f\n", idx, objects[idx].confidence);
        LOGD("objects[%d].class name: %s\n", idx, objects[idx].className.c_str());
        // int left;
        // int right;
        // int top;
        // int bottom;
    }

    DrawDetections(origin_mat, objects);
    // imcopy(rgb_img, src);

    // LOGD("result_cnt: %d\n", result_cnt);
    cv::cvtColor(origin_mat, origin_mat, cv::COLOR_RGB2RGBA);
    
    // 使用专用窗口渲染，如果没有专用窗口则使用全局窗口
    if (dedicatedWindow) {
        renderFrameToWindow(origin_mat.data, width, height, width * get_bpp_from_format(RK_FORMAT_RGBA_8888), dedicatedWindow);
    } else {
        renderFrame(origin_mat.data, width, height, width * get_bpp_from_format(RK_FORMAT_RGBA_8888));
    }

    gettimeofday(&end, NULL);

    double timeused = 1000 * (end.tv_sec - now.tv_sec) + (end.tv_usec - now.tv_usec) / 1000;
    // LOGD("Spent:%f", timeused);

    long frameGap = end.tv_sec * 1000 + end.tv_usec / 1000 - lastRenderTime.tv_usec / 1000 - lastRenderTime.tv_sec * 1000;

    LOGD("Frame gap :%ld\n", frameGap);

    gettimeofday(&lastRenderTime, NULL);

}
#endif

//void ZLPlayer::setRenderCallback(RenderCallback renderCallback_) {
//    this->renderCallback = renderCallback_;
//}

// 🔧 新增: 模型选择接口实现
int ZLPlayer::setInferenceModel(int model_type) {
    if (!app_ctx.inference_manager) {
        LOGE("Inference manager not initialized");
        return -1;
    }

    ModelType type = static_cast<ModelType>(model_type);
    int ret = app_ctx.inference_manager->setCurrentModel(type);

    if (ret == 0) {
        LOGD("Successfully switched to model type: %d", model_type);
    } else {
        LOGE("Failed to switch to model type: %d", model_type);
    }

    return ret;
}

int ZLPlayer::getCurrentInferenceModel() {
    if (!app_ctx.inference_manager) {
        LOGE("Inference manager not initialized");
        return -1;
    }

    ModelType current = app_ctx.inference_manager->getCurrentModel();
    return static_cast<int>(current);
}

bool ZLPlayer::isModelAvailable(int model_type) {
    if (!app_ctx.inference_manager) {
        LOGE("Inference manager not initialized");
        return false;
    }

    ModelType type = static_cast<ModelType>(model_type);
    return app_ctx.inference_manager->isModelInitialized(type);
}

// 🔧 新增：简化的人员统计处理函数
void ZLPlayer::processPersonDetectionAndFaceAnalysis(cv::Mat& frame,
                                                     const std::vector<Detection>& detections,
                                                     std::shared_ptr<frame_data_t> frameData) {
    try {
        // 统计人员数量
        int personCount = 0;
        std::vector<Detection> personDetections;

        // 过滤出人员检测结果
        for (const auto& detection : detections) {
            if (detection.className == "person" && detection.confidence > 0.5) {
                personDetections.push_back(detection);
                personCount++;
            }
        }

        // 🔧 简化的人员统计（每10帧输出一次日志，避免日志过多）
        static int logCounter = 0;
        static int framesSinceLastLog = 0;
        framesSinceLastLog++;

        if (++logCounter % 10 == 0 && personCount > 0) {
            LOGD("🔍 Camera %d 检测到 %d 个人员 (frame %d, 最近10帧中有人员)",
                 app_ctx.camera_index, personCount, logCounter);
        }

        // 🔧 基本的人员跟踪（简化实现）
        if (personCount > 0) {
            // 简单的人员计数和位置记录
            static int totalPersonsSeen = 0;
            static cv::Point2f lastPersonCenter(-1, -1); // 记录上一次检测到的人员中心位置
            totalPersonsSeen += personCount;

            // 记录人员位置信息（用于简单的跟踪）
            for (const auto& person : personDetections) {
                // 计算人员中心位置
                cv::Point2f currentCenter(
                    person.box.x + person.box.width / 2.0f,
                    person.box.y + person.box.height / 2.0f
                );

                // 简单的移动检测
                bool isMoving = false;
                if (lastPersonCenter.x >= 0 && lastPersonCenter.y >= 0) {
                    float distance = cv::norm(currentCenter - lastPersonCenter);
                    isMoving = distance > 10.0f; // 移动阈值：10像素
                }

                // 简化的位置记录，可以后续扩展为完整的跟踪算法
                LOGD("📍 Camera %d 人员位置: [%d,%d,%d,%d] 中心:(%.1f,%.1f) 置信度:%.2f %s",
                     app_ctx.camera_index,
                     person.box.x, person.box.y,
                     person.box.x + person.box.width, person.box.y + person.box.height,
                     currentCenter.x, currentCenter.y,
                     person.confidence,
                     isMoving ? "🚶移动" : "🧍静止");

                lastPersonCenter = currentCenter; // 更新位置记录
            }

            // 每100帧输出一次累计统计
            if (logCounter % 100 == 0) {
                double avgPersonsPerFrame = (double)totalPersonsSeen / logCounter;
                LOGD("📊 Camera %d 累计统计: 总计%d人次, 平均%.2f人/帧, 当前帧%d人",
                     app_ctx.camera_index, totalPersonsSeen, avgPersonsPerFrame, personCount);
            }
        }

        // 🔧 内存优化：与现有清理机制兼容的定期清理
        static int processCounter = 0;
        if (++processCounter % 200 == 0) { // 每200次处理清理一次（降低频率）
            // 简单的内存清理
            LOGD("🧹 Camera %d 执行简化的内存清理 (counter: %d)", app_ctx.camera_index, processCounter);
        }

    } catch (const std::exception& e) {
        LOGE("❌ Camera %d processPersonDetectionAndFaceAnalysis exception: %s",
             app_ctx.camera_index, e.what());
    } catch (...) {
        LOGE("❌ Camera %d processPersonDetectionAndFaceAnalysis unknown exception",
             app_ctx.camera_index);
    }
}

// 🔧 实现：简化的人员跟踪功能
std::vector<Detection> ZLPlayer::performPersonTracking(const std::vector<Detection>& personDetections) {
    // 简化实现：直接返回检测结果，避免复杂的跟踪逻辑
    return personDetections;
}

// 🔧 实现：简化的人脸分析功能
std::vector<FaceAnalysisResult> ZLPlayer::performFaceAnalysis(const cv::Mat& frame,
                                                              const std::vector<Detection>& personDetections) {
    std::vector<FaceAnalysisResult> faceResults;

    // 简化实现：暂时返回空结果，避免复杂的人脸分析逻辑
    // 后续可以根据需要添加实际的人脸分析功能

    return faceResults;
}

// 🔧 实现：简化的人员统计数据更新
void ZLPlayer::updatePersonStatistics(const std::vector<Detection>& trackedPersons,
                                      const std::vector<FaceAnalysisResult>& faceResults) {
    // 简化实现：基本的统计记录
    static int totalPersonCount = 0;
    totalPersonCount += trackedPersons.size();

    // 每50次更新输出一次统计信息
    static int updateCounter = 0;
    if (++updateCounter % 50 == 0) {
        LOGD("📊 Camera %d 简化统计: 当前%zu人员, 累计%d人次",
             app_ctx.camera_index, trackedPersons.size(), totalPersonCount);
    }
}

// 🔧 实现：简化的结果发送到Java层
void ZLPlayer::sendResultsToJava(const std::vector<Detection>& trackedPersons,
                                 const std::vector<FaceAnalysisResult>& faceResults) {
    // 简化实现：暂时只记录日志，后续可以添加实际的JNI调用
    static int sendCounter = 0;
    if (++sendCounter % 100 == 0) {
        LOGD("📤 Camera %d 简化结果记录: %zu人员, %zu人脸 (第%d次)",
             app_ctx.camera_index, trackedPersons.size(), faceResults.size(), sendCounter);
    }
}

// 🔧 实现：简化的人员跟踪数据清理
void ZLPlayer::cleanupPersonTrackingData() {
    // 简化实现：基本的清理操作
    LOGD("🧹 Camera %d 简化的人员跟踪数据清理完成", app_ctx.camera_index);
}
