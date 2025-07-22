/**
 * @file person_tracker.h
 * @brief Phase 1: 基于IoU的多目标人员跟踪算法
 * @author AI Assistant
 * @date 2025-07-22
 */

#ifndef PERSON_TRACKER_H
#define PERSON_TRACKER_H

#include <vector>
#include <chrono>
#include <map>
#include <memory>
#include <mutex>
#include <opencv2/opencv.hpp>
#include "person_detection_types.h"
#include "yolo_datatype.h"

// 🔧 Phase 1: 扩展现有的TrackedPerson结构
struct EnhancedTrackedPerson : public TrackedPerson {
    cv::Point2f velocity;                            // 速度向量
    float confidence;                                // 置信度
    
    // 历史轨迹（最近N个位置）
    std::vector<cv::Point2f> trajectory;
    static const size_t MAX_TRAJECTORY_SIZE = 10;

    EnhancedTrackedPerson() : velocity(0, 0), confidence(0.0f) {
        track_id = 0;
        last_box = BoundingBox();
        last_seen = std::chrono::steady_clock::now();
        consecutive_misses = 0;
        is_active = true;
    }

    EnhancedTrackedPerson(int id_, const cv::Rect& box, float conf) :
        velocity(0, 0), confidence(conf) {

        track_id = id_;
        last_box = BoundingBox(box);
        last_seen = std::chrono::steady_clock::now();
        consecutive_misses = 0;
        is_active = true;

        // 添加初始位置到轨迹
        cv::Point2f center(box.x + box.width / 2.0f, box.y + box.height / 2.0f);
        trajectory.push_back(center);
    }
    
    // 更新位置和轨迹
    void updatePosition(const cv::Rect& newBox, float newConf) {
        cv::Rect oldRect = last_box.toRect();
        cv::Point2f oldCenter(oldRect.x + oldRect.width / 2.0f, oldRect.y + oldRect.height / 2.0f);
        cv::Point2f newCenter(newBox.x + newBox.width / 2.0f, newBox.y + newBox.height / 2.0f);

        // 计算速度
        velocity = newCenter - oldCenter;

        // 更新位置和置信度
        last_box = BoundingBox(newBox);
        confidence = newConf;
        last_seen = std::chrono::steady_clock::now();
        consecutive_misses = 0;

        // 更新轨迹
        trajectory.push_back(newCenter);
        if (trajectory.size() > MAX_TRAJECTORY_SIZE) {
            trajectory.erase(trajectory.begin());
        }
    }
    
    // 预测下一个位置
    cv::Rect predictNextPosition() const {
        cv::Rect currentRect = last_box.toRect();
        cv::Point2f predictedCenter(
            currentRect.x + currentRect.width / 2.0f + velocity.x,
            currentRect.y + currentRect.height / 2.0f + velocity.y
        );

        return cv::Rect(
            static_cast<int>(predictedCenter.x - currentRect.width / 2.0f),
            static_cast<int>(predictedCenter.y - currentRect.height / 2.0f),
            currentRect.width,
            currentRect.height
        );
    }
    
    // 获取移动距离
    float getMovementDistance() const {
        return cv::norm(velocity);
    }
    
    // 判断是否移动
    bool isMoving(float threshold = 5.0f) const {
        return getMovementDistance() > threshold;
    }
};

// 🔧 Phase 1: 人员跟踪器
class PersonTracker {
private:
    std::vector<EnhancedTrackedPerson> trackedPersons;
    int nextPersonId;
    int cameraId;
    
    // 跟踪参数
    float iouThreshold;          // IoU阈值
    int maxMissedFrames;         // 最大丢失帧数
    float movementThreshold;     // 移动检测阈值
    
    // 性能统计
    std::chrono::steady_clock::time_point lastUpdateTime;
    int totalTrackedPersons;
    int currentActivePersons;
    
public:
    PersonTracker(int camera_id = 0);
    ~PersonTracker();
    
    // 主要接口
    std::vector<Detection> updateTracking(const std::vector<Detection>& detections);
    
    // 配置接口
    void setIoUThreshold(float threshold) { iouThreshold = threshold; }
    void setMaxMissedFrames(int frames) { maxMissedFrames = frames; }
    void setMovementThreshold(float threshold) { movementThreshold = threshold; }
    
    // 统计接口
    int getActivePersonCount() const { return currentActivePersons; }
    int getTotalTrackedPersons() const { return totalTrackedPersons; }
    std::vector<EnhancedTrackedPerson> getActivePersons() const;
    
    // 事件检测
    std::vector<int> getNewEntries() const;  // 新进入的人员ID
    std::vector<int> getExits() const;       // 离开的人员ID
    
    // 清理和重置
    void cleanupInactivePersons();
    void reset();
    
private:
    // 核心算法
    float calculateIoU(const cv::Rect& box1, const cv::Rect& box2);
    std::vector<std::pair<int, int>> hungarianAssignment(
        const std::vector<std::vector<float>>& costMatrix);
    
    // 辅助函数
    void updatePersonVelocities();
    void detectEnterExitEvents();
    bool isValidDetection(const Detection& detection);
    
    // 性能监控
    void updatePerformanceStats();
};

// 🔧 Phase 1: 全局跟踪器管理
class TrackerManager {
private:
    std::map<int, std::unique_ptr<PersonTracker>> trackers;
    std::mutex trackers_mutex;
    
public:
    PersonTracker* getTracker(int camera_id);
    void removeTracker(int camera_id);
    void resetAllTrackers();
    
    // 统计接口
    int getTotalActivePersons();
    std::map<int, int> getCameraPersonCounts();
};

// 全局跟踪器管理器实例
extern TrackerManager g_tracker_manager;

#endif // PERSON_TRACKER_H
