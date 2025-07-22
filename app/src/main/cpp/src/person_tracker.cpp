/**
 * @file person_tracker.cpp
 * @brief Phase 1: 基于IoU的多目标人员跟踪算法实现
 * @author AI Assistant
 * @date 2025-07-22
 */

#include "../include/person_tracker.h"
#include <android/log.h>
#include <algorithm>
#include <set>

#define TAG "PersonTracker"
#define LOGD(...) __android_log_print(ANDROID_LOG_DEBUG, TAG, __VA_ARGS__)
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR, TAG, __VA_ARGS__)
#define LOGW(...) __android_log_print(ANDROID_LOG_WARN, TAG, __VA_ARGS__)

// 全局跟踪器管理器实例
TrackerManager g_tracker_manager;

// 🔧 Phase 1: PersonTracker实现
PersonTracker::PersonTracker(int camera_id) : 
    cameraId(camera_id), nextPersonId(1), 
    iouThreshold(0.3f), maxMissedFrames(10), movementThreshold(5.0f),
    lastUpdateTime(std::chrono::steady_clock::now()),
    totalTrackedPersons(0), currentActivePersons(0) {
    
    LOGD("🔧 PersonTracker初始化 Camera %d", camera_id);
}

PersonTracker::~PersonTracker() {
    LOGD("🔧 PersonTracker销毁 Camera %d", cameraId);
}

std::vector<Detection> PersonTracker::updateTracking(const std::vector<Detection>& detections) {
    auto start_time = std::chrono::steady_clock::now();
    
    try {
        std::vector<Detection> trackedDetections;
        
        // 过滤有效检测
        std::vector<Detection> validDetections;
        for (const auto& detection : detections) {
            if (isValidDetection(detection)) {
                validDetections.push_back(detection);
            }
        }
        
        if (validDetections.empty()) {
            // 没有有效检测，更新所有跟踪目标的丢失计数
            for (auto& person : trackedPersons) {
                person.consecutive_misses++;
            }
            cleanupInactivePersons();
            return trackedDetections;
        }
        
        // 1. 计算IoU矩阵
        std::vector<std::vector<float>> iouMatrix(trackedPersons.size(), 
                                                 std::vector<float>(validDetections.size(), 0.0f));
        
        for (size_t i = 0; i < trackedPersons.size(); i++) {
            cv::Rect predictedBox = trackedPersons[i].predictNextPosition();
            for (size_t j = 0; j < validDetections.size(); j++) {
                iouMatrix[i][j] = calculateIoU(predictedBox, validDetections[j].box);
            }
        }
        
        // 2. 匈牙利算法匹配（简化版本）
        auto assignments = hungarianAssignment(iouMatrix);
        
        // 3. 更新已匹配的跟踪目标
        std::set<int> matchedDetections;
        for (const auto& assignment : assignments) {
            int trackerIdx = assignment.first;
            int detectionIdx = assignment.second;
            
            if (trackerIdx >= 0 && detectionIdx >= 0 && 
                iouMatrix[trackerIdx][detectionIdx] > iouThreshold) {
                
                // 更新跟踪目标
                trackedPersons[trackerIdx].updatePosition(validDetections[detectionIdx].box, 
                                                         validDetections[detectionIdx].confidence);
                
                // 创建跟踪检测结果
                Detection trackedDetection = validDetections[detectionIdx];
                trackedDetection.class_id = trackedPersons[trackerIdx].track_id;
                trackedDetections.push_back(trackedDetection);
                
                matchedDetections.insert(detectionIdx);
                
                LOGD("🔄 Camera %d 更新跟踪ID %d, IoU=%.2f",
                     cameraId, trackedPersons[trackerIdx].track_id, iouMatrix[trackerIdx][detectionIdx]);
            } else {
                // 未匹配成功，增加丢失计数
                trackedPersons[trackerIdx].consecutive_misses++;
            }
        }
        
        // 4. 为未匹配的检测创建新跟踪目标
        for (size_t j = 0; j < validDetections.size(); j++) {
            if (matchedDetections.find(j) == matchedDetections.end()) {
                EnhancedTrackedPerson newPerson(nextPersonId++, validDetections[j].box, validDetections[j].confidence);
                trackedPersons.push_back(newPerson);
                totalTrackedPersons++;

                Detection trackedDetection = validDetections[j];
                trackedDetection.class_id = newPerson.track_id;
                trackedDetections.push_back(trackedDetection);

                cv::Rect lastRect = newPerson.last_box.toRect();
                LOGD("🆕 Camera %d 新建跟踪ID %d, 位置[%d,%d,%d,%d]",
                     cameraId, newPerson.track_id,
                     lastRect.x, lastRect.y, lastRect.width, lastRect.height);
            }
        }
        
        // 5. 清理不活跃的跟踪目标
        cleanupInactivePersons();
        
        // 6. 更新统计信息
        currentActivePersons = 0;
        for (const auto& person : trackedPersons) {
            if (person.is_active) {
                currentActivePersons++;
            }
        }
        
        updatePerformanceStats();
        
        auto end_time = std::chrono::steady_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time);
        
        // 每50次更新输出一次跟踪统计
        static int update_counter = 0;
        if (++update_counter % 50 == 0) {
            LOGD("📊 Camera %d 跟踪统计: %d活跃, %d总计, 耗时%lldms",
                 cameraId, currentActivePersons, totalTrackedPersons, duration.count());
        }
        
        return trackedDetections;
        
    } catch (const std::exception& e) {
        LOGE("❌ Camera %d 跟踪更新异常: %s", cameraId, e.what());
        return std::vector<Detection>();
    }
}

float PersonTracker::calculateIoU(const cv::Rect& box1, const cv::Rect& box2) {
    cv::Rect intersection = box1 & box2;
    float intersectionArea = static_cast<float>(intersection.area());
    float unionArea = static_cast<float>(box1.area() + box2.area()) - intersectionArea;
    
    return unionArea > 0 ? intersectionArea / unionArea : 0.0f;
}

std::vector<std::pair<int, int>> PersonTracker::hungarianAssignment(
    const std::vector<std::vector<float>>& costMatrix) {
    
    std::vector<std::pair<int, int>> assignments;
    
    // 简化的贪心匹配算法（替代完整的匈牙利算法）
    std::set<int> usedTrackers;
    std::set<int> usedDetections;
    
    // 按IoU值从大到小排序进行匹配
    std::vector<std::tuple<float, int, int>> candidates;
    for (size_t i = 0; i < costMatrix.size(); i++) {
        for (size_t j = 0; j < costMatrix[i].size(); j++) {
            candidates.emplace_back(costMatrix[i][j], i, j);
        }
    }
    
    std::sort(candidates.begin(), candidates.end(),
              [](const std::tuple<float, int, int>& a, const std::tuple<float, int, int>& b) {
                  return std::get<0>(a) > std::get<0>(b);
              });
    
    for (const auto& candidate : candidates) {
        float iou = std::get<0>(candidate);
        int trackerIdx = std::get<1>(candidate);
        int detectionIdx = std::get<2>(candidate);
        
        if (iou > iouThreshold && 
            usedTrackers.find(trackerIdx) == usedTrackers.end() &&
            usedDetections.find(detectionIdx) == usedDetections.end()) {
            
            assignments.emplace_back(trackerIdx, detectionIdx);
            usedTrackers.insert(trackerIdx);
            usedDetections.insert(detectionIdx);
        }
    }
    
    // 为未匹配的跟踪器添加-1匹配
    for (size_t i = 0; i < costMatrix.size(); i++) {
        if (usedTrackers.find(i) == usedTrackers.end()) {
            assignments.emplace_back(i, -1);
        }
    }
    
    return assignments;
}

void PersonTracker::cleanupInactivePersons() {
    auto it = trackedPersons.begin();
    while (it != trackedPersons.end()) {
        if (it->consecutive_misses > maxMissedFrames) {
            LOGD("🗑️ Camera %d 清理跟踪ID %d (丢失%d帧)",
                 cameraId, it->track_id, it->consecutive_misses);
            it = trackedPersons.erase(it);
        } else {
            ++it;
        }
    }
}

bool PersonTracker::isValidDetection(const Detection& detection) {
    // 检查置信度
    if (detection.confidence < 0.3f) {
        return false;
    }
    
    // 检查边界框大小
    if (detection.box.width < 20 || detection.box.height < 30) {
        return false;
    }
    
    // 检查类别
    if (detection.className != "person") {
        return false;
    }
    
    return true;
}

void PersonTracker::updatePerformanceStats() {
    lastUpdateTime = std::chrono::steady_clock::now();
}

std::vector<EnhancedTrackedPerson> PersonTracker::getActivePersons() const {
    std::vector<EnhancedTrackedPerson> activePersons;
    for (const auto& person : trackedPersons) {
        if (person.is_active && person.consecutive_misses == 0) {
            activePersons.push_back(person);
        }
    }
    return activePersons;
}

void PersonTracker::reset() {
    trackedPersons.clear();
    nextPersonId = 1;
    totalTrackedPersons = 0;
    currentActivePersons = 0;
    LOGD("🔄 Camera %d 跟踪器重置", cameraId);
}

// 🔧 Phase 1: TrackerManager实现
PersonTracker* TrackerManager::getTracker(int camera_id) {
    std::lock_guard<std::mutex> lock(trackers_mutex);
    
    auto it = trackers.find(camera_id);
    if (it == trackers.end()) {
        trackers[camera_id] = std::unique_ptr<PersonTracker>(new PersonTracker(camera_id));
        LOGD("🔧 创建新跟踪器 Camera %d", camera_id);
    }
    
    return trackers[camera_id].get();
}

void TrackerManager::removeTracker(int camera_id) {
    std::lock_guard<std::mutex> lock(trackers_mutex);
    trackers.erase(camera_id);
    LOGD("🗑️ 移除跟踪器 Camera %d", camera_id);
}

void TrackerManager::resetAllTrackers() {
    std::lock_guard<std::mutex> lock(trackers_mutex);
    for (auto& pair : trackers) {
        pair.second->reset();
    }
    LOGD("🔄 重置所有跟踪器");
}

int TrackerManager::getTotalActivePersons() {
    std::lock_guard<std::mutex> lock(trackers_mutex);
    int total = 0;
    for (const auto& pair : trackers) {
        total += pair.second->getActivePersonCount();
    }
    return total;
}

std::map<int, int> TrackerManager::getCameraPersonCounts() {
    std::lock_guard<std::mutex> lock(trackers_mutex);
    std::map<int, int> counts;
    for (const auto& pair : trackers) {
        counts[pair.first] = pair.second->getActivePersonCount();
    }
    return counts;
}
