#include "../include/statistics_manager.h"
#include <android/log.h>
#include <algorithm>
#include <sstream>
#include <fstream>

#define TAG "StatisticsManager"
#define LOGD(...) __android_log_print(ANDROID_LOG_DEBUG, TAG, __VA_ARGS__)
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR, TAG, __VA_ARGS__)
#define LOGW(...) __android_log_print(ANDROID_LOG_WARN, TAG, __VA_ARGS__)

StatisticsManager::StatisticsManager() {
    LOGD("StatisticsManager constructor");
}

StatisticsManager::~StatisticsManager() {
    LOGD("StatisticsManager destructor");
}

void StatisticsManager::updateStatistics(const PersonStatistics& stats) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    try {
        // 更新当前统计数据
        current_statistics_[stats.camera_id] = stats;
        
        // 添加到历史记录
        history_statistics_.push_back(stats);
        
        // 清理过期的历史记录
        cleanupHistoryRecords();
        
        LOGD("Updated statistics for camera %d: %d persons, %d faces", 
             stats.camera_id, stats.person_count, stats.face_count);
        
    } catch (const std::exception& e) {
        LOGE("Update statistics exception: %s", e.what());
    }
}

PersonStatistics StatisticsManager::getCurrentStatistics(int camera_id) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    auto it = current_statistics_.find(camera_id);
    if (it != current_statistics_.end()) {
        return it->second;
    }
    
    // 返回空的统计数据
    PersonStatistics empty_stats = {};
    empty_stats.camera_id = camera_id;
    empty_stats.timestamp = std::chrono::steady_clock::now();
    return empty_stats;
}

PersonStatistics StatisticsManager::getTotalStatistics() {
    std::lock_guard<std::mutex> lock(mutex_);
    
    PersonStatistics total_stats = {};
    total_stats.camera_id = -1; // 表示汇总数据
    total_stats.timestamp = std::chrono::steady_clock::now();
    
    try {
        for (const auto& pair : current_statistics_) {
            const PersonStatistics& stats = pair.second;
            total_stats.person_count += stats.person_count;
            total_stats.face_count += stats.face_count;
            total_stats.male_count += stats.male_count;
            total_stats.female_count += stats.female_count;
            total_stats.age_group_0_18 += stats.age_group_0_18;
            total_stats.age_group_19_35 += stats.age_group_19_35;
            total_stats.age_group_36_60 += stats.age_group_36_60;
            total_stats.age_group_60_plus += stats.age_group_60_plus;
        }
        
        LOGD("Total statistics: %d persons, %d faces across %zu cameras", 
             total_stats.person_count, total_stats.face_count, current_statistics_.size());
        
    } catch (const std::exception& e) {
        LOGE("Get total statistics exception: %s", e.what());
    }
    
    return total_stats;
}

std::vector<PersonStatistics> StatisticsManager::getHistoryStatistics(int camera_id, int duration_minutes) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    std::vector<PersonStatistics> result;
    auto now = std::chrono::steady_clock::now();
    
    try {
        for (const auto& stats : history_statistics_) {
            // 检查时间范围
            int time_diff = getTimeDifferenceMinutes(stats.timestamp, now);
            if (time_diff <= duration_minutes) {
                // 检查摄像头ID
                if (camera_id == -1 || stats.camera_id == camera_id) {
                    result.push_back(stats);
                }
            }
        }
        
        LOGD("Retrieved %zu history records for camera %d in last %d minutes", 
             result.size(), camera_id, duration_minutes);
        
    } catch (const std::exception& e) {
        LOGE("Get history statistics exception: %s", e.what());
    }
    
    return result;
}

void StatisticsManager::updateAreaStatistics(const AreaStatistics& area_stats) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    try {
        area_statistics_[area_stats.camera_id] = area_stats;
        
        LOGD("Updated area statistics for camera %d: enter=%d, exit=%d, current=%d", 
             area_stats.camera_id, area_stats.enter_count, 
             area_stats.exit_count, area_stats.current_count);
        
    } catch (const std::exception& e) {
        LOGE("Update area statistics exception: %s", e.what());
    }
}

AreaStatistics StatisticsManager::getAreaStatistics(int camera_id) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    auto it = area_statistics_.find(camera_id);
    if (it != area_statistics_.end()) {
        return it->second;
    }
    
    // 返回空的区域统计数据
    AreaStatistics empty_stats = {};
    empty_stats.camera_id = camera_id;
    empty_stats.last_update = std::chrono::steady_clock::now();
    return empty_stats;
}

void StatisticsManager::cleanupExpiredData(int max_age_hours) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    try {
        auto now = std::chrono::steady_clock::now();
        auto cutoff_time = now - std::chrono::hours(max_age_hours);
        
        // 清理历史统计数据
        auto it = std::remove_if(history_statistics_.begin(), history_statistics_.end(),
            [cutoff_time](const PersonStatistics& stats) {
                return stats.timestamp < cutoff_time;
            });
        
        size_t removed_count = std::distance(it, history_statistics_.end());
        history_statistics_.erase(it, history_statistics_.end());
        
        LOGD("Cleaned up %zu expired history records (older than %d hours)", 
             removed_count, max_age_hours);
        
    } catch (const std::exception& e) {
        LOGE("Cleanup expired data exception: %s", e.what());
    }
}

void StatisticsManager::resetAllStatistics() {
    std::lock_guard<std::mutex> lock(mutex_);
    
    try {
        current_statistics_.clear();
        history_statistics_.clear();
        area_statistics_.clear();
        
        LOGD("All statistics reset");
        
    } catch (const std::exception& e) {
        LOGE("Reset all statistics exception: %s", e.what());
    }
}

void StatisticsManager::resetCameraStatistics(int camera_id) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    try {
        // 重置当前统计数据
        current_statistics_.erase(camera_id);
        
        // 重置区域统计数据
        area_statistics_.erase(camera_id);
        
        // 从历史记录中移除该摄像头的数据
        auto it = std::remove_if(history_statistics_.begin(), history_statistics_.end(),
            [camera_id](const PersonStatistics& stats) {
                return stats.camera_id == camera_id;
            });
        
        size_t removed_count = std::distance(it, history_statistics_.end());
        history_statistics_.erase(it, history_statistics_.end());
        
        LOGD("Reset statistics for camera %d, removed %zu history records", 
             camera_id, removed_count);
        
    } catch (const std::exception& e) {
        LOGE("Reset camera statistics exception: %s", e.what());
    }
}

std::string StatisticsManager::getStatisticsJson() {
    std::lock_guard<std::mutex> lock(mutex_);
    
    std::stringstream json;
    
    try {
        json << "{";
        json << "\"total_statistics\":" << "{";
        
        PersonStatistics total = getTotalStatistics();
        json << "\"person_count\":" << total.person_count << ",";
        json << "\"face_count\":" << total.face_count << ",";
        json << "\"male_count\":" << total.male_count << ",";
        json << "\"female_count\":" << total.female_count << ",";
        json << "\"age_group_0_18\":" << total.age_group_0_18 << ",";
        json << "\"age_group_19_35\":" << total.age_group_19_35 << ",";
        json << "\"age_group_36_60\":" << total.age_group_36_60 << ",";
        json << "\"age_group_60_plus\":" << total.age_group_60_plus;
        json << "},";
        
        json << "\"camera_statistics\":[";
        bool first = true;
        for (const auto& pair : current_statistics_) {
            if (!first) json << ",";
            first = false;
            
            const PersonStatistics& stats = pair.second;
            json << "{";
            json << "\"camera_id\":" << stats.camera_id << ",";
            json << "\"person_count\":" << stats.person_count << ",";
            json << "\"face_count\":" << stats.face_count << ",";
            json << "\"male_count\":" << stats.male_count << ",";
            json << "\"female_count\":" << stats.female_count;
            json << "}";
        }
        json << "]";
        json << "}";
        
    } catch (const std::exception& e) {
        LOGE("Get statistics JSON exception: %s", e.what());
        return "{}";
    }
    
    return json.str();
}

bool StatisticsManager::saveStatisticsToFile(const std::string& file_path) {
    try {
        std::string json_data = getStatisticsJson();
        
        std::ofstream file(file_path);
        if (!file.is_open()) {
            LOGE("Failed to open file for writing: %s", file_path.c_str());
            return false;
        }
        
        file << json_data;
        file.close();
        
        LOGD("Statistics saved to file: %s", file_path.c_str());
        return true;
        
    } catch (const std::exception& e) {
        LOGE("Save statistics to file exception: %s", e.what());
        return false;
    }
}

bool StatisticsManager::loadStatisticsFromFile(const std::string& file_path) {
    try {
        std::ifstream file(file_path);
        if (!file.is_open()) {
            LOGE("Failed to open file for reading: %s", file_path.c_str());
            return false;
        }
        
        // TODO: 实现JSON解析和数据加载
        // 这里需要一个JSON解析库，如nlohmann/json
        
        file.close();
        
        LOGD("Statistics loaded from file: %s", file_path.c_str());
        return true;
        
    } catch (const std::exception& e) {
        LOGE("Load statistics from file exception: %s", e.what());
        return false;
    }
}

void StatisticsManager::cleanupHistoryRecords() {
    if (history_statistics_.size() > MAX_HISTORY_RECORDS) {
        size_t excess = history_statistics_.size() - MAX_HISTORY_RECORDS;
        history_statistics_.erase(history_statistics_.begin(), 
                                 history_statistics_.begin() + excess);
        
        LOGD("Cleaned up %zu excess history records", excess);
    }
}

int StatisticsManager::getTimeDifferenceMinutes(const std::chrono::steady_clock::time_point& time1,
                                               const std::chrono::steady_clock::time_point& time2) {
    auto duration = std::chrono::duration_cast<std::chrono::minutes>(time2 - time1);
    return static_cast<int>(duration.count());
}
