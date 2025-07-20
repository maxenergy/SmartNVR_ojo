package com.wulala.myyolov5rtspthreadpool;

import android.content.Context;
import android.util.Log;

import java.util.ArrayList;
import java.util.List;
import java.util.Set;

/**
 * 检测结果过滤器
 * 根据用户设置过滤YOLO检测结果
 */
public class DetectionResultFilter {
    private static final String TAG = "DetectionResultFilter";
    
    private DetectionSettingsManager settingsManager;
    
    public DetectionResultFilter(Context context) {
        this.settingsManager = new DetectionSettingsManager(context);
    }
    
    /**
     * 过滤检测结果
     * @param allResults 所有检测结果
     * @return 过滤后的检测结果
     */
    public List<DetectionResult> filterResults(List<DetectionResult> allResults) {
        if (allResults == null || allResults.isEmpty()) {
            return new ArrayList<>();
        }
        
        // 检查检测是否启用
        if (!settingsManager.isDetectionEnabled()) {
            Log.d(TAG, "目标检测已禁用，返回空结果");
            return new ArrayList<>();
        }
        
        Set<String> enabledClasses = settingsManager.getEnabledClasses();
        float confidenceThreshold = settingsManager.getConfidenceThreshold();
        
        List<DetectionResult> filteredResults = new ArrayList<>();
        
        for (DetectionResult result : allResults) {
            // 检查类别是否启用
            if (!enabledClasses.contains(result.className)) {
                continue;
            }
            
            // 检查置信度是否满足阈值
            if (result.confidence < confidenceThreshold) {
                continue;
            }
            
            // 检查结果是否有效
            if (!result.isValid()) {
                continue;
            }
            
            filteredResults.add(result);
        }
        
        Log.d(TAG, String.format("检测结果过滤: %d -> %d (启用类别: %s, 置信度阈值: %.2f)", 
                allResults.size(), filteredResults.size(), 
                enabledClasses.toString(), confidenceThreshold));
        
        return filteredResults;
    }
    
    /**
     * 检测结果数据类（与RealYOLOInference.DetectionResult保持一致）
     */
    public static class DetectionResult {
        public int classId;
        public float confidence;
        public float x1, y1, x2, y2;
        public String className;
        
        public DetectionResult(int classId, float confidence, 
                             float x1, float y1, float x2, float y2, 
                             String className) {
            this.classId = classId;
            this.confidence = confidence;
            this.x1 = x1;
            this.y1 = y1;
            this.x2 = x2;
            this.y2 = y2;
            this.className = className;
        }
        
        /**
         * 检查是否为人员检测结果
         */
        public boolean isPerson() {
            return "person".equals(className) || classId == 0;
        }
        
        /**
         * 获取边界框宽度
         */
        public float getWidth() {
            return x2 - x1;
        }
        
        /**
         * 获取边界框高度
         */
        public float getHeight() {
            return y2 - y1;
        }
        
        /**
         * 获取边界框面积
         */
        public float getArea() {
            return getWidth() * getHeight();
        }
        
        /**
         * 检查边界框是否有效
         */
        public boolean isValid() {
            return x1 >= 0 && y1 >= 0 && x2 > x1 && y2 > y1 && 
                   confidence > 0 && !className.isEmpty();
        }
        
        @Override
        public String toString() {
            return String.format("DetectionResult{class=%s(%d), conf=%.3f, box=[%.1f,%.1f,%.1f,%.1f]}", 
                               className, classId, confidence, x1, y1, x2, y2);
        }
    }
    
    /**
     * 从RealYOLOInference.DetectionResult转换
     */
    public static DetectionResult fromRealYOLOResult(RealYOLOInference.DetectionResult yoloResult) {
        return new DetectionResult(
            yoloResult.classId,
            yoloResult.confidence,
            yoloResult.x1,
            yoloResult.y1,
            yoloResult.x2,
            yoloResult.y2,
            yoloResult.className
        );
    }
    
    /**
     * 转换为RealYOLOInference.DetectionResult
     */
    public static RealYOLOInference.DetectionResult toRealYOLOResult(DetectionResult result) {
        return new RealYOLOInference.DetectionResult(
            result.classId,
            result.confidence,
            result.x1,
            result.y1,
            result.x2,
            result.y2,
            result.className
        );
    }
    
    /**
     * 批量转换从RealYOLOInference.DetectionResult
     */
    public static List<DetectionResult> fromRealYOLOResults(RealYOLOInference.DetectionResult[] yoloResults) {
        List<DetectionResult> results = new ArrayList<>();
        if (yoloResults != null) {
            for (RealYOLOInference.DetectionResult yoloResult : yoloResults) {
                results.add(fromRealYOLOResult(yoloResult));
            }
        }
        return results;
    }
    
    /**
     * 批量转换为RealYOLOInference.DetectionResult
     */
    public static RealYOLOInference.DetectionResult[] toRealYOLOResults(List<DetectionResult> results) {
        RealYOLOInference.DetectionResult[] yoloResults = new RealYOLOInference.DetectionResult[results.size()];
        for (int i = 0; i < results.size(); i++) {
            yoloResults[i] = toRealYOLOResult(results.get(i));
        }
        return yoloResults;
    }
}
