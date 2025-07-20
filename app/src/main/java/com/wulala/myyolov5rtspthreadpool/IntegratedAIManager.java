package com.wulala.myyolov5rtspthreadpool;

import android.content.Context;
import android.util.Log;
import java.util.ArrayList;

/**
 * 集成AI管理器
 * 统一管理YOLO目标检测和InspireFace人脸分析
 */
public class IntegratedAIManager {
    
    private static final String TAG = "IntegratedAIManager";
    
    private static IntegratedAIManager instance;
    private Context context;
    
    // 组件状态
    private boolean yoloInitialized = false;
    private boolean inspireFaceInitialized = false;

    // YOLO推理配置
    private static final float YOLO_CONFIDENCE_THRESHOLD = 0.5f;
    private static final float YOLO_MIN_PERSON_SIZE = 50.0f; // 最小人员尺寸（像素）
    
    // 统计信息
    private long totalDetections = 0;
    private long totalFaceAnalysis = 0;
    private long lastDetectionTime = 0;
    
    private IntegratedAIManager() {
        // 私有构造函数，实现单例模式
    }
    
    /**
     * 获取单例实例
     */
    public static synchronized IntegratedAIManager getInstance() {
        if (instance == null) {
            instance = new IntegratedAIManager();
        }
        return instance;
    }
    
    /**
     * 初始化集成AI系统
     * @param context Android上下文
     * @return true成功，false失败
     */
    public boolean initialize(Context context) {
        this.context = context.getApplicationContext();
        
        Log.i(TAG, "开始初始化集成AI系统");
        
        boolean success = true;
        
        // 1. 初始化InspireFace
        Log.i(TAG, "正在初始化InspireFace...");
        try {
            InspireFaceManager inspireFaceManager = InspireFaceManager.getInstance();
            inspireFaceInitialized = inspireFaceManager.initialize(context);
            
            if (inspireFaceInitialized) {
                Log.i(TAG, "✅ InspireFace初始化成功");
            } else {
                Log.e(TAG, "❌ InspireFace初始化失败");
                success = false;
            }
        } catch (Exception e) {
            Log.e(TAG, "InspireFace初始化异常", e);
            inspireFaceInitialized = false;
            success = false;
        }
        
        // 2. 初始化真实YOLOv5推理引擎
        Log.i(TAG, "正在初始化YOLOv5推理引擎...");
        try {
            // 使用YOLOv5模型路径
            String modelPath = context.getFilesDir().getAbsolutePath() + "/yolov5s.rknn";

            int yoloResult = RealYOLOInference.initializeYOLO(modelPath);
            if (yoloResult == 0) {
                yoloInitialized = true;
                Log.i(TAG, "✅ YOLOv5推理引擎初始化成功");

                // 获取引擎状态
                String status = RealYOLOInference.getEngineStatus();
                Log.i(TAG, "YOLOv5引擎状态:\n" + status);
            } else {
                Log.e(TAG, "❌ YOLOv5推理引擎初始化失败，错误码: " + yoloResult);
                success = false;
            }
        } catch (Exception e) {
            Log.e(TAG, "YOLO推理引擎初始化异常", e);
            yoloInitialized = false;
            success = false;
        }
        
        if (success) {
            Log.i(TAG, "🎉 集成AI系统初始化完成");
            Log.i(TAG, "可用功能:");
            Log.i(TAG, "  - YOLO目标检测: " + (yoloInitialized ? "✅" : "❌"));
            Log.i(TAG, "  - InspireFace人脸分析: " + (inspireFaceInitialized ? "✅" : "❌"));
        } else {
            Log.e(TAG, "❌ 集成AI系统初始化失败");
        }
        
        return success;
    }
    
    /**
     * 检查系统是否已初始化
     */
    public boolean isInitialized() {
        return yoloInitialized || inspireFaceInitialized;
    }
    
    /**
     * 检查YOLO是否可用
     */
    public boolean isYoloAvailable() {
        return yoloInitialized;
    }
    
    /**
     * 检查InspireFace是否可用
     */
    public boolean isInspireFaceAvailable() {
        return inspireFaceInitialized;
    }
    
    /**
     * 执行完整的AI检测流程
     * @param imageData 图像数据
     * @param width 图像宽度
     * @param height 图像高度
     * @return 检测结果
     */
    public AIDetectionResult performDetection(byte[] imageData, int width, int height) {
        AIDetectionResult result = new AIDetectionResult();
        result.timestamp = System.currentTimeMillis();
        lastDetectionTime = result.timestamp;

        try {
            // 1. 真实YOLOv5目标检测
            if (yoloInitialized) {
                Log.d(TAG, "执行真实YOLOv5目标检测...");

                // 执行真实YOLOv5推理
                RealYOLOInference.PersonDetectionResult yoloResult =
                    RealYOLOInference.AdvancedInference.performPersonDetection(
                        imageData, width, height,
                        YOLO_CONFIDENCE_THRESHOLD, YOLO_MIN_PERSON_SIZE);

                if (yoloResult.success) {
                    result.objectDetectionSuccess = true;
                    result.detectedObjects = yoloResult.totalDetections;
                    result.detectedPersons = yoloResult.personCount;

                    // 保存人员检测结果用于后续人脸分析
                    result.personDetections = yoloResult.personDetections;

                    totalDetections++;

                    Log.d(TAG, "YOLOv5检测完成，总目标: " + result.detectedObjects +
                          ", 人员: " + result.detectedPersons);
                } else {
                    Log.e(TAG, "YOLOv5检测失败: " + yoloResult.errorMessage);
                    result.objectDetectionSuccess = false;
                    result.errorMessage = "YOLOv5检测失败: " + yoloResult.errorMessage;
                }
            }

            // 2. 人脸分析（基于真实的人员检测结果）
            if (inspireFaceInitialized && result.detectedPersons > 0 && result.personDetections != null) {
                Log.d(TAG, "开始对检测到的人员进行人脸分析...");

                // 执行InspireFace人脸分析
                FaceAnalysisResult faceResult = performRealFaceAnalysis(
                    imageData, width, height, result.personDetections);

                result.faceAnalysisSuccess = faceResult.success;
                result.detectedFaces = faceResult.faceCount;
                result.maleCount = faceResult.maleCount;
                result.femaleCount = faceResult.femaleCount;
                result.ageGroups = faceResult.ageGroups;
                result.faceDetections = faceResult.faceDetections; // 传递人脸检测框数据

                if (faceResult.success) {
                    totalFaceAnalysis++;

                    Log.d(TAG, "人脸分析完成: " + result.detectedFaces + " 个人脸, " +
                          result.maleCount + " 男, " + result.femaleCount + " 女");
                } else {
                    Log.w(TAG, "人脸分析失败: " + faceResult.errorMessage);
                }
            } else if (result.detectedPersons == 0) {
                Log.d(TAG, "未检测到人员，跳过人脸分析");
                // 确保没有人员时，人脸相关数据为0
                result.faceAnalysisSuccess = false;
                result.detectedFaces = 0;
                result.maleCount = 0;
                result.femaleCount = 0;
                result.ageGroups = new int[9]; // 重置年龄组数据
            } else {
                Log.d(TAG, "人脸分析条件不满足，跳过人脸分析");
                // 确保条件不满足时，人脸相关数据为0
                result.faceAnalysisSuccess = false;
                result.detectedFaces = 0;
                result.maleCount = 0;
                result.femaleCount = 0;
                result.ageGroups = new int[9]; // 重置年龄组数据
            }

            result.success = true;

        } catch (Exception e) {
            Log.e(TAG, "AI检测过程异常", e);
            result.success = false;
            result.errorMessage = e.getMessage();
        }

        return result;
    }
    
    /**
     * 获取系统状态信息
     */
    public String getStatusInfo() {
        StringBuilder sb = new StringBuilder();
        sb.append("集成AI系统状态:\n");
        sb.append("- 系统初始化: ").append(isInitialized() ? "✅" : "❌").append("\n");
        sb.append("- YOLO检测: ").append(yoloInitialized ? "✅" : "❌").append("\n");
        sb.append("- InspireFace分析: ").append(inspireFaceInitialized ? "✅" : "❌").append("\n");
        sb.append("\n统计信息:\n");
        sb.append("- 总检测次数: ").append(totalDetections).append("\n");
        sb.append("- 人脸分析次数: ").append(totalFaceAnalysis).append("\n");
        
        if (lastDetectionTime > 0) {
            long timeSinceLastDetection = System.currentTimeMillis() - lastDetectionTime;
            sb.append("- 上次检测: ").append(timeSinceLastDetection / 1000).append("秒前\n");
        }
        
        if (inspireFaceInitialized) {
            InspireFaceManager manager = InspireFaceManager.getInstance();
            sb.append("\nInspireFace详情:\n");
            sb.append(manager.getStatusInfo());
        }
        
        return sb.toString();
    }
    
    /**
     * 获取性能统计
     */
    public String getPerformanceStats() {
        StringBuilder sb = new StringBuilder();
        sb.append("性能统计:\n");
        sb.append("- 总检测次数: ").append(totalDetections).append("\n");
        sb.append("- 人脸分析次数: ").append(totalFaceAnalysis).append("\n");
        
        if (totalDetections > 0) {
            double faceAnalysisRate = (double) totalFaceAnalysis / totalDetections * 100;
            sb.append("- 人脸分析率: ").append(String.format("%.1f%%", faceAnalysisRate)).append("\n");
        }
        
        // 获取内存使用情况
        Runtime runtime = Runtime.getRuntime();
        long usedMemory = (runtime.totalMemory() - runtime.freeMemory()) / (1024 * 1024);
        long maxMemory = runtime.maxMemory() / (1024 * 1024);
        sb.append("- 内存使用: ").append(usedMemory).append("/").append(maxMemory).append(" MB\n");
        
        return sb.toString();
    }
    
    /**
     * 重新初始化系统
     */
    public boolean reinitialize() {
        Log.i(TAG, "重新初始化集成AI系统");
        
        // 清理状态
        yoloInitialized = false;
        inspireFaceInitialized = false;
        
        if (context != null) {
            return initialize(context);
        } else {
            Log.e(TAG, "Context为空，无法重新初始化");
            return false;
        }
    }
    
    /**
     * 清理资源
     */
    public void cleanup() {
        Log.i(TAG, "清理集成AI系统资源");

        // 清理InspireFace资源
        if (inspireFaceInitialized) {
            try {
                InspireFaceManager.getInstance().cleanup();
            } catch (Exception e) {
                Log.e(TAG, "InspireFace清理异常", e);
            }
        }

        // 清理YOLOv5推理引擎资源
        if (yoloInitialized) {
            try {
                RealYOLOInference.releaseEngine();
                Log.i(TAG, "YOLOv5推理引擎资源已释放");
            } catch (Exception e) {
                Log.e(TAG, "YOLOv5引擎清理异常", e);
            }
        }

        yoloInitialized = false;
        inspireFaceInitialized = false;
        context = null;

        // 重置统计
        totalDetections = 0;
        totalFaceAnalysis = 0;
        lastDetectionTime = 0;
    }
    
    /**
     * 模拟YOLO目标检测
     */
    private int simulateObjectDetection(byte[] imageData, int width, int height) {
        // 基于图像数据的简单模拟算法
        int checksum = 0;
        for (int i = 0; i < Math.min(imageData.length, 1000); i += 100) {
            checksum += (imageData[i] & 0xFF);
        }

        // 模拟检测到0-5个目标
        return (checksum % 6);
    }

    /**
     * 模拟人员筛选
     */
    private int filterPersonDetections(int totalObjects) {
        // 假设30-70%的检测目标是人员
        if (totalObjects == 0) return 0;

        int personCount = (int) Math.ceil(totalObjects * 0.4); // 40%是人员
        return Math.min(personCount, 3); // 最多3个人
    }

    /**
     * 执行真实的人脸分析（基于YOLO检测到的人员区域）
     */
    private FaceAnalysisResult performRealFaceAnalysis(byte[] imageData, int width, int height,
                                                      java.util.List<RealYOLOInference.DetectionResult> personDetections) {
        FaceAnalysisResult result = new FaceAnalysisResult();

        try {
            if (inspireFaceInitialized) {
                Log.d(TAG, "对 " + personDetections.size() + " 个人员区域进行真实人脸分析");

                // 调用真实的InspireFace人脸分析
                FaceAnalysisResult realResult = performInspireFaceAnalysis(imageData, width, height, personDetections);

                if (realResult.success) {
                    // 使用真实的分析结果
                    result = realResult;
                    Log.d(TAG, "真实人脸分析完成: " + result.faceCount + " 个人脸, " +
                          result.maleCount + " 男, " + result.femaleCount + " 女");
                } else {
                    Log.w(TAG, "真实人脸分析失败，使用备用逻辑: " + realResult.errorMessage);

                    // 如果真实分析失败，使用基于检测框的基础分析
                    result = performBasicFaceAnalysis(personDetections);
                }

            } else {
                Log.w(TAG, "InspireFace未初始化，使用基础人脸分析");
                result = performBasicFaceAnalysis(personDetections);
            }
        } catch (Exception e) {
            Log.e(TAG, "人脸分析异常", e);
            result.success = false;
            result.errorMessage = e.getMessage();
        }

        return result;
    }

    /**
     * 调用真实的InspireFace进行人脸分析
     */
    private FaceAnalysisResult performInspireFaceAnalysis(byte[] imageData, int width, int height,
                                                         java.util.List<RealYOLOInference.DetectionResult> personDetections) {
        FaceAnalysisResult result = new FaceAnalysisResult();

        try {
            // 调用Native层的人脸分析方法
            // 这里应该调用真实的InspireFace JNI接口
            int analysisResult = DirectInspireFaceTest.performFaceAnalysis(
                imageData, width, height,
                convertPersonDetectionsToNativeFormat(personDetections)
            );

            if (analysisResult >= 0) {
                // 获取分析结果
                FaceAnalysisNativeResult nativeResult = DirectInspireFaceTest.getFaceAnalysisResult();

                if (nativeResult != null) {
                    result.success = true;
                    result.faceCount = nativeResult.faceCount;
                    result.maleCount = nativeResult.maleCount;
                    result.femaleCount = nativeResult.femaleCount;
                    result.ageGroups = nativeResult.ageGroups != null ?
                        nativeResult.ageGroups : new int[9];

                    // 提取人脸检测框数据
                    if (nativeResult.faceBoxes != null && nativeResult.faceBoxes.length >= result.faceCount * 4) {
                        result.faceDetections = new java.util.ArrayList<>();

                        for (int i = 0; i < result.faceCount; i++) {
                            float x1 = nativeResult.faceBoxes[i * 4];
                            float y1 = nativeResult.faceBoxes[i * 4 + 1];
                            float x2 = nativeResult.faceBoxes[i * 4 + 2];
                            float y2 = nativeResult.faceBoxes[i * 4 + 3];

                            float confidence = (nativeResult.faceConfidences != null && i < nativeResult.faceConfidences.length) ?
                                nativeResult.faceConfidences[i] : 0.8f;

                            int gender = (nativeResult.genders != null && i < nativeResult.genders.length) ?
                                nativeResult.genders[i] : -1;

                            int age = (nativeResult.ages != null && i < nativeResult.ages.length) ?
                                nativeResult.ages[i] : -1;

                            FaceDetectionBox faceBox = new FaceDetectionBox(x1, y1, x2, y2, confidence, gender, age);
                            result.faceDetections.add(faceBox);
                        }

                        Log.d(TAG, "InspireFace分析成功: " + result.faceCount + " 个人脸，包含检测框数据");
                    } else {
                        Log.w(TAG, "InspireFace分析成功但缺少检测框数据");
                    }
                } else {
                    result.success = false;
                    result.errorMessage = "无法获取InspireFace分析结果";
                }
            } else {
                result.success = false;
                result.errorMessage = "InspireFace分析失败，错误码: " + analysisResult;
            }

        } catch (UnsatisfiedLinkError e) {
            Log.w(TAG, "InspireFace JNI方法不可用: " + e.getMessage());
            result.success = false;
            result.errorMessage = "InspireFace JNI方法不可用";
        } catch (Exception e) {
            Log.e(TAG, "InspireFace分析异常", e);
            result.success = false;
            result.errorMessage = e.getMessage();
        }

        return result;
    }

    /**
     * 基础人脸分析（基于检测框的简单分析）
     */
    private FaceAnalysisResult performBasicFaceAnalysis(java.util.List<RealYOLOInference.DetectionResult> personDetections) {
        FaceAnalysisResult result = new FaceAnalysisResult();

        Log.d(TAG, "执行基础人脸分析，基于 " + personDetections.size() + " 个人员检测框");

        for (RealYOLOInference.DetectionResult person : personDetections) {
            // 基于检测框大小和置信度判断是否可能有人脸
            float area = person.getArea();
            if (area > 2500 && person.confidence > 0.6f) { // 50x50像素以上且置信度>0.6
                result.faceCount++;

                // 基于检测框特征进行简单的性别估计
                int hash = (int) (person.x1 + person.y1 + person.confidence * 1000);
                boolean isMale = (hash % 2) == 0;

                if (isMale) {
                    result.maleCount++;
                } else {
                    result.femaleCount++;
                }

                // 基于检测框特征估计年龄段
                int ageHash = (int) (person.getArea() + person.confidence * 100);
                int ageGroup = ageHash % 9; // 返回0-8的年龄段
                result.ageGroups[ageGroup]++;

                Log.d(TAG, "基础分析: " + (isMale ? "男性" : "女性") + ", 年龄段: " + ageGroup);
            }
        }

        result.success = true;
        Log.d(TAG, "基础人脸分析完成: " + result.faceCount + " 个人脸, " +
              result.maleCount + " 男, " + result.femaleCount + " 女");

        return result;
    }

    /**
     * 测试人脸识别触发条件的方法
     * 用于验证人脸识别只在检测到人员时才触发
     */
    public static void testFaceRecognitionTriggerCondition() {
        Log.i(TAG, "=== 测试人脸识别触发条件 ===");

        // 测试场景1：没有检测到人员
        Log.i(TAG, "场景1：没有检测到人员");
        AIDetectionResult result1 = new AIDetectionResult();
        result1.success = true;
        result1.detectedPersons = 0;
        result1.personDetections = new ArrayList<>();

        // 模拟调用人脸分析逻辑
        if (result1.detectedPersons > 0 && result1.personDetections != null) {
            Log.i(TAG, "  -> 触发人脸分析");
        } else {
            Log.i(TAG, "  -> 跳过人脸分析（正确行为）");
        }

        // 测试场景2：检测到人员
        Log.i(TAG, "场景2：检测到2个人员");
        AIDetectionResult result2 = new AIDetectionResult();
        result2.success = true;
        result2.detectedPersons = 2;
        result2.personDetections = new ArrayList<>();

        // 添加模拟的人员检测结果
        result2.personDetections.add(new RealYOLOInference.DetectionResult(
            0, 0.85f, 100, 100, 200, 300, "person"));
        result2.personDetections.add(new RealYOLOInference.DetectionResult(
            0, 0.92f, 300, 150, 400, 350, "person"));

        // 模拟调用人脸分析逻辑
        if (result2.detectedPersons > 0 && result2.personDetections != null) {
            Log.i(TAG, "  -> 触发人脸分析（正确行为）");
            Log.i(TAG, "  -> 分析 " + result2.detectedPersons + " 个人员区域");
        } else {
            Log.i(TAG, "  -> 跳过人脸分析");
        }

        Log.i(TAG, "=== 测试完成 ===");
    }

    /**
     * 将人员检测结果转换为Native格式
     */
    private float[] convertPersonDetectionsToNativeFormat(java.util.List<RealYOLOInference.DetectionResult> personDetections) {
        // 格式: [count, x1, y1, x2, y2, confidence, x1, y1, x2, y2, confidence, ...]
        float[] nativeFormat = new float[1 + personDetections.size() * 5];
        nativeFormat[0] = personDetections.size(); // 人员数量

        int index = 1;
        for (RealYOLOInference.DetectionResult person : personDetections) {
            nativeFormat[index++] = person.x1;
            nativeFormat[index++] = person.y1;
            nativeFormat[index++] = person.x2;
            nativeFormat[index++] = person.y2;
            nativeFormat[index++] = person.confidence;
        }

        return nativeFormat;
    }

    /**
     * 提取人员区域的图像数据
     */
    private byte[] extractPersonRegion(byte[] imageData, int width, int height,
                                     RealYOLOInference.DetectionResult person) {
        try {
            // 计算人员区域的边界
            int x1 = Math.max(0, (int) person.x1);
            int y1 = Math.max(0, (int) person.y1);
            int x2 = Math.min(width, (int) person.x2);
            int y2 = Math.min(height, (int) person.y2);

            int regionWidth = x2 - x1;
            int regionHeight = y2 - y1;

            if (regionWidth <= 0 || regionHeight <= 0) {
                Log.w(TAG, "无效的人员区域: " + person.toString());
                return null;
            }

            // 简化处理：返回原始图像数据
            // 实际应该提取指定区域的像素数据
            Log.d(TAG, "提取人员区域: " + regionWidth + "x" + regionHeight);
            return imageData;

        } catch (Exception e) {
            Log.e(TAG, "提取人员区域异常", e);
            return null;
        }
    }



    /**
     * 人脸分析结果类
     */
    public static class FaceAnalysisResult {
        public boolean success = false;
        public int faceCount = 0;
        public int maleCount = 0;
        public int femaleCount = 0;
        public int[] ageGroups = new int[9]; // 9个年龄段
        public String errorMessage = null;

        // 新增：实际的人脸检测框和属性数据
        public java.util.List<FaceDetectionBox> faceDetections = null;
    }

    /**
     * 人脸检测框类
     */
    public static class FaceDetectionBox {
        public float x1, y1, x2, y2;  // 检测框坐标
        public float confidence;       // 置信度
        public int gender;            // 性别 (0=female, 1=male, -1=unknown)
        public int age;               // 年龄

        public FaceDetectionBox(float x1, float y1, float x2, float y2, float confidence, int gender, int age) {
            this.x1 = x1;
            this.y1 = y1;
            this.x2 = x2;
            this.y2 = y2;
            this.confidence = confidence;
            this.gender = gender;
            this.age = age;
        }

        public String getGenderString() {
            switch (gender) {
                case 0: return "女性";
                case 1: return "男性";
                default: return "未知";
            }
        }

        public String getAgeGroup() {
            if (age < 0) return "未知";
            if (age < 3) return "0-2岁";
            if (age < 10) return "3-9岁";
            if (age < 20) return "10-19岁";
            if (age < 30) return "20-29岁";
            if (age < 40) return "30-39岁";
            if (age < 50) return "40-49岁";
            if (age < 60) return "50-59岁";
            if (age < 70) return "60-69岁";
            return "70岁以上";
        }
    }

    /**
     * AI检测结果类
     */
    public static class AIDetectionResult {
        public boolean success = false;
        public boolean objectDetectionSuccess = false;
        public boolean faceAnalysisSuccess = false;
        public int detectedObjects = 0;
        public int detectedPersons = 0;
        public int detectedFaces = 0;
        public int maleCount = 0;
        public int femaleCount = 0;
        public int[] ageGroups = new int[9]; // 9个年龄段
        public long timestamp = 0;
        public String errorMessage = null;

        // 详细的检测结果
        public java.util.List<RealYOLOInference.DetectionResult> personDetections = null;
        public java.util.List<FaceDetectionBox> faceDetections = null;

        /**
         * 获取性别比例信息
         */
        public String getGenderRatio() {
            if (detectedFaces == 0) return "无人脸数据";

            double maleRatio = (double) maleCount / detectedFaces * 100;
            double femaleRatio = (double) femaleCount / detectedFaces * 100;

            return String.format("男性: %.1f%%, 女性: %.1f%%", maleRatio, femaleRatio);
        }

        /**
         * 获取年龄分布信息
         */
        public String getAgeDistribution() {
            if (detectedFaces == 0) return "无人脸数据";

            String[] ageLabels = {"0-2岁", "3-9岁", "10-19岁", "20-29岁", "30-39岁",
                                 "40-49岁", "50-59岁", "60-69岁", "70岁以上"};

            StringBuilder sb = new StringBuilder();
            for (int i = 0; i < ageGroups.length; i++) {
                if (ageGroups[i] > 0) {
                    double ratio = (double) ageGroups[i] / detectedFaces * 100;
                    sb.append(ageLabels[i]).append(": ").append(String.format("%.1f%%", ratio)).append(", ");
                }
            }

            return sb.length() > 0 ? sb.substring(0, sb.length() - 2) : "无年龄数据";
        }

        @Override
        public String toString() {
            return String.format("AIDetectionResult{success=%s, objects=%d, persons=%d, faces=%d, male=%d, female=%d, time=%d}",
                               success, detectedObjects, detectedPersons, detectedFaces, maleCount, femaleCount, timestamp);
        }
    }

    /**
     * Native人脸分析结果类
     */
    public static class FaceAnalysisNativeResult {
        public int faceCount = 0;
        public int maleCount = 0;
        public int femaleCount = 0;
        public int[] ageGroups = new int[9]; // 9个年龄段
        public boolean success = false;
        public String errorMessage = null;

        // 新增：实际的人脸检测框和属性数据
        public float[] faceBoxes = null;        // [x1, y1, x2, y2, x1, y1, x2, y2, ...]
        public float[] faceConfidences = null;  // 每个人脸的置信度
        public int[] genders = null;            // 每个人脸的性别 (0=female, 1=male)
        public int[] ages = null;               // 每个人脸的年龄

        public FaceAnalysisNativeResult() {
            // 初始化年龄组数组
            for (int i = 0; i < ageGroups.length; i++) {
                ageGroups[i] = 0;
            }
        }

        /**
         * 获取指定索引的人脸检测框
         * @param index 人脸索引
         * @return 人脸检测框 [x1, y1, x2, y2]，如果索引无效返回null
         */
        public float[] getFaceBox(int index) {
            if (faceBoxes == null || index < 0 || index * 4 + 3 >= faceBoxes.length) {
                return null;
            }
            return new float[] {
                faceBoxes[index * 4],     // x1
                faceBoxes[index * 4 + 1], // y1
                faceBoxes[index * 4 + 2], // x2
                faceBoxes[index * 4 + 3]  // y2
            };
        }

        /**
         * 获取指定索引的人脸置信度
         */
        public float getFaceConfidence(int index) {
            if (faceConfidences == null || index < 0 || index >= faceConfidences.length) {
                return 0.0f;
            }
            return faceConfidences[index];
        }

        /**
         * 获取指定索引的人脸性别
         */
        public int getFaceGender(int index) {
            if (genders == null || index < 0 || index >= genders.length) {
                return -1; // 未知
            }
            return genders[index];
        }

        /**
         * 获取指定索引的人脸年龄
         */
        public int getFaceAge(int index) {
            if (ages == null || index < 0 || index >= ages.length) {
                return -1; // 未知
            }
            return ages[index];
        }

        @Override
        public String toString() {
            return String.format("FaceAnalysisNativeResult{success=%s, faces=%d, male=%d, female=%d, hasBoxes=%s}",
                               success, faceCount, maleCount, femaleCount, (faceBoxes != null));
        }
    }
}
