package com.wulala.myyolov5rtspthreadpool;

import android.content.Context;
import android.util.Log;

import java.util.ArrayList;
import java.util.HashSet;
import java.util.List;
import java.util.Set;

/**
 * 检测设置功能验证器
 * 用于验证检测设置功能是否正常工作
 */
public class DetectionSettingsValidator {
    private static final String TAG = "DetectionValidator";
    
    private Context context;
    private DetectionSettingsManager settingsManager;
    private DetectionResultFilter resultFilter;
    
    public DetectionSettingsValidator(Context context) {
        this.context = context;
        this.settingsManager = new DetectionSettingsManager(context);
        this.resultFilter = new DetectionResultFilter(context);
    }
    
    /**
     * 运行完整的功能验证
     */
    public ValidationResult runFullValidation() {
        ValidationResult result = new ValidationResult();
        
        Log.i(TAG, "开始运行检测设置功能验证...");
        
        // 1. 验证设置管理器
        result.settingsManagerValid = validateSettingsManager();
        
        // 2. 验证检测过滤器
        result.filterValid = validateDetectionFilter();
        
        // 3. 验证数据持久化
        result.persistenceValid = validatePersistence();
        
        // 4. 验证类别映射
        result.classMappingValid = validateClassMapping();
        
        // 5. 验证边界条件
        result.boundaryConditionsValid = validateBoundaryConditions();
        
        result.overallValid = result.settingsManagerValid && 
                             result.filterValid && 
                             result.persistenceValid && 
                             result.classMappingValid && 
                             result.boundaryConditionsValid;
        
        Log.i(TAG, "验证完成，总体结果: " + (result.overallValid ? "通过" : "失败"));
        return result;
    }
    
    /**
     * 验证设置管理器功能
     */
    private boolean validateSettingsManager() {
        try {
            Log.d(TAG, "验证设置管理器...");
            
            // 测试检测启用/禁用
            settingsManager.setDetectionEnabled(true);
            if (!settingsManager.isDetectionEnabled()) {
                Log.e(TAG, "检测启用设置失败");
                return false;
            }
            
            settingsManager.setDetectionEnabled(false);
            if (settingsManager.isDetectionEnabled()) {
                Log.e(TAG, "检测禁用设置失败");
                return false;
            }
            
            // 测试置信度阈值
            settingsManager.setConfidenceThreshold(0.75f);
            if (Math.abs(settingsManager.getConfidenceThreshold() - 0.75f) > 0.001f) {
                Log.e(TAG, "置信度阈值设置失败");
                return false;
            }
            
            // 测试类别启用/禁用
            settingsManager.setClassEnabled("car", true);
            if (!settingsManager.isClassEnabled("car")) {
                Log.e(TAG, "类别启用设置失败");
                return false;
            }
            
            settingsManager.setClassEnabled("car", false);
            if (settingsManager.isClassEnabled("car")) {
                Log.e(TAG, "类别禁用设置失败");
                return false;
            }
            
            Log.d(TAG, "设置管理器验证通过");
            return true;
            
        } catch (Exception e) {
            Log.e(TAG, "设置管理器验证异常", e);
            return false;
        }
    }
    
    /**
     * 验证检测过滤器功能
     */
    private boolean validateDetectionFilter() {
        try {
            Log.d(TAG, "验证检测过滤器...");
            
            // 重置为已知状态
            settingsManager.resetToDefaults();
            
            // 创建测试检测结果
            List<DetectionResultFilter.DetectionResult> testResults = createTestDetections();
            
            // 测试过滤功能
            List<DetectionResultFilter.DetectionResult> filteredResults = 
                resultFilter.filterResults(testResults);
            
            // 验证只有person类别被保留（默认设置）
            for (DetectionResultFilter.DetectionResult result : filteredResults) {
                if (!"person".equals(result.className)) {
                    Log.e(TAG, "过滤器未正确过滤非person类别: " + result.className);
                    return false;
                }
            }
            
            // 测试置信度过滤
            settingsManager.setConfidenceThreshold(0.8f);
            filteredResults = resultFilter.filterResults(testResults);
            
            for (DetectionResultFilter.DetectionResult result : filteredResults) {
                if (result.confidence < 0.8f) {
                    Log.e(TAG, "过滤器未正确过滤低置信度结果: " + result.confidence);
                    return false;
                }
            }
            
            Log.d(TAG, "检测过滤器验证通过");
            return true;
            
        } catch (Exception e) {
            Log.e(TAG, "检测过滤器验证异常", e);
            return false;
        }
    }
    
    /**
     * 验证数据持久化功能
     */
    private boolean validatePersistence() {
        try {
            Log.d(TAG, "验证数据持久化...");
            
            // 设置一些值
            Set<String> testClasses = new HashSet<>();
            testClasses.add("person");
            testClasses.add("car");
            testClasses.add("bicycle");
            
            settingsManager.setEnabledClasses(testClasses);
            settingsManager.setConfidenceThreshold(0.65f);
            settingsManager.setDetectionEnabled(true);
            
            // 创建新的管理器实例来测试持久化
            DetectionSettingsManager newManager = new DetectionSettingsManager(context);
            
            // 验证设置是否正确保存
            Set<String> savedClasses = newManager.getEnabledClasses();
            if (!savedClasses.equals(testClasses)) {
                Log.e(TAG, "类别设置持久化失败");
                return false;
            }
            
            if (Math.abs(newManager.getConfidenceThreshold() - 0.65f) > 0.001f) {
                Log.e(TAG, "置信度设置持久化失败");
                return false;
            }
            
            if (!newManager.isDetectionEnabled()) {
                Log.e(TAG, "检测启用设置持久化失败");
                return false;
            }
            
            Log.d(TAG, "数据持久化验证通过");
            return true;
            
        } catch (Exception e) {
            Log.e(TAG, "数据持久化验证异常", e);
            return false;
        }
    }
    
    /**
     * 验证类别映射功能
     */
    private boolean validateClassMapping() {
        try {
            Log.d(TAG, "验证类别映射...");
            
            // 验证类别索引映射
            int personIndex = DetectionSettingsManager.getClassIndex("person");
            if (personIndex != 0) {
                Log.e(TAG, "person类别索引错误: " + personIndex);
                return false;
            }
            
            // 验证索引到类别映射
            String className = DetectionSettingsManager.getClassName(0);
            if (!"person".equals(className)) {
                Log.e(TAG, "索引0对应类别错误: " + className);
                return false;
            }
            
            // 验证显示名称映射
            String displayName = DetectionSettingsManager.getDisplayName("person");
            if (!"人员".equals(displayName)) {
                Log.e(TAG, "person显示名称错误: " + displayName);
                return false;
            }
            
            Log.d(TAG, "类别映射验证通过");
            return true;
            
        } catch (Exception e) {
            Log.e(TAG, "类别映射验证异常", e);
            return false;
        }
    }
    
    /**
     * 验证边界条件
     */
    private boolean validateBoundaryConditions() {
        try {
            Log.d(TAG, "验证边界条件...");
            
            // 测试空检测结果
            List<DetectionResultFilter.DetectionResult> emptyResults = new ArrayList<>();
            List<DetectionResultFilter.DetectionResult> filtered = resultFilter.filterResults(emptyResults);
            if (!filtered.isEmpty()) {
                Log.e(TAG, "空结果过滤失败");
                return false;
            }
            
            // 测试null输入
            filtered = resultFilter.filterResults(null);
            if (!filtered.isEmpty()) {
                Log.e(TAG, "null输入过滤失败");
                return false;
            }
            
            // 测试极端置信度值
            settingsManager.setConfidenceThreshold(0.0f);
            settingsManager.setConfidenceThreshold(1.0f);
            
            // 测试无效类别
            int invalidIndex = DetectionSettingsManager.getClassIndex("invalid_class");
            if (invalidIndex != -1) {
                Log.e(TAG, "无效类别索引处理错误");
                return false;
            }
            
            Log.d(TAG, "边界条件验证通过");
            return true;
            
        } catch (Exception e) {
            Log.e(TAG, "边界条件验证异常", e);
            return false;
        }
    }
    
    /**
     * 创建测试检测结果
     */
    private List<DetectionResultFilter.DetectionResult> createTestDetections() {
        List<DetectionResultFilter.DetectionResult> results = new ArrayList<>();
        
        // 添加不同类别和置信度的检测结果
        results.add(new DetectionResultFilter.DetectionResult(0, 0.9f, 10, 10, 100, 200, "person"));
        results.add(new DetectionResultFilter.DetectionResult(2, 0.7f, 200, 50, 300, 150, "car"));
        results.add(new DetectionResultFilter.DetectionResult(1, 0.6f, 400, 100, 500, 200, "bicycle"));
        results.add(new DetectionResultFilter.DetectionResult(0, 0.5f, 600, 150, 700, 250, "person"));
        
        return results;
    }
    
    /**
     * 验证结果数据类
     */
    public static class ValidationResult {
        public boolean settingsManagerValid = false;
        public boolean filterValid = false;
        public boolean persistenceValid = false;
        public boolean classMappingValid = false;
        public boolean boundaryConditionsValid = false;
        public boolean overallValid = false;
        
        @Override
        public String toString() {
            return String.format("ValidationResult{" +
                    "settingsManager=%s, filter=%s, persistence=%s, " +
                    "classMapping=%s, boundaryConditions=%s, overall=%s}",
                    settingsManagerValid, filterValid, persistenceValid,
                    classMappingValid, boundaryConditionsValid, overallValid);
        }
    }
}
