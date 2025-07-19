#include "yolov8_engine.h"
#include "../include/logging.h"
#include "../process/yolov8_postprocess.h"
#include <fstream>
#include <chrono>

YOLOv8Engine::YOLOv8Engine()
    : ctx_(0)
    , scale_w_(1.0f)
    , scale_h_(1.0f)
    , initialized_(false)
    , n_input_(1)
    , n_output_(3) {
    
    // 初始化内存指针
    for (int i = 0; i < 1; i++) {
        input_mems_[i] = nullptr;
    }
    for (int i = 0; i < 3; i++) {
        output_mems_[i] = nullptr;
    }
    
    LOGD("YOLOv8Engine: 构造函数");
}

YOLOv8Engine::~YOLOv8Engine() {
    LOGD("YOLOv8Engine: 析构函数");
    release();
}

int YOLOv8Engine::initialize(const ModelConfig& config) {
    LOGD("YOLOv8Engine: 开始初始化");
    
    config_ = config;
    
    // 1. 加载RKNN模型
    if (loadModel(config.model_path) != 0) {
        LOGE("YOLOv8Engine: 模型加载失败");
        return -1;
    }
    
    // 2. 初始化输入输出张量
    if (initializeTensors() != 0) {
        LOGE("YOLOv8Engine: 张量初始化失败");
        return -1;
    }
    
    // 3. 计算缩放比例
    scale_w_ = static_cast<float>(config.input_width) / 640.0f;   // 假设原始图像宽度为640
    scale_h_ = static_cast<float>(config.input_height) / 640.0f; // 假设原始图像高度为640
    
    initialized_ = true;
    LOGD("YOLOv8Engine: 初始化成功");
    
    return 0;
}

int YOLOv8Engine::inference(const cv::Mat& input_image, InferenceResultGroup& results) {
    if (!initialized_) {
        LOGE("YOLOv8Engine: 引擎未初始化");
        return -1;
    }
    
    auto start_time = std::chrono::high_resolution_clock::now();
    
    // 1. 预处理
    void* processed_data = nullptr;
    if (preprocessImage(input_image, &processed_data) != 0) {
        LOGE("YOLOv8Engine: 图像预处理失败");
        return -1;
    }
    
    // 2. 执行推理
    rknn_input inputs[1];
    inputs[0].index = 0;
    inputs[0].type = RKNN_TENSOR_UINT8;
    inputs[0].size = input_mems_[0]->size;
    inputs[0].fmt = RKNN_TENSOR_NHWC;
    inputs[0].buf = input_mems_[0]->virt_addr;
    
    int ret = rknn_inputs_set(ctx_, n_input_, inputs);
    if (ret < 0) {
        LOGE("YOLOv8Engine: 设置输入失败: %d", ret);
        return -1;
    }
    
    ret = rknn_run(ctx_, nullptr);
    if (ret < 0) {
        LOGE("YOLOv8Engine: 推理执行失败: %d", ret);
        return -1;
    }
    
    // 3. 获取输出
    rknn_output outputs[3];
    for (int i = 0; i < 3; i++) {
        outputs[i].want_float = 0;
        outputs[i].is_prealloc = 1;
        outputs[i].index = i;
        outputs[i].buf = output_mems_[i]->virt_addr;
        outputs[i].size = output_mems_[i]->size;
    }
    
    ret = rknn_outputs_get(ctx_, n_output_, outputs, nullptr);
    if (ret < 0) {
        LOGE("YOLOv8Engine: 获取输出失败: %d", ret);
        return -1;
    }
    
    // 4. 后处理
    void* output_data[3] = {outputs[0].buf, outputs[1].buf, outputs[2].buf};
    if (postprocessResults(output_data, results) != 0) {
        LOGE("YOLOv8Engine: 后处理失败");
        rknn_outputs_release(ctx_, n_output_, outputs);
        return -1;
    }
    
    rknn_outputs_release(ctx_, n_output_, outputs);
    
    auto end_time = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time);
    
    LOGD("YOLOv8Engine: 推理完成，检测数量: %zu, 耗时: %lld ms",
         results.results.size(), (long long)duration.count());
    
    return 0;
}

void YOLOv8Engine::release() {
    LOGD("YOLOv8Engine: 释放资源");
    
    // 释放输入输出内存
    for (int i = 0; i < 1; i++) {
        if (input_mems_[i]) {
            rknn_destroy_mem(ctx_, input_mems_[i]);
            input_mems_[i] = nullptr;
        }
    }
    
    for (int i = 0; i < 3; i++) {
        if (output_mems_[i]) {
            rknn_destroy_mem(ctx_, output_mems_[i]);
            output_mems_[i] = nullptr;
        }
    }
    
    // 释放RKNN上下文
    if (ctx_ > 0) {
        rknn_destroy(ctx_);
        ctx_ = 0;
    }
    
    // 清理量化参数
    out_scales_.clear();
    out_zps_.clear();
    
    initialized_ = false;
}

bool YOLOv8Engine::isInitialized() const {
    return initialized_;
}

int YOLOv8Engine::loadModel(const std::string& model_path) {
    LOGD("YOLOv8Engine: 加载模型 %s", model_path.c_str());
    
    // 读取模型文件
    std::ifstream file(model_path, std::ios::binary | std::ios::ate);
    if (!file.is_open()) {
        LOGE("YOLOv8Engine: 无法打开模型文件: %s", model_path.c_str());
        return -1;
    }
    
    size_t model_size = file.tellg();
    file.seekg(0, std::ios::beg);
    
    std::vector<char> model_data(model_size);
    if (!file.read(model_data.data(), model_size)) {
        LOGE("YOLOv8Engine: 读取模型文件失败");
        return -1;
    }
    file.close();
    
    // 初始化RKNN
    int ret = rknn_init(&ctx_, model_data.data(), model_size, 0, nullptr);
    if (ret < 0) {
        LOGE("YOLOv8Engine: RKNN初始化失败: %d", ret);
        return -1;
    }
    
    LOGD("YOLOv8Engine: 模型加载成功，大小: %zu bytes", model_size);
    return 0;
}

int YOLOv8Engine::initializeTensors() {
    LOGD("YOLOv8Engine: 初始化张量");
    
    // 获取输入属性
    int ret = rknn_query(ctx_, RKNN_QUERY_INPUT_ATTR, &input_attrs_[0], sizeof(rknn_tensor_attr));
    if (ret < 0) {
        LOGE("YOLOv8Engine: 查询输入属性失败: %d", ret);
        return -1;
    }
    
    // 获取输出属性
    for (int i = 0; i < 3; i++) {
        output_attrs_[i].index = i;
        ret = rknn_query(ctx_, RKNN_QUERY_OUTPUT_ATTR, &output_attrs_[i], sizeof(rknn_tensor_attr));
        if (ret < 0) {
            LOGE("YOLOv8Engine: 查询输出属性失败: %d", ret);
            return -1;
        }
    }
    
    // 创建输入内存
    input_mems_[0] = rknn_create_mem(ctx_, input_attrs_[0].size_with_stride);
    if (!input_mems_[0]) {
        LOGE("YOLOv8Engine: 创建输入内存失败");
        return -1;
    }
    
    // 创建输出内存
    for (int i = 0; i < 3; i++) {
        output_mems_[i] = rknn_create_mem(ctx_, output_attrs_[i].size_with_stride);
        if (!output_mems_[i]) {
            LOGE("YOLOv8Engine: 创建输出内存失败: %d", i);
            return -1;
        }
        
        // 保存量化参数
        out_scales_.push_back(output_attrs_[i].scale);
        out_zps_.push_back(output_attrs_[i].zp);
    }
    
    LOGD("YOLOv8Engine: 张量初始化成功");
    return 0;
}

int YOLOv8Engine::preprocessImage(const cv::Mat& input_image, void** processed_data) {
    // 调整图像大小到模型输入尺寸
    cv::Mat resized_image;
    cv::resize(input_image, resized_image, cv::Size(config_.input_width, config_.input_height));

    // 转换颜色格式 BGR -> RGB
    cv::Mat rgb_image;
    cv::cvtColor(resized_image, rgb_image, cv::COLOR_BGR2RGB);

    // 复制数据到输入内存
    memcpy(input_mems_[0]->virt_addr, rgb_image.data, input_mems_[0]->size);

    // 更新缩放比例
    scale_w_ = static_cast<float>(input_image.cols) / config_.input_width;
    scale_h_ = static_cast<float>(input_image.rows) / config_.input_height;

    *processed_data = input_mems_[0]->virt_addr;
    return 0;
}

int YOLOv8Engine::postprocessResults(void** output_data, InferenceResultGroup& results) {
    // 使用YOLOv8后处理模块
    yolov8_detect_result_group_t yolov8_group;

    int detect_count = yolov8_post_process(
        static_cast<int8_t*>(output_data[0]),
        static_cast<int8_t*>(output_data[1]),
        static_cast<int8_t*>(output_data[2]),
        config_.input_height, config_.input_width,
        config_.conf_threshold, config_.nms_threshold,
        scale_w_, scale_h_,
        out_zps_, out_scales_,
        &yolov8_group
    );

    if (detect_count < 0) {
        LOGE("YOLOv8Engine: 后处理失败");
        return -1;
    }

    // 转换到统一格式
    convertYOLOv8ToUnifiedResults(yolov8_group, results);
    results.model_type = ModelType::YOLOV8N;
    results.timestamp = std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::system_clock::now().time_since_epoch()).count();

    return 0;
}

void YOLOv8Engine::performNMS(const std::vector<cv::Rect2f>& boxes,
                              const std::vector<float>& scores,
                              const std::vector<int>& class_ids,
                              float conf_threshold,
                              float nms_threshold,
                              std::vector<int>& indices) {
    yolov8_nms(boxes, scores, conf_threshold, nms_threshold, indices);
}

float YOLOv8Engine::calculateIoU(const cv::Rect2f& box1, const cv::Rect2f& box2) {
    return yolov8_calculate_iou(box1, box2);
}
