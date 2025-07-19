package com.wulala.myyolov5rtspthreadpool;

/**
 * 扩展推理JNI接口
 * 提供目标检测 + 人脸分析 + 统计功能的统一接口
 */
public class ExtendedInferenceJNI {
    
    static {
        System.loadLibrary("myyolov5rtspthreadpool");
    }
    
    // ==================== 数据结构定义 ====================
    
    /**
     * 人脸属性信息
     */
    public static class FaceAttributes {
        public int gender;              // 0: 女性, 1: 男性, -1: 未知
        public float genderConfidence;
        public int ageBracket;          // 年龄段 0-8, -1: 未知
        public float ageConfidence;
        public int race;                // 种族 0-4, -1: 未知
        public float raceConfidence;
        
        public String getGenderString() {
            switch (gender) {
                case 0: return "女性";
                case 1: return "男性";
                default: return "未知";
            }
        }
        
        public String getAgeBracketString() {
            String[] ageLabels = {
                "0-2岁", "3-9岁", "10-19岁", "20-29岁", "30-39岁",
                "40-49岁", "50-59岁", "60-69岁", "70岁以上"
            };
            if (ageBracket >= 0 && ageBracket < ageLabels.length) {
                return ageLabels[ageBracket];
            }
            return "未知";
        }
        
        public String getRaceString() {
            String[] raceLabels = {"黑人", "亚洲人", "拉丁裔", "中东人", "白人"};
            if (race >= 0 && race < raceLabels.length) {
                return raceLabels[race];
            }
            return "未知";
        }
        
        public boolean isValid() {
            return gender >= 0 && ageBracket >= 0;
        }
    }
    
    /**
     * 人脸信息
     */
    public static class FaceInfo {
        public float[] faceRect;        // [x, y, width, height] 人脸边界框
        public float confidence;        // 检测置信度
        public FaceAttributes attributes; // 属性信息
        public float[][] landmarks;     // 关键点 (可选)
        
        public FaceInfo() {
            attributes = new FaceAttributes();
        }
    }
    
    /**
     * 人脸分析结果
     */
    public static class FaceAnalysisResult {
        public int personId;            // 关联的人员ID
        public Detection personDetection; // 人员检测结果
        public FaceInfo[] faces;        // 检测到的人脸
        
        public boolean hasValidFaces() {
            if (faces == null) return false;
            for (FaceInfo face : faces) {
                if (face.attributes.isValid()) return true;
            }
            return false;
        }
        
        public int getValidFaceCount() {
            if (faces == null) return 0;
            int count = 0;
            for (FaceInfo face : faces) {
                if (face.attributes.isValid()) count++;
            }
            return count;
        }
        
        public FaceInfo getBestFace() {
            if (faces == null || faces.length == 0) return null;
            FaceInfo best = faces[0];
            for (FaceInfo face : faces) {
                if (face.confidence > best.confidence) {
                    best = face;
                }
            }
            return best;
        }
    }
    
    /**
     * 统计数据
     */
    public static class StatisticsData {
        public int totalPersonCount;
        public int totalFaceCount;
        public int validFaceCount;
        
        // 性别统计
        public int maleCount;
        public int femaleCount;
        public int unknownGenderCount;
        
        // 年龄统计 (9个年龄段)
        public int[] ageBracketCounts;
        
        // 种族统计 (5种)
        public int[] raceCounts;
        
        // 时间信息
        public long lastUpdateTimestamp;
        public int frameCount;
        public int analysisCount;
        
        public StatisticsData() {
            ageBracketCounts = new int[9];
            raceCounts = new int[5];
        }
        
        /**
         * 获取性别分布百分比
         * @return [男性百分比, 女性百分比]
         */
        public float[] getGenderPercentage() {
            int total = maleCount + femaleCount;
            if (total == 0) return new float[]{0.0f, 0.0f};
            
            float malePercent = (float) maleCount / total * 100.0f;
            float femalePercent = (float) femaleCount / total * 100.0f;
            return new float[]{malePercent, femalePercent};
        }
        
        /**
         * 获取年龄分布百分比
         */
        public float[] getAgeBracketPercentage() {
            float[] percentages = new float[9];
            if (validFaceCount == 0) return percentages;
            
            for (int i = 0; i < 9; i++) {
                percentages[i] = (float) ageBracketCounts[i] / validFaceCount * 100.0f;
            }
            return percentages;
        }
        
        /**
         * 获取主要年龄段
         */
        public int getDominantAgeBracket() {
            int maxIndex = -1;
            int maxCount = 0;
            for (int i = 0; i < ageBracketCounts.length; i++) {
                if (ageBracketCounts[i] > maxCount) {
                    maxCount = ageBracketCounts[i];
                    maxIndex = i;
                }
            }
            return maxCount > 0 ? maxIndex : -1;
        }
        
        /**
         * 格式化统计信息
         */
        public String formatSummary() {
            StringBuilder sb = new StringBuilder();
            sb.append("人员统计:\n");
            sb.append("总人数: ").append(totalPersonCount).append("\n");
            sb.append("检测到人脸: ").append(totalFaceCount).append("\n");
            sb.append("有效人脸: ").append(validFaceCount).append("\n\n");
            
            if (validFaceCount > 0) {
                float[] genderPercent = getGenderPercentage();
                sb.append("性别分布:\n");
                sb.append("男性: ").append(maleCount)
                  .append(" (").append(String.format("%.1f", genderPercent[0])).append("%)\n");
                sb.append("女性: ").append(femaleCount)
                  .append(" (").append(String.format("%.1f", genderPercent[1])).append("%)\n");
                sb.append("未知: ").append(unknownGenderCount).append("\n\n");
                
                sb.append("年龄分布:\n");
                String[] ageLabels = {
                    "0-2岁", "3-9岁", "10-19岁", "20-29岁", "30-39岁",
                    "40-49岁", "50-59岁", "60-69岁", "70岁以上"
                };
                float[] agePercent = getAgeBracketPercentage();
                for (int i = 0; i < ageBracketCounts.length; i++) {
                    if (ageBracketCounts[i] > 0) {
                        sb.append(ageLabels[i]).append(": ").append(ageBracketCounts[i])
                          .append(" (").append(String.format("%.1f", agePercent[i])).append("%)\n");
                    }
                }
            }
            
            return sb.toString();
        }
    }
    
    /**
     * 性能信息
     */
    public static class PerformanceInfo {
        public long objectDetectionTime;    // 目标检测耗时 (ms)
        public long faceAnalysisTime;       // 人脸分析耗时 (ms)
        public long totalTime;              // 总耗时 (ms)
        public int processedPersonCount;    // 处理的人员数
        public int detectedFaceCount;       // 检测到的人脸数
    }
    
    /**
     * 扩展推理结果
     */
    public static class ExtendedInferenceResult {
        public Detection[] objectDetections;        // 目标检测结果
        public FaceAnalysisResult[] faceAnalysisResults; // 人脸分析结果
        public StatisticsData statistics;           // 统计数据
        public PerformanceInfo performanceInfo;     // 性能信息
        
        public ExtendedInferenceResult() {
            statistics = new StatisticsData();
            performanceInfo = new PerformanceInfo();
        }
        
        public boolean hasPersonDetections() {
            if (objectDetections == null) return false;
            for (Detection detection : objectDetections) {
                if ("person".equals(detection.className)) return true;
            }
            return false;
        }
        
        public int getPersonCount() {
            if (objectDetections == null) return 0;
            int count = 0;
            for (Detection detection : objectDetections) {
                if ("person".equals(detection.className)) count++;
            }
            return count;
        }
        
        public int getValidFaceCount() {
            if (faceAnalysisResults == null) return 0;
            int count = 0;
            for (FaceAnalysisResult result : faceAnalysisResults) {
                count += result.getValidFaceCount();
            }
            return count;
        }
    }
    
    // ==================== Native方法定义 ====================
    
    /**
     * 初始化扩展推理管理器
     * @param yolov5ModelPath YOLOv5模型路径
     * @param yolov8ModelPath YOLOv8模型路径 (可选)
     * @return true成功，false失败
     */
    public static native boolean initializeExtendedInference(String yolov5ModelPath, 
                                                           String yolov8ModelPath);
    
    /**
     * 初始化人脸分析功能
     * @param inspireFaceModelPath InspireFace模型路径
     * @return true成功，false失败
     */
    public static native boolean initializeFaceAnalysis(String inspireFaceModelPath);
    
    /**
     * 初始化统计功能
     * @return true成功，false失败
     */
    public static native boolean initializeStatistics();
    
    /**
     * 释放所有资源
     */
    public static native void releaseAll();
    
    /**
     * 设置当前使用的模型
     * @param modelType 模型类型 (0: YOLOv5, 1: YOLOv8n)
     * @return true成功，false失败
     */
    public static native boolean setCurrentModel(int modelType);
    
    /**
     * 获取当前模型类型
     * @return 模型类型 (0: YOLOv5, 1: YOLOv8n)
     */
    public static native int getCurrentModel();
    
    /**
     * 原有推理接口 (保持兼容性)
     * @param cameraIndex 摄像头索引
     * @param imageData 图像数据
     * @param width 图像宽度
     * @param height 图像高度
     * @return 检测结果
     */
    public static native Detection[] inference(int cameraIndex, byte[] imageData, 
                                             int width, int height);
    
    /**
     * 扩展推理接口 (新功能)
     * @param cameraIndex 摄像头索引
     * @param imageData 图像数据
     * @param width 图像宽度
     * @param height 图像高度
     * @return 扩展推理结果
     */
    public static native ExtendedInferenceResult extendedInference(int cameraIndex, 
                                                                 byte[] imageData, 
                                                                 int width, int height);
    
    /**
     * 配置级联检测参数
     * @param enableFaceAnalysis 启用人脸分析
     * @param enableStatistics 启用统计
     * @param personConfidenceThreshold 人员检测置信度阈值
     * @param minPersonPixelSize 最小人员像素尺寸
     * @param maxPersonsPerFrame 每帧最大人员数
     */
    public static native void setCascadeConfig(boolean enableFaceAnalysis,
                                             boolean enableStatistics,
                                             float personConfidenceThreshold,
                                             int minPersonPixelSize,
                                             int maxPersonsPerFrame);
    
    /**
     * 配置人脸分析参数
     * @param enableGender 启用性别检测
     * @param enableAge 启用年龄检测
     * @param enableRace 启用种族检测
     * @param faceThreshold 人脸检测阈值
     * @param maxFacesPerPerson 每人最大人脸数
     */
    public static native void setFaceAnalysisConfig(boolean enableGender,
                                                   boolean enableAge,
                                                   boolean enableRace,
                                                   float faceThreshold,
                                                   int maxFacesPerPerson);
    
    /**
     * 获取当前统计数据
     * @return 统计数据
     */
    public static native StatisticsData getCurrentStatistics();
    
    /**
     * 重置统计数据
     */
    public static native void resetStatistics();
    
    /**
     * 获取性能监控数据
     * @return 性能信息字符串
     */
    public static native String getPerformanceReport();
    
    /**
     * 检查功能状态
     * @return [推理已初始化, 人脸分析已启用, 统计已启用]
     */
    public static native boolean[] getFeatureStatus();
    
    /**
     * 获取统计摘要
     * @return 格式化的统计摘要字符串
     */
    public static native String getStatisticsSummary();
}
