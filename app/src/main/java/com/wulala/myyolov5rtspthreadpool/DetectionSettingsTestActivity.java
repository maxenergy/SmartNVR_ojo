package com.wulala.myyolov5rtspthreadpool;

import android.app.Activity;
import android.os.Bundle;
import android.util.Log;
import android.widget.Button;
import android.widget.TextView;

import java.util.ArrayList;
import java.util.List;
import java.util.Set;

/**
 * 检测设置测试活动
 * 用于测试检测过滤器功能
 */
public class DetectionSettingsTestActivity extends Activity {
    private static final String TAG = "DetectionSettingsTest";
    
    private DetectionSettingsManager settingsManager;
    private DetectionResultFilter resultFilter;
    private TextView textResults;
    
    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        
        // 创建简单的测试界面
        setContentView(createTestLayout());
        
        // 初始化组件
        settingsManager = new DetectionSettingsManager(this);
        resultFilter = new DetectionResultFilter(this);
        
        // 运行测试
        runDetectionFilterTest();
    }
    
    private int createTestLayout() {
        // 创建简单的线性布局
        android.widget.LinearLayout layout = new android.widget.LinearLayout(this);
        layout.setOrientation(android.widget.LinearLayout.VERTICAL);
        layout.setPadding(32, 32, 32, 32);
        
        // 标题
        TextView title = new TextView(this);
        title.setText("检测设置测试");
        title.setTextSize(20);
        title.setPadding(0, 0, 0, 24);
        layout.addView(title);
        
        // 结果显示
        textResults = new TextView(this);
        textResults.setText("正在运行测试...");
        textResults.setTextSize(14);
        textResults.setPadding(0, 0, 0, 24);
        layout.addView(textResults);
        
        // 重置按钮
        Button btnReset = new Button(this);
        btnReset.setText("重置设置");
        btnReset.setOnClickListener(v -> {
            settingsManager.resetToDefaults();
            runDetectionFilterTest();
        });
        layout.addView(btnReset);
        
        // 启用所有类别按钮
        Button btnEnableAll = new Button(this);
        btnEnableAll.setText("启用所有类别");
        btnEnableAll.setOnClickListener(v -> {
            Set<String> allClasses = new java.util.HashSet<>();
            for (String className : DetectionSettingsManager.ALL_YOLO_CLASSES) {
                allClasses.add(className);
            }
            settingsManager.setEnabledClasses(allClasses);
            runDetectionFilterTest();
        });
        layout.addView(btnEnableAll);
        
        setContentView(layout);
        return 0; // 不使用资源ID
    }
    
    private void runDetectionFilterTest() {
        StringBuilder results = new StringBuilder();
        results.append("=== 检测设置测试结果 ===\n\n");
        
        // 测试当前设置
        Set<String> enabledClasses = settingsManager.getEnabledClasses();
        float confidenceThreshold = settingsManager.getConfidenceThreshold();
        boolean detectionEnabled = settingsManager.isDetectionEnabled();
        
        results.append("当前设置:\n");
        results.append("- 检测启用: ").append(detectionEnabled).append("\n");
        results.append("- 置信度阈值: ").append(String.format("%.2f", confidenceThreshold)).append("\n");
        results.append("- 启用类别数: ").append(enabledClasses.size()).append("/").append(DetectionSettingsManager.ALL_YOLO_CLASSES.length).append("\n");
        results.append("- 启用类别: ").append(enabledClasses.toString()).append("\n\n");
        
        // 创建测试检测结果
        List<DetectionResultFilter.DetectionResult> testDetections = createTestDetections();
        results.append("测试检测结果 (").append(testDetections.size()).append(" 个):\n");
        for (DetectionResultFilter.DetectionResult detection : testDetections) {
            results.append("- ").append(detection.toString()).append("\n");
        }
        results.append("\n");
        
        // 应用过滤器
        List<DetectionResultFilter.DetectionResult> filteredResults = resultFilter.filterResults(testDetections);
        results.append("过滤后结果 (").append(filteredResults.size()).append(" 个):\n");
        for (DetectionResultFilter.DetectionResult detection : filteredResults) {
            results.append("- ").append(detection.toString()).append("\n");
        }
        results.append("\n");
        
        // 统计信息
        results.append("过滤统计:\n");
        results.append("- 原始检测: ").append(testDetections.size()).append(" 个\n");
        results.append("- 过滤后: ").append(filteredResults.size()).append(" 个\n");
        results.append("- 过滤率: ").append(String.format("%.1f%%", 
            (1.0 - (double)filteredResults.size() / testDetections.size()) * 100)).append("\n");
        
        // 更新显示
        textResults.setText(results.toString());
        
        Log.d(TAG, "检测过滤测试完成: " + testDetections.size() + " -> " + filteredResults.size());
    }
    
    private List<DetectionResultFilter.DetectionResult> createTestDetections() {
        List<DetectionResultFilter.DetectionResult> detections = new ArrayList<>();
        
        // 添加各种类别的测试检测结果
        String[] testClasses = {"person", "car", "bicycle", "sofa", "tv", "laptop", "bottle", "chair"};
        float[] testConfidences = {0.9f, 0.8f, 0.7f, 0.6f, 0.5f, 0.4f, 0.3f, 0.2f};
        
        for (int i = 0; i < testClasses.length; i++) {
            String className = testClasses[i];
            float confidence = testConfidences[i];
            int classId = DetectionSettingsManager.getClassIndex(className);
            
            DetectionResultFilter.DetectionResult detection = new DetectionResultFilter.DetectionResult(
                classId, confidence, 
                50 + i * 80, 50 + i * 60,  // x1, y1
                130 + i * 80, 170 + i * 60, // x2, y2
                className
            );
            
            detections.add(detection);
        }
        
        return detections;
    }
}
