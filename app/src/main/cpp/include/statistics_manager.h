#ifndef STATISTICS_MANAGER_H
#define STATISTICS_MANAGER_H

#include <vector>
#include <map>
#include <mutex>
#include <chrono>
#include "person_detection_types.h"

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

#endif // STATISTICS_MANAGER_H
