

#ifndef RK3588_DEMO_YOLOV5_H
#define RK3588_DEMO_YOLOV5_H

#include "yolo_datatype.h"
#include "engine.h"
#include "preprocess.h"
#include "user_comm.h"

class Yolov5 {
public:
    Yolov5();

    ~Yolov5();
    nn_error_e LoadModelWithData(char *modelData, int modelSize);
    nn_error_e LoadModel(const char *model_path);                        // 加载模型
    nn_error_e Run(const cv::Mat &img, std::vector <Detection> &objects); // 运行模型
    nn_error_e RunWithFrameData(const std::shared_ptr <frame_data_t> frameData, std::vector <Detection> &objects);

    // NPU核心管理
    void SetNPUCore(int core_id);                                        // 设置NPU核心
    int GetNPUCore() const;                                              // 获取当前NPU核心

private:
    nn_error_e Preprocess(const cv::Mat &img, const std::string process_type, cv::Mat &image_letterbox);   // 图像预处理
    nn_error_e Inference();                                                      // 推理
    nn_error_e Postprocess(const cv::Mat &img, std::vector <Detection> &objects); // 后处理

    LetterBoxInfo letterbox_info_;
    tensor_data_s input_tensor_;
    std::vector <tensor_data_s> output_tensors_;
    std::vector <int32_t> out_zps_;
    std::vector<float> out_scales_;
    std::shared_ptr <NNEngine> engine_;
};

#endif // RK3588_DEMO_YOLOV5_H
