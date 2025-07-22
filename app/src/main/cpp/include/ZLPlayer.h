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
#include <memory>

// 🔧 新增: 人员统计相关头文件
#include "person_detection_types.h"

// 🔧 新增: 前向声明统一推理管理器
class InferenceManager;

// 🔧 新增: 前向声明人脸分析和统计管理器
class FaceAnalysisManager;
class StatisticsManager;

typedef struct g_rknn_app_context_t {
    FILE *out_fp;
    MppDecoder *decoder;
    Yolov5ThreadPool *yolov5ThreadPool;
    RenderFrameQueue *renderFrameQueue;

    // 🔧 新增: 统一推理管理器（支持YOLOv5和YOLOv8n）
    InferenceManager *inference_manager;

    // 🔧 新增: 人脸分析和统计管理器
    FaceAnalysisManager *face_analysis_manager;
    StatisticsManager *statistics_manager;
    // MppEncoder *encoder;
    // mk_media media;
    // mk_pusher pusher;
    const char *push_url;
    uint64_t pts;
    uint64_t dts;

    int job_cnt;
    int result_cnt;
    int frame_cnt;

    // 性能优化相关
    int thread_pool_size;        // 动态线程池大小
    int camera_index;            // 摄像头索引，用于优先级控制
    bool performance_mode;       // 性能模式标志
    std::chrono::steady_clock::time_point last_frame_time; // 帧率控制

    // 卡住检测相关
    std::chrono::steady_clock::time_point last_successful_frame; // 最后成功帧时间
    int consecutive_failures;    // 连续失败次数
    bool is_stuck;              // 是否卡住状态
    int restart_attempts;       // 重启尝试次数

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

    // 性能优化方法
    void setPerformanceConfig(int cameraIndex, int totalCameras, bool performanceMode = true);
    void optimizeThreadPool();
    void setFrameRateLimit(int targetFps);
    void logMemoryUsage();  // 内存使用监控

    // 卡住检测和恢复方法
    bool isStuck();         // 检测是否卡住
    void resetStuckState(); // 重置卡住状态
    bool attemptRestart();  // 尝试重启RTSP流
    void updateFrameStatus(bool success); // 更新帧状态
    
    // 🔧 新增: 类别过滤相关方法
    std::set<std::string> getEnabledClassesFromJava(); // 从Java层获取启用的类别

    // void setRenderCallback(RenderCallback renderCallback_);

    void display();

    void get_detect_result();

    // 🔧 新增：获取当前检测结果
    bool getCurrentDetectionResults(std::vector<Detection>& results);

    // 渲染到专用窗口（用于多摄像头）
    void renderFrameToWindow(uint8_t *src_data, int width, int height, int src_line_size, ANativeWindow *targetWindow);

    // 🔧 新增: 模型选择接口
    /**
     * @brief 设置当前使用的推理模型
     * @param model_type 0=YOLOv5, 1=YOLOv8n
     * @return 0成功，-1失败
     */
    int setInferenceModel(int model_type);

    /**
     * @brief 获取当前使用的推理模型
     * @return 0=YOLOv5, 1=YOLOv8n, -1=未初始化
     */
    int getCurrentInferenceModel();

    /**
     * @brief 检查指定模型是否可用
     * @param model_type 0=YOLOv5, 1=YOLOv8n
     * @return true可用，false不可用
     */
    bool isModelAvailable(int model_type);

    // 🔧 新增：人员统计和人脸识别功能
    /**
     * @brief 处理人员检测和人脸分析
     * @param frame 当前帧图像
     * @param detections 检测结果
     * @param frameData 帧数据
     */
    void processPersonDetectionAndFaceAnalysis(cv::Mat& frame,
                                               const std::vector<Detection>& detections,
                                               std::shared_ptr<frame_data_t> frameData);

    /**
     * @brief 执行人员跟踪，避免重复计数
     * @param personDetections 人员检测结果
     * @return 跟踪后的人员列表
     */
    std::vector<Detection> performPersonTracking(const std::vector<Detection>& personDetections);

    /**
     * @brief 执行人脸分析和识别
     * @param frame 当前帧图像
     * @param personDetections 人员检测结果
     * @return 人脸分析结果
     */
    std::vector<FaceAnalysisResult> performFaceAnalysis(const cv::Mat& frame,
                                                        const std::vector<Detection>& personDetections);

    /**
     * @brief 更新人员统计数据
     * @param trackedPersons 跟踪的人员
     * @param faceResults 人脸分析结果
     */
    void updatePersonStatistics(const std::vector<Detection>& trackedPersons,
                               const std::vector<FaceAnalysisResult>& faceResults);

    /**
     * @brief 将结果发送到Java层
     * @param trackedPersons 跟踪的人员
     * @param faceResults 人脸分析结果
     */
    void sendResultsToJava(const std::vector<Detection>& trackedPersons,
                          const std::vector<FaceAnalysisResult>& faceResults);

    /**
     * @brief 清理人员跟踪数据
     */
    void cleanupPersonTrackingData();
};

#endif //AIBOX_ZLPLAYER_H
