package com.wulala.myyolov5rtspthreadpool;

import android.app.Activity;
import android.content.res.AssetManager;
import android.os.Bundle;
import android.os.Handler;
import android.os.Looper;
import android.util.Log;
import android.widget.Button;
import android.widget.ScrollView;
import android.widget.TextView;

import java.io.File;
import java.io.FileOutputStream;
import java.io.InputStream;

/**
 * 模型文件测试Activity
 * 专门用于测试模型文件的复制和验证
 */
public class ModelFileTestActivity extends Activity {
    
    private static final String TAG = "ModelFileTest";
    
    private TextView logTextView;
    private ScrollView scrollView;
    private Button copyButton;
    private Button validateButton;
    private Button clearButton;
    
    private StringBuilder logBuffer = new StringBuilder();
    private Handler mainHandler = new Handler(Looper.getMainLooper());
    
    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        
        createLayout();
        setupButtonListeners();
        
        appendLog("模型文件测试界面已启动");
        appendLog("点击按钮开始测试模型文件复制功能");
    }
    
    private void createLayout() {
        android.widget.LinearLayout layout = new android.widget.LinearLayout(this);
        layout.setOrientation(android.widget.LinearLayout.VERTICAL);
        layout.setPadding(20, 20, 20, 20);
        
        TextView titleView = new TextView(this);
        titleView.setText("模型文件复制测试");
        titleView.setTextSize(20);
        titleView.setPadding(0, 0, 0, 20);
        layout.addView(titleView);
        
        copyButton = new Button(this);
        copyButton.setText("复制模型文件");
        layout.addView(copyButton);
        
        validateButton = new Button(this);
        validateButton.setText("验证模型文件");
        layout.addView(validateButton);
        
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
        copyButton.setOnClickListener(v -> testModelFileCopy());
        validateButton.setOnClickListener(v -> testModelFileValidation());
        clearButton.setOnClickListener(v -> clearLog());
    }
    
    private void testModelFileCopy() {
        appendLog("=== 开始模型文件复制测试 ===");
        
        new Thread(() -> {
            try {
                AssetManager assetManager = getAssets();
                File filesDir = getFilesDir();
                File modelDir = new File(filesDir, "inspireface");
                
                appendLog("创建模型目录: " + modelDir.getAbsolutePath());
                
                if (!modelDir.exists()) {
                    boolean created = modelDir.mkdirs();
                    appendLog("目录创建结果: " + (created ? "成功" : "失败"));
                } else {
                    appendLog("目录已存在");
                }
                
                // 列出assets中的模型文件
                appendLog("正在列出assets中的模型文件...");
                String[] assetFiles = assetManager.list("inspireface");
                
                if (assetFiles != null && assetFiles.length > 0) {
                    appendLog("找到 " + assetFiles.length + " 个模型文件");
                    
                    long totalCopied = 0;
                    int successCount = 0;
                    
                    for (String fileName : assetFiles) {
                        appendLog("正在复制: " + fileName);
                        
                        try {
                            long fileSize = copyAssetFile(assetManager, "inspireface/" + fileName, 
                                                        new File(modelDir, fileName));
                            if (fileSize > 0) {
                                totalCopied += fileSize;
                                successCount++;
                                appendLog("✅ " + fileName + " (" + formatFileSize(fileSize) + ")");
                            } else {
                                appendLog("❌ " + fileName + " (复制失败)");
                            }
                        } catch (Exception e) {
                            appendLog("❌ " + fileName + " (异常: " + e.getMessage() + ")");
                        }
                    }
                    
                    appendLog("复制完成: " + successCount + "/" + assetFiles.length + " 个文件");
                    appendLog("总大小: " + formatFileSize(totalCopied));
                    
                } else {
                    appendLog("❌ 未找到assets中的模型文件");
                }
                
            } catch (Exception e) {
                appendLog("❌ 复制异常: " + e.getMessage());
                Log.e(TAG, "Copy error", e);
            }
            
            appendLog("=== 模型文件复制测试完成 ===\n");
        }).start();
    }
    
    private void testModelFileValidation() {
        appendLog("=== 开始模型文件验证测试 ===");
        
        new Thread(() -> {
            try {
                File filesDir = getFilesDir();
                File modelDir = new File(filesDir, "inspireface");
                
                appendLog("检查模型目录: " + modelDir.getAbsolutePath());
                
                if (!modelDir.exists()) {
                    appendLog("❌ 模型目录不存在");
                    return;
                }
                
                File[] files = modelDir.listFiles();
                if (files == null || files.length == 0) {
                    appendLog("❌ 模型目录为空");
                    return;
                }
                
                appendLog("找到 " + files.length + " 个文件:");
                
                long totalSize = 0;
                int rknnCount = 0;
                int mnnCount = 0;
                int configCount = 0;
                
                for (File file : files) {
                    if (file.isFile()) {
                        long size = file.length();
                        totalSize += size;
                        
                        String name = file.getName();
                        if (name.endsWith(".rknn")) {
                            rknnCount++;
                        } else if (name.endsWith(".mnn")) {
                            mnnCount++;
                        } else if (name.equals("__inspire__")) {
                            configCount++;
                        }
                        
                        appendLog("  " + name + " (" + formatFileSize(size) + ")");
                    }
                }
                
                appendLog("📊 统计信息:");
                appendLog("  - RKNN模型: " + rknnCount + " 个");
                appendLog("  - MNN模型: " + mnnCount + " 个");
                appendLog("  - 配置文件: " + configCount + " 个");
                appendLog("  - 总大小: " + formatFileSize(totalSize));
                
                // 验证关键文件
                String[] criticalFiles = {
                    "__inspire__",
                    "_00_scrfd_2_5g_bnkps_shape320x320_rk3588.rknn",
                    "_08_fairface_model_rk3588.rknn"
                };
                
                appendLog("🔍 验证关键文件:");
                boolean allExist = true;
                for (String fileName : criticalFiles) {
                    File file = new File(modelDir, fileName);
                    if (file.exists()) {
                        appendLog("  ✅ " + fileName + " (" + formatFileSize(file.length()) + ")");
                    } else {
                        appendLog("  ❌ " + fileName + " (缺失)");
                        allExist = false;
                    }
                }
                
                if (allExist) {
                    appendLog("✅ 所有关键文件都存在");
                } else {
                    appendLog("❌ 部分关键文件缺失");
                }
                
            } catch (Exception e) {
                appendLog("❌ 验证异常: " + e.getMessage());
                Log.e(TAG, "Validation error", e);
            }
            
            appendLog("=== 模型文件验证测试完成 ===\n");
        }).start();
    }
    
    private long copyAssetFile(AssetManager assetManager, String assetPath, File targetFile) throws Exception {
        InputStream inputStream = assetManager.open(assetPath);
        FileOutputStream outputStream = new FileOutputStream(targetFile);
        
        byte[] buffer = new byte[8192];
        int bytesRead;
        long totalBytes = 0;
        
        while ((bytesRead = inputStream.read(buffer)) != -1) {
            outputStream.write(buffer, 0, bytesRead);
            totalBytes += bytesRead;
        }
        
        inputStream.close();
        outputStream.close();
        
        return totalBytes;
    }
    
    private String formatFileSize(long bytes) {
        if (bytes < 1024) {
            return bytes + " B";
        } else if (bytes < 1024 * 1024) {
            return String.format("%.1f KB", bytes / 1024.0);
        } else {
            return String.format("%.1f MB", bytes / (1024.0 * 1024.0));
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
}
