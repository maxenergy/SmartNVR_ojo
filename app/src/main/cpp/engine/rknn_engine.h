// 继承自NNEngine，实现NNEngine的接口

#ifndef RK3588_DEMO_RKNN_ENGINE_H
#define RK3588_DEMO_RKNN_ENGINE_H

#include "engine.h"

#include <vector>

#include <rknn_api.h>

// 继承自NNEngine，实现NNEngine的接口
class RKEngine : public NNEngine
{
public:
    RKEngine() : rknn_ctx_(0), ctx_created_(false), input_num_(0), output_num_(0), npu_core_id_(AllocateNextCore()) {}; // 构造函数，自动分配NPU核心
    ~RKEngine() override;                                                            // 析构函数

    nn_error_e LoadModelData(char *modelData, int dataSize) override;
    nn_error_e LoadModelFile(const char *model_file) override;                                                         // 加载模型文件
    const std::vector<tensor_attr_s> &GetInputShapes() override;                                                       // 获取输入张量的形状
    const std::vector<tensor_attr_s> &GetOutputShapes() override;                                                      // 获取输出张量的形状
    nn_error_e Run(std::vector<tensor_data_s> &inputs, std::vector<tensor_data_s> &outputs, bool want_float) override; // 运行模型

private:
    // rknn context
    rknn_context rknn_ctx_; // rknn context
    bool ctx_created_;      // rknn context是否创建

    uint32_t input_num_;  // 输入的数量
    uint32_t output_num_; // 输出的数量

    std::vector<tensor_attr_s> in_shapes_;  // 输入张量的形状
    std::vector<tensor_attr_s> out_shapes_; // 输出张量的形状

    // NPU多核心支持
    int npu_core_id_;                       // 当前使用的NPU核心ID (0, 1, 2)
    static std::atomic<int> next_core_id_;  // 静态核心分配计数器

public:
    // NPU核心管理方法
    void SetNPUCore(int core_id);           // 设置指定的NPU核心
    int GetNPUCore() const;                 // 获取当前NPU核心ID
    static int AllocateNextCore();          // 自动分配下一个可用核心
};

#endif // RK3588_DEMO_RKNN_ENGINE_H
