#ifndef AIBOX_ZLPLAYER_H
#define AIBOX_ZLPLAYER_H

#include "safe_queue.h"
#include "util.h"
#include "rknn_api.h"
#include <unistd.h>
#include "rk_mpi.h"
#include "im2d.h"
#include "rga.h"
#include "RgaUtils.h"
#include "im2d.hpp"
#include "rga_utils.h"
#include "mpp_decoder.h"
#include "yolov5_thread_pool.h"
#include "display_queue.h"
#include <android/native_window.h>
#include <vector>
#include <map>
#include <string>

typedef struct g_rknn_app_context_t {
    FILE *out_fp;
    MppDecoder *decoder;
    Yolov5ThreadPool *yolov5ThreadPool;
    RenderFrameQueue *renderFrameQueue;
    // MppEncoder *encoder;
    // mk_media media;
    // mk_pusher pusher;
    const char *push_url;
    uint64_t pts;
    uint64_t dts;

    int job_cnt;
    int result_cnt;
    int frame_cnt;

} rknn_app_context_t;

class ZLPlayer {

private:
    char *data_source = 0; // 指针 请赋初始值
    bool isStreaming = 0; // 是否播放
    pthread_t pid_rtsp = 0;
    pthread_t pid_render = 0;
    char *modelFileContent = 0;
    int modelFileSize = 0;
    ANativeWindow *dedicatedWindow = nullptr; // 专用渲染窗口

    std::chrono::steady_clock::time_point nextRendTime;

public:
    // static RenderCallback renderCallback;
    rknn_app_context_t app_ctx;
    char *rtsp_url = nullptr; // 移除硬编码的URL

    // ZLPlayer(const char *data_source, JNICallbackHelper *helper);
    ZLPlayer(char *modelFileData, int modelDataLen);

    ~ZLPlayer();

    static void mpp_decoder_frame_callback(void *userdata, int width_stride, int height_stride, int width, int height, int format, int fd, void *data);

    int process_video_rtsp();

    // int process_video_rtsp(rknn_app_context_t *ctx, const char *url);
    void setModelFile(char *data, int dataLen);

    // 获取模型数据（用于创建多实例）
    char* getModelData() const { return modelFileContent; }
    int getModelSize() const { return modelFileSize; }

    // 初始化模型数据（用于后续设置）
    void initializeModelData(char* modelData, int modelSize);

    // 设置RTSP URL
    void setRtspUrl(const char *url);

    // 启动RTSP流
    void startRtspStream();

    // 停止RTSP流
    void stopRtspStream();

    // 检查是否正在运行
    bool isRtspRunning() const { return pid_rtsp != 0; }

    // 设置专用的渲染窗口（用于多摄像头）
    void setNativeWindow(ANativeWindow *window);

    // void setRenderCallback(RenderCallback renderCallback_);

    void display();

    void get_detect_result();
    
    // 渲染到专用窗口（用于多摄像头）
    void renderFrameToWindow(uint8_t *src_data, int width, int height, int src_line_size, ANativeWindow *targetWindow);
};

#endif //AIBOX_ZLPLAYER_H
