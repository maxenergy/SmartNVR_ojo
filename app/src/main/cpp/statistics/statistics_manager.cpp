#include "statistics_manager.h"
#include "log4c.h"
#include <algorithm>
#include <sstream>
#include <iomanip>

StatisticsManager::StatisticsManager() {
    m_config = StatisticsConfig();
    m_currentStats.reset();
    m_lastResetTime = std::chrono::steady_clock::now();
    m_lastSnapshotTime = m_lastResetTime;
    
    // 🔧 初始化时间窗口配置
    m_timeWindowConfig = TimeWindowConfig();
    m_trackedPersons.reserve(m_timeWindowConfig.maxTrackedPersons);
    
    LOGI("StatisticsManager created");
}

void StatisticsManager::setConfig(const StatisticsConfig& config) {
    std::lock_guard<std::mutex> lock(m_mutex);
    
    if (!config.isValid()) {
        LOGE("Invalid StatisticsConfig provided");
        return;
    }
    
    m_config = config;
    m_historicalStats.snapshotInterval = config.snapshotInterval;
    m_historicalStats.maxSnapshots = config.maxHistoricalSnapshots;
    
    LOGI("StatisticsConfig updated");
}

StatisticsConfig StatisticsManager::getConfig() const {
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_config;
}

void StatisticsManager::updateStatistics(const std::vector<FaceAnalysisResult>& results) {
    auto startTime = std::chrono::steady_clock::now();
    
    std::lock_guard<std::mutex> lock(m_mutex);
    
    // 更新当前统计
    updateCurrentStatistics(results);
    
    // 检查自动重置
    if (m_config.enableRealTimeStats) {
        checkAutoReset();
    }
    
    // 检查快照创建
    if (m_config.enableHistoricalStats) {
        checkSnapshotCreation();
    }
    
    auto endTime = std::chrono::steady_clock::now();
    auto updateTime = std::chrono::duration_cast<std::chrono::milliseconds>(endTime - startTime);
    
    // 更新性能指标
    if (m_config.enablePerformanceStats) {
        updatePerformanceMetrics(updateTime);
    }
    
    LOGD("Statistics updated with %zu face analysis results", results.size());
}

void StatisticsManager::incrementFrameCount() {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_currentStats.frameCount++;
}

void StatisticsManager::incrementAnalysisCount() {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_currentStats.analysisCount++;
}

StatisticsData StatisticsManager::getCurrentStatistics() const {
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_currentStats;
}

HistoricalStatistics StatisticsManager::getHistoricalStatistics() const {
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_historicalStats;
}

StatisticsData StatisticsManager::getAverageHistoricalStatistics() const {
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_historicalStats.getAverageStatistics();
}

void StatisticsManager::resetCurrentStatistics() {
    std::lock_guard<std::mutex> lock(m_mutex);
    
    LOGI("Resetting current statistics");
    m_currentStats.reset();
    m_lastResetTime = std::chrono::steady_clock::now();
    
    // 重置性能指标
    if (m_config.enablePerformanceStats) {
        m_performanceMetrics = PerformanceMetrics();
    }
}

void StatisticsManager::resetHistoricalStatistics() {
    std::lock_guard<std::mutex> lock(m_mutex);
    
    LOGI("Resetting historical statistics");
    m_historicalStats.clear();
    m_lastSnapshotTime = std::chrono::steady_clock::now();
}

void StatisticsManager::resetAll() {
    std::lock_guard<std::mutex> lock(m_mutex);
    
    LOGI("Resetting all statistics");
    m_currentStats.reset();
    m_historicalStats.clear();
    m_performanceMetrics = PerformanceMetrics();
    
    auto now = std::chrono::steady_clock::now();
    m_lastResetTime = now;
    m_lastSnapshotTime = now;
}

void StatisticsManager::checkAutoReset() {
    auto now = std::chrono::steady_clock::now();
    auto timeSinceReset = std::chrono::duration_cast<std::chrono::seconds>(now - m_lastResetTime);
    
    if (timeSinceReset >= m_config.resetInterval) {
        LOGI("Auto-resetting statistics after %lld seconds", timeSinceReset.count());
        
        // 在重置前创建快照
        if (m_config.enableHistoricalStats) {
            createSnapshot();
        }
        
        m_currentStats.reset();
        m_lastResetTime = now;
    }
}

void StatisticsManager::checkSnapshotCreation() {
    auto now = std::chrono::steady_clock::now();
    auto timeSinceSnapshot = std::chrono::duration_cast<std::chrono::seconds>(now - m_lastSnapshotTime);
    
    if (timeSinceSnapshot >= m_config.snapshotInterval) {
        createSnapshot();
        m_lastSnapshotTime = now;
    }
}

StatisticsManager::PerformanceMetrics StatisticsManager::getPerformanceMetrics() const {
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_performanceMetrics;
}

void StatisticsManager::resetPerformanceMetrics() {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_performanceMetrics = PerformanceMetrics();
    LOGI("Performance metrics reset");
}

std::string StatisticsManager::exportCurrentStatistics() const {
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_currentStats.toString();
}

std::string StatisticsManager::exportHistoricalStatistics() const {
    std::lock_guard<std::mutex> lock(m_mutex);
    
    std::stringstream ss;
    ss << "=== 历史统计数据 ===\n";
    ss << "快照数量: " << m_historicalStats.snapshots.size() << "\n";
    ss << "快照间隔: " << m_historicalStats.snapshotInterval.count() << " 秒\n\n";
    
    if (!m_historicalStats.snapshots.empty()) {
        ss << "平均统计数据:\n";
        auto avgStats = m_historicalStats.getAverageStatistics();
        ss << avgStats.toString();
    }
    
    return ss.str();
}

bool StatisticsManager::saveStatisticsToFile(const std::string& filePath) const {
    // TODO: 实现文件保存功能
    LOGD("Save statistics to file: %s (placeholder)", filePath.c_str());
    return true;
}

bool StatisticsManager::hasValidStatistics() const {
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_currentStats.totalPersonCount > 0 || m_currentStats.totalFaceCount > 0;
}

double StatisticsManager::getStatisticsDuration() const {
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_currentStats.getDurationSeconds();
}

// ==================== 私有方法实现 ====================

void StatisticsManager::updateCurrentStatistics(const std::vector<FaceAnalysisResult>& results) {
    // 🔧 优化：首先更新人员跟踪
    updatePersonTracking(results);
    
    // 🔧 基于跟踪结果更新统计（避免重复计数）
    auto activePersons = getActivePersons();
    m_currentStats.totalPersonCount = static_cast<int>(activePersons.size());
    
    // 🔧 基于跟踪的人员进行统计（避免重复计数）
    int totalFaces = 0;
    int validFaces = 0;
    int males = 0, females = 0, unknownGender = 0;
    
    // 重置年龄和种族计数
    m_currentStats.ageBracketCounts.fill(0);
    m_currentStats.raceCounts.fill(0);
    
    // 统计活跃人员的属性
    for (const auto& person : activePersons) {
        if (person.confidence > 0.0f) { // 有有效的人脸属性
            validFaces++;
            
            // 统计性别
            if (person.gender == 1) {
                males++;
            } else if (person.gender == 0) {
                females++;
            } else {
                unknownGender++;
            }
            
            // 统计年龄段
            if (person.ageBracket >= 0 && person.ageBracket < 9) {
                m_currentStats.ageBracketCounts[person.ageBracket]++;
            }
            
            // 统计种族
            if (person.race >= 0 && person.race < 5) {
                m_currentStats.raceCounts[person.race]++;
            }
        }
    }
    
    // 计算当前帧的总人脸数（用于参考）
    for (const auto& result : results) {
        totalFaces += static_cast<int>(result.faces.size());
    }
    
    // 更新统计数据
    m_currentStats.totalFaceCount = totalFaces;
    m_currentStats.validFaceCount = validFaces;
    m_currentStats.maleCount = males;
    m_currentStats.femaleCount = females;
    m_currentStats.unknownGenderCount = unknownGender;
    m_currentStats.lastUpdateTime = std::chrono::steady_clock::now();
    
    // 记录统计更新日志
    logStatisticsUpdate(results);
}

void StatisticsManager::updatePerformanceMetrics(std::chrono::milliseconds updateTime) {
    m_performanceMetrics.updateCount++;
    m_performanceMetrics.totalUpdateTime += updateTime;
    m_performanceMetrics.lastUpdateTime = std::chrono::steady_clock::now();
}

void StatisticsManager::createSnapshot() {
    if (hasValidStatistics()) {
        m_historicalStats.addSnapshot(m_currentStats);
        LOGD("Created statistics snapshot (%zu total)", m_historicalStats.snapshots.size());
    }
}

void StatisticsManager::cleanupOldSnapshots() {
    // HistoricalStatistics::addSnapshot 已经处理了清理逻辑
}

bool StatisticsManager::isValidFaceForStats(const FaceInfo& face) const {
    return face.confidence >= m_config.minConfidenceForStats &&
           face.faceRect.width >= m_config.minFaceSizeForStats &&
           face.faceRect.height >= m_config.minFaceSizeForStats &&
           face.attributes.isValid();
}

void StatisticsManager::logStatisticsUpdate(const std::vector<FaceAnalysisResult>& results) const {
    if (results.empty()) return;
    
    LOGD("Statistics update: %d persons, %d faces (%d valid), %d male, %d female",
         m_currentStats.totalPersonCount,
         m_currentStats.totalFaceCount,
         m_currentStats.validFaceCount,
         m_currentStats.maleCount,
         m_currentStats.femaleCount);
}

// ==================== 工具函数实现 ====================

namespace StatisticsUtils {

std::string formatDuration(std::chrono::seconds duration) {
    auto hours = std::chrono::duration_cast<std::chrono::hours>(duration);
    auto minutes = std::chrono::duration_cast<std::chrono::minutes>(duration % std::chrono::hours(1));
    auto seconds = duration % std::chrono::minutes(1);
    
    std::stringstream ss;
    if (hours.count() > 0) {
        ss << hours.count() << "h ";
    }
    if (minutes.count() > 0) {
        ss << minutes.count() << "m ";
    }
    ss << seconds.count() << "s";
    
    return ss.str();
}

std::string formatPercentage(double percentage) {
    std::stringstream ss;
    ss << std::fixed << std::setprecision(1) << percentage << "%";
    return ss.str();
}

std::pair<int, std::string> findDominantGender(const StatisticsData& stats) {
    if (stats.maleCount > stats.femaleCount) {
        return {stats.maleCount, "男性"};
    } else if (stats.femaleCount > stats.maleCount) {
        return {stats.femaleCount, "女性"};
    } else {
        return {stats.maleCount, "平衡"};
    }
}

std::pair<int, std::string> findDominantAgeBracket(const StatisticsData& stats) {
    int maxCount = 0;
    int maxIndex = -1;
    
    for (int i = 0; i < 9; i++) {
        if (stats.ageBracketCounts[i] > maxCount) {
            maxCount = stats.ageBracketCounts[i];
            maxIndex = i;
        }
    }
    
    if (maxIndex >= 0) {
        const char* ageLabels[] = {
            "0-2岁", "3-9岁", "10-19岁", "20-29岁", "30-39岁",
            "40-49岁", "50-59岁", "60-69岁", "70岁以上"
        };
        return {maxCount, ageLabels[maxIndex]};
    }
    
    return {0, "未知"};
}

TrendAnalysis analyzePersonCountTrend(const HistoricalStatistics& historical) {
    TrendAnalysis trend;
    trend.isIncreasing = false;
    trend.changeRate = 0.0;
    trend.description = "数据不足";
    
    if (historical.snapshots.size() < 2) {
        return trend;
    }
    
    // 简单的趋势分析：比较最近几个快照
    auto recent = historical.snapshots.end() - 1;
    auto previous = historical.snapshots.end() - 2;
    
    int recentCount = recent->totalPersonCount;
    int previousCount = previous->totalPersonCount;
    
    if (previousCount > 0) {
        trend.changeRate = static_cast<double>(recentCount - previousCount) / previousCount * 100.0;
        trend.isIncreasing = trend.changeRate > 0;
        
        if (std::abs(trend.changeRate) < 5.0) {
            trend.description = "稳定";
        } else if (trend.isIncreasing) {
            trend.description = "上升趋势";
        } else {
            trend.description = "下降趋势";
        }
    }
    
    return trend;
}

TrendAnalysis analyzeGenderTrend(const HistoricalStatistics& historical) {
    TrendAnalysis trend;
    trend.isIncreasing = false;
    trend.changeRate = 0.0;
    trend.description = "数据不足";
    
    if (historical.snapshots.size() < 2) {
        return trend;
    }
    
    // 分析性别比例趋势
    auto recent = historical.snapshots.end() - 1;
    auto previous = historical.snapshots.end() - 2;
    
    auto recentGenderPercent = recent->getGenderPercentage();
    auto previousGenderPercent = previous->getGenderPercentage();
    
    double maleChange = recentGenderPercent.first - previousGenderPercent.first;
    
    trend.changeRate = maleChange;
    trend.isIncreasing = maleChange > 0;
    
    if (std::abs(maleChange) < 2.0) {
        trend.description = "性别比例稳定";
    } else if (trend.isIncreasing) {
        trend.description = "男性比例上升";
    } else {
        trend.description = "女性比例上升";
    }
    
    return trend;
}

std::string generateSummaryReport(const StatisticsData& current,
                                 const HistoricalStatistics& historical) {
    std::stringstream ss;
    
    ss << "=== 统计摘要报告 ===\n";
    ss << "生成时间: " << std::chrono::duration_cast<std::chrono::seconds>(
        std::chrono::system_clock::now().time_since_epoch()).count() << "\n\n";
    
    // 当前统计
    ss << "当前统计:\n";
    ss << "  总人数: " << current.totalPersonCount << "\n";
    ss << "  有效人脸: " << current.validFaceCount << "\n";
    
    if (current.validFaceCount > 0) {
        auto genderPercent = current.getGenderPercentage();
        ss << "  性别分布: 男性 " << formatPercentage(genderPercent.first) 
           << ", 女性 " << formatPercentage(genderPercent.second) << "\n";
        
        auto dominantAge = findDominantAgeBracket(current);
        ss << "  主要年龄段: " << dominantAge.second << " (" << dominantAge.first << "人)\n";
    }
    
    // 趋势分析
    if (historical.snapshots.size() >= 2) {
        ss << "\n趋势分析:\n";
        auto personTrend = analyzePersonCountTrend(historical);
        ss << "  人数趋势: " << personTrend.description;
        if (personTrend.changeRate != 0) {
            ss << " (" << formatPercentage(std::abs(personTrend.changeRate)) << ")";
        }
        ss << "\n";
        
        auto genderTrend = analyzeGenderTrend(historical);
        ss << "  性别趋势: " << genderTrend.description << "\n";
    }
    
    return ss.str();
}

std::string generateDetailedReport(const StatisticsData& current,
                                  const HistoricalStatistics& historical,
                                  const StatisticsManager::PerformanceMetrics& performance) {
    std::stringstream ss;
    
    // 基础摘要
    ss << generateSummaryReport(current, historical);
    
    // 详细性能信息
    ss << "\n=== 性能指标 ===\n";
    ss << "更新次数: " << performance.updateCount << "\n";
    ss << "平均更新时间: " << performance.getAverageUpdateTime() << " ms\n";
    
    // 详细历史信息
    if (!historical.snapshots.empty()) {
        ss << "\n=== 历史数据 ===\n";
        ss << "快照数量: " << historical.snapshots.size() << "\n";
        ss << "数据跨度: " << formatDuration(std::chrono::seconds(
            historical.snapshots.size() * historical.snapshotInterval.count())) << "\n";
        
        auto avgStats = historical.getAverageStatistics();
        ss << "平均人数: " << avgStats.totalPersonCount << "\n";
        ss << "平均有效人脸: " << avgStats.validFaceCount << "\n";
    }
    
    return ss.str();
}

} // namespace StatisticsUtils

// ==================== 🔧 人员跟踪算法实现 ====================

void StatisticsManager::setTimeWindowConfig(const TimeWindowConfig& config) {
    std::lock_guard<std::mutex> lock(m_mutex);
    
    if (!config.isValid()) {
        LOGE("Invalid TimeWindowConfig provided");
        return;
    }
    
    m_timeWindowConfig = config;
    
    // 调整跟踪容器大小
    m_trackedPersons.reserve(config.maxTrackedPersons);
    
    LOGI("TimeWindowConfig updated: tracking_window=%llds, stats_window=%llds, overlap_threshold=%.2f",
         config.personTrackingWindow.count(), config.statisticsWindow.count(), config.overlapThreshold);
}

TimeWindowConfig StatisticsManager::getTimeWindowConfig() const {
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_timeWindowConfig;
}

void StatisticsManager::updatePersonTracking(const std::vector<FaceAnalysisResult>& results) {
    std::lock_guard<std::mutex> lock(m_mutex);
    
    LOGD("🔧 开始人员跟踪更新，输入%zu个人员检测结果", results.size());
    
    // 1. 清理过期的人员
    cleanupExpiredPersons();
    
    // 2. 标记所有现有跟踪为未匹配
    for (auto& person : m_trackedPersons) {
        person.markMissed();
    }
    
    // 3. 处理当前检测结果
    for (const auto& result : results) {
        if (result.faces.empty()) continue;
        
        // 计算人员边界框（基于人员检测结果）
        cv::Rect personBBox(
            static_cast<int>(result.personDetection.x1),
            static_cast<int>(result.personDetection.y1),
            static_cast<int>(result.personDetection.x2 - result.personDetection.x1),
            static_cast<int>(result.personDetection.y2 - result.personDetection.y1)
        );
        
        // 查找最佳匹配的现有跟踪
        int bestMatchIndex = -1;
        float bestOverlap = 0.0f;
        
        for (size_t i = 0; i < m_trackedPersons.size(); ++i) {
            if (!m_trackedPersons[i].isActive) continue;
            
            float overlap = calculateBoundingBoxOverlap(personBBox, m_trackedPersons[i].lastBoundingBox);
            if (overlap > m_timeWindowConfig.overlapThreshold && overlap > bestOverlap) {
                bestOverlap = overlap;
                bestMatchIndex = static_cast<int>(i);
            }
        }
        
        if (bestMatchIndex >= 0) {
            // 更新现有跟踪
            auto& person = m_trackedPersons[bestMatchIndex];
            person.updateTracking(personBBox);
            
            // 更新人脸属性（使用最佳人脸）
            if (result.hasValidFaces()) {
                auto bestFace = result.getBestFace();
                person.gender = bestFace.attributes.gender;
                person.ageBracket = bestFace.attributes.ageBracket;
                person.race = bestFace.attributes.race;
                person.confidence = bestFace.confidence;
            }
            
            LOGD("✅ 更新现有人员跟踪 ID=%d, overlap=%.2f", person.personId, bestOverlap);
        } else {
            // 创建新的人员跟踪
            if (m_trackedPersons.size() < static_cast<size_t>(m_timeWindowConfig.maxTrackedPersons)) {
                PersonTrackingInfo newPerson(m_nextPersonId++, personBBox);
                
                // 设置人脸属性
                if (result.hasValidFaces()) {
                    auto bestFace = result.getBestFace();
                    newPerson.gender = bestFace.attributes.gender;
                    newPerson.ageBracket = bestFace.attributes.ageBracket;
                    newPerson.race = bestFace.attributes.race;
                    newPerson.confidence = bestFace.confidence;
                }
                
                m_trackedPersons.push_back(newPerson);
                LOGD("🆕 创建新人员跟踪 ID=%d", newPerson.personId);
            } else {
                LOGW("⚠️ 达到最大跟踪人员数限制: %d", m_timeWindowConfig.maxTrackedPersons);
            }
        }
    }
    
    LOGD("🔧 人员跟踪更新完成，当前活跃人员: %d", getActivePersonCount());
}

void StatisticsManager::cleanupExpiredPersons() {
    // 注意：此方法假设已经持有锁
    
    auto now = std::chrono::steady_clock::now();
    size_t originalSize = m_trackedPersons.size();
    
    // 移除过期的人员跟踪
    m_trackedPersons.erase(
        std::remove_if(m_trackedPersons.begin(), m_trackedPersons.end(),
            [this, now](const PersonTrackingInfo& person) {
                bool shouldRemove = person.shouldBeRemoved(m_timeWindowConfig.personTrackingWindow);
                if (shouldRemove) {
                    LOGD("🗑️ 清理过期人员跟踪 ID=%d, 存在时长=%.1fs", 
                         person.personId, person.getLifetimeSeconds());
                }
                return shouldRemove;
            }),
        m_trackedPersons.end()
    );
    
    size_t removedCount = originalSize - m_trackedPersons.size();
    if (removedCount > 0) {
        LOGD("🧹 清理了%zu个过期人员跟踪", removedCount);
    }
}

int StatisticsManager::getActivePersonCount() const {
    // 注意：此方法假设已经持有锁或在锁保护的上下文中调用
    return static_cast<int>(std::count_if(m_trackedPersons.begin(), m_trackedPersons.end(),
        [](const PersonTrackingInfo& person) { return person.isActive; }));
}

std::vector<PersonTrackingInfo> StatisticsManager::getActivePersons() const {
    std::lock_guard<std::mutex> lock(m_mutex);
    
    std::vector<PersonTrackingInfo> activePersons;
    std::copy_if(m_trackedPersons.begin(), m_trackedPersons.end(),
                 std::back_inserter(activePersons),
                 [](const PersonTrackingInfo& person) { return person.isActive; });
    
    return activePersons;
}

// 🔧 辅助函数：计算边界框重叠度
float StatisticsManager::calculateBoundingBoxOverlap(const cv::Rect& rect1, const cv::Rect& rect2) {
    // 计算交集
    cv::Rect intersection = rect1 & rect2;
    if (intersection.area() == 0) return 0.0f;
    
    // 计算并集
    int unionArea = rect1.area() + rect2.area() - intersection.area();
    if (unionArea == 0) return 0.0f;
    
    // 返回IoU (Intersection over Union)
    return static_cast<float>(intersection.area()) / static_cast<float>(unionArea);
}
