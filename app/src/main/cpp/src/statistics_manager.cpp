#include "../include/statistics_manager.h"
#include <android/log.h>
#include <algorithm>
#include <sstream>
#include <fstream>
#include <iomanip>

#define TAG "StatisticsManager"
#define LOGD(...) __android_log_print(ANDROID_LOG_DEBUG, TAG, __VA_ARGS__)
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR, TAG, __VA_ARGS__)
#define LOGW(...) __android_log_print(ANDROID_LOG_WARN, TAG, __VA_ARGS__)

// 🔧 Phase 1: 全局统计收集器实例定义
StatisticsCollector g_stats_collector;

// 🔧 Phase 1: StatisticsCollector实现
void StatisticsCollector::updateCameraStats(int camera_id, const EnhancedPersonStatistics& stats) {
    std::lock_guard<std::mutex> lock(stats_mutex);
    camera_stats[camera_id] = stats;
    LOGD("📊 更新Camera %d统计: 当前%d人, 累计%d人次",
         camera_id, stats.current_person_count, stats.total_person_count);
}

EnhancedPersonStatistics StatisticsCollector::getCameraStats(int camera_id) {
    std::lock_guard<std::mutex> lock(stats_mutex);
    auto it = camera_stats.find(camera_id);
    if (it != camera_stats.end()) {
        return it->second;
    }

    // 返回默认统计数据
    EnhancedPersonStatistics default_stats;
    default_stats.camera_id = camera_id;
    return default_stats;
}

std::map<int, EnhancedPersonStatistics> StatisticsCollector::getAllStats() {
    std::lock_guard<std::mutex> lock(stats_mutex);
    return camera_stats;
}

void StatisticsCollector::resetStats() {
    std::lock_guard<std::mutex> lock(stats_mutex);
    camera_stats.clear();
    LOGD("🔄 重置所有统计数据");
}

void StatisticsCollector::recordPerformanceMetric(int camera_id, const std::string& metric, double value) {
    std::lock_guard<std::mutex> lock(stats_mutex);
    auto& stats = camera_stats[camera_id];

    if (metric == "detection_time") {
        // 计算移动平均
        stats.avg_detection_time = (stats.avg_detection_time * stats.frames_processed + value) / (stats.frames_processed + 1);
    } else if (metric == "tracking_time") {
        stats.avg_tracking_time = (stats.avg_tracking_time * stats.frames_processed + value) / (stats.frames_processed + 1);
    }

    stats.frames_processed++;
}

StatisticsManager::StatisticsManager() :
    frame_count_(0), analysis_count_(0), person_count_(0), face_analysis_count_(0),
    male_count_(0), female_count_(0), age_group_0_18_(0), age_group_19_35_(0),
    age_group_36_60_(0), age_group_60_plus_(0) {
    LOGD("🔧 Phase 2: StatisticsManager constructor with enhanced initialization");
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

// 🔧 Phase 1: 增强接口实现
void StatisticsManager::updateEnhancedStatistics(const EnhancedPersonStatistics& stats) {
    try {
        // 更新全局统计收集器
        g_stats_collector.updateCameraStats(stats.camera_id, stats);

        // 转换为原有格式并更新
        PersonStatistics legacy_stats;
        legacy_stats.camera_id = stats.camera_id;
        legacy_stats.person_count = stats.current_person_count;
        legacy_stats.timestamp = stats.last_reset;

        updateStatistics(legacy_stats);

        LOGD("📊 更新增强统计 Camera %d: 当前%d人, 进入%d, 离开%d",
             stats.camera_id, stats.current_person_count, stats.enter_count, stats.exit_count);

    } catch (const std::exception& e) {
        LOGE("❌ 更新增强统计异常: %s", e.what());
    }
}

EnhancedPersonStatistics StatisticsManager::getEnhancedStatistics(int camera_id) {
    try {
        return g_stats_collector.getCameraStats(camera_id);
    } catch (const std::exception& e) {
        LOGE("❌ 获取增强统计异常: %s", e.what());
        EnhancedPersonStatistics default_stats;
        default_stats.camera_id = camera_id;
        return default_stats;
    }
}

void StatisticsManager::recordEnterEvent(int camera_id) {
    try {
        auto stats = g_stats_collector.getCameraStats(camera_id);
        stats.enter_count++;
        g_stats_collector.updateCameraStats(camera_id, stats);

        LOGD("🚪 Camera %d 记录进入事件, 累计进入: %d", camera_id, stats.enter_count);
    } catch (const std::exception& e) {
        LOGE("❌ 记录进入事件异常: %s", e.what());
    }
}

void StatisticsManager::recordExitEvent(int camera_id) {
    try {
        auto stats = g_stats_collector.getCameraStats(camera_id);
        stats.exit_count++;
        g_stats_collector.updateCameraStats(camera_id, stats);

        LOGD("🚪 Camera %d 记录离开事件, 累计离开: %d", camera_id, stats.exit_count);
    } catch (const std::exception& e) {
        LOGE("❌ 记录离开事件异常: %s", e.what());
    }
}

void StatisticsManager::recordPerformanceMetric(int camera_id, const std::string& metric, double value) {
    try {
        g_stats_collector.recordPerformanceMetric(camera_id, metric, value);

        // 每100次记录输出一次性能统计
        static int record_counter = 0;
        if (++record_counter % 100 == 0) {
            auto stats = g_stats_collector.getCameraStats(camera_id);
            LOGD("⚡ Camera %d 性能统计: 检测%.1fms, 跟踪%.1fms, 处理%d帧",
                 camera_id, stats.avg_detection_time, stats.avg_tracking_time, stats.frames_processed);
        }
    } catch (const std::exception& e) {
        LOGE("❌ 记录性能指标异常: %s", e.what());
    }
}

// 🔧 Phase 2: 添加extended_inference_manager.cpp需要的StatisticsManager方法
void StatisticsManager::incrementFrameCount() {
    std::lock_guard<std::mutex> lock(mutex_);
    frame_count_++;
    LOGD("🔧 Phase 2: Frame count incremented to %d", frame_count_);
}

void StatisticsManager::incrementAnalysisCount() {
    std::lock_guard<std::mutex> lock(mutex_);
    analysis_count_++;
    LOGD("🔧 Phase 2: Analysis count incremented to %d", analysis_count_);
}

void StatisticsManager::updateStatistics(const std::vector<FaceAnalysisResult>& faceResults) {
    std::lock_guard<std::mutex> lock(mutex_);

    LOGD("🔧 Phase 2: Updating statistics with %zu face results", faceResults.size());

    // 更新人脸统计
    for (const auto& face : faceResults) {
        if (face.face_detected) {
            face_analysis_count_++;

            // 统计性别
            if (face.gender == 0) {
                male_count_++;
            } else {
                female_count_++;
            }

            // 统计年龄组
            if (face.age <= 18) {
                age_group_0_18_++;
            } else if (face.age <= 35) {
                age_group_19_35_++;
            } else if (face.age <= 60) {
                age_group_36_60_++;
            } else {
                age_group_60_plus_++;
            }
        }
    }

    LOGD("🔧 Phase 2: Statistics updated - faces: %d, male: %d, female: %d",
         face_analysis_count_, male_count_, female_count_);
}

PersonStatistics StatisticsManager::getCurrentStatistics() const {
    std::lock_guard<std::mutex> lock(mutex_);

    PersonStatistics stats = {};
    stats.camera_id = -1; // 全局统计
    stats.person_count = person_count_;
    stats.face_count = face_analysis_count_;
    stats.male_count = male_count_;
    stats.female_count = female_count_;
    stats.age_group_0_18 = age_group_0_18_;
    stats.age_group_19_35 = age_group_19_35_;
    stats.age_group_36_60 = age_group_36_60_;
    stats.age_group_60_plus = age_group_60_plus_;
    stats.timestamp = std::chrono::steady_clock::now();

    LOGD("🔧 Phase 2: Getting current statistics - %d persons, %d faces",
         stats.person_count, stats.face_count);

    return stats;
}

void StatisticsManager::setConfig(const StatisticsConfig& config) {
    std::lock_guard<std::mutex> lock(mutex_);
    config_ = config;
    LOGD("🔧 Phase 2: Statistics config updated");
}

void StatisticsManager::resetCurrentStatistics() {
    std::lock_guard<std::mutex> lock(mutex_);

    frame_count_ = 0;
    analysis_count_ = 0;
    person_count_ = 0;
    face_analysis_count_ = 0;
    male_count_ = 0;
    female_count_ = 0;
    age_group_0_18_ = 0;
    age_group_19_35_ = 0;
    age_group_36_60_ = 0;
    age_group_60_plus_ = 0;

    LOGD("🔧 Phase 2: Current statistics reset");
}

std::string StatisticsManager::exportCurrentStatistics() const {
    std::lock_guard<std::mutex> lock(mutex_);

    std::ostringstream oss;
    oss << "Statistics Export:\n";
    oss << "Frames: " << frame_count_ << "\n";
    oss << "Analysis: " << analysis_count_ << "\n";
    oss << "Persons: " << person_count_ << "\n";
    oss << "Faces: " << face_analysis_count_ << "\n";
    oss << "Male: " << male_count_ << ", Female: " << female_count_ << "\n";
    oss << "Age Groups: 0-18=" << age_group_0_18_ << ", 19-35=" << age_group_19_35_
        << ", 36-60=" << age_group_36_60_ << ", 60+=" << age_group_60_plus_;

    std::string result = oss.str();
    LOGD("🔧 Phase 2: Exporting statistics: %s", result.c_str());

    return result;
}
