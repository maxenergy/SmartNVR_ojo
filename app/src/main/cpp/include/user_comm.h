#ifndef MY_YOLOV5_RTSP_THREAD_POOL_USER_COMM_H
#define MY_YOLOV5_RTSP_THREAD_POOL_USER_COMM_H

#include "mpp_decoder.h"

typedef struct g_frame_data_t {
    char *data;
    long dataSize;
    int screenStride;
    int screenW;
    int screenH;
    int widthStride;
    int heightStride;
    int frameId;
    int frameFormat;

    // 🔧 添加析构函数来自动释放内存
    ~g_frame_data_t() {
        if (data) {
            delete[] data;
            data = nullptr;
        }
    }

    // 🔧 添加构造函数
    g_frame_data_t() : data(nullptr), dataSize(0), screenStride(0),
                       screenW(0), screenH(0), widthStride(0),
                       heightStride(0), frameId(0), frameFormat(0) {}
} frame_data_t;

#endif //MY_YOLOV5_RTSP_THREAD_POOL_USER_COMM_H
