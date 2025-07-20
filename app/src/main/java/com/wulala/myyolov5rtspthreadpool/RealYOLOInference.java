package com.wulala.myyolov5rtspthreadpool;

import android.util.Log;

/**
 * 真实YOLO推理接口
 * 提供对底层YOLOv8引擎的直接访问
 */
public class RealYOLOInference {
    
    private static final String TAG = "RealYOLOInference";
    
    static {
        try {
            System.loadLibrary("myyolov5rtspthreadpool");
            Log.i(TAG, "Native library loaded successfully");
        } catch (UnsatisfiedLinkError e) {
            Log.e(TAG, "Failed to load native library", e);
        }
    }
    
    /**
     * 初始化YOLO推理引擎
     * @param modelPath 模型文件路径
     * @return 0成功，负数失败
     */
    public static native int initializeYOLO(String modelPath);
    
    /**
     * 执行YOLO推理
     * @param imageData 图像数据
     * @param width 图像宽度
     * @param height 图像高度
     * @return 检测结果数组
     */
    public static native DetectionResult[] performInference(byte[] imageData, int width, int height);
    
    /**
     * 获取引擎状态信息
     * @return 状态信息字符串
     */
    public static native String getEngineStatus();
    
    /**
     * 释放推理引擎资源
     */
    public static native void releaseEngine();
    
    /**
     * 检查引擎是否已初始化
     * @return true已初始化，false未初始化
     */
    public static native boolean isInitialized();
    
    /**
     * 检测结果数据类
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
     * 高级推理接口 - 带人员筛选
     */
    public static class AdvancedInference {
        
        /**
         * 执行推理并筛选人员检测结果
         * @param imageData 图像数据
         * @param width 图像宽度
         * @param height 图像高度
         * @param minConfidence 最小置信度阈值
         * @param minSize 最小尺寸阈值（像素）
         * @return 人员检测结果
         */
        public static PersonDetectionResult performPersonDetection(
                byte[] imageData, int width, int height, 
                float minConfidence, float minSize) {
            
            PersonDetectionResult result = new PersonDetectionResult();
            
            try {
                // 执行YOLO推理
                DetectionResult[] allDetections = performInference(imageData, width, height);
                
                if (allDetections == null) {
                    Log.w(TAG, "YOLO推理返回null结果");
                    return result;
                }
                
                result.totalDetections = allDetections.length;
                
                // 筛选人员检测结果
                for (DetectionResult detection : allDetections) {
                    if (detection.isPerson() && 
                        detection.confidence >= minConfidence &&
                        detection.getWidth() >= minSize &&
                        detection.getHeight() >= minSize &&
                        detection.isValid()) {
                        
                        result.personDetections.add(detection);
                    }
                }
                
                result.personCount = result.personDetections.size();
                result.success = true;
                
                Log.d(TAG, "人员检测完成: 总检测=" + result.totalDetections + 
                          ", 人员=" + result.personCount);
                
            } catch (Exception e) {
                Log.e(TAG, "人员检测异常", e);
                result.success = false;
                result.errorMessage = e.getMessage();
            }
            
            return result;
        }
    }
    
    /**
     * 人员检测结果类
     */
    public static class PersonDetectionResult {
        public boolean success = false;
        public int totalDetections = 0;
        public int personCount = 0;
        public java.util.List<DetectionResult> personDetections = new java.util.ArrayList<>();
        public String errorMessage = null;
        
        /**
         * 获取最大的人员检测框
         */
        public DetectionResult getLargestPerson() {
            if (personDetections.isEmpty()) return null;
            
            DetectionResult largest = personDetections.get(0);
            for (DetectionResult detection : personDetections) {
                if (detection.getArea() > largest.getArea()) {
                    largest = detection;
                }
            }
            return largest;
        }
        
        /**
         * 获取置信度最高的人员检测框
         */
        public DetectionResult getMostConfidentPerson() {
            if (personDetections.isEmpty()) return null;
            
            DetectionResult mostConfident = personDetections.get(0);
            for (DetectionResult detection : personDetections) {
                if (detection.confidence > mostConfident.confidence) {
                    mostConfident = detection;
                }
            }
            return mostConfident;
        }
        
        @Override
        public String toString() {
            return String.format("PersonDetectionResult{success=%s, total=%d, persons=%d}", 
                               success, totalDetections, personCount);
        }
    }
}
