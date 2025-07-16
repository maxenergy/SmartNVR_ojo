
#include "yolov5_thread_pool.h"
#include "cv_draw.h"
#include "sys/time.h"

// NPULoadBalancer实现
NPULoadBalancer::NPULoadBalancer() {
    auto now = std::chrono::steady_clock::now();
    for (int i = 0; i < 3; i++) {
        core_loads_[i].store(0);
        last_used_[i] = now;
    }
}

int NPULoadBalancer::SelectOptimalCore() {
    std::lock_guard<std::mutex> lock(balancer_mutex_);

    int min_load = core_loads_[0].load();
    int selected_core = 0;

    // 选择负载最小的核心
    for (int i = 1; i < 3; i++) {
        int current_load = core_loads_[i].load();
        if (current_load < min_load) {
            min_load = current_load;
            selected_core = i;
        }
    }

    // 更新负载计数和使用时间
    core_loads_[selected_core]++;
    last_used_[selected_core] = std::chrono::steady_clock::now();

    LOGD("NPU Load Balancer: Selected Core %d (load: %d)", selected_core, min_load + 1);
    return selected_core;
}

void NPULoadBalancer::TaskCompleted(int core_id) {
    if (core_id >= 0 && core_id < 3) {
        core_loads_[core_id]--;
        LOGD("NPU Load Balancer: Core %d task completed (load: %d)", core_id, core_loads_[core_id].load());
    }
}

void NPULoadBalancer::GetCoreStatus(int core_loads[3]) {
    for (int i = 0; i < 3; i++) {
        core_loads[i] = core_loads_[i].load();
    }
}

void Yolov5ThreadPool::worker(int id) {
    while (!stop) {
        // std::pair<int, cv::Mat> task;
        std::shared_ptr<frame_data_t> taskFrameData;
        std::shared_ptr<Yolov5> instance = yolov5_instances[id];
        {
            std::unique_lock<std::mutex> lock(mtx1);
            cv_task.wait(lock, [&] { return !tasks.empty() || stop; });

            if (stop) {
                return;
            }

            taskFrameData = tasks.front();
            tasks.pop();
        }

        std::vector<Detection> detections;
        struct timeval start, end;
        int npu_core = thread_npu_cores_[id];

        gettimeofday(&start, NULL);
        instance->RunWithFrameData(taskFrameData, detections);
        gettimeofday(&end, NULL);

        float time_use = (end.tv_sec - start.tv_sec) * 1000 + (end.tv_usec - start.tv_usec) / 1000;
        LOGD("thread %d (NPU Core %d), time_use: %f ms", id, npu_core, time_use);

        // 通知负载均衡器任务完成
        if (load_balancer_) {
            load_balancer_->TaskCompleted(npu_core);
        }
        {
            std::lock_guard<std::mutex> lock(mtx2);
            results.insert({taskFrameData->frameId, detections});
            // DrawDetections(task.second, detections);
            // img_results.insert({task.first, task.second});
            img_results.insert({taskFrameData->frameId, taskFrameData});
            // cv_result.notify_one();
        }
    }
}


nn_error_e Yolov5ThreadPool::setUpWithModelData(int num_threads, char *modelData, int modelSize) {
    // 初始化负载均衡器
    load_balancer_.reset(new NPULoadBalancer());
    thread_npu_cores_.resize(num_threads);

    // 遍历线程数量，创建模型实例，均匀分配到3个NPU核心
    for (size_t i = 0; i < num_threads; ++i) {
        std::shared_ptr<Yolov5> yolov5 = std::make_shared<Yolov5>();

        // 为每个实例分配NPU核心（轮询分配）
        int assigned_core = i % 3;
        thread_npu_cores_[i] = assigned_core;

        // 设置NPU核心后加载模型
        yolov5->SetNPUCore(assigned_core);
        yolov5->LoadModelWithData(modelData, modelSize);
        yolov5_instances.push_back(yolov5);

        LOGD("Thread %zu assigned to NPU Core %d", i, assigned_core);
        usleep(1000);
    }

    // 遍历线程数量，创建线程
    for (size_t i = 0; i < num_threads; ++i) {
        threads.emplace_back(&Yolov5ThreadPool::worker, this, i);
    }

    LOGD("YOLOv5 ThreadPool initialized with %d threads across 3 NPU cores", num_threads);
    return NN_SUCCESS;
}


nn_error_e Yolov5ThreadPool::setUp(std::string &model_path, int num_threads) {
    for (size_t i = 0; i < num_threads; ++i) {
        std::shared_ptr<Yolov5> yolov5 = std::make_shared<Yolov5>();
        yolov5->LoadModel(model_path.c_str());
        yolov5_instances.push_back(yolov5);
    }
    for (size_t i = 0; i < num_threads; ++i) {
        threads.emplace_back(&Yolov5ThreadPool::worker, this, i);
    }
    return NN_SUCCESS;
}

Yolov5ThreadPool::Yolov5ThreadPool() { stop = false; }

Yolov5ThreadPool::~Yolov5ThreadPool() {
    stop = true;
    cv_task.notify_all();
    for (auto &thread: threads) {
        if (thread.joinable()) {
            thread.join();
        }
    }
}

nn_error_e Yolov5ThreadPool::submitTask(const std::shared_ptr<frame_data_t> frameData) {
    while (tasks.size() > MAX_TASK) {
        // sleep 1ms
        LOGD("mpp_decoder_frame_callback waiting");
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }
    {
        std::lock_guard<std::mutex> lock(mtx1);
        LOGD("Submit task %d", frameData->frameId);
        tasks.push(frameData);
        // tasks.push({id, img});
    }
    cv_task.notify_one();
    return NN_SUCCESS;
}

nn_error_e Yolov5ThreadPool::getTargetResult(std::vector<Detection> &objects, int id) {
    while (results.find(id) == results.end()) {
        // sleep 1ms
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }
    std::lock_guard<std::mutex> lock(mtx2);
    objects = results[id];
    // remove from map
    results.erase(id);
    img_results.erase(id);

    return NN_SUCCESS;
}

nn_error_e Yolov5ThreadPool::getTargetResultNonBlock(std::vector<Detection> &objects, int id) {
    if (results.find(id) == results.end()) {
        return NN_RESULT_NOT_READY;
    }
    std::lock_guard<std::mutex> lock(mtx2);
    objects = results[id];
    // remove from map
    results.erase(id);
    // img_results.erase(id);

    return NN_SUCCESS;
}

std::shared_ptr<frame_data_t> Yolov5ThreadPool::getTargetImgResult(int id) {
    int loop_cnt = 0;
    //    while (img_results.find(id) == img_results.end()) {
    //        // sleep 1ms
    //        std::this_thread::sleep_for(std::chrono::milliseconds(5));
    //        loop_cnt++;
    //        if (loop_cnt > 1000) {
    //            NN_LOG_ERROR("getTargetImgResult timeout");
    //            return nullptr;
    //        }
    //    }
    std::lock_guard<std::mutex> lock(mtx2);
    auto frameData = img_results[id];
    // img = img_results[id];
    // remove from map
    // img_results.erase(id);
    return frameData;
    // results.erase(id);
    // return ;
}

// 停止所有线程
void Yolov5ThreadPool::stopAll() {
    stop = true;
    cv_task.notify_all();
}
