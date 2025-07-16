
#ifndef RK3588_DEMO_YOLOV5_THREAD_POOL_H
#define RK3588_DEMO_YOLOV5_THREAD_POOL_H

#include <iostream>
#include <vector>
#include <queue>
#include <map>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <atomic>
#include <chrono>
#include <array>
#include "user_comm.h"
#include "yolov5.h"

// NPU负载均衡器
class NPULoadBalancer {
private:
    std::array<std::atomic<int>, 3> core_loads_;
    std::array<std::chrono::steady_clock::time_point, 3> last_used_;
    std::mutex balancer_mutex_;

public:
    NPULoadBalancer();
    int SelectOptimalCore();
    void TaskCompleted(int core_id);
    void GetCoreStatus(int core_loads[3]);
};

#define MAX_TASK 22

class Yolov5ThreadPool {

private:

    // std::queue <std::pair<int, cv::Mat>> tasks;
    std::vector <std::shared_ptr<Yolov5>> yolov5_instances;
    std::queue<std::shared_ptr<frame_data_t>> tasks;
    std::map<int, std::vector<Detection>> results;
    // std::map<int, cv::Mat> img_results;
    std::map<int, std::shared_ptr<frame_data_t>> img_results;
    std::vector <std::thread> threads;
    std::mutex mtx1;
    std::mutex mtx2;
    std::condition_variable cv_task, cv_result;
    bool stop;

    // NPU负载均衡支持
    std::unique_ptr<NPULoadBalancer> load_balancer_;
    std::vector<int> thread_npu_cores_;  // 每个线程对应的NPU核心ID

    void worker(int id);

public:
    Yolov5ThreadPool();

    ~Yolov5ThreadPool();

    void stopAll(); // 停止所有线程
    nn_error_e setUpWithModelData(int num_threads, char *modelData, int modelSize);
    nn_error_e setUp(std::string &model_path, int num_threads = 12);

    nn_error_e submitTask(const std::shared_ptr<frame_data_t> frameData);

    nn_error_e getTargetResult(std::vector <Detection> &objects, int id);

    nn_error_e getTargetResultNonBlock(std::vector <Detection> &objects, int id);

    std::shared_ptr<frame_data_t>  getTargetImgResult(int id);
    
    int get_task_size() {
        return tasks.size();
    }
};

#endif // RK3588_DEMO_YOLOV5_THREAD_POOL_H