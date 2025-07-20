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
 * 集成AI测试Activity
 * 测试YOLO + InspireFace的完整集成功能
 */
public class IntegratedAITestActivity extends Activity {
    
    private static final String TAG = "IntegratedAITest";
    
    private TextView logTextView;
    private ScrollView scrollView;
    private Button initButton;
    private Button statusButton;
    private Button testButton;
    private Button performanceButton;
    private Button clearButton;
    
    private StringBuilder logBuffer = new StringBuilder();
    private Handler mainHandler = new Handler(Looper.getMainLooper());
    
    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        
        createLayout();
        setupButtonListeners();
        
        appendLog("集成AI测试界面已启动");
        appendLog("测试YOLO目标检测 + InspireFace人脸分析的完整集成");
        appendLog("点击按钮开始测试各项功能");
    }
    
    private void createLayout() {
        android.widget.LinearLayout layout = new android.widget.LinearLayout(this);
        layout.setOrientation(android.widget.LinearLayout.VERTICAL);
        layout.setPadding(20, 20, 20, 20);
        
        TextView titleView = new TextView(this);
        titleView.setText("集成AI系统测试");
        titleView.setTextSize(20);
        titleView.setPadding(0, 0, 0, 20);
        layout.addView(titleView);
        
        initButton = new Button(this);
        initButton.setText("1. 初始化集成系统");
        layout.addView(initButton);
        
        statusButton = new Button(this);
        statusButton.setText("2. 获取系统状态");
        layout.addView(statusButton);
        
        testButton = new Button(this);
        testButton.setText("3. 测试检测流程");
        layout.addView(testButton);
        
        performanceButton = new Button(this);
        performanceButton.setText("4. 性能统计");
        layout.addView(performanceButton);
        
        clearButton = new Button(this);
        clearButton.setText("清除日志");
        layout.addView(clearButton);
        
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
        statusButton.setOnClickListener(v -> testSystemStatus());
        testButton.setOnClickListener(v -> testDetectionPipeline());
        performanceButton.setOnClickListener(v -> testPerformanceStats());
        clearButton.setOnClickListener(v -> clearLog());
    }
    
    private void testInitialization() {
        appendLog("=== 开始集成系统初始化测试 ===");
        
        new Thread(() -> {
            try {
                IntegratedAIManager manager = IntegratedAIManager.getInstance();
                
                appendLog("正在初始化集成AI系统...");
                appendLog("包括：YOLO目标检测 + InspireFace人脸分析");
                appendLog("这可能需要几秒钟时间...");
                
                long startTime = System.currentTimeMillis();
                boolean success = manager.initialize(this);
                long endTime = System.currentTimeMillis();
                
                if (success) {
                    appendLog("🎉 集成AI系统初始化成功！");
                    appendLog("⏱️ 初始化耗时: " + (endTime - startTime) + " ms");
                    
                    appendLog("📊 系统状态:");
                    appendLog("  - YOLO检测: " + (manager.isYoloAvailable() ? "✅ 可用" : "❌ 不可用"));
                    appendLog("  - InspireFace分析: " + (manager.isInspireFaceAvailable() ? "✅ 可用" : "❌ 不可用"));
                    
                } else {
                    appendLog("❌ 集成AI系统初始化失败");
                    appendLog("请检查各个组件的状态");
                }
                
                // 获取内存使用情况
                Runtime runtime = Runtime.getRuntime();
                long usedMemory = (runtime.totalMemory() - runtime.freeMemory()) / (1024 * 1024);
                appendLog("💾 当前内存使用: " + usedMemory + " MB");
                
            } catch (Exception e) {
                appendLog("❌ 初始化异常: " + e.getMessage());
                Log.e(TAG, "Initialization error", e);
            }
            
            appendLog("=== 集成系统初始化测试完成 ===\n");
        }).start();
    }
    
    private void testSystemStatus() {
        appendLog("=== 开始系统状态测试 ===");
        
        new Thread(() -> {
            try {
                IntegratedAIManager manager = IntegratedAIManager.getInstance();
                
                appendLog("正在获取系统状态信息...");
                
                String statusInfo = manager.getStatusInfo();
                appendLog("📋 系统状态详情:");
                String[] lines = statusInfo.split("\n");
                for (String line : lines) {
                    appendLog("  " + line);
                }
                
            } catch (Exception e) {
                appendLog("❌ 状态获取异常: " + e.getMessage());
                Log.e(TAG, "Status error", e);
            }
            
            appendLog("=== 系统状态测试完成 ===\n");
        }).start();
    }
    
    private void testDetectionPipeline() {
        appendLog("=== 开始检测流程测试 ===");
        
        new Thread(() -> {
            try {
                IntegratedAIManager manager = IntegratedAIManager.getInstance();
                
                if (!manager.isInitialized()) {
                    appendLog("❌ 系统未初始化，请先初始化系统");
                    return;
                }
                
                appendLog("正在测试完整的AI检测流程...");
                appendLog("模拟图像: 320x320 BGR格式");
                
                // 创建模拟图像数据
                int width = 320;
                int height = 320;
                byte[] imageData = new byte[width * height * 3];
                // 填充一些模拟数据
                for (int i = 0; i < imageData.length; i++) {
                    imageData[i] = (byte) (128 + (i % 50)); // 创建一些变化
                }
                
                long startTime = System.currentTimeMillis();
                IntegratedAIManager.AIDetectionResult result = manager.performDetection(imageData, width, height);
                long endTime = System.currentTimeMillis();
                
                appendLog("📊 检测结果:");
                appendLog("  - 整体成功: " + (result.success ? "✅" : "❌"));
                appendLog("  - YOLO检测: " + (result.objectDetectionSuccess ? "✅" : "❌"));
                appendLog("  - 人脸分析: " + (result.faceAnalysisSuccess ? "✅" : "❌"));
                appendLog("  - 检测对象数: " + result.detectedObjects);
                appendLog("  - 检测人脸数: " + result.detectedFaces);
                appendLog("  - 处理耗时: " + (endTime - startTime) + " ms");
                
                if (result.errorMessage != null) {
                    appendLog("  - 错误信息: " + result.errorMessage);
                }
                
                // 进行多次测试以验证稳定性
                appendLog("正在进行稳定性测试（5次检测）...");
                int successCount = 0;
                long totalTime = 0;
                
                for (int i = 0; i < 5; i++) {
                    long testStart = System.currentTimeMillis();
                    IntegratedAIManager.AIDetectionResult testResult = manager.performDetection(imageData, width, height);
                    long testEnd = System.currentTimeMillis();
                    
                    if (testResult.success) {
                        successCount++;
                    }
                    totalTime += (testEnd - testStart);
                    
                    appendLog("  测试 " + (i + 1) + ": " + (testResult.success ? "✅" : "❌") + " (" + (testEnd - testStart) + "ms)");
                }
                
                appendLog("🎯 稳定性测试结果:");
                appendLog("  - 成功率: " + successCount + "/5 (" + (successCount * 20) + "%)");
                appendLog("  - 平均耗时: " + (totalTime / 5) + " ms");
                
            } catch (Exception e) {
                appendLog("❌ 检测流程异常: " + e.getMessage());
                Log.e(TAG, "Detection pipeline error", e);
            }
            
            appendLog("=== 检测流程测试完成 ===\n");
        }).start();
    }
    
    private void testPerformanceStats() {
        appendLog("=== 开始性能统计测试 ===");
        
        new Thread(() -> {
            try {
                IntegratedAIManager manager = IntegratedAIManager.getInstance();
                
                appendLog("正在获取性能统计信息...");
                
                String performanceStats = manager.getPerformanceStats();
                appendLog("📈 性能统计:");
                String[] lines = performanceStats.split("\n");
                for (String line : lines) {
                    appendLog("  " + line);
                }
                
                // 获取系统资源信息
                appendLog("🔧 系统资源信息:");
                appendLog("  - Android版本: " + android.os.Build.VERSION.RELEASE);
                appendLog("  - API级别: " + android.os.Build.VERSION.SDK_INT);
                appendLog("  - 设备型号: " + android.os.Build.MODEL);
                appendLog("  - CPU架构: " + android.os.Build.CPU_ABI);
                
                // 获取详细内存信息
                Runtime runtime = Runtime.getRuntime();
                long maxMemory = runtime.maxMemory() / (1024 * 1024);
                long totalMemory = runtime.totalMemory() / (1024 * 1024);
                long freeMemory = runtime.freeMemory() / (1024 * 1024);
                long usedMemory = totalMemory - freeMemory;
                
                appendLog("💾 详细内存信息:");
                appendLog("  - 最大可用: " + maxMemory + " MB");
                appendLog("  - 已分配: " + totalMemory + " MB");
                appendLog("  - 已使用: " + usedMemory + " MB");
                appendLog("  - 可用: " + freeMemory + " MB");
                appendLog("  - 使用率: " + String.format("%.1f%%", (double) usedMemory / maxMemory * 100));
                
            } catch (Exception e) {
                appendLog("❌ 性能统计异常: " + e.getMessage());
                Log.e(TAG, "Performance stats error", e);
            }
            
            appendLog("=== 性能统计测试完成 ===\n");
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
            scrollView.post(() -> scrollView.fullScroll(ScrollView.FOCUS_DOWN));
        });
        
        Log.i(TAG, message);
    }
    
    @Override
    protected void onDestroy() {
        super.onDestroy();
        
        // 清理资源
        try {
            IntegratedAIManager.getInstance().cleanup();
        } catch (Exception e) {
            Log.e(TAG, "Cleanup error", e);
        }
    }
}
