package com.wulala.myyolov5rtspthreadpool;

import android.content.Context;
import android.content.SharedPreferences;

public class SharedPreferencesManager {
    private static final String SP_ROTATION_ENABLED = "rot_en";
    private static final String SP_RTSP_URL = "rtsp_url";
    private static final String SP_MODEL_CONFIDENCE = "model_confidence";

    public static void saveRotationEnabled(Context ctx, boolean enabled) {
        SharedPreferences sharedPref = ctx.getSharedPreferences(SP_ROTATION_ENABLED, Context.MODE_PRIVATE);
        sharedPref.edit().putBoolean(SP_ROTATION_ENABLED, enabled).apply();
    }

    public static boolean loadRotationEnabled(Context ctx) {
        SharedPreferences sharedPref = ctx.getSharedPreferences(SP_ROTATION_ENABLED, Context.MODE_PRIVATE);
        return sharedPref.getBoolean(SP_ROTATION_ENABLED, false);
    }

    public static void toggleRotationEnabled(Context ctx) {
        saveRotationEnabled(ctx, ! loadRotationEnabled(ctx));
    }

    // YOLOv5 specific settings
    public static void saveRtspUrl(Context ctx, String url) {
        SharedPreferences sharedPref = ctx.getSharedPreferences("yolov5_settings", Context.MODE_PRIVATE);
        sharedPref.edit().putString(SP_RTSP_URL, url).apply();
    }

    public static String loadRtspUrl(Context ctx) {
        SharedPreferences sharedPref = ctx.getSharedPreferences("yolov5_settings", Context.MODE_PRIVATE);
        return sharedPref.getString(SP_RTSP_URL, "rtsp://admin:sharpi1688@192.168.1.2:554/1/1");  // 使用本地网络示例URL
    }

    public static void saveModelConfidence(Context ctx, float confidence) {
        SharedPreferences sharedPref = ctx.getSharedPreferences("yolov5_settings", Context.MODE_PRIVATE);
        sharedPref.edit().putFloat(SP_MODEL_CONFIDENCE, confidence).apply();
    }

    public static float loadModelConfidence(Context ctx) {
        SharedPreferences sharedPref = ctx.getSharedPreferences("yolov5_settings", Context.MODE_PRIVATE);
        return sharedPref.getFloat(SP_MODEL_CONFIDENCE, 0.5f);
    }
}