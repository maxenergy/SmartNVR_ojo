/**
 * @file EnhancedStatisticsJNI.java
 * @brief Phase 2: 增强统计功能和InspireFace初始化的JNI接口
 * @author AI Assistant
 * @date 2025-07-22
 */

package com.wulala.myyolov5rtspthreadpool;

import android.content.res.AssetManager;
import android.util.Log;

public class EnhancedStatisticsJNI {
    private static final String TAG = "EnhancedStatisticsJNI";
    
    static {
        try {
            System.loadLibrary("myyolov5rtspthreadpool");
            Log.d(TAG, "🔧 Phase 2: Native library loaded successfully");
        } catch (UnsatisfiedLinkError e) {
            Log.e(TAG, "🔧 Phase 2: Failed to load native library", e);
        }
    }
    
    /**
     * 🔧 Phase 2: 初始化InspireFace功能
     * @param assetManager Android AssetManager
     * @param internalDataPath 应用内部数据路径
     * @return 0成功，负数表示错误码
     */
    public static native int initializeInspireFace(AssetManager assetManager, String internalDataPath);
    
    /**
     * 🔧 Phase 2: 测试InspireFace功能
     * @return true成功，false失败
     */
    public static native boolean testInspireFaceIntegration();
    
    /**
     * 🔧 Phase 2: 释放InspireFace资源
     */
    public static native void releaseInspireFace();
    
    /**
     * 🔧 Phase 2: 获取InspireFace状态
     * @return 状态字符串
     */
    public static native String getInspireFaceStatus();
}
