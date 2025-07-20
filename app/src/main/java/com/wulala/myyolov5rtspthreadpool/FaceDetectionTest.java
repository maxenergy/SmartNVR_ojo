package com.wulala.myyolov5rtspthreadpool;

import android.content.res.AssetManager;

/**
 * 人脸检测测试类
 * 测试实际的人脸检测和属性分析功能
 */
public class FaceDetectionTest {
    
    static {
        try {
            System.loadLibrary("myyolov5rtspthreadpool");
        } catch (UnsatisfiedLinkError e) {
            e.printStackTrace();
        }
    }
    
    /**
     * 测试人脸检测功能
     * @param assetManager Android AssetManager
     * @param internalDataPath 应用内部数据路径
     * @return 0成功，负数失败
     */
    public static native int testFaceDetection(AssetManager assetManager, String internalDataPath);
    
    /**
     * 获取人脸检测能力信息
     * @return 能力信息字符串
     */
    public static native String getFaceDetectionCapabilities();
}
