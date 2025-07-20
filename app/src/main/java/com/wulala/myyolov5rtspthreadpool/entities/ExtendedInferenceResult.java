package com.wulala.myyolov5rtspthreadpool.entities;

import java.util.List;
import java.util.ArrayList;

/**
 * 扩展推理结果类
 * 包含目标检测、人脸分析和统计数据的完整结果
 */
public class ExtendedInferenceResult {
    
    /**
     * 标准目标检测结果
     */
    private List<Detection> detections;
    
    /**
     * 人脸分析结果
     */
    private List<FaceAnalysisResult> faceResults;
    
    /**
     * 统计数据
     */
    private StatisticsData statistics;
    
    /**
     * 推理耗时（毫秒）
     */
    private long inferenceTimeMs;
    
    /**
     * 人脸分析耗时（毫秒）
     */
    private long faceAnalysisTimeMs;
    
    /**
     * 总处理耗时（毫秒）
     */
    private long totalProcessingTimeMs;
    
    /**
     * 是否启用了人脸分析
     */
    private boolean faceAnalysisEnabled;
    
    /**
     * 使用的模型类型
     */
    private String modelType;
    
    public ExtendedInferenceResult() {
        this.detections = new ArrayList<>();
        this.faceResults = new ArrayList<>();
        this.statistics = new StatisticsData();
        this.inferenceTimeMs = 0;
        this.faceAnalysisTimeMs = 0;
        this.totalProcessingTimeMs = 0;
        this.faceAnalysisEnabled = false;
        this.modelType = "UNKNOWN";
    }
    
    // Getters and Setters
    public List<Detection> getDetections() {
        return detections;
    }
    
    public void setDetections(List<Detection> detections) {
        this.detections = detections != null ? detections : new ArrayList<>();
    }
    
    public List<FaceAnalysisResult> getFaceResults() {
        return faceResults;
    }
    
    public void setFaceResults(List<FaceAnalysisResult> faceResults) {
        this.faceResults = faceResults != null ? faceResults : new ArrayList<>();
    }
    
    public StatisticsData getStatistics() {
        return statistics;
    }
    
    public void setStatistics(StatisticsData statistics) {
        this.statistics = statistics != null ? statistics : new StatisticsData();
    }
    
    public long getInferenceTimeMs() {
        return inferenceTimeMs;
    }
    
    public void setInferenceTimeMs(long inferenceTimeMs) {
        this.inferenceTimeMs = inferenceTimeMs;
    }
    
    public long getFaceAnalysisTimeMs() {
        return faceAnalysisTimeMs;
    }
    
    public void setFaceAnalysisTimeMs(long faceAnalysisTimeMs) {
        this.faceAnalysisTimeMs = faceAnalysisTimeMs;
    }
    
    public long getTotalProcessingTimeMs() {
        return totalProcessingTimeMs;
    }
    
    public void setTotalProcessingTimeMs(long totalProcessingTimeMs) {
        this.totalProcessingTimeMs = totalProcessingTimeMs;
    }
    
    public boolean isFaceAnalysisEnabled() {
        return faceAnalysisEnabled;
    }
    
    public void setFaceAnalysisEnabled(boolean faceAnalysisEnabled) {
        this.faceAnalysisEnabled = faceAnalysisEnabled;
    }
    
    public String getModelType() {
        return modelType;
    }
    
    public void setModelType(String modelType) {
        this.modelType = modelType != null ? modelType : "UNKNOWN";
    }
    
    // 便利方法
    
    /**
     * 获取检测到的人员数量
     */
    public int getPersonCount() {
        if (detections == null) return 0;
        return (int) detections.stream()
                .filter(d -> "person".equals(d.className))
                .count();
    }
    
    /**
     * 获取检测到的人脸数量
     */
    public int getFaceCount() {
        if (faceResults == null) return 0;
        return faceResults.size();
    }
    
    /**
     * 获取总检测对象数量
     */
    public int getTotalDetectionCount() {
        return detections != null ? detections.size() : 0;
    }
    
    /**
     * 检查是否有人脸分析结果
     */
    public boolean hasFaceAnalysis() {
        return faceAnalysisEnabled && faceResults != null && !faceResults.isEmpty();
    }
    
    /**
     * 获取性能摘要字符串
     */
    public String getPerformanceSummary() {
        StringBuilder sb = new StringBuilder();
        sb.append("推理: ").append(inferenceTimeMs).append("ms");
        if (faceAnalysisEnabled) {
            sb.append(", 人脸分析: ").append(faceAnalysisTimeMs).append("ms");
        }
        sb.append(", 总计: ").append(totalProcessingTimeMs).append("ms");
        return sb.toString();
    }
    
    /**
     * 获取检测摘要字符串
     */
    public String getDetectionSummary() {
        StringBuilder sb = new StringBuilder();
        sb.append("检测: ").append(getTotalDetectionCount()).append("个对象");
        sb.append(", 人员: ").append(getPersonCount()).append("个");
        if (hasFaceAnalysis()) {
            sb.append(", 人脸: ").append(getFaceCount()).append("个");
        }
        return sb.toString();
    }
    
    @Override
    public String toString() {
        return "ExtendedInferenceResult{" +
                "detections=" + (detections != null ? detections.size() : 0) +
                ", faceResults=" + (faceResults != null ? faceResults.size() : 0) +
                ", inferenceTimeMs=" + inferenceTimeMs +
                ", faceAnalysisTimeMs=" + faceAnalysisTimeMs +
                ", totalProcessingTimeMs=" + totalProcessingTimeMs +
                ", faceAnalysisEnabled=" + faceAnalysisEnabled +
                ", modelType='" + modelType + '\'' +
                '}';
    }
}
