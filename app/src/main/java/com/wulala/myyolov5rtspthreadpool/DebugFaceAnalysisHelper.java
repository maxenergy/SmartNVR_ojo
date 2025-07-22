package com.wulala.myyolov5rtspthreadpool;

import android.content.Context;
import android.util.Log;

/**
 * 🔧 人脸分析调试助手
 * 用于诊断和解决人脸分析不显示的问题
 */
public class DebugFaceAnalysisHelper {
    private static final String TAG = "DebugFaceAnalysis";
    
    /**
     * 执行完整的人脸分析诊断
     */
    public static void runDiagnostics(Context context) {
        Log.i(TAG, "=== 开始人脸分析诊断 ===");
        
        // 1. 检查模型文件
        checkModelFiles(context);
        
        // 2. 检查InspireFace初始化
        checkInspireFaceInitialization(context);
        
        // 3. 检查YOLO检测功能
        checkYOLODetection(context);
        
        // 4. 检查人脸分析触发条件
        checkFaceAnalysisTrigger();
        
        // 5. 检查统计数据获取
        checkStatisticsRetrieval();
        
        Log.i(TAG, "=== 人脸分析诊断完成 ===");
    }
    
    /**
     * 检查模型文件
     */
    private static void checkModelFiles(Context context) {
        Log.i(TAG, "--- 模型文件检查 ---");
        
        try {
            // 检查InspireFace模型
            String internalDataPath = context.getFilesDir().getAbsolutePath();
            int modelCheck = DirectInspireFaceTest.testModelValidation(internalDataPath);
            
            if (modelCheck == 0) {
                Log.i(TAG, "✅ InspireFace模型文件验证通过");
            } else {
                Log.e(TAG, "❌ InspireFace模型文件验证失败，错误码: " + modelCheck);
            }
            
            // 检查YOLO模型
            String yoloModelPath = internalDataPath + "/yolov5s.rknn";
            java.io.File yoloFile = new java.io.File(yoloModelPath);
            if (yoloFile.exists()) {
                Log.i(TAG, "✅ YOLO模型文件存在: " + yoloFile.length() + " bytes");
            } else {
                Log.e(TAG, "❌ YOLO模型文件缺失: " + yoloModelPath);
            }
            
        } catch (Exception e) {
            Log.e(TAG, "模型文件检查异常", e);
        }
    }
    
    /**
     * 检查InspireFace初始化
     */
    private static void checkInspireFaceInitialization(Context context) {
        Log.i(TAG, "--- InspireFace初始化检查 ---");
        
        try {
            // 获取InspireFace信息
            String info = DirectInspireFaceTest.getInspireFaceInfo();
            Log.i(TAG, "InspireFace信息: " + info);
            
            // 测试直接初始化
            android.content.res.AssetManager assetManager = context.getAssets();
            String internalDataPath = context.getFilesDir().getAbsolutePath();
            
            int initResult = DirectInspireFaceTest.testInspireFaceInit(assetManager, internalDataPath);
            if (initResult == 0) {
                Log.i(TAG, "✅ InspireFace直接初始化成功");
            } else {
                Log.e(TAG, "❌ InspireFace直接初始化失败，错误码: " + initResult);
            }
            
        } catch (Exception e) {
            Log.e(TAG, "InspireFace初始化检查异常", e);
        }
    }
    
    /**
     * 检查YOLO检测功能
     */
    private static void checkYOLODetection(Context context) {
        Log.i(TAG, "--- YOLO检测功能检查 ---");
        
        try {
            // 检查YOLO引擎状态
            String status = RealYOLOInference.getEngineStatus();
            Log.i(TAG, "YOLO引擎状态: " + status);
            
            // 检查集成AI管理器状态
            IntegratedAIManager aiManager = IntegratedAIManager.getInstance();
            Log.i(TAG, "AI管理器状态: " + aiManager.getStatusInfo());
            
        } catch (Exception e) {
            Log.e(TAG, "YOLO检测功能检查异常", e);
        }
    }
    
    /**
     * 检查人脸分析触发条件
     */
    private static void checkFaceAnalysisTrigger() {
        Log.i(TAG, "--- 人脸分析触发条件检查 ---");
        
        try {
            // 测试触发条件
            IntegratedAIManager.testFaceRecognitionTriggerCondition();
            
        } catch (Exception e) {
            Log.e(TAG, "人脸分析触发条件检查异常", e);
        }
    }
    
    /**
     * 检查统计数据获取
     */
    private static void checkStatisticsRetrieval() {
        Log.i(TAG, "--- 统计数据获取检查 ---");
        
        try {
            // 获取C++层统计数据
            BatchStatisticsResult stats = DirectInspireFaceTest.getCurrentStatistics();
            
            if (stats != null && stats.success) {
                Log.i(TAG, "✅ 统计数据获取成功: " + stats.formatForDisplay());
                Log.i(TAG, "详细统计: " + stats.getDetailedInfo());
            } else {
                Log.e(TAG, "❌ 统计数据获取失败");
                Log.e(TAG, "统计错误: " + (stats != null ? stats.errorMessage : "null"));
            }
            
            // 重置统计数据
            DirectInspireFaceTest.resetStatistics();
            Log.i(TAG, "统计数据已重置");
            
        } catch (Exception e) {
            Log.e(TAG, "统计数据获取检查异常", e);
        }
    }
    
    /**
     * 测试人脸分析流程
     */
    public static void testFaceAnalysisFlow(Context context, byte[] testImageData, int width, int height) {
        Log.i(TAG, "=== 测试人脸分析流程 ===");
        
        try {
            // 创建模拟人员检测结果
            float[] mockPersonDetections = {
                2,  // 2个人员
                100, 100, 200, 250, 0.95f,  // 第1个人员
                300, 150, 400, 350, 0.87f   // 第2个人员
            };
            
            // 执行人脸分析
            int analysisResult = DirectInspireFaceTest.performFaceAnalysis(
                testImageData, width, height, mockPersonDetections);
            
            if (analysisResult == 0) {
                Log.i(TAG, "✅ 人脸分析执行成功");
                
                // 获取分析结果
                IntegratedAIManager.FaceAnalysisNativeResult result = 
                    DirectInspireFaceTest.getFaceAnalysisResult();
                
                if (result != null && result.success) {
                    Log.i(TAG, "✅ 人脸分析结果获取成功");
                    Log.i(TAG, "检测到: " + result.faceCount + " 个人脸");
                    Log.i(TAG, "男性: " + result.maleCount + " 人");
                    Log.i(TAG, "女性: " + result.femaleCount + " 人");
                } else {
                    Log.e(TAG, "❌ 人脸分析结果获取失败");
                }
            } else {
                Log.e(TAG, "❌ 人脸分析执行失败，错误码: " + analysisResult);
            }
            
        } catch (Exception e) {
            Log.e(TAG, "人脸分析流程测试异常", e);
        }
    }
    
    /**
     * 获取诊断报告
     */
    public static String getDiagnosticReport(Context context) {
        StringBuilder report = new StringBuilder();
        report.append("=== 人脸分析诊断报告 ===\n\n");
        
        try {
            // 系统状态
            report.append("系统状态:\n");
            report.append(IntegratedAIManager.getInstance().getStatusInfo()).append("\n\n");
            
            // 性能统计
            report.append("性能统计:\n");
            report.append(IntegratedAIManager.getInstance().getPerformanceStats()).append("\n\n");
            
            // 模型文件状态
            try {
                String modelInfo = InspireFaceManager.getInstance().getDetailedModelInfo();
                report.append("模型文件:\n").append(modelInfo).append("\n\n");
            } catch (Exception e) {
                report.append("模型文件: 获取失败 - ").append(e.getMessage()).append("\n\n");
            }
            
            // 统计数据
            try {
                BatchStatisticsResult stats = DirectInspireFaceTest.getCurrentStatistics();
                if (stats != null && stats.success) {
                    report.append("当前统计:\n").append(stats.getDetailedInfo()).append("\n\n");
                } else {
                    report.append("当前统计: 无数据\n\n");
                }
            } catch (Exception e) {
                report.append("当前统计: 获取失败 - ").append(e.getMessage()).append("\n\n");
            }
            
        } catch (Exception e) {
            report.append("诊断报告生成异常: ").append(e.getMessage());
        }
        
        return report.toString();
    }
}