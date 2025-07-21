#ifndef STATISTICS_MANAGER_H
#define STATISTICS_MANAGER_H

#include <array>
#include <chrono>
#include <mutex>
#include <vector>
#include <string>
#include <map>
#include "face/face_analysis_manager.h"

// 🔧 人员跟踪相关数据结构
struct PersonTrackingInfo {
    int personId;                                    // 人员ID
    cv::Rect lastBoundingBox;                       // 最后检测到的边界框
    std::chrono::steady_clock::time_point firstSeen; // 首次检测时间
    std::chrono::steady_clock::time_point lastSeen;  // 最后检测时间
    
    // 人脸属性信息（用于统计）
    int gender = -1;                                 // 性别 (0=女, 1=男, -1=未知)
    int ageBracket = -1;                            // 年龄段 (0-8)
    int race = -1;                                  // 种族 (0-4)
    float confidence = 0.0f;                        // 置信度
    
    // 跟踪状态
    bool isActive = true;                           // 是否活跃
    int consecutiveMisses = 0;                      // 连续未检测到的次数
    
    PersonTrackingInfo(int id, const cv::Rect& bbox) 
        : personId(id), lastBoundingBox(bbox) {
        auto now = std::chrono::steady_clock::now();
        firstSeen = lastSeen = now;
    }
    
    // 更新跟踪信息
    void updateTracking(const cv::Rect& bbox) {
        lastBoundingBox = bbox;
        lastSeen = std::chrono::steady_clock::now();
        consecutiveMisses = 0;
        isActive = true;
    }
    
    // 标记为未检测到
    void markMissed() {
        consecutiveMisses++;
        if (consecutiveMisses > 5) { // 连续5次未检测到则标记为非活跃
            isActive = false;
        }
    }
    
    // 获取存在时长（秒）
    double getLifetimeSeconds() const {
        auto duration = lastSeen - firstSeen;
        return std::chrono::duration<double>(duration).count();
    }
    
    // 检查是否应该被清理（超过时间窗口）
    bool shouldBeRemoved(std::chrono::seconds timeWindow) const {
        auto now = std::chrono::steady_clock::now();
        auto timeSinceLastSeen = now - lastSeen;
        return timeSinceLastSeen > timeWindow;
    }
};

// 时间窗口统计配置
struct TimeWindowConfig {
    std::chrono::seconds personTrackingWindow{30};   // 人员跟踪时间窗口
    std::chrono::seconds statisticsWindow{60};       // 统计时间窗口
    float overlapThreshold = 0.3f;                   // 边界框重叠阈值
    int maxTrackedPersons = 50;                      // 最大跟踪人员数
    int missThreshold = 5;                           // 连续未检测阈值
    
    bool isValid() const {
        return personTrackingWindow.count() > 0 && 
               statisticsWindow.count() > 0 &&
               overlapThreshold > 0.0f && overlapThreshold < 1.0f &&
               maxTrackedPersons > 0 && missThreshold > 0;
    }
};

// 统计数据结构
struct StatisticsData {
    // 基础统计
    int totalPersonCount = 0;
    int totalFaceCount = 0;
    int validFaceCount = 0;             // 有效人脸数量 (有属性信息)
    
    // 性别统计
    int maleCount = 0;
    int femaleCount = 0;
    int unknownGenderCount = 0;
    
    // 年龄统计 (9个年龄段)
    std::array<int, 9> ageBracketCounts = {0};
    
    // 种族统计 (5种)
    std::array<int, 5> raceCounts = {0};
    
    // 时间信息
    std::chrono::steady_clock::time_point startTime;
    std::chrono::steady_clock::time_point lastUpdateTime;
    
    // 统计周期信息
    int frameCount = 0;                 // 处理的帧数
    int analysisCount = 0;              // 执行人脸分析的次数
    
    void reset() {
        totalPersonCount = 0;
        totalFaceCount = 0;
        validFaceCount = 0;
        maleCount = femaleCount = unknownGenderCount = 0;
        ageBracketCounts.fill(0);
        raceCounts.fill(0);
        frameCount = 0;
        analysisCount = 0;
        startTime = lastUpdateTime = std::chrono::steady_clock::now();
    }
    
    // 获取统计持续时间 (秒)
    double getDurationSeconds() const {
        auto now = std::chrono::steady_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::seconds>(now - startTime);
        return duration.count();
    }
    
    // 获取性别分布百分比
    std::pair<double, double> getGenderPercentage() const {
        int total = maleCount + femaleCount;
        if (total == 0) return {0.0, 0.0};
        
        double malePercent = static_cast<double>(maleCount) / total * 100.0;
        double femalePercent = static_cast<double>(femaleCount) / total * 100.0;
        return {malePercent, femalePercent};
    }
    
    // 获取年龄分布百分比
    std::vector<double> getAgeBracketPercentage() const {
        std::vector<double> percentages(9, 0.0);
        int total = validFaceCount;
        if (total == 0) return percentages;
        
        for (int i = 0; i < 9; i++) {
            percentages[i] = static_cast<double>(ageBracketCounts[i]) / total * 100.0;
        }
        return percentages;
    }
    
    // 获取主要年龄段
    int getDominantAgeBracket() const {
        auto maxIt = std::max_element(ageBracketCounts.begin(), ageBracketCounts.end());
        if (*maxIt == 0) return -1;
        return static_cast<int>(maxIt - ageBracketCounts.begin());
    }
    
    // 格式化输出
    std::string toString() const {
        std::string result;
        result += "=== 人员统计报告 ===\n";
        result += "统计时长: " + std::to_string(static_cast<int>(getDurationSeconds())) + "秒\n";
        result += "处理帧数: " + std::to_string(frameCount) + "\n";
        result += "分析次数: " + std::to_string(analysisCount) + "\n\n";
        
        result += "人员检测:\n";
        result += "  总人数: " + std::to_string(totalPersonCount) + "\n";
        result += "  检测到人脸: " + std::to_string(totalFaceCount) + "\n";
        result += "  有效人脸: " + std::to_string(validFaceCount) + "\n\n";
        
        if (validFaceCount > 0) {
            auto genderPercent = getGenderPercentage();
            result += "性别分布:\n";
            result += "  男性: " + std::to_string(maleCount) + 
                     " (" + std::to_string(static_cast<int>(genderPercent.first)) + "%)\n";
            result += "  女性: " + std::to_string(femaleCount) + 
                     " (" + std::to_string(static_cast<int>(genderPercent.second)) + "%)\n";
            result += "  未知: " + std::to_string(unknownGenderCount) + "\n\n";
            
            result += "年龄分布:\n";
            const char* ageLabels[] = {
                "0-2岁", "3-9岁", "10-19岁", "20-29岁", "30-39岁",
                "40-49岁", "50-59岁", "60-69岁", "70岁以上"
            };
            auto agePercent = getAgeBracketPercentage();
            for (int i = 0; i < 9; i++) {
                if (ageBracketCounts[i] > 0) {
                    result += "  " + std::string(ageLabels[i]) + ": " + 
                             std::to_string(ageBracketCounts[i]) + 
                             " (" + std::to_string(static_cast<int>(agePercent[i])) + "%)\n";
                }
            }
        }
        
        return result;
    }
};

// 历史统计数据
struct HistoricalStatistics {
    std::vector<StatisticsData> snapshots;
    std::chrono::seconds snapshotInterval{60};  // 快照间隔
    size_t maxSnapshots = 60;                   // 最大保存快照数
    
    void addSnapshot(const StatisticsData& data) {
        snapshots.push_back(data);
        if (snapshots.size() > maxSnapshots) {
            snapshots.erase(snapshots.begin());
        }
    }
    
    StatisticsData getAverageStatistics() const {
        if (snapshots.empty()) return StatisticsData();
        
        StatisticsData avg;
        for (const auto& snapshot : snapshots) {
            avg.totalPersonCount += snapshot.totalPersonCount;
            avg.totalFaceCount += snapshot.totalFaceCount;
            avg.validFaceCount += snapshot.validFaceCount;
            avg.maleCount += snapshot.maleCount;
            avg.femaleCount += snapshot.femaleCount;
            avg.unknownGenderCount += snapshot.unknownGenderCount;
            
            for (int i = 0; i < 9; i++) {
                avg.ageBracketCounts[i] += snapshot.ageBracketCounts[i];
            }
            for (int i = 0; i < 5; i++) {
                avg.raceCounts[i] += snapshot.raceCounts[i];
            }
        }
        
        // 计算平均值
        size_t count = snapshots.size();
        avg.totalPersonCount /= count;
        avg.totalFaceCount /= count;
        avg.validFaceCount /= count;
        avg.maleCount /= count;
        avg.femaleCount /= count;
        avg.unknownGenderCount /= count;
        
        for (int i = 0; i < 9; i++) {
            avg.ageBracketCounts[i] /= count;
        }
        for (int i = 0; i < 5; i++) {
            avg.raceCounts[i] /= count;
        }
        
        return avg;
    }
    
    void clear() {
        snapshots.clear();
    }
};

// 统计配置
struct StatisticsConfig {
    bool enableRealTimeStats = true;
    bool enableHistoricalStats = true;
    bool enablePerformanceStats = true;
    
    std::chrono::seconds resetInterval{300};        // 自动重置间隔 (5分钟)
    std::chrono::seconds snapshotInterval{60};      // 历史快照间隔 (1分钟)
    size_t maxHistoricalSnapshots = 60;             // 最大历史快照数
    
    // 统计阈值
    float minConfidenceForStats = 0.3f;             // 最小置信度
    int minFaceSizeForStats = 20;                   // 最小人脸尺寸
    
    bool isValid() const {
        return resetInterval.count() > 0 && 
               snapshotInterval.count() > 0 &&
               maxHistoricalSnapshots > 0;
    }
};

// 统计数据管理器
class StatisticsManager {
public:
    // 性能统计结构
    struct PerformanceMetrics {
        int updateCount = 0;
        std::chrono::milliseconds totalUpdateTime{0};
        std::chrono::steady_clock::time_point lastUpdateTime;

        double getAverageUpdateTime() const {
            if (updateCount == 0) return 0.0;
            return static_cast<double>(totalUpdateTime.count()) / updateCount;
        }
    };

private:
    StatisticsData m_currentStats;
    HistoricalStatistics m_historicalStats;
    StatisticsConfig m_config;
    
    // 🔧 人员跟踪相关成员
    TimeWindowConfig m_timeWindowConfig;
    std::vector<PersonTrackingInfo> m_trackedPersons;
    int m_nextPersonId = 1;                          // 下一个人员ID

    mutable std::mutex m_mutex;

    // 自动重置定时器
    std::chrono::steady_clock::time_point m_lastResetTime;
    std::chrono::steady_clock::time_point m_lastSnapshotTime;

    // 性能统计实例
    PerformanceMetrics m_performanceMetrics;

public:
    StatisticsManager();
    ~StatisticsManager() = default;
    
    // 配置管理
    void setConfig(const StatisticsConfig& config);
    StatisticsConfig getConfig() const;
    
    // 统计更新
    void updateStatistics(const std::vector<FaceAnalysisResult>& results);
    void incrementFrameCount();
    void incrementAnalysisCount();
    
    // 🔧 人员跟踪相关方法
    void setTimeWindowConfig(const TimeWindowConfig& config);
    TimeWindowConfig getTimeWindowConfig() const;
    void updatePersonTracking(const std::vector<FaceAnalysisResult>& results);
    void cleanupExpiredPersons();
    int getActivePersonCount() const;
    std::vector<PersonTrackingInfo> getActivePersons() const;
    
    // 统计获取
    StatisticsData getCurrentStatistics() const;
    HistoricalStatistics getHistoricalStatistics() const;
    StatisticsData getAverageHistoricalStatistics() const;
    
    // 重置操作
    void resetCurrentStatistics();
    void resetHistoricalStatistics();
    void resetAll();
    
    // 自动管理
    void checkAutoReset();
    void checkSnapshotCreation();
    
    // 性能监控
    PerformanceMetrics getPerformanceMetrics() const;
    void resetPerformanceMetrics();
    
    // 数据导出
    std::string exportCurrentStatistics() const;
    std::string exportHistoricalStatistics() const;
    bool saveStatisticsToFile(const std::string& filePath) const;
    
    // 状态查询
    bool hasValidStatistics() const;
    double getStatisticsDuration() const;

private:
    // 内部更新方法
    void updateCurrentStatistics(const std::vector<FaceAnalysisResult>& results);
    void updatePerformanceMetrics(std::chrono::milliseconds updateTime);
    
    // 历史数据管理
    void createSnapshot();
    void cleanupOldSnapshots();
    
    // 数据验证
    bool isValidFaceForStats(const FaceInfo& face) const;
    
    // 日志记录
    void logStatisticsUpdate(const std::vector<FaceAnalysisResult>& results) const;
    
    // 🔧 人员跟踪辅助方法
    float calculateBoundingBoxOverlap(const cv::Rect& rect1, const cv::Rect& rect2);
};

// 统计工具函数
namespace StatisticsUtils {
    // 数据格式化
    std::string formatDuration(std::chrono::seconds duration);
    std::string formatPercentage(double percentage);
    
    // 数据分析
    std::pair<int, std::string> findDominantGender(const StatisticsData& stats);
    std::pair<int, std::string> findDominantAgeBracket(const StatisticsData& stats);
    
    // 趋势分析
    struct TrendAnalysis {
        bool isIncreasing;
        double changeRate;
        std::string description;
    };
    
    TrendAnalysis analyzePersonCountTrend(const HistoricalStatistics& historical);
    TrendAnalysis analyzeGenderTrend(const HistoricalStatistics& historical);
    
    // 报告生成
    std::string generateSummaryReport(const StatisticsData& current,
                                     const HistoricalStatistics& historical);
    
    std::string generateDetailedReport(const StatisticsData& current,
                                      const HistoricalStatistics& historical,
                                      const StatisticsManager::PerformanceMetrics& performance);
}

#endif // STATISTICS_MANAGER_H
