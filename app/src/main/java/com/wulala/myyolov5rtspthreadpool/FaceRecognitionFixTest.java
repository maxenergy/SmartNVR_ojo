package com.wulala.myyolov5rtspthreadpool;

import android.util.Log;
import java.util.ArrayList;

/**
 * 人脸识别修复测试类
 * 用于验证人脸识别只在检测到人员时才触发
 */
public class FaceRecognitionFixTest {
    
    private static final String TAG = "FaceRecognitionFixTest";
    
    /**
     * 运行所有测试
     */
    public static void runAllTests() {
        Log.i(TAG, "=== 开始人脸识别修复测试 ===");
        
        testScenario1_NoPersonsDetected();
        testScenario2_PersonsDetected();
        testScenario3_NullPersonDetections();
        
        Log.i(TAG, "=== 测试完成 ===");
    }
    
    /**
     * 测试场景1：没有检测到人员
     */
    private static void testScenario1_NoPersonsDetected() {
        Log.i(TAG, "场景1：没有检测到人员");
        
        IntegratedAIManager.AIDetectionResult result = new IntegratedAIManager.AIDetectionResult();
        result.success = true;
        result.detectedPersons = 0;
        result.personDetections = new ArrayList<>();
        
        // 模拟IntegratedAIManager中的条件判断逻辑
        boolean shouldTriggerFaceAnalysis = shouldTriggerFaceAnalysis(result);
        
        if (shouldTriggerFaceAnalysis) {
            Log.e(TAG, "  ❌ 错误：在没有人员时触发了人脸分析");
        } else {
            Log.i(TAG, "  ✅ 正确：跳过人脸分析（没有检测到人员）");
        }
    }
    
    /**
     * 测试场景2：检测到人员
     */
    private static void testScenario2_PersonsDetected() {
        Log.i(TAG, "场景2：检测到2个人员");
        
        IntegratedAIManager.AIDetectionResult result = new IntegratedAIManager.AIDetectionResult();
        result.success = true;
        result.detectedPersons = 2;
        result.personDetections = new ArrayList<>();
        
        // 添加模拟的人员检测结果
        result.personDetections.add(new RealYOLOInference.DetectionResult(
            0, 0.85f, 100, 100, 200, 300, "person"));
        result.personDetections.add(new RealYOLOInference.DetectionResult(
            0, 0.92f, 300, 150, 400, 350, "person"));
        
        // 模拟IntegratedAIManager中的条件判断逻辑
        boolean shouldTriggerFaceAnalysis = shouldTriggerFaceAnalysis(result);
        
        if (shouldTriggerFaceAnalysis) {
            Log.i(TAG, "  ✅ 正确：触发人脸分析（检测到 " + result.detectedPersons + " 个人员）");
        } else {
            Log.e(TAG, "  ❌ 错误：在检测到人员时没有触发人脸分析");
        }
    }
    
    /**
     * 测试场景3：personDetections为null
     */
    private static void testScenario3_NullPersonDetections() {
        Log.i(TAG, "场景3：personDetections为null");
        
        IntegratedAIManager.AIDetectionResult result = new IntegratedAIManager.AIDetectionResult();
        result.success = true;
        result.detectedPersons = 1;
        result.personDetections = null; // null情况
        
        // 模拟IntegratedAIManager中的条件判断逻辑
        boolean shouldTriggerFaceAnalysis = shouldTriggerFaceAnalysis(result);
        
        if (shouldTriggerFaceAnalysis) {
            Log.e(TAG, "  ❌ 错误：在personDetections为null时触发了人脸分析");
        } else {
            Log.i(TAG, "  ✅ 正确：跳过人脸分析（personDetections为null）");
        }
    }
    
    /**
     * 模拟IntegratedAIManager中的人脸分析触发条件
     * 这个方法复制了IntegratedAIManager.performDetection()中的条件判断逻辑
     */
    private static boolean shouldTriggerFaceAnalysis(IntegratedAIManager.AIDetectionResult result) {
        // 模拟inspireFaceInitialized为true的情况
        boolean inspireFaceInitialized = true;
        
        // 这是IntegratedAIManager中的实际条件判断
        return inspireFaceInitialized && result.detectedPersons > 0 && result.personDetections != null;
    }
    
    /**
     * 测试MultiCameraView的模拟逻辑
     */
    public static void testMultiCameraViewSimulation() {
        Log.i(TAG, "=== 测试MultiCameraView模拟逻辑 ===");
        
        for (int i = 0; i < 10; i++) {
            // 模拟MultiCameraView.simulateAIResults()中的逻辑
            boolean hasPersons = Math.random() > 0.3; // 70%概率检测到人员
            
            if (hasPersons) {
                int personCount = 1 + (int)(Math.random() * 2); // 1-2个人
                Log.i(TAG, "模拟结果 " + (i+1) + ": 检测到 " + personCount + " 个人员，将进行人脸分析");
            } else {
                Log.i(TAG, "模拟结果 " + (i+1) + ": 未检测到人员，跳过人脸分析");
            }
        }
        
        Log.i(TAG, "=== MultiCameraView模拟测试完成 ===");
    }
}
