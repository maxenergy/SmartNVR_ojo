package com.wulala.myyolov5rtspthreadpool;

import android.content.res.AssetManager;

/**
 * 直接InspireFace测试类
 * 绕过ExtendedInferenceManager直接测试InspireFace功能
 */
public class DirectInspireFaceTest {
    
    static {
        try {
            System.loadLibrary("myyolov5rtspthreadpool");
        } catch (UnsatisfiedLinkError e) {
            e.printStackTrace();
        }
    }
    
    /**
     * 测试InspireFace初始化
     * @param assetManager Android AssetManager
     * @param internalDataPath 应用内部数据路径
     * @return 0成功，负数失败
     */
    public static native int testInspireFaceInit(AssetManager assetManager, String internalDataPath);
    
    /**
     * 测试模型文件验证
     * @param internalDataPath 应用内部数据路径
     * @return 0成功，负数失败
     */
    public static native int testModelValidation(String internalDataPath);
    
    /**
     * 获取InspireFace库信息
     * @return 库信息字符串
     */
    public static native String getInspireFaceInfo();

    /**
     * 执行人脸分析
     * @param imageData 图像数据
     * @param width 图像宽度
     * @param height 图像高度
     * @param personDetections 人员检测结果（格式：[count, x1, y1, x2, y2, confidence, ...]）
     * @return 0成功，负数失败
     */
    public static native int performFaceAnalysis(byte[] imageData, int width, int height, float[] personDetections);

    /**
     * 获取人脸分析结果
     * @return 人脸分析结果对象
     */
    public static native IntegratedAIManager.FaceAnalysisNativeResult getFaceAnalysisResult();
}
