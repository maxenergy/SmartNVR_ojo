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
 * 真实YOLO推理测试Activity
 * 专门测试RealYOLOInference的功能
 */
public class RealYOLOTestActivity extends Activity {
    
    private static final String TAG = "RealYOLOTest";
    
    private TextView logTextView;
    private ScrollView scrollView;
    private Button initButton;
    private Button statusButton;
    private Button inferenceButton;
    private Button personDetectionButton;
    private Button clearButton;
    
    private StringBuilder logBuffer = new StringBuilder();
    private Handler mainHandler = new Handler(Looper.getMainLooper());
    
    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        
        createLayout();
        setupButtonListeners();
        
        appendLog("真实YOLO推理测试界面已启动");
        appendLog("测试RealYOLOInference的完整功能");
        appendLog("设备: " + android.os.Build.MODEL + " (Android " + android.os.Build.VERSION.RELEASE + ")");
    }
    
    private void createLayout() {
        android.widget.LinearLayout layout = new android.widget.LinearLayout(this);
        layout.setOrientation(android.widget.LinearLayout.VERTICAL);
        layout.setPadding(20, 20, 20, 20);
        
        TextView titleView = new TextView(this);
        titleView.setText("真实YOLO推理测试");
        titleView.setTextSize(20);
        titleView.setPadding(0, 0, 0, 20);
        layout.addView(titleView);
        
        initButton = new Button(this);
        initButton.setText("1. 初始化YOLO引擎");
        layout.addView(initButton);
        
        statusButton = new Button(this);
        statusButton.setText("2. 获取引擎状态");
        layout.addView(statusButton);
        
        inferenceButton = new Button(this);
        inferenceButton.setText("3. 测试基础推理");
        layout.addView(inferenceButton);
        
        personDetectionButton = new Button(this);
        personDetectionButton.setText("4. 测试人员检测");
        layout.addView(personDetectionButton);
        
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
        initButton.setOnClickListener(v -> testYOLOInitialization());
        statusButton.setOnClickListener(v -> testEngineStatus());
        inferenceButton.setOnClickListener(v -> testBasicInference());
        personDetectionButton.setOnClickListener(v -> testPersonDetection());
        clearButton.setOnClickListener(v -> clearLog());
    }
    
    private void testYOLOInitialization() {
        appendLog("=== 开始YOLO引擎初始化测试 ===");
        
        new Thread(() -> {
            try {
                appendLog("正在初始化真实YOLO推理引擎...");
                
                // 使用默认模型路径
                String modelPath = getFilesDir().getAbsolutePath() + "/yolov5s.rknn";
                appendLog("模型路径: " + modelPath);
                
                long startTime = System.currentTimeMillis();
                int result = RealYOLOInference.initializeYOLO(modelPath);
                long endTime = System.currentTimeMillis();
                
                if (result == 0) {
                    appendLog("🎉 YOLO引擎初始化成功！");
                    appendLog("⏱️ 初始化耗时: " + (endTime - startTime) + " ms");
                    
                    // 检查初始化状态
                    boolean isInitialized = RealYOLOInference.isInitialized();
                    appendLog("📊 引擎状态: " + (isInitialized ? "✅ 已初始化" : "❌ 未初始化"));
                    
                } else {
                    appendLog("❌ YOLO引擎初始化失败");
                    appendLog("错误码: " + result);
                    
                    // 分析错误码
                    String errorMsg = getErrorMessage(result);
                    appendLog("错误说明: " + errorMsg);
                }
                
                // 获取内存使用情况
                Runtime runtime = Runtime.getRuntime();
                long usedMemory = (runtime.totalMemory() - runtime.freeMemory()) / (1024 * 1024);
                appendLog("💾 当前内存使用: " + usedMemory + " MB");
                
            } catch (Exception e) {
                appendLog("❌ 初始化异常: " + e.getMessage());
                Log.e(TAG, "YOLO initialization error", e);
            }
            
            appendLog("=== YOLO引擎初始化测试完成 ===\n");
        }).start();
    }
    
    private void testEngineStatus() {
        appendLog("=== 开始引擎状态测试 ===");
        
        new Thread(() -> {
            try {
                appendLog("正在获取YOLO引擎状态...");
                
                String status = RealYOLOInference.getEngineStatus();
                appendLog("📋 引擎状态详情:");
                
                String[] lines = status.split("\n");
                for (String line : lines) {
                    appendLog("  " + line);
                }
                
                // 检查初始化状态
                boolean isInitialized = RealYOLOInference.isInitialized();
                appendLog("🔧 当前状态: " + (isInitialized ? "引擎已就绪" : "引擎未初始化"));
                
            } catch (Exception e) {
                appendLog("❌ 状态获取异常: " + e.getMessage());
                Log.e(TAG, "Status error", e);
            }
            
            appendLog("=== 引擎状态测试完成 ===\n");
        }).start();
    }
    
    private void testBasicInference() {
        appendLog("=== 开始基础推理测试 ===");
        
        new Thread(() -> {
            try {
                if (!RealYOLOInference.isInitialized()) {
                    appendLog("❌ YOLO引擎未初始化，请先初始化");
                    return;
                }
                
                appendLog("正在进行基础推理测试...");
                appendLog("创建测试图像: 640x640 BGR格式");
                
                // 创建测试图像数据
                int width = 640;
                int height = 640;
                byte[] imageData = new byte[width * height * 3];
                
                // 填充一些模拟数据
                for (int i = 0; i < imageData.length; i++) {
                    imageData[i] = (byte) (100 + (i % 155)); // 创建一些变化
                }
                
                long startTime = System.currentTimeMillis();
                RealYOLOInference.DetectionResult[] results = RealYOLOInference.performInference(imageData, width, height);
                long endTime = System.currentTimeMillis();
                
                if (results != null) {
                    appendLog("🎉 基础推理成功！");
                    appendLog("⏱️ 推理耗时: " + (endTime - startTime) + " ms");
                    appendLog("📊 检测结果: " + results.length + " 个目标");
                    
                    // 显示前几个检测结果
                    for (int i = 0; i < Math.min(results.length, 5); i++) {
                        RealYOLOInference.DetectionResult result = results[i];
                        appendLog("  目标 " + (i + 1) + ": " + result.toString());
                    }
                    
                    if (results.length > 5) {
                        appendLog("  ... 还有 " + (results.length - 5) + " 个目标");
                    }
                    
                } else {
                    appendLog("❌ 基础推理失败，返回null结果");
                }
                
            } catch (Exception e) {
                appendLog("❌ 推理异常: " + e.getMessage());
                Log.e(TAG, "Inference error", e);
            }
            
            appendLog("=== 基础推理测试完成 ===\n");
        }).start();
    }
    
    private void testPersonDetection() {
        appendLog("=== 开始人员检测测试 ===");
        
        new Thread(() -> {
            try {
                if (!RealYOLOInference.isInitialized()) {
                    appendLog("❌ YOLO引擎未初始化，请先初始化");
                    return;
                }
                
                appendLog("正在进行人员检测测试...");
                appendLog("使用高级推理接口进行人员筛选");
                
                // 创建测试图像数据
                int width = 640;
                int height = 640;
                byte[] imageData = new byte[width * height * 3];
                
                // 填充一些模拟数据
                for (int i = 0; i < imageData.length; i++) {
                    imageData[i] = (byte) (120 + (i % 135)); // 创建一些变化
                }
                
                float minConfidence = 0.5f;
                float minSize = 50.0f;
                
                appendLog("检测参数: 最小置信度=" + minConfidence + ", 最小尺寸=" + minSize + "px");
                
                long startTime = System.currentTimeMillis();
                RealYOLOInference.PersonDetectionResult result = 
                    RealYOLOInference.AdvancedInference.performPersonDetection(
                        imageData, width, height, minConfidence, minSize);
                long endTime = System.currentTimeMillis();
                
                if (result.success) {
                    appendLog("🎉 人员检测成功！");
                    appendLog("⏱️ 检测耗时: " + (endTime - startTime) + " ms");
                    appendLog("📊 检测统计:");
                    appendLog("  - 总检测目标: " + result.totalDetections);
                    appendLog("  - 人员数量: " + result.personCount);
                    
                    if (result.personCount > 0) {
                        appendLog("👥 人员检测详情:");
                        for (int i = 0; i < Math.min(result.personDetections.size(), 3); i++) {
                            RealYOLOInference.DetectionResult person = result.personDetections.get(i);
                            appendLog("  人员 " + (i + 1) + ": " + person.toString());
                        }
                        
                        // 显示最大和最高置信度的人员
                        RealYOLOInference.DetectionResult largest = result.getLargestPerson();
                        RealYOLOInference.DetectionResult mostConfident = result.getMostConfidentPerson();
                        
                        if (largest != null) {
                            appendLog("📏 最大人员: 面积=" + String.format("%.0f", largest.getArea()) + "px²");
                        }
                        if (mostConfident != null) {
                            appendLog("🎯 最高置信度: " + String.format("%.3f", mostConfident.confidence));
                        }
                    } else {
                        appendLog("👤 未检测到符合条件的人员");
                    }
                    
                } else {
                    appendLog("❌ 人员检测失败: " + result.errorMessage);
                }
                
            } catch (Exception e) {
                appendLog("❌ 人员检测异常: " + e.getMessage());
                Log.e(TAG, "Person detection error", e);
            }
            
            appendLog("=== 人员检测测试完成 ===\n");
        }).start();
    }
    
    private String getErrorMessage(int errorCode) {
        switch (errorCode) {
            case -1: return "推理管理器初始化失败";
            case -2: return "设置YOLOv8模型失败";
            case -3: return "初始化过程异常";
            default: return "未知错误码: " + errorCode;
        }
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
            RealYOLOInference.releaseEngine();
        } catch (Exception e) {
            Log.e(TAG, "Cleanup error", e);
        }
    }
}
