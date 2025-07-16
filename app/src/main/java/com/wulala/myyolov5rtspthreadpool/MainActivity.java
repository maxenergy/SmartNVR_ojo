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
            android.util.Log.d("MainActivity", "FAB clicked - testing multi-instance");
            testMultiInstanceCreation();
        });
        
        // Initialize native components
        assetManager = getAssets();
        setNativeAssetManager(assetManager);
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

    // 手动切换摄像头的方法
    public void switchCameraManually() {
        android.util.Log.d("MainActivity", "Manually switching camera");
        switchCamera();
    }

    // 测试多实例创建的方法
    public void testMultiInstanceCreation() {
        android.util.Log.d("MainActivity", "Testing multi-instance creation");
        if (nativePlayerObj != 0) {
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
            }

            // 启动所有流
            startAllRtspStreams(nativePlayerObj, 4);
        }
    }

}