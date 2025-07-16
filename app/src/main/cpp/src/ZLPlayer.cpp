#include <mk_common.h>
#include <mk_player.h>
#include <android/native_window_jni.h>
#include <chrono>
#include <thread>
#include <opencv2/opencv.hpp>
#include "ZLPlayer.h"
#include "mpp_err.h"
#include "cv_draw.h"
// Yolov8ThreadPool *yolov8_thread_pool;   // 线程池

extern pthread_mutex_t windowMutex;     // 静态初始化 所
extern ANativeWindow *window;

void *rtps_process(void *arg) {
    ZLPlayer *player = (ZLPlayer *) arg;
    if (player) {
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
        this->modelFileContent = (char *) malloc(modelSize);
        memcpy(this->modelFileContent, modelData, modelSize);

        // 重新初始化YOLOv5线程池
        if (app_ctx.yolov5ThreadPool != nullptr) {
            app_ctx.yolov5ThreadPool->setUpWithModelData(20, this->modelFileContent, this->modelFileSize);
            LOGD("YOLOv5 thread pool re-initialized with new model data");
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
    pthread_create(&pid_rtsp, nullptr, rtps_process, this);
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

ZLPlayer::ZLPlayer(char *modelFileData, int modelDataLen) {

    // 使用本地网络示例URL，避免连接到无效的演示URL
    const char *default_url = "rtsp://admin:sharpi1688@192.168.1.2:554/1/1";
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
    // app_ctx.job_cnt = 1;
    // app_ctx.result_cnt = 1;
    // app_ctx.mppDataThreadPool = new MppDataThreadPool();
    // yolov8_thread_pool = new Yolov8ThreadPool(); // 创建线程池
    // yolov8_thread_pool->setUpWithModelData(20, this->modelFileContent, this->modelFileSize);
    app_ctx.yolov5ThreadPool = new Yolov5ThreadPool(); // 创建线程池
    if (this->modelFileContent != nullptr && this->modelFileSize > 0) {
        app_ctx.yolov5ThreadPool->setUpWithModelData(20, this->modelFileContent, this->modelFileSize);
        LOGD("YOLOv5 thread pool initialized with model data");
    } else {
        LOGW("YOLOv5 thread pool created without model data - will initialize later");
    }

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

    pthread_mutex_lock(&windowMutex);
    if (!targetWindow) {
        LOGD("renderFrameToWindow: target window is null, skipping render");
        pthread_mutex_unlock(&windowMutex);
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
    // int queueSize = app_ctx.renderFrameQueue->size();
    // LOGD("app_ctx.renderFrameQueue.size() :%d", queueSize);

    // auto frameDataPtr = app_ctx.renderFrameQueue->pop();
    //    if (frameDataPtr == nullptr) {
    //        LOGD("frameDataPtr is null");
    //        return;
    //    }
    std::this_thread::sleep_until(nextRendTime);
    // renderFrame((uint8_t *) frameDataPtr->data, frameDataPtr->screenW, frameDataPtr->screenH, frameDataPtr->screenStride);
    // 释放内存
    // delete frameDataPtr->data;
    // frameDataPtr->data = nullptr;

    // 设置下一次执行的时间点
    nextRendTime = std::chrono::steady_clock::now() + std::chrono::milliseconds(30);

}

void ZLPlayer::get_detect_result() {
    std::vector<Detection> objects;
    // LOGD("decoder_callback Getting result count :%d", app_ctx.result_cnt);
    auto ret_code = app_ctx.yolov5ThreadPool->getTargetResultNonBlock(objects, app_ctx.result_cnt);
    if (ret_code == NN_SUCCESS) {

        uint8_t idx;
        for (idx = 0; idx < objects.size(); idx++) {
            LOGD("objects[%d].classId: %d\n", idx, objects[idx].class_id);
            LOGD("objects[%d].prop: %f\n", idx, objects[idx].confidence);
            LOGD("objects[%d].class name: %s\n", idx, objects[idx].className.c_str());
        }
        auto frameData = app_ctx.yolov5ThreadPool->getTargetImgResult(app_ctx.result_cnt);
        app_ctx.result_cnt++;
        LOGD("Get detect result counter:%d start display", app_ctx.result_cnt);
        
        // 在显示之前绘制检测框
        if (objects.size() > 0) {
            // 将RGBA数据转换为cv::Mat进行绘制
            cv::Mat display_mat(frameData->screenH, frameData->screenW, CV_8UC4, frameData->data);
            
            // 转换为RGB格式进行绘制（OpenCV绘制需要RGB格式）
            cv::Mat rgb_mat;
            cv::cvtColor(display_mat, rgb_mat, cv::COLOR_RGBA2RGB);
            
            // 绘制检测框
            DrawDetections(rgb_mat, objects);
            LOGD("Drew %zu detection boxes", objects.size());
            
            // 转换回RGBA格式
            cv::cvtColor(rgb_mat, display_mat, cv::COLOR_RGB2RGBA);
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
        if (dedicatedWindow) {
            renderFrameToWindow((uint8_t *) frameData->data, frameData->screenW, frameData->screenH, frameData->screenStride, dedicatedWindow);
        } else {
            renderFrame((uint8_t *) frameData->data, frameData->screenW, frameData->screenH, frameData->screenStride);
        }
        // 释放内存
        delete frameData->data;
        frameData->data = nullptr;

    } else if (NN_RESULT_NOT_READY == ret_code) {
        // LOGD("decoder_callback wait for result ready");
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
    
    try {
        // 添加URL预验证
        if (strlen(rtsp_url) < 10 ||
            (strncmp(rtsp_url, "rtsp://", 7) != 0 &&
             strncmp(rtsp_url, "http://", 7) != 0 &&
             strncmp(rtsp_url, "https://", 8) != 0)) {
            LOGE("Invalid RTSP URL format: %s", rtsp_url);
            return -1;
        }

        mk_env_init(&config);
        LOGD("mk_env_init completed");

        mk_player player = mk_player_create();
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

        // 添加连接状态检查和错误恢复
        int status_check_count = 0;
        int connection_timeout_count = 0;
        bool connection_established = false;

        while (true) {
            // 减少主循环频率，避免过度消耗CPU
            std::this_thread::sleep_for(std::chrono::milliseconds(100));

            try {
                get_detect_result();

                // 如果能正常获取结果，说明连接正常
                if (!connection_established) {
                    connection_established = true;
                    connection_timeout_count = 0;
                    LOGD("RTSP connection established successfully");
                }

            } catch (...) {
                LOGW("Error in get_detect_result, connection may be unstable");
                connection_timeout_count++;
            }

            status_check_count++;

            // 每5秒检查一次连接状态
            if (status_check_count % 50 == 0) { // 5秒 = 50 * 100ms
                if (connection_established) {
                    LOGD("RTSP connection active for %d seconds", status_check_count * 100 / 1000);
                } else {
                    LOGW("RTSP connection not established after %d seconds", status_check_count * 100 / 1000);
                    connection_timeout_count++;
                }
            }

            // 如果连接超时次数过多，尝试重连
            if (connection_timeout_count > 30) { // 30次检查失败
                LOGE("RTSP connection timeout, attempting to restart");
                break; // 退出循环，让上层重新启动
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
    if (rtsp_url != nullptr) {
        delete[] rtsp_url;
        rtsp_url = nullptr;
    }
    
    if (modelFileContent != nullptr) {
        free(modelFileContent);
        modelFileContent = nullptr;
    }
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
    
    // 检查推理线程池是否过载
    const int MAX_QUEUE_SIZE = 5; // 最大队列长度
    bool shouldInference = (ctx->frame_cnt % 3 == 0 && detectPoolSize < MAX_QUEUE_SIZE);
    
    if (shouldInference) {
        // 每第3帧才进行推理，并且队列不能过载
        ctx->yolov5ThreadPool->submitTask(frameData);
        ctx->job_cnt++;
        LOGD("Frame %d submitted to inference pool", ctx->frame_cnt);
    } else {
        // 跳过推理，直接释放内存
        // 不要直接渲染，让get_detect_result()处理显示时机
        delete frameData->data;
        frameData->data = nullptr;
        LOGD("Frame %d skipped inference (pool size: %d)", ctx->frame_cnt, detectPoolSize);
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
