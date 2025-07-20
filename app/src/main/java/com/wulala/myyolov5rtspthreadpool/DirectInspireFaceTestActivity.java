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
 * 直接InspireFace测试Activity
 * 绕过ExtendedInferenceManager直接测试InspireFace功能
 */
public class DirectInspireFaceTestActivity extends Activity {
    
    private static final String TAG = "DirectInspireFaceTest";
    
    private TextView logTextView;
    private ScrollView scrollView;
    private Button validateButton;
    private Button initButton;
    private Button infoButton;
    private Button clearButton;
    
    private StringBuilder logBuffer = new StringBuilder();
    private Handler mainHandler = new Handler(Looper.getMainLooper());
    
    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        
        createLayout();
        setupButtonListeners();
        
        appendLog("直接InspireFace测试界面已启动");
        appendLog("这个测试绕过ExtendedInferenceManager直接测试InspireFace");
        appendLog("点击按钮开始测试各项功能");
    }
    
    private void createLayout() {
        android.widget.LinearLayout layout = new android.widget.LinearLayout(this);
        layout.setOrientation(android.widget.LinearLayout.VERTICAL);
        layout.setPadding(20, 20, 20, 20);
        
        TextView titleView = new TextView(this);
        titleView.setText("直接 InspireFace 测试");
        titleView.setTextSize(20);
        titleView.setPadding(0, 0, 0, 20);
        layout.addView(titleView);
        
        validateButton = new Button(this);
        validateButton.setText("1. 验证模型文件");
        layout.addView(validateButton);
        
        infoButton = new Button(this);
        infoButton.setText("2. 获取库信息");
        layout.addView(infoButton);
        
        initButton = new Button(this);
        initButton.setText("3. 测试完整初始化");
        layout.addView(initButton);
        
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
        validateButton.setOnClickListener(v -> testModelValidation());
        infoButton.setOnClickListener(v -> testLibraryInfo());
        initButton.setOnClickListener(v -> testFullInitialization());
        clearButton.setOnClickListener(v -> clearLog());
    }
    
    private void testModelValidation() {
        appendLog("=== 开始模型文件验证测试 ===");
        
        new Thread(() -> {
            try {
                String internalDataPath = getFilesDir().getAbsolutePath();
                appendLog("内部数据路径: " + internalDataPath);
                
                long startTime = System.currentTimeMillis();
                int result = DirectInspireFaceTest.testModelValidation(internalDataPath);
                long endTime = System.currentTimeMillis();
                
                if (result == 0) {
                    appendLog("✅ 模型文件验证成功！");
                } else {
                    appendLog("❌ 模型文件验证失败，错误码: " + result);
                }
                
                appendLog("⏱️ 验证耗时: " + (endTime - startTime) + " ms");
                
            } catch (Exception e) {
                appendLog("❌ 验证异常: " + e.getMessage());
                Log.e(TAG, "Validation error", e);
            }
            
            appendLog("=== 模型文件验证测试完成 ===\n");
        }).start();
    }
    
    private void testLibraryInfo() {
        appendLog("=== 开始库信息测试 ===");
        
        new Thread(() -> {
            try {
                long startTime = System.currentTimeMillis();
                String info = DirectInspireFaceTest.getInspireFaceInfo();
                long endTime = System.currentTimeMillis();
                
                if (info != null && !info.startsWith("Error:")) {
                    appendLog("✅ 库信息获取成功：");
                    String[] lines = info.split("\n");
                    for (String line : lines) {
                        appendLog("  " + line);
                    }
                } else {
                    appendLog("❌ 库信息获取失败: " + (info != null ? info : "null"));
                }
                
                appendLog("⏱️ 获取耗时: " + (endTime - startTime) + " ms");
                
            } catch (Exception e) {
                appendLog("❌ 库信息异常: " + e.getMessage());
                Log.e(TAG, "Library info error", e);
            }
            
            appendLog("=== 库信息测试完成 ===\n");
        }).start();
    }
    
    private void testFullInitialization() {
        appendLog("=== 开始完整初始化测试 ===");
        
        new Thread(() -> {
            try {
                String internalDataPath = getFilesDir().getAbsolutePath();
                appendLog("内部数据路径: " + internalDataPath);
                appendLog("正在进行完整的InspireFace初始化...");
                appendLog("这可能需要几秒钟时间...");
                
                long startTime = System.currentTimeMillis();
                int result = DirectInspireFaceTest.testInspireFaceInit(getAssets(), internalDataPath);
                long endTime = System.currentTimeMillis();
                
                if (result == 0) {
                    appendLog("🎉 InspireFace完整初始化成功！");
                    appendLog("✅ 所有组件都已正确初始化");
                    appendLog("✅ 人脸检测功能可用");
                    appendLog("✅ 属性分析功能可用");
                } else {
                    appendLog("❌ InspireFace初始化失败，错误码: " + result);
                    
                    switch (result) {
                        case -1:
                            appendLog("  原因: AssetManager获取失败");
                            break;
                        case -2:
                            appendLog("  原因: 内部数据路径获取失败");
                            break;
                        case -3:
                            appendLog("  原因: InspireFace库初始化失败");
                            break;
                        case -4:
                            appendLog("  原因: InspireFace会话创建失败");
                            break;
                        case -5:
                            appendLog("  原因: 人脸检测器初始化失败");
                            break;
                        case -6:
                            appendLog("  原因: C++异常");
                            break;
                        case -7:
                            appendLog("  原因: 未知异常");
                            break;
                        default:
                            appendLog("  原因: 未知错误码");
                            break;
                    }
                }
                
                appendLog("⏱️ 初始化耗时: " + (endTime - startTime) + " ms");
                
                // 获取内存使用情况
                Runtime runtime = Runtime.getRuntime();
                long usedMemory = (runtime.totalMemory() - runtime.freeMemory()) / (1024 * 1024);
                appendLog("💾 当前内存使用: " + usedMemory + " MB");
                
            } catch (Exception e) {
                appendLog("❌ 初始化异常: " + e.getMessage());
                Log.e(TAG, "Initialization error", e);
            }
            
            appendLog("=== 完整初始化测试完成 ===\n");
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
}
