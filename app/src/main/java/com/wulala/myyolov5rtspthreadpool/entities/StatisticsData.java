package com.wulala.myyolov5rtspthreadpool.entities;

import java.util.HashMap;
import java.util.Map;

/**
 * 统计数据类
 * 包含推理性能和检测统计信息
 */
public class StatisticsData {
    
    /**
     * 总推理次数
     */
    private long totalInferences;
    
    /**
     * 平均推理时间（毫秒）
     */
    private double averageInferenceTimeMs;
    
    /**
     * 最小推理时间（毫秒）
     */
    private long minInferenceTimeMs;
    
    /**
     * 最大推理时间（毫秒）
     */
    private long maxInferenceTimeMs;
    
    /**
     * 当前FPS
     */
    private double currentFps;
    
    /**
     * 平均FPS
     */
    private double averageFps;
    
    /**
     * 总检测对象数
     */
    private long totalDetections;
    
    /**
     * 按类别统计的检测数量
     */
    private Map<String, Integer> detectionsByClass;
    
    /**
     * 人脸分析统计
     */
    private FaceAnalysisStatistics faceStats;
    
    /**
     * 内存使用情况（MB）
     */
    private double memoryUsageMB;
    
    /**
     * CPU使用率（百分比）
     */
    private double cpuUsagePercent;
    
    /**
     * 统计开始时间
     */
    private long startTimeMs;
    
    /**
     * 最后更新时间
     */
    private long lastUpdateTimeMs;
    
    public StatisticsData() {
        this.totalInferences = 0;
        this.averageInferenceTimeMs = 0.0;
        this.minInferenceTimeMs = Long.MAX_VALUE;
        this.maxInferenceTimeMs = 0;
        this.currentFps = 0.0;
        this.averageFps = 0.0;
        this.totalDetections = 0;
        this.detectionsByClass = new HashMap<>();
        this.faceStats = new FaceAnalysisStatistics();
        this.memoryUsageMB = 0.0;
        this.cpuUsagePercent = 0.0;
        this.startTimeMs = System.currentTimeMillis();
        this.lastUpdateTimeMs = this.startTimeMs;
    }
    
    // Getters and Setters
    public long getTotalInferences() {
        return totalInferences;
    }
    
    public void setTotalInferences(long totalInferences) {
        this.totalInferences = totalInferences;
    }
    
    public double getAverageInferenceTimeMs() {
        return averageInferenceTimeMs;
    }
    
    public void setAverageInferenceTimeMs(double averageInferenceTimeMs) {
        this.averageInferenceTimeMs = averageInferenceTimeMs;
    }
    
    public long getMinInferenceTimeMs() {
        return minInferenceTimeMs == Long.MAX_VALUE ? 0 : minInferenceTimeMs;
    }
    
    public void setMinInferenceTimeMs(long minInferenceTimeMs) {
        this.minInferenceTimeMs = minInferenceTimeMs;
    }
    
    public long getMaxInferenceTimeMs() {
        return maxInferenceTimeMs;
    }
    
    public void setMaxInferenceTimeMs(long maxInferenceTimeMs) {
        this.maxInferenceTimeMs = maxInferenceTimeMs;
    }
    
    public double getCurrentFps() {
        return currentFps;
    }
    
    public void setCurrentFps(double currentFps) {
        this.currentFps = currentFps;
    }
    
    public double getAverageFps() {
        return averageFps;
    }
    
    public void setAverageFps(double averageFps) {
        this.averageFps = averageFps;
    }
    
    public long getTotalDetections() {
        return totalDetections;
    }
    
    public void setTotalDetections(long totalDetections) {
        this.totalDetections = totalDetections;
    }
    
    public Map<String, Integer> getDetectionsByClass() {
        return detectionsByClass;
    }
    
    public void setDetectionsByClass(Map<String, Integer> detectionsByClass) {
        this.detectionsByClass = detectionsByClass != null ? detectionsByClass : new HashMap<>();
    }
    
    public FaceAnalysisStatistics getFaceStats() {
        return faceStats;
    }
    
    public void setFaceStats(FaceAnalysisStatistics faceStats) {
        this.faceStats = faceStats != null ? faceStats : new FaceAnalysisStatistics();
    }
    
    public double getMemoryUsageMB() {
        return memoryUsageMB;
    }
    
    public void setMemoryUsageMB(double memoryUsageMB) {
        this.memoryUsageMB = memoryUsageMB;
    }
    
    public double getCpuUsagePercent() {
        return cpuUsagePercent;
    }
    
    public void setCpuUsagePercent(double cpuUsagePercent) {
        this.cpuUsagePercent = cpuUsagePercent;
    }
    
    public long getStartTimeMs() {
        return startTimeMs;
    }
    
    public void setStartTimeMs(long startTimeMs) {
        this.startTimeMs = startTimeMs;
    }
    
    public long getLastUpdateTimeMs() {
        return lastUpdateTimeMs;
    }
    
    public void setLastUpdateTimeMs(long lastUpdateTimeMs) {
        this.lastUpdateTimeMs = lastUpdateTimeMs;
    }
    
    // 便利方法
    
    /**
     * 获取运行时长（秒）
     */
    public long getUptimeSeconds() {
        return (lastUpdateTimeMs - startTimeMs) / 1000;
    }
    
    /**
     * 获取特定类别的检测数量
     */
    public int getDetectionCount(String className) {
        return detectionsByClass.getOrDefault(className, 0);
    }
    
    /**
     * 获取人员检测数量
     */
    public int getPersonCount() {
        return getDetectionCount("person");
    }
    
    /**
     * 获取性能摘要字符串
     */
    public String getPerformanceSummary() {
        return String.format("FPS: %.1f (平均: %.1f), 推理: %.1fms (范围: %d-%dms)", 
                currentFps, averageFps, averageInferenceTimeMs, 
                getMinInferenceTimeMs(), maxInferenceTimeMs);
    }
    
    /**
     * 获取检测摘要字符串
     */
    public String getDetectionSummary() {
        StringBuilder sb = new StringBuilder();
        sb.append("总检测: ").append(totalDetections);
        sb.append(", 人员: ").append(getPersonCount());
        if (faceStats.getTotalFaces() > 0) {
            sb.append(", 人脸: ").append(faceStats.getTotalFaces());
        }
        return sb.toString();
    }
    
    /**
     * 获取资源使用摘要字符串
     */
    public String getResourceSummary() {
        return String.format("内存: %.1fMB, CPU: %.1f%%", memoryUsageMB, cpuUsagePercent);
    }
    
    /**
     * 重置统计数据
     */
    public void reset() {
        this.totalInferences = 0;
        this.averageInferenceTimeMs = 0.0;
        this.minInferenceTimeMs = Long.MAX_VALUE;
        this.maxInferenceTimeMs = 0;
        this.currentFps = 0.0;
        this.averageFps = 0.0;
        this.totalDetections = 0;
        this.detectionsByClass.clear();
        this.faceStats.reset();
        this.startTimeMs = System.currentTimeMillis();
        this.lastUpdateTimeMs = this.startTimeMs;
    }
    
    @Override
    public String toString() {
        return "StatisticsData{" +
                "totalInferences=" + totalInferences +
                ", averageInferenceTimeMs=" + averageInferenceTimeMs +
                ", currentFps=" + currentFps +
                ", totalDetections=" + totalDetections +
                ", uptimeSeconds=" + getUptimeSeconds() +
                '}';
    }
    
    /**
     * 人脸分析统计子类
     */
    public static class FaceAnalysisStatistics {
        private long totalFaces;
        private long totalFaceAnalyses;
        private double averageFaceAnalysisTimeMs;
        private Map<String, Integer> genderDistribution;
        private Map<String, Integer> ageDistribution;
        private Map<String, Integer> raceDistribution;
        
        public FaceAnalysisStatistics() {
            this.totalFaces = 0;
            this.totalFaceAnalyses = 0;
            this.averageFaceAnalysisTimeMs = 0.0;
            this.genderDistribution = new HashMap<>();
            this.ageDistribution = new HashMap<>();
            this.raceDistribution = new HashMap<>();
        }
        
        // Getters and Setters
        public long getTotalFaces() { return totalFaces; }
        public void setTotalFaces(long totalFaces) { this.totalFaces = totalFaces; }
        
        public long getTotalFaceAnalyses() { return totalFaceAnalyses; }
        public void setTotalFaceAnalyses(long totalFaceAnalyses) { this.totalFaceAnalyses = totalFaceAnalyses; }
        
        public double getAverageFaceAnalysisTimeMs() { return averageFaceAnalysisTimeMs; }
        public void setAverageFaceAnalysisTimeMs(double averageFaceAnalysisTimeMs) { 
            this.averageFaceAnalysisTimeMs = averageFaceAnalysisTimeMs; 
        }
        
        public Map<String, Integer> getGenderDistribution() { return genderDistribution; }
        public void setGenderDistribution(Map<String, Integer> genderDistribution) { 
            this.genderDistribution = genderDistribution != null ? genderDistribution : new HashMap<>(); 
        }
        
        public Map<String, Integer> getAgeDistribution() { return ageDistribution; }
        public void setAgeDistribution(Map<String, Integer> ageDistribution) { 
            this.ageDistribution = ageDistribution != null ? ageDistribution : new HashMap<>(); 
        }
        
        public Map<String, Integer> getRaceDistribution() { return raceDistribution; }
        public void setRaceDistribution(Map<String, Integer> raceDistribution) { 
            this.raceDistribution = raceDistribution != null ? raceDistribution : new HashMap<>(); 
        }
        
        public void reset() {
            totalFaces = 0;
            totalFaceAnalyses = 0;
            averageFaceAnalysisTimeMs = 0.0;
            genderDistribution.clear();
            ageDistribution.clear();
            raceDistribution.clear();
        }
    }
}
