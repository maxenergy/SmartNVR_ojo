#ifndef MY_YOLOV5_RTSP_THREAD_POOL_USER_COMM_H
#define MY_YOLOV5_RTSP_THREAD_POOL_USER_COMM_H

#include "mpp_decoder.h"

// 🔧 新增: YOLOv8n相关常量定义
#define MODEL_TYPE_YOLOV5  0
#define MODEL_TYPE_YOLOV8N 1

// YOLOv8n模型文件路径（自定义转换模型）
#define YOLOV8N_MODEL_PATH "/data/data/com.wulala.myyolov5rtspthreadpool/files/best_rk3588.rknn"
#define YOLOV8N_LABELS_PATH "/data/data/com.wulala.myyolov5rtspthreadpool/files/best_labels.txt"

// 推理模型选择
typedef enum {
    INFERENCE_MODEL_YOLOV5 = 0,
    INFERENCE_MODEL_YOLOV8N = 1
} inference_model_type_t;

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
