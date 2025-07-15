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

public class MainActivity extends AppCompatActivity {

    static {
        System.loadLibrary("myyolov5rtspthreadpool");
    }

    private static final String TAG = "MainActivity";

    private ActivityMainBinding binding;
    private OnBackButtonPressedListener onBackButtonPressedListener;
    private AssetManager assetManager;
    private long nativePlayerObj = 0;

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
        
        // Initialize native components
        assetManager = getAssets();
        setNativeAssetManager(assetManager);
        nativePlayerObj = prepareNative();
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
        loadAndSetRtspUrl();
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
    
    // 从设置中加载RTSP URL并应用到native播放器
    public void loadAndSetRtspUrl() {
        Settings settings = Settings.fromDisk(this);
        String rtspUrl = settings.getCurrentRtspUrl();
        
        android.util.Log.d("MainActivity", "Loading RTSP URL from settings: " + rtspUrl);
        
        if (nativePlayerObj != 0 && rtspUrl != null && !rtspUrl.isEmpty()) {
            // 简单的URL验证
            if (isValidRtspUrl(rtspUrl)) {
                setRtspUrl(nativePlayerObj, rtspUrl);
                android.util.Log.d("MainActivity", "RTSP URL set to native player: " + rtspUrl);
                
                // 延迟启动RTSP流，给时间让URL设置完成
                new android.os.Handler().postDelayed(() -> {
                    startRtspStream(nativePlayerObj);
                    android.util.Log.d("MainActivity", "RTSP stream started");
                }, 1000); // 延迟1秒启动
            } else {
                android.util.Log.w("MainActivity", "Invalid RTSP URL, skipping auto-start: " + rtspUrl);
            }
        } else {
            android.util.Log.w("MainActivity", "Cannot set RTSP URL - player obj: " + nativePlayerObj + ", url: " + rtspUrl);
        }
    }
    
    // 验证RTSP URL的有效性
    private boolean isValidRtspUrl(String url) {
        if (url == null || url.trim().isEmpty()) {
            return false;
        }
        
        // 检查是否以rtsp://开头
        if (!url.toLowerCase().startsWith("rtsp://")) {
            return false;
        }
        
        // 检查是否包含基本的URL结构
        if (!url.contains("://") || url.length() < 10) {
            return false;
        }
        
        // 避免使用已知的无效演示URL
        if (url.contains("ipvmdemo.dyndns.org") || url.contains("demo:demo")) {
            android.util.Log.w("MainActivity", "Skipping demo URL that may cause crashes: " + url);
            return false;
        }
        
        return true;
    }
    
    // 手动启动RTSP流，供用户主动调用
    public void startRtspStreamManually() {
        if (nativePlayerObj != 0) {
            android.util.Log.d("MainActivity", "Starting RTSP stream manually");
            loadAndSetRtspUrl();
        } else {
            android.util.Log.w("MainActivity", "Cannot start RTSP stream - native player not initialized");
        }
    }

    // jni native methods
    public native long prepareNative();
    public native void setNativeAssetManager(AssetManager assetManager);
    public native void setNativeSurface(Surface surface);
    public native void setRtspUrl(long nativePlayerObj, String rtspUrl);
    public native void startRtspStream(long nativePlayerObj);

}