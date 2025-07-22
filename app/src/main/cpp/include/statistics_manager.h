#ifndef STATISTICS_MANAGER_H
#define STATISTICS_MANAGER_H

#include <vector>
#include <map>
#include <mutex>
#include <chrono>
#include "person_detection_types.h"

// 🔧 Phase 1: 增强的统计数据结构
struct EnhancedPersonStatistics {
    // 基础统计
    int camera_id;
    int current_person_count;
    int total_person_count;

    // 区域统计
    int enter_count;
    int exit_count;

    // 时间序列数据
    std::vector<int> hourly_counts;
    std::chrono::steady_clock::time_point last_reset;

    // 性能指标
    double avg_detection_time;
    double avg_tracking_time;
    int frames_processed;
    int frames_skipped;

    // 构造函数
    EnhancedPersonStatistics() :
        camera_id(0), current_person_count(0), total_person_count(0),
        enter_count(0), exit_count(0), hourly_counts(24, 0),
        last_reset(std::chrono::steady_clock::now()),
        avg_detection_time(0.0), avg_tracking_time(0.0),
        frames_processed(0), frames_skipped(0) {}
};

// 🔧 Phase 1: 全局统计收集器
class StatisticsCollector {
private:
    std::mutex stats_mutex;
    std::map<int, EnhancedPersonStatistics> camera_stats;

public:
    void updateCameraStats(int camera_id, const EnhancedPersonStatistics& stats);
    EnhancedPersonStatistics getCameraStats(int camera_id);
    std::map<int, EnhancedPersonStatistics> getAllStats();
    void resetStats();
    void recordPerformanceMetric(int camera_id, const std::string& metric, double value);
};

/**
 * @brief 统计管理器
 * 负责管理人员统计数据，包括计数、年龄分布、性别分布等
 */
class StatisticsManager {
public:
    StatisticsManager();
    ~StatisticsManager();

    /**
     * @brief 更新统计数据
     * @param stats 新的统计数据
     */
    void updateStatistics(const PersonStatistics& stats);

    /**
     * @brief 获取指定摄像头的当前统计数据
     * @param camera_id 摄像头ID
     * @return 统计数据
     */
    PersonStatistics getCurrentStatistics(int camera_id);

    /**
     * @brief 获取所有摄像头的汇总统计数据
     * @return 汇总统计数据
     */
    PersonStatistics getTotalStatistics();

    /**
     * @brief 获取历史统计数据
     * @param camera_id 摄像头ID，-1表示所有摄像头
     * @param duration_minutes 历史时长（分钟）
     * @return 历史统计数据列表
     */
    std::vector<PersonStatistics> getHistoryStatistics(int camera_id, int duration_minutes);

    /**
     * @brief 更新区域进入/离开统计
     * @param area_stats 区域统计数据
     */
    void updateAreaStatistics(const AreaStatistics& area_stats);

    /**
     * @brief 获取区域统计数据
     * @param camera_id 摄像头ID
     * @return 区域统计数据
     */
    AreaStatistics getAreaStatistics(int camera_id);

    /**
     * @brief 清理过期的统计数据
     * @param max_age_hours 最大保留时长（小时）
     */
    void cleanupExpiredData(int max_age_hours = 24);

    /**
     * @brief 重置所有统计数据
     */
    void resetAllStatistics();

    /**
     * @brief 重置指定摄像头的统计数据
     * @param camera_id 摄像头ID
     */
    void resetCameraStatistics(int camera_id);

    /**
     * @brief 获取统计数据的JSON格式字符串
     * @return JSON字符串
     */
    std::string getStatisticsJson();

    /**
     * @brief 保存统计数据到文件
     * @param file_path 文件路径
     * @return true成功，false失败
     */
    bool saveStatisticsToFile(const std::string& file_path);

    /**
     * @brief 从文件加载统计数据
     * @param file_path 文件路径
     * @return true成功，false失败
     */
    bool loadStatisticsFromFile(const std::string& file_path);

    // 🔧 Phase 1: 增强接口
    /**
     * @brief 更新增强统计数据
     * @param stats 增强统计数据
     */
    void updateEnhancedStatistics(const EnhancedPersonStatistics& stats);

    /**
     * @brief 获取增强统计数据
     * @param camera_id 摄像头ID
     * @return 增强统计数据
     */
    EnhancedPersonStatistics getEnhancedStatistics(int camera_id);

    /**
     * @brief 记录进入事件
     * @param camera_id 摄像头ID
     */
    void recordEnterEvent(int camera_id);

    /**
     * @brief 记录离开事件
     * @param camera_id 摄像头ID
     */
    void recordExitEvent(int camera_id);

    /**
     * @brief 记录性能指标
     * @param camera_id 摄像头ID
     * @param metric 指标名称
     * @param value 指标值
     */
    void recordPerformanceMetric(int camera_id, const std::string& metric, double value);

private:
    std::mutex mutex_;
    
    // 当前统计数据（按摄像头ID索引）
    std::map<int, PersonStatistics> current_statistics_;
    
    // 历史统计数据
    std::vector<PersonStatistics> history_statistics_;
    
    // 区域统计数据
    std::map<int, AreaStatistics> area_statistics_;
    
    // 最大历史记录数量
    static const size_t MAX_HISTORY_RECORDS = 10000;
    
    /**
     * @brief 清理过期的历史记录
     */
    void cleanupHistoryRecords();
    
    /**
     * @brief 计算时间差（分钟）
     * @param time1 时间点1
     * @param time2 时间点2
     * @return 时间差（分钟）
     */
    int getTimeDifferenceMinutes(const std::chrono::steady_clock::time_point& time1,
                                const std::chrono::steady_clock::time_point& time2);
};

// 🔧 Phase 1: 全局统计收集器实例声明
extern StatisticsCollector g_stats_collector;

#endif // STATISTICS_MANAGER_H
