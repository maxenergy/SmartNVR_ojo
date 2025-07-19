package com.wulala.myyolov5rtspthreadpool;

import android.content.Intent;
import android.content.pm.ActivityInfo;
import android.content.res.AssetManager;
import android.os.Build;
import android.os.Bundle;
import android.view.Surface;
import android.view.View;
import android.view.WindowManager;

import androidx.appcompat.app.AppCompatActivity;

import com.wulala.myyolov5rtspthreadpool.databinding.ActivityMainBinding;
import com.wulala.myyolov5rtspthreadpool.ui.OnBackButtonPressedListener;
import com.wulala.myyolov5rtspthreadpool.entities.Camera;

import java.util.ArrayList;
import java.util.List;

public class MainActivity extends AppCompatActivity {

    static {
        System.loadLibrary("myyolov5rtspthreadpool");
    }

    private static final String TAG = "MainActivity";

    private ActivityMainBinding binding;
    private OnBackButtonPressedListener onBackButtonPressedListener;
    private AssetManager assetManager;
    private long nativePlayerObj = 0;
    private List<Surface> cameraSurfaces;
    private int currentCameraCount = 0;

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);

        // Set fullscreen before setting content view
        getWindow().setFlags(
            WindowManager.LayoutParams.FLAG_FULLSCREEN,
            WindowManager.LayoutParams.FLAG_FULLSCREEN
        );

        // Interface can go below notches
        if (Build.VERSION.SDK_INT >= 28) {
            getWindow().setFlags(
                WindowManager.LayoutParams.FLAG_LAYOUT_NO_LIMITS,
                WindowManager.LayoutParams.FLAG_LAYOUT_NO_LIMITS
            );
            getWindow().getAttributes().layoutInDisplayCutoutMode =
                WindowManager.LayoutParams.LAYOUT_IN_DISPLAY_CUTOUT_MODE_SHORT_EDGES;
        }

        binding = ActivityMainBinding.inflate(getLayoutInflater());
        setContentView(binding.getRoot());

        binding.fab.setOnClickListener(view -> {
            android.util.Log.d("MainActivity", "FAB clicked - opening settings");
            openSettings();
        });

        // 长按FAB测试多实例创建
        binding.fab.setOnLongClickListener(view -> {
            android.util.Log.d("MainActivity", "FAB long clicked - testing multi-instance");
            testMultiInstanceCreation();
            return true;
        });
        
        // Initialize native components
        assetManager = getAssets();
        setNativeAssetManager(assetManager);

        // 🔧 修复: 复制ZLMediaKit配置文件到应用私有目录
        copyZLMediaKitConfig();

        nativePlayerObj = prepareNative();
        
        // Initialize multi-camera support
        cameraSurfaces = new ArrayList<>();
        for (int i = 0; i < 16; i++) { // 支持最多16路摄像头
            cameraSurfaces.add(null);
        }
    }

    @Override
    protected void onStart() {
        // For YOLOv5 RTSP, we'll keep landscape orientation for better video display
        this.setRequestedOrientation(ActivityInfo.SCREEN_ORIENTATION_LANDSCAPE);
        super.onStart();
    }

    @Override
    protected void onResume() {
        super.onResume();
        
        // Ensure fullscreen mode is maintained
        getWindow().setFlags(
            WindowManager.LayoutParams.FLAG_FULLSCREEN,
            WindowManager.LayoutParams.FLAG_FULLSCREEN
        );
        
        // Hide system UI for immersive experience
        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.R) {
            getWindow().setDecorFitsSystemWindows(false);
        } else {
            getWindow().getDecorView().setSystemUiVisibility(
                View.SYSTEM_UI_FLAG_FULLSCREEN |
                View.SYSTEM_UI_FLAG_HIDE_NAVIGATION |
                View.SYSTEM_UI_FLAG_IMMERSIVE_STICKY |
                View.SYSTEM_UI_FLAG_LAYOUT_STABLE |
                View.SYSTEM_UI_FLAG_LAYOUT_HIDE_NAVIGATION |
                View.SYSTEM_UI_FLAG_LAYOUT_FULLSCREEN
            );
        }
        
        // 重新启用安全的RTSP URL自动加载
        // 现在会验证URL有效性，避免崩溃
        loadAndSetMultipleRtspUrls();
    }

    public void setOnBackButtonPressedListener(OnBackButtonPressedListener onBackButtonPressedListener) {
        this.onBackButtonPressedListener = onBackButtonPressedListener;
    }

    @Override
    public void onBackPressed() {
        if (this.onBackButtonPressedListener != null && this.onBackButtonPressedListener.onBackPressed())
            return;
        super.onBackPressed();
    }

    private void openSettings() {
        android.util.Log.d("MainActivity", "openSettings called");
        Intent i = new Intent(this, com.wulala.myyolov5rtspthreadpool.ui.SettingsActivity.class);
        startActivity(i);
    }
    
    // Expose these methods for the fragment to use
    public long getNativePlayerObj() {
        return nativePlayerObj;
    }
    
    public AssetManager getNativeAssetManager() {
        return assetManager;
    }
    
    // 从设置中加载多路RTSP URL并应用到native播放器 - 增强错误处理
    public void loadAndSetMultipleRtspUrls() {
        try {
            Settings settings = Settings.fromDisk(this);
            List<Camera> cameras = settings.getCameras();

            android.util.Log.d("MainActivity", "Loading " + cameras.size() + " RTSP URLs from settings");

            if (nativePlayerObj == 0) {
                android.util.Log.e("MainActivity", "Native player object is null, cannot set RTSP URLs");
                return;
            }

            if (cameras.isEmpty()) {
                android.util.Log.i("MainActivity", "No cameras configured, skipping RTSP setup");
                return;
            }

            int validCameraCount = 0;
            for (int i = 0; i < cameras.size() && i < 16; i++) {
                Camera camera = cameras.get(i);
                String rtspUrl = camera.getRtspUrl();

                if (isValidRtspUrl(rtspUrl)) {
                    try {
                        setRtspUrlForCamera(nativePlayerObj, i, rtspUrl);
                        validCameraCount++;
                        android.util.Log.d("MainActivity", "RTSP URL set for camera " + i + ": " + rtspUrl);
                    } catch (Exception e) {
                        android.util.Log.e("MainActivity", "Failed to set RTSP URL for camera " + i + ": " + e.getMessage());
                    }
                } else {
                    android.util.Log.w("MainActivity", "Invalid RTSP URL for camera " + i + ": " + rtspUrl);
                }
            }

            if (validCameraCount > 0) {
                // 延迟启动所有RTSP流，给系统时间准备
                final long playerObj = nativePlayerObj;
                final int finalValidCameraCount = validCameraCount;
                new android.os.Handler().postDelayed(() -> {
                    try {
                        startAllRtspStreams(playerObj, finalValidCameraCount);
                        android.util.Log.d("MainActivity", "Started " + finalValidCameraCount + " RTSP streams");
                    } catch (Exception e) {
                        android.util.Log.e("MainActivity", "Failed to start RTSP streams: " + e.getMessage());
                    }
                }, 2000); // 增加延迟到2秒
            } else {
                android.util.Log.w("MainActivity", "No valid RTSP URLs found");
            }

        } catch (Exception e) {
            android.util.Log.e("MainActivity", "Error in loadAndSetMultipleRtspUrls: " + e.getMessage());
        }
    }
    
    // 兼容性方法，保留单摄像头支持
    public void loadAndSetRtspUrl() {
        loadAndSetMultipleRtspUrls();
    }
    
    // 验证RTSP URL的有效性 - 增强版本
    private boolean isValidRtspUrl(String url) {
        if (url == null || url.trim().isEmpty()) {
            return false;
        }

        url = url.trim().toLowerCase();

        // 检查协议
        if (!url.startsWith("rtsp://") && !url.startsWith("http://") && !url.startsWith("https://")) {
            android.util.Log.w("MainActivity", "Invalid protocol in URL: " + url);
            return false;
        }

        // 检查最小长度
        if (url.length() < 10) {
            android.util.Log.w("MainActivity", "URL too short: " + url);
            return false;
        }

        // 避免使用已知的有问题的演示URL
        String[] problematicUrls = {
            "ipvmdemo.dyndns.org",
            "demo:demo",
            "test:test",
            "admin:admin@192.168.1.1",
            "localhost",
            "127.0.0.1",
            "0.0.0.0",
            "example.com"
        };

        for (String problematic : problematicUrls) {
            if (url.contains(problematic.toLowerCase())) {
                android.util.Log.w("MainActivity", "Skipping problematic URL: " + url);
                return false;
            }
        }

        // 基本URL结构验证
        try {
            java.net.URI uri = java.net.URI.create(url);
            String host = uri.getHost();
            int port = uri.getPort();

            if (host == null || host.trim().isEmpty()) {
                android.util.Log.w("MainActivity", "Invalid host in URL: " + url);
                return false;
            }

            if (port != -1 && (port < 1 || port > 65535)) {
                android.util.Log.w("MainActivity", "Invalid port in URL: " + url);
                return false;
            }

            android.util.Log.d("MainActivity", "URL validation passed: " + url);
            return true;
        } catch (Exception e) {
            android.util.Log.w("MainActivity", "URL parsing failed: " + url + ", error: " + e.getMessage());
            return false;
        }
    }
    
    // 多摄像头数量变化回调
    public void onCameraCountChanged(int count) {
        currentCameraCount = count;
        android.util.Log.d("MainActivity", "Camera count changed to: " + count);
        
        // 通知native代码摄像头数量变化
        if (nativePlayerObj != 0) {
            setCameraCount(nativePlayerObj, count);
        }
    }
    
    // 设置指定摄像头的Surface
    public void setNativeSurface(int cameraIndex, Surface surface) {
        if (cameraIndex >= 0 && cameraIndex < cameraSurfaces.size()) {
            cameraSurfaces.set(cameraIndex, surface);
            android.util.Log.d("MainActivity", "Setting surface for camera " + cameraIndex + ": " + surface);
            
            if (nativePlayerObj != 0) {
                setNativeSurfaceForCamera(nativePlayerObj, cameraIndex, surface);
            }
        } else {
            android.util.Log.w("MainActivity", "Invalid camera index: " + cameraIndex);
        }
    }
    
    // 手动启动RTSP流，供用户主动调用
    public void startRtspStreamManually() {
        if (nativePlayerObj != 0) {
            android.util.Log.d("MainActivity", "Starting RTSP stream manually");
            loadAndSetMultipleRtspUrls();
        } else {
            android.util.Log.w("MainActivity", "Cannot start RTSP stream - native player not initialized");
        }
    }

    // jni native methods
    public native long prepareNative();
    public native void setNativeAssetManager(AssetManager assetManager);
    
    // 单摄像头支持（兼容性）
    public native void setNativeSurface(Surface surface);
    public native void setRtspUrl(long nativePlayerObj, String rtspUrl);
    public native void startRtspStream(long nativePlayerObj);
    
    // 多摄像头支持
    public native void setCameraCount(long nativePlayerObj, int count);
    public native void setNativeSurfaceForCamera(long nativePlayerObj, int cameraIndex, Surface surface);
    public native void setRtspUrlForCamera(long nativePlayerObj, int cameraIndex, String rtspUrl);
    public native void startAllRtspStreams(long nativePlayerObj, int cameraCount);
    public native void switchCamera();
    public native void checkAndRecoverStuckCameras();

    // 🔧 新增: YOLOv8n模型选择接口
    /**
     * 设置指定摄像头的推理模型
     * @param cameraIndex 摄像头索引
     * @param modelType 模型类型 (0=YOLOv5, 1=YOLOv8n)
     * @return 0成功，-1失败
     */
    public native int setInferenceModel(int cameraIndex, int modelType);

    /**
     * 获取指定摄像头当前使用的推理模型
     * @param cameraIndex 摄像头索引
     * @return 模型类型 (0=YOLOv5, 1=YOLOv8n, -1=错误)
     */
    public native int getCurrentInferenceModel(int cameraIndex);

    /**
     * 检查指定摄像头的模型是否可用
     * @param cameraIndex 摄像头索引
     * @param modelType 模型类型 (0=YOLOv5, 1=YOLOv8n)
     * @return true可用，false不可用
     */
    public native boolean isModelAvailable(int cameraIndex, int modelType);

    // 手动切换摄像头的方法
    public void switchCameraManually() {
        android.util.Log.d("MainActivity", "Manually switching camera");
        switchCamera();
    }

    // 测试多实例创建的方法（在后台线程执行）
    public void testMultiInstanceCreation() {
        android.util.Log.d("MainActivity", "Testing multi-instance creation");

        // 在后台线程执行，避免阻塞UI
        new Thread(() -> {
            try {
                if (nativePlayerObj != 0) {
                    android.util.Log.d("MainActivity", "Creating 4 camera instances");

                    // 设置4个测试摄像头
                    setCameraCount(nativePlayerObj, 4);

                    // 设置测试RTSP URL
                    String[] testUrls = {
                        "rtsp://admin:sharpi1688@192.168.1.2:554/1/1",
                        "rtsp://admin:sharpi1688@192.168.1.3:554/1/1",
                        "rtsp://admin:sharpi1688@192.168.1.4:554/1/1",
                        "rtsp://admin:sharpi1688@192.168.1.5:554/1/1"
                    };

                    for (int i = 0; i < testUrls.length; i++) {
                        setRtspUrlForCamera(nativePlayerObj, i, testUrls[i]);
                        android.util.Log.d("MainActivity", "Set RTSP URL for camera " + i);
                    }

                    // 启动所有流
                    startAllRtspStreams(nativePlayerObj, 4);
                    android.util.Log.d("MainActivity", "All RTSP streams started");

                    // 启动卡住检测任务（生产环境稳定性保障功能）
                    // 每30秒自动检查摄像头状态，检测卡住并自动恢复
                    startStuckDetectionTask();

                    // 在UI线程显示结果
                    runOnUiThread(() -> {
                        android.widget.Toast.makeText(this, "多实例创建完成", android.widget.Toast.LENGTH_SHORT).show();
                    });
                }
            } catch (Exception e) {
                android.util.Log.e("MainActivity", "Error in testMultiInstanceCreation: " + e.getMessage());
                runOnUiThread(() -> {
                    android.widget.Toast.makeText(this, "多实例创建失败: " + e.getMessage(), android.widget.Toast.LENGTH_LONG).show();
                });
            }
        }).start();
    }

    // 启动卡住检测任务
    private void startStuckDetectionTask() {
        android.util.Log.d("MainActivity", "Starting stuck detection task");

        // 每30秒检查一次卡住状态
        android.os.Handler handler = new android.os.Handler();
        Runnable stuckDetectionRunnable = new Runnable() {
            @Override
            public void run() {
                try {
                    if (nativePlayerObj != 0) {
                        checkAndRecoverStuckCameras();
                    }
                } catch (Exception e) {
                    android.util.Log.e("MainActivity", "Error in stuck detection: " + e.getMessage());
                }

                // 继续下一次检查
                handler.postDelayed(this, 30000); // 30秒后再次检查
            }
        };

        // 启动第一次检查
        handler.postDelayed(stuckDetectionRunnable, 30000);
    }

    // 🔧 修复: 复制ZLMediaKit配置文件到应用私有目录
    private void copyZLMediaKitConfig() {
        try {
            java.io.InputStream inputStream = getAssets().open("zlmediakit_config.ini");
            java.io.File configFile = new java.io.File(getFilesDir(), "zlmediakit_config.ini");

            java.io.FileOutputStream outputStream = new java.io.FileOutputStream(configFile);
            byte[] buffer = new byte[1024];
            int length;
            while ((length = inputStream.read(buffer)) > 0) {
                outputStream.write(buffer, 0, length);
            }

            outputStream.close();
            inputStream.close();

            android.util.Log.d("MainActivity", "ZLMediaKit config file copied to: " + configFile.getAbsolutePath());
        } catch (java.io.IOException e) {
            android.util.Log.e("MainActivity", "Failed to copy ZLMediaKit config file: " + e.getMessage());
        }
    }

    // 🔧 新增: YOLOv8n模型选择便捷方法

    /**
     * 模型类型常量
     */
    public static final int MODEL_YOLOV5 = 0;
    public static final int MODEL_YOLOV8N = 1;

    /**
     * 为所有摄像头设置推理模型
     * @param modelType 模型类型 (MODEL_YOLOV5 或 MODEL_YOLOV8N)
     * @return 成功设置的摄像头数量
     */
    public int setInferenceModelForAllCameras(int modelType) {
        int successCount = 0;
        for (int i = 0; i < currentCameraCount; i++) {
            if (setInferenceModel(i, modelType) == 0) {
                successCount++;
                android.util.Log.d(TAG, "Successfully set model " + modelType + " for camera " + i);
            } else {
                android.util.Log.e(TAG, "Failed to set model " + modelType + " for camera " + i);
            }
        }
        android.util.Log.i(TAG, "Set inference model " + modelType + " for " + successCount + "/" + currentCameraCount + " cameras");
        return successCount;
    }

    /**
     * 获取模型类型的字符串描述
     * @param modelType 模型类型
     * @return 模型描述字符串
     */
    public String getModelTypeName(int modelType) {
        switch (modelType) {
            case MODEL_YOLOV5:
                return "YOLOv5";
            case MODEL_YOLOV8N:
                return "YOLOv8n";
            default:
                return "Unknown";
        }
    }

    /**
     * 检查YOLOv8n模型是否在所有摄像头上可用
     * @return true如果所有摄像头都支持YOLOv8n，false否则
     */
    public boolean isYOLOv8nAvailableForAllCameras() {
        for (int i = 0; i < currentCameraCount; i++) {
            if (!isModelAvailable(i, MODEL_YOLOV8N)) {
                android.util.Log.w(TAG, "YOLOv8n not available for camera " + i);
                return false;
            }
        }
        return true;
    }

    /**
     * 打印所有摄像头的模型状态
     */
    public void logModelStatus() {
        android.util.Log.i(TAG, "=== Model Status Report ===");
        for (int i = 0; i < currentCameraCount; i++) {
            int currentModel = getCurrentInferenceModel(i);
            boolean yolov5Available = isModelAvailable(i, MODEL_YOLOV5);
            boolean yolov8Available = isModelAvailable(i, MODEL_YOLOV8N);

            android.util.Log.i(TAG, String.format("Camera %d: Current=%s, YOLOv5=%s, YOLOv8n=%s",
                    i, getModelTypeName(currentModel),
                    yolov5Available ? "✓" : "✗",
                    yolov8Available ? "✓" : "✗"));
        }
        android.util.Log.i(TAG, "=========================");
    }

}