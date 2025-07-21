package com.wulala.myyolov5rtspthreadpool;

/**
 * 🔧 批量统计结果类
 * 用于从C++层一次性获取所有统计数据，减少JNI调用开销
 */
public class BatchStatisticsResult {
    
    // 基础统计数据
    public boolean success = false;
    public int personCount = 0;
    public int maleCount = 0;
    public int femaleCount = 0;
    public int totalFaceCount = 0;
    public int[] ageBrackets = new int[9]; // 9个年龄段分布
    
    // 性能指标
    public double averageProcessingTime = 0.0;
    public int totalAnalysisCount = 0;
    public double successRate = 0.0;
    
    // 错误信息
    public String errorMessage = "";
    
    /**
     * 默认构造函数
     */
    public BatchStatisticsResult() {
        // 初始化年龄分布数组
        for (int i = 0; i < ageBrackets.length; i++) {
            ageBrackets[i] = 0;
        }
    }
    
    /**
     * 获取总人数（男性+女性）
     */
    public int getTotalGenderCount() {
        return maleCount + femaleCount;
    }
    
    /**
     * 获取性别分布比例
     */
    public String getGenderRatio() {
        int total = getTotalGenderCount();
        if (total == 0) return "无数据";
        
        double maleRatio = (double) maleCount / total * 100;
        double femaleRatio = (double) femaleCount / total * 100;
        
        return String.format("男性%.1f%% 女性%.1f%%", maleRatio, femaleRatio);
    }
    
    /**
     * 获取主要年龄段
     */
    public String getDominantAgeBracket() {
        int maxCount = 0;
        int maxIndex = -1;
        
        for (int i = 0; i < ageBrackets.length; i++) {
            if (ageBrackets[i] > maxCount) {
                maxCount = ageBrackets[i];
                maxIndex = i;
            }
        }
        
        if (maxIndex == -1 || maxCount == 0) return "无数据";
        
        String[] ageLabels = {
            "0-2岁", "3-9岁", "10-19岁", "20-29岁", "30-39岁", 
            "40-49岁", "50-59岁", "60-69岁", "70岁以上"
        };
        
        return ageLabels[maxIndex] + "(" + maxCount + "人)";
    }
    
    /**
     * 格式化统计信息为显示字符串
     */
    public String formatForDisplay() {
        if (!success) {
            return "统计数据获取失败";
        }
        
        return String.format("👥%d 👨%d 👩%d", personCount, maleCount, femaleCount);
    }
    
    /**
     * 获取详细统计信息
     */
    public String getDetailedInfo() {
        if (!success) {
            return "统计数据不可用: " + errorMessage;
        }
        
        StringBuilder sb = new StringBuilder();
        sb.append("人员统计:\n");
        sb.append("  总人数: ").append(personCount).append("\n");
        sb.append("  检测到人脸: ").append(totalFaceCount).append("\n");
        sb.append("  性别分布: ").append(getGenderRatio()).append("\n");
        sb.append("  主要年龄段: ").append(getDominantAgeBracket()).append("\n");
        
        if (totalAnalysisCount > 0) {
            sb.append("性能指标:\n");
            sb.append("  平均处理时间: ").append(String.format("%.1fms", averageProcessingTime)).append("\n");
            sb.append("  成功率: ").append(String.format("%.1f%%", successRate)).append("\n");
            sb.append("  总分析次数: ").append(totalAnalysisCount).append("\n");
        }
        
        return sb.toString();
    }
    
    @Override
    public String toString() {
        return formatForDisplay();
    }
}