
#include "yolov5.h"

#include <memory>

#include "logging.h"
#include "preprocess.h"
#include "yolov5_postprocess.h"
#include "rknn_engine.h"  // 添加RKEngine头文件

#include <ctime>

void DetectionGrp2DetectionArray(yolov5::detect_result_group_t &det_grp, std::vector <Detection> &objects) {
    // 根据当前系统时间生成随机数种子
    std::srand(static_cast<unsigned int>(std::time(nullptr)));

    for (int i = 0; i < det_grp.count; i++) {
        Detection det;
        det.className = det_grp.results[i].name;

        det.box = cv::Rect(det_grp.results[i].box.left,
                           det_grp.results[i].box.top,
                           det_grp.results[i].box.right - det_grp.results[i].box.left,
                           det_grp.results[i].box.bottom - det_grp.results[i].box.top);

        det.confidence = det_grp.results[i].prop;
        // 🔧 修复：使用实际的类别ID，而不是硬编码为0
        det.class_id = det_grp.results[i].id;
        // generate random cv::Scalar color
        // det.color = cv::Scalar(rand() % 255, rand() % 255, rand() % 255);
        // green
        det.color = cv::Scalar(0, 255, 0);
        objects.push_back(det);
    }
}

// 构造函数
Yolov5::Yolov5() {
    engine_ = CreateRKNNEngine();
    input_tensor_.data = nullptr;
}

// 析构函数
Yolov5::~Yolov5() {
    if (input_tensor_.data != nullptr) {
        free(input_tensor_.data);
        input_tensor_.data = nullptr;
    }
    for (auto &tensor: output_tensors_) {
        free(tensor.data);
        tensor.data = nullptr;
    }
}

// 加载模型，获取输入输出属性
nn_error_e Yolov5::LoadModel(const char *model_path) {
    auto ret = engine_->LoadModelFile(model_path);
    if (ret != NN_SUCCESS) {
        NN_LOG_ERROR("yolo load model file failed");
        return ret;
    }
    // get input tensor
    auto input_shapes = engine_->GetInputShapes();

    // check number of input and n_dims
    if (input_shapes.size() != 1) {
        NN_LOG_ERROR("yolo input tensor number is not 1, but %ld", input_shapes.size());
        return NN_RKNN_INPUT_ATTR_ERROR;
    }
    nn_tensor_attr_to_cvimg_input_data(input_shapes[0], input_tensor_);
    input_tensor_.data = malloc(input_tensor_.attr.size);

    auto output_shapes = engine_->GetOutputShapes();

    for (int i = 0; i < output_shapes.size(); i++) {
        tensor_data_s tensor;
        tensor.attr.n_elems = output_shapes[i].n_elems;
        tensor.attr.n_dims = output_shapes[i].n_dims;
        for (int j = 0; j < output_shapes[i].n_dims; j++) {
            tensor.attr.dims[j] = output_shapes[i].dims[j];
        }
        // output tensor needs to be float32
        if (output_shapes[i].type != NN_TENSOR_INT8) {
            NN_LOG_ERROR("yolo output tensor type is not int8, but %d", output_shapes[i].type);
            return NN_RKNN_OUTPUT_ATTR_ERROR;
        }
        tensor.attr.type = output_shapes[i].type;
        tensor.attr.index = i;
        tensor.attr.size = output_shapes[i].n_elems * nn_tensor_type_to_size(tensor.attr.type);
        tensor.data = malloc(tensor.attr.size);
        output_tensors_.push_back(tensor);
        out_zps_.push_back(output_shapes[i].zp);
        out_scales_.push_back(output_shapes[i].scale);
    }
    return NN_SUCCESS;
}



// 加载模型，获取输入输出属性
nn_error_e Yolov5::LoadModelWithData(char *modelData, int modelSize) {
    auto ret = engine_->LoadModelData(modelData, modelSize);
    if (ret != NN_SUCCESS) {
        NN_LOG_ERROR("yolo load model file failed");
        return ret;
    }
    // get input tensor
    auto input_shapes = engine_->GetInputShapes();

    // check number of input and n_dims
    if (input_shapes.size() != 1) {
        NN_LOG_ERROR("yolo input tensor number is not 1, but %ld", input_shapes.size());
        return NN_RKNN_INPUT_ATTR_ERROR;
    }
    nn_tensor_attr_to_cvimg_input_data(input_shapes[0], input_tensor_);
    input_tensor_.data = malloc(input_tensor_.attr.size);

    auto output_shapes = engine_->GetOutputShapes();

    for (int i = 0; i < output_shapes.size(); i++) {
        tensor_data_s tensor;
        tensor.attr.n_elems = output_shapes[i].n_elems;
        tensor.attr.n_dims = output_shapes[i].n_dims;
        for (int j = 0; j < output_shapes[i].n_dims; j++) {
            tensor.attr.dims[j] = output_shapes[i].dims[j];
        }
        // output tensor needs to be float32
        if (output_shapes[i].type != NN_TENSOR_INT8) {
            NN_LOG_ERROR("yolo output tensor type is not int8, but %d", output_shapes[i].type);
            return NN_RKNN_OUTPUT_ATTR_ERROR;
        }
        tensor.attr.type = output_shapes[i].type;
        tensor.attr.index = i;
        tensor.attr.size = output_shapes[i].n_elems * nn_tensor_type_to_size(tensor.attr.type);
        tensor.data = malloc(tensor.attr.size);
        output_tensors_.push_back(tensor);
        out_zps_.push_back(output_shapes[i].zp);
        out_scales_.push_back(output_shapes[i].scale);
    }
    return NN_SUCCESS;
}


// 图像预处理
nn_error_e Yolov5::Preprocess(const cv::Mat &img, const std::string process_type, cv::Mat &image_letterbox) {

    // 预处理包含：letterbox、归一化、BGR2RGB、NCWH
    // 其中RKNN会做：归一化、NCWH转换（详见课程文档），所以这里只需要做letterbox、BGR2RGB

    // 比例
    float wh_ratio = (float) input_tensor_.attr.dims[2] / (float) input_tensor_.attr.dims[1];

    // lettorbox

    if (process_type == "opencv") {
        // BGR2RGB，resize，再放入input_tensor_中
        letterbox_info_ = letterbox(img, image_letterbox, wh_ratio);
        cvimg2tensor(image_letterbox, input_tensor_.attr.dims[2], input_tensor_.attr.dims[1], input_tensor_);
    } else if (process_type == "rga") {
        // rga resize
        letterbox_info_ = letterbox_rga(img, image_letterbox, wh_ratio);
        // save img
        // cv::imwrite("rga.jpg", image_letterbox);
        cvimg2tensor_rga(image_letterbox, input_tensor_.attr.dims[2], input_tensor_.attr.dims[1], input_tensor_);
    }

    return NN_SUCCESS;
}

// 推理
nn_error_e Yolov5::Inference() {
    std::vector <tensor_data_s> inputs;
    // 将input_tensor_放入inputs中
    inputs.push_back(input_tensor_);
    // 运行模型
    engine_->Run(inputs, output_tensors_, false);
    return NN_SUCCESS;
}

// 运行模型
nn_error_e Yolov5::Run(const cv::Mat &img, std::vector <Detection> &objects) {
    // letterbox后的图像
    cv::Mat image_letterbox;
    // 预处理，支持opencv或rga
    Preprocess(img, "opencv", image_letterbox);
    // Preprocess(img, "rga", image_letterbox);
    // 推理
    Inference();
    // 后处理
    Postprocess(image_letterbox, objects);
    return NN_SUCCESS;

}

nn_error_e Yolov5::RunWithFrameData(const std::shared_ptr <frame_data_t> frameData, std::vector <Detection> &objects) {
    // letterbox后的图像
    cv::Mat image_letterbox;
    rga_buffer_t origin;
    struct timeval start, end;
    gettimeofday(&start, NULL);

    int inputWidth = frameData->widthStride;
    int inputHeight = frameData->heightStride;

    // LOGD("RunWithFrameData inputWidth :%d inputHeight:%d", inputWidth, inputHeight);
    // im_rect src_rect;
    // memset(&src_rect, 0, sizeof(src_rect));
    // rga_letter_box(&origin, frameData->data, inputWidth, inputHeight, frameData->frameFormat, &letterbox_info_.hor, &letterbox_info_.pad);

    origin = wrapbuffer_virtualaddr((void *) frameData->data, inputWidth, inputHeight, frameData->frameFormat);
    cv::Mat origin_mat = cv::Mat::zeros(inputHeight, inputWidth, CV_8UC3);
    // 先转成cv matrix
    rga_buffer_t rgb_img = wrapbuffer_virtualaddr((void *) origin_mat.data, inputWidth, inputHeight, RK_FORMAT_RGB_888);
    imcopy(origin, rgb_img);

    // destroy frameData;
    // LOGD("RunWithFrameData Start preprocess");
    // 预处理，支持opencv或rga
    Preprocess(origin_mat, "opencv", image_letterbox);

    // LOGD("RunWithFrameData Start inference");
    // 不可以用rga, 不然直接硬件嗝屁了.
    // Preprocess(origin_mat, "rga", image_letterbox);
    // 推理
    Inference();
    // LOGD("RunWithFrameData Start post-process");
    // 后处理
    Postprocess(image_letterbox, objects);

    // LOGD("RunWithFrameData letterbox_decode");
    // letterbox_decode(objects, letterbox_info_.hor, letterbox_info_.pad);

    // LOGD("RunWithFrameData End post-process");
    gettimeofday(&end, NULL);
    // LOGD("RunWithFrameData time cost: %ld ms", (end.tv_sec - start.tv_sec) * 1000 + (end.tv_usec - start.tv_usec) / 1000);

    return NN_SUCCESS;

}


void letterbox_decode(std::vector <Detection> &objects, bool hor, int pad) {
    for (auto &obj: objects) {
        if (hor) {
            obj.box.x -= pad;
        } else {
            obj.box.y -= pad;
        }
    }
}

// 后处理
nn_error_e Yolov5::Postprocess(const cv::Mat &img, std::vector <Detection> &objects) {
    int height = input_tensor_.attr.dims[1];
    int width = input_tensor_.attr.dims[2];
    float scale_w = height * 1.f / img.cols; // 保证为浮点类型
    float scale_h = width * 1.f / img.rows;

    yolov5::detect_result_group_t detections;

    yolov5::post_process((int8_t *) output_tensors_[0].data,
                         (int8_t *) output_tensors_[1].data,
                         (int8_t *) output_tensors_[2].data,
                         height, width,
                         BOX_THRESH, NMS_THRESH,
                         scale_w, scale_h,
                         out_zps_, out_scales_,
                         &detections);

    DetectionGrp2DetectionArray(detections, objects);
    letterbox_decode(objects, letterbox_info_.hor, letterbox_info_.pad);

    return NN_SUCCESS;
}

// NPU核心管理方法实现
void Yolov5::SetNPUCore(int core_id) {
    if (engine_) {
        // 将engine_转换为RKEngine指针并设置NPU核心
        auto rk_engine = std::dynamic_pointer_cast<RKEngine>(engine_);
        if (rk_engine) {
            rk_engine->SetNPUCore(core_id);
            NN_LOG_INFO("Yolov5: NPU Core set to %d", core_id);
        } else {
            NN_LOG_ERROR("Yolov5: Failed to cast engine to RKEngine");
        }
    } else {
        NN_LOG_ERROR("Yolov5: Engine not initialized, cannot set NPU core");
    }
}

int Yolov5::GetNPUCore() const {
    if (engine_) {
        auto rk_engine = std::dynamic_pointer_cast<RKEngine>(engine_);
        if (rk_engine) {
            return rk_engine->GetNPUCore();
        }
    }
    return -1; // 返回-1表示获取失败
}
