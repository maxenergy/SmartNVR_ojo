package com.wulala.myyolov5rtspthreadpool;

import android.app.Activity;
import android.os.Bundle;
import android.os.Handler;
import android.os.Looper;
import android.util.Log;
import android.widget.Button;
import android.widget.ScrollView;
import android.widget.TextView;

/**
 * InspireFace测试Activity
 * 用于验证InspireFace模块的初始化和基本功能
 */
public class InspireFaceTestActivity extends Activity {
    
    private static final String TAG = "InspireFaceTest";
    
    private TextView logTextView;
    private ScrollView scrollView;
    private Button initButton;
    private Button validateButton;
    private Button infoButton;
    private Button clearButton;
    
    private StringBuilder logBuffer = new StringBuilder();
    private Handler mainHandler = new Handler(Looper.getMainLooper());
    
    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        
        // 创建简单的布局
        createLayout();
        
        // 设置按钮监听器
        setupButtonListeners();
        
        appendLog("InspireFace测试界面已启动");
        appendLog("点击按钮开始测试各项功能");
    }
    
    private void createLayout() {
        // 创建垂直线性布局
        android.widget.LinearLayout layout = new android.widget.LinearLayout(this);
        layout.setOrientation(android.widget.LinearLayout.VERTICAL);
        layout.setPadding(20, 20, 20, 20);
        
        // 标题
        TextView titleView = new TextView(this);
        titleView.setText("InspireFace 集成测试");
        titleView.setTextSize(20);
        titleView.setPadding(0, 0, 0, 20);
        layout.addView(titleView);
        
        // 初始化按钮
        initButton = new Button(this);
        initButton.setText("初始化 InspireFace");
        layout.addView(initButton);
        
        // 验证按钮
        validateButton = new Button(this);
        validateButton.setText("验证模型文件");
        layout.addView(validateButton);
        
        // 信息按钮
        infoButton = new Button(this);
        infoButton.setText("获取详细信息");
        layout.addView(infoButton);
        
        // 清除日志按钮
        clearButton = new Button(this);
        clearButton.setText("清除日志");
        layout.addView(clearButton);
        
        // 日志显示区域
        logTextView = new TextView(this);
        logTextView.setTextSize(12);
        logTextView.setPadding(10, 10, 10, 10);
        logTextView.setBackgroundColor(0xFF000000);
        logTextView.setTextColor(0xFF00FF00);
        
        scrollView = new ScrollView(this);
        scrollView.addView(logTextView);
        
        android.widget.LinearLayout.LayoutParams scrollParams = 
            new android.widget.LinearLayout.LayoutParams(
                android.widget.LinearLayout.LayoutParams.MATCH_PARENT, 
                0, 1.0f);
        scrollView.setLayoutParams(scrollParams);
        
        layout.addView(scrollView);
        
        setContentView(layout);
    }
    
    private void setupButtonListeners() {
        initButton.setOnClickListener(v -> testInitialization());
        validateButton.setOnClickListener(v -> testValidation());
        infoButton.setOnClickListener(v -> testDetailedInfo());
        clearButton.setOnClickListener(v -> clearLog());
    }
    
    private void testInitialization() {
        appendLog("=== 开始初始化测试 ===");
        
        // 在后台线程执行
        new Thread(() -> {
            try {
                InspireFaceManager manager = InspireFaceManager.getInstance();
                
                appendLog("正在初始化 InspireFace...");
                long startTime = System.currentTimeMillis();
                
                boolean success = manager.initialize(this);
                
                long endTime = System.currentTimeMillis();
                long duration = endTime - startTime;
                
                if (success) {
                    appendLog("✅ InspireFace 初始化成功！");
                    appendLog("⏱️ 初始化耗时: " + duration + " ms");
                    appendLog("📊 " + manager.getStatusInfo());
                } else {
                    appendLog("❌ InspireFace 初始化失败");
                }
                
            } catch (Exception e) {
                appendLog("❌ 初始化异常: " + e.getMessage());
                Log.e(TAG, "Initialization error", e);
            }
            
            appendLog("=== 初始化测试完成 ===\n");
        }).start();
    }
    
    private void testValidation() {
        appendLog("=== 开始验证测试 ===");
        
        new Thread(() -> {
            try {
                InspireFaceManager manager = InspireFaceManager.getInstance();
                
                appendLog("正在验证模型文件...");
                
                boolean isInitialized = manager.isInitialized();
                appendLog("初始化状态: " + (isInitialized ? "已初始化" : "未初始化"));
                
                if (isInitialized) {
                    boolean isValid = manager.validateModelFiles();
                    appendLog("模型文件验证: " + (isValid ? "✅ 通过" : "❌ 失败"));
                    
                    appendLog("📁 " + manager.getModelInfo());
                } else {
                    appendLog("⚠️ 请先初始化 InspireFace");
                }
                
            } catch (Exception e) {
                appendLog("❌ 验证异常: " + e.getMessage());
                Log.e(TAG, "Validation error", e);
            }
            
            appendLog("=== 验证测试完成 ===\n");
        }).start();
    }
    
    private void testDetailedInfo() {
        appendLog("=== 获取详细信息 ===");
        
        new Thread(() -> {
            try {
                InspireFaceManager manager = InspireFaceManager.getInstance();
                
                appendLog("📋 状态信息:");
                appendLog(manager.getStatusInfo());
                
                appendLog("📁 详细模型信息:");
                appendLog(manager.getDetailedModelInfo());
                
                // 获取系统信息
                appendLog("🔧 系统信息:");
                appendLog("- Android版本: " + android.os.Build.VERSION.RELEASE);
                appendLog("- API级别: " + android.os.Build.VERSION.SDK_INT);
                appendLog("- 设备型号: " + android.os.Build.MODEL);
                appendLog("- CPU架构: " + android.os.Build.CPU_ABI);
                
                // 获取内存信息
                Runtime runtime = Runtime.getRuntime();
                long maxMemory = runtime.maxMemory() / (1024 * 1024);
                long totalMemory = runtime.totalMemory() / (1024 * 1024);
                long freeMemory = runtime.freeMemory() / (1024 * 1024);
                long usedMemory = totalMemory - freeMemory;
                
                appendLog("💾 内存信息:");
                appendLog("- 最大内存: " + maxMemory + " MB");
                appendLog("- 已分配: " + totalMemory + " MB");
                appendLog("- 已使用: " + usedMemory + " MB");
                appendLog("- 可用: " + freeMemory + " MB");
                
            } catch (Exception e) {
                appendLog("❌ 获取信息异常: " + e.getMessage());
                Log.e(TAG, "Info error", e);
            }
            
            appendLog("=== 详细信息获取完成 ===\n");
        }).start();
    }
    
    private void clearLog() {
        logBuffer.setLength(0);
        mainHandler.post(() -> {
            logTextView.setText("");
        });
        appendLog("日志已清除");
    }
    
    private void appendLog(String message) {
        String timestamp = new java.text.SimpleDateFormat("HH:mm:ss", 
                java.util.Locale.getDefault()).format(new java.util.Date());
        String logLine = "[" + timestamp + "] " + message + "\n";
        
        logBuffer.append(logLine);
        
        mainHandler.post(() -> {
            logTextView.setText(logBuffer.toString());
            // 滚动到底部
            scrollView.post(() -> scrollView.fullScroll(ScrollView.FOCUS_DOWN));
        });
        
        // 同时输出到Logcat
        Log.i(TAG, message);
    }
    
    @Override
    protected void onDestroy() {
        super.onDestroy();
        
        // 清理资源
        try {
            InspireFaceManager.getInstance().cleanup();
        } catch (Exception e) {
            Log.e(TAG, "Cleanup error", e);
        }
    }
}
