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
import com.wulala.myyolov5rtspthreadpool.ui.SurveillanceFragment;
import com.wulala.myyolov5rtspthreadpool.ui.MultiCameraView;
import com.wulala.myyolov5rtspthreadpool.entities.Camera;
import com.wulala.myyolov5rtspthreadpool.IntegratedAIManager;

import java.util.ArrayList;
import java.util.List;
import java.util.Timer;
import java.util.TimerTask;

public class MainActivity extends AppCompatActivity {

    static {
        System.loadLibrary("myyolov5rtspthreadpool");
    }

    private static final String TAG = "MainActivity";
    private static final int MAX_CAMERAS = 4; // 🔧 新增：最大摄像头数量

    private ActivityMainBinding binding;
    private OnBackButtonPressedListener onBackButtonPressedListener;
    private AssetManager assetManager;
    private long nativePlayerObj = 0;
    private List<Surface> cameraSurfaces;
    private int currentCameraCount = 0;

    // AI分析相关
    private IntegratedAIManager aiManager;
    private Timer aiAnalysisTimer;
    private boolean aiAnalysisEnabled = false;
    private SurveillanceFragment surveillanceFragment;
    private long lastAIAnalysisTime = 0; // 🔧 新增：上次AI分析时间，用于限制调用频率

    // 检测设置管理器
    private DetectionSettingsManager settingsManager;

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

        // AI分析切换按钮
        if (binding.fabAiToggle != null) {
            android.util.Log.d("MainActivity", "Setting up AI toggle FAB click listener");
            binding.fabAiToggle.setOnClickListener(view -> {
                android.util.Log.d("MainActivity", "=== AI TOGGLE FAB CLICKED ===");
                android.util.Log.d("MainActivity", "Current AI analysis state: " + aiAnalysisEnabled);
                try {
                    toggleAIAnalysis();
                    android.util.Log.d("MainActivity", "toggleAIAnalysis() completed successfully");
                } catch (Exception e) {
                    android.util.Log.e("MainActivity", "Error in toggleAIAnalysis()", e);
                }
            });
        } else {
            android.util.Log.e("MainActivity", "ERROR: fabAiToggle is null!");
        }

        // 设置按钮
        if (binding.fabSettings != null) {
            android.util.Log.d("MainActivity", "Setting up settings FAB click listener");
            binding.fabSettings.setOnClickListener(view -> {
                android.util.Log.d("MainActivity", "=== SETTINGS FAB CLICKED ===");
                openSettings();
            });
        } else {
            android.util.Log.e("MainActivity", "ERROR: fabSettings is null!");
        }

        // 初始化AI按钮状态（默认禁用）
        binding.fabAiToggle.setImageResource(R.drawable.ic_eye_off);
        binding.fabAiToggle.setBackgroundTintList(
            android.content.res.ColorStateList.valueOf(0xFF9E9E9E)); // 灰色

        // 初始化检测设置管理器
        settingsManager = new DetectionSettingsManager(this);

        // 测试DetectionSettingsManager功能
        testDetectionSettingsManager();

        // 🔧 已移除设置演示功能
        // 系统现在只读取用户配置，不进行自动演示切换

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

        // 初始化AI分析管理器
        initializeAIManager();

        // 🔧 自动启用AI分析功能（用于调试和演示）
        // 延迟启用，确保UI完全初始化
        new android.os.Handler().postDelayed(() -> {
            if (!aiAnalysisEnabled) {
                android.util.Log.d(TAG, "🔧 自动启用AI分析功能");
                toggleAIAnalysis();
            }
        }, 2000); // 2秒后自动启用
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
                        "rtsp://192.168.31.22:8554/unicast",
                        "rtsp://192.168.31.64:8554/unicast",
                        "rtsp://192.168.31.22:8554/unicast",
                        "rtsp://192.168.31.64:8554/unicast"
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

    /**
     * 初始化AI分析管理器
     */
    private void initializeAIManager() {
        try {
            aiManager = IntegratedAIManager.getInstance();
            android.util.Log.d(TAG, "AI分析管理器初始化完成");
        } catch (Exception e) {
            android.util.Log.e(TAG, "AI分析管理器初始化失败", e);
        }
    }

    /**
     * 切换AI分析状态
     */
    private void toggleAIAnalysis() {
        aiAnalysisEnabled = !aiAnalysisEnabled;

        android.util.Log.d(TAG, "AI分析状态切换: " + (aiAnalysisEnabled ? "启用" : "禁用"));

        // 获取SurveillanceFragment
        surveillanceFragment = (SurveillanceFragment) getSupportFragmentManager()
            .findFragmentById(binding.fragmentSurveillance.getId());

        if (surveillanceFragment != null) {
            MultiCameraView multiCameraView = surveillanceFragment.getMultiCameraView();
            if (multiCameraView != null) {
                // 启用/禁用所有摄像头的AI分析
                multiCameraView.enableAllAIAnalysis(aiAnalysisEnabled);

                if (aiAnalysisEnabled) {
                    startRealAIAnalysis(multiCameraView);
                    // 更新AI按钮图标为启用状态（眼睛开启）
                    binding.fabAiToggle.setImageResource(R.drawable.ic_eye);
                    binding.fabAiToggle.setBackgroundTintList(
                        android.content.res.ColorStateList.valueOf(0xFF4CAF50)); // 绿色
                    android.widget.Toast.makeText(this, "AI分析已启用", android.widget.Toast.LENGTH_SHORT).show();
                } else {
                    stopRealAIAnalysis();
                    // 更新AI按钮图标为禁用状态（眼睛关闭）
                    binding.fabAiToggle.setImageResource(R.drawable.ic_eye_off);
                    binding.fabAiToggle.setBackgroundTintList(
                        android.content.res.ColorStateList.valueOf(0xFF9E9E9E)); // 灰色
                    android.widget.Toast.makeText(this, "AI分析已禁用", android.widget.Toast.LENGTH_SHORT).show();
                }
            }
        }

        // 显示状态提示
        String message = aiAnalysisEnabled ? "AI分析已启用" : "AI分析已禁用";
        android.widget.Toast.makeText(this, message, android.widget.Toast.LENGTH_SHORT).show();
    }

    /**
     * 启动真实AI分析
     */
    private void startRealAIAnalysis(MultiCameraView multiCameraView) {
        if (aiAnalysisTimer != null) {
            aiAnalysisTimer.cancel();
        }

        // 确保AI管理器已初始化
        if (aiManager == null) {
            aiManager = IntegratedAIManager.getInstance();
            aiManager.initialize(this);
        }

        aiAnalysisTimer = new Timer();
        aiAnalysisTimer.scheduleAtFixedRate(new TimerTask() {
            @Override
            public void run() {
                runOnUiThread(() -> {
                    try {
                        // 为每个摄像头执行真实AI分析
                        int cameraCount = multiCameraView.getCameraCount();
                        for (int i = 0; i < cameraCount; i++) {
                            if (multiCameraView.isAIAnalysisEnabled(i)) {
                                performRealAIAnalysis(multiCameraView, i);
                            }
                        }
                    } catch (Exception e) {
                        android.util.Log.e(TAG, "AI分析更新失败", e);
                    }
                });
            }
        }, 1000, 3000); // 每3秒更新一次（真实AI分析需要更多时间）

        android.util.Log.d(TAG, "真实AI分析已启动");
    }

    /**
     * 停止真实AI分析
     */
    private void stopRealAIAnalysis() {
        if (aiAnalysisTimer != null) {
            aiAnalysisTimer.cancel();
            aiAnalysisTimer = null;
        }
        android.util.Log.d(TAG, "真实AI分析已停止");
    }

    /**
     * 🔧 修改：执行真实AI分析（使用现有检测结果，增强稳定性）
     */
    private void performRealAIAnalysis(MultiCameraView multiCameraView, int cameraIndex) {
        try {
            // 🛡️ 安全检查：确保AI分析已启用
            if (!aiAnalysisEnabled) {
                return;
            }

            // 🛡️ 安全检查：限制调用频率，避免过度调用JNI
            long currentTime = System.currentTimeMillis();
            if (currentTime - lastAIAnalysisTime < 500) { // 最少间隔500ms
                return;
            }
            lastAIAnalysisTime = currentTime;

            // 1. 首先尝试获取当前YOLOv5引擎的检测结果
            java.util.List<RealYOLOInference.DetectionResult> currentDetections = null;
            try {
                currentDetections = getCurrentYOLOv5Detections(cameraIndex);
            } catch (Exception e) {
                android.util.Log.w(TAG, "获取摄像头 " + cameraIndex + " 检测结果失败: " + e.getMessage());
            }

            // 2. 获取当前摄像头的视频帧（用于人脸分析）
            android.graphics.Bitmap currentFrame = null;
            byte[] imageData = null;
            int width = 0, height = 0;

            try {
                currentFrame = multiCameraView.getCurrentFrame(cameraIndex);
                if (currentFrame != null) {
                    // 将Bitmap转换为byte数组
                    java.io.ByteArrayOutputStream stream = new java.io.ByteArrayOutputStream();
                    currentFrame.compress(android.graphics.Bitmap.CompressFormat.JPEG, 90, stream);
                    imageData = stream.toByteArray();
                    width = currentFrame.getWidth();
                    height = currentFrame.getHeight();
                }
            } catch (Exception e) {
                android.util.Log.w(TAG, "获取摄像头 " + cameraIndex + " 视频帧失败: " + e.getMessage());
            }

            // 3. 使用现有检测结果进行AI分析
            IntegratedAIManager.AIDetectionResult result = null;
            try {
                if (currentDetections != null && !currentDetections.isEmpty()) {
                    android.util.Log.d(TAG, "🔧 使用当前YOLOv5检测结果进行AI分析: " + currentDetections.size() + " 个目标");
                    result = aiManager.performDetectionWithExistingResults(
                        currentDetections, imageData, width, height);
                } else {
                    android.util.Log.d(TAG, "🔧 当前无检测结果，使用独立推理");
                    if (imageData != null) {
                        result = aiManager.performDetection(imageData, width, height);
                    } else {
                        android.util.Log.w(TAG, "摄像头 " + cameraIndex + " 无可用帧，跳过AI分析");
                        return;
                    }
                }
            } catch (Exception e) {
                android.util.Log.e(TAG, "AI分析过程异常: " + e.getMessage());
                return;
            }

            // 4. 处理分析结果
            try {
                if (result != null && result.success) {
                    // 直接传递结果到MultiCameraView，让它处理过滤
                    multiCameraView.updateAIResults(cameraIndex, result);

                    android.util.Log.d(TAG, "摄像头 " + cameraIndex + " AI分析完成: " +
                                     result.detectedPersons + " 人员, " +
                                     result.detectedFaces + " 人脸");
                } else {
                    android.util.Log.w(TAG, "摄像头 " + cameraIndex + " AI分析失败");
                }
            } catch (Exception e) {
                android.util.Log.e(TAG, "处理AI分析结果异常: " + e.getMessage());
            }

        } catch (Exception e) {
            android.util.Log.e(TAG, "执行AI分析时出错", e);
        }
    }

    /**
     * 🔧 新增：获取当前YOLOv5引擎的检测结果（增强稳定性）
     */
    private java.util.List<RealYOLOInference.DetectionResult> getCurrentYOLOv5Detections(int cameraIndex) {
        // 🛡️ 安全检查：确保摄像头索引有效
        if (cameraIndex < 0 || cameraIndex >= MAX_CAMERAS) {
            android.util.Log.w(TAG, "🛡️ 无效的摄像头索引: " + cameraIndex);
            return null;
        }

        try {
            // 调用JNI接口获取当前检测结果（使用超时保护）
            RealYOLOInference.DetectionResult[] currentResults = null;

            try {
                currentResults = getCurrentDetectionResults(cameraIndex);
            } catch (Exception e) {
                android.util.Log.e(TAG, "🛡️ JNI调用异常: " + e.getMessage());
                return null;
            }

            // 🛡️ 安全检查：确保结果有效
            if (currentResults == null) {
                return null;
            }

            // 🛡️ 安全检查：确保结果不为空
            if (currentResults.length == 0) {
                return null;
            }

            // 创建结果列表
            java.util.List<RealYOLOInference.DetectionResult> resultList = new java.util.ArrayList<>();

            // 🛡️ 安全检查：过滤无效结果
            for (RealYOLOInference.DetectionResult result : currentResults) {
                if (result != null) {
                    resultList.add(result);
                }
            }

            if (!resultList.isEmpty()) {
                android.util.Log.d(TAG, "🔧 获取到当前检测结果: " + resultList.size() + " 个目标");
                return resultList;
            } else {
                return null;
            }
        } catch (Exception e) {
            android.util.Log.e(TAG, "获取当前检测结果失败: " + e.getMessage());
            return null;
        }
    }

    /**
     * 🔧 新增：JNI方法 - 获取指定摄像头的当前检测结果
     */
    public native RealYOLOInference.DetectionResult[] getCurrentDetectionResults(int cameraIndex);

    /**
     * 测试DetectionSettingsManager功能
     */
    private void testDetectionSettingsManager() {
        android.util.Log.d(TAG, "=== 开始测试DetectionSettingsManager ===");

        try {
            DetectionSettingsManager settingsManager = new DetectionSettingsManager(this);

            // 1. 测试获取初始设置
            java.util.Set<String> initialClasses = settingsManager.getEnabledClasses();
            android.util.Log.d(TAG, "初始启用类别: " + initialClasses.toString());

            // 2. 测试禁用一个类别
            android.util.Log.d(TAG, "测试禁用bicycle类别");
            settingsManager.setClassEnabled("bicycle", false);
            java.util.Set<String> afterDisable = settingsManager.getEnabledClasses();
            android.util.Log.d(TAG, "禁用bicycle后: " + afterDisable.toString());

            // 3. 测试启用一个类别
            android.util.Log.d(TAG, "测试启用motorcycle类别");
            settingsManager.setClassEnabled("motorcycle", true);
            java.util.Set<String> afterEnable = settingsManager.getEnabledClasses();
            android.util.Log.d(TAG, "启用motorcycle后: " + afterEnable.toString());

            // 4. 测试过滤器
            DetectionResultFilter filter = new DetectionResultFilter(this);
            java.util.List<DetectionResultFilter.DetectionResult> testResults = new java.util.ArrayList<>();
            testResults.add(new DetectionResultFilter.DetectionResult(0, 0.8f, 100f, 100f, 200f, 200f, "person"));
            testResults.add(new DetectionResultFilter.DetectionResult(2, 0.7f, 300f, 300f, 400f, 400f, "car"));
            testResults.add(new DetectionResultFilter.DetectionResult(1, 0.6f, 500f, 500f, 600f, 600f, "bicycle"));
            testResults.add(new DetectionResultFilter.DetectionResult(3, 0.9f, 700f, 700f, 800f, 800f, "motorcycle"));

            java.util.List<DetectionResultFilter.DetectionResult> filteredResults = filter.filterResults(testResults);
            android.util.Log.d(TAG, "过滤测试: " + testResults.size() + " -> " + filteredResults.size());

            for (DetectionResultFilter.DetectionResult result : filteredResults) {
                android.util.Log.d(TAG, "保留: " + result.className + " (置信度: " + result.confidence + ")");
            }

            android.util.Log.d(TAG, "=== DetectionSettingsManager测试完成 ===");

        } catch (Exception e) {
            android.util.Log.e(TAG, "DetectionSettingsManager测试失败", e);
        }
    }

    /**
     * 启动设置演示 - 定期切换检测类别以演示实时过滤效果
     */
    /**
     * 🔧 已移除设置演示功能
     * 系统现在只读取用户配置的类别进行过滤
     * 用户可以通过SettingsActivity界面手动配置检测类别
     */

    /**
     * 🔧 已移除演示切换功能
     * 系统现在只读取用户配置的类别进行过滤
     * 用户可以通过SettingsActivity界面配置检测类别
     */

    @Override
    protected void onDestroy() {
        super.onDestroy();

        // 清理AI相关资源
        stopRealAIAnalysis();
        if (aiManager != null) {
            aiManager.cleanup();
        }

        // 🔧 演示功能已移除，无需清理演示定时器
    }

}