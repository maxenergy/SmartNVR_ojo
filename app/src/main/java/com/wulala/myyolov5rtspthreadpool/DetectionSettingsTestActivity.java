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
    private DetectionSettingsValidator validator;
    private TextView textResults;
    
    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);

        // 初始化组件
        settingsManager = new DetectionSettingsManager(this);
        resultFilter = new DetectionResultFilter(this);
        validator = new DetectionSettingsValidator(this);

        // 创建简单的测试界面
        createTestLayout();

        // 运行测试
        runDetectionFilterTest();
    }
    
    private void createTestLayout() {
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

        // 功能验证按钮
        Button btnValidate = new Button(this);
        btnValidate.setText("运行功能验证");
        btnValidate.setOnClickListener(v -> {
            runFunctionValidation();
        });
        layout.addView(btnValidate);

        // 测试类别切换功能
        Button btnTestToggle = new Button(this);
        btnTestToggle.setText("测试类别切换功能");
        btnTestToggle.setOnClickListener(v -> {
            testClassToggleFunction();
        });
        layout.addView(btnTestToggle);

        setContentView(layout);
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

    /**
     * 运行功能验证
     */
    private void runFunctionValidation() {
        StringBuilder results = new StringBuilder();
        results.append("=== 功能验证结果 ===\n\n");

        // 运行验证
        DetectionSettingsValidator.ValidationResult validationResult = validator.runFullValidation();

        results.append("验证项目:\n");
        results.append("✅ 设置管理器: ").append(validationResult.settingsManagerValid ? "通过" : "❌ 失败").append("\n");
        results.append("✅ 检测过滤器: ").append(validationResult.filterValid ? "通过" : "❌ 失败").append("\n");
        results.append("✅ 数据持久化: ").append(validationResult.persistenceValid ? "通过" : "❌ 失败").append("\n");
        results.append("✅ 类别映射: ").append(validationResult.classMappingValid ? "通过" : "❌ 失败").append("\n");
        results.append("✅ 边界条件: ").append(validationResult.boundaryConditionsValid ? "通过" : "❌ 失败").append("\n\n");

        results.append("总体结果: ").append(validationResult.overallValid ? "🎉 全部通过" : "❌ 存在问题").append("\n\n");

        if (validationResult.overallValid) {
            results.append("🎯 功能验证完成！检测设置功能工作正常。\n");
            results.append("您可以安全地使用所有检测设置功能。\n");
        } else {
            results.append("⚠️ 发现问题！请检查日志获取详细信息。\n");
        }

        // 更新显示
        textResults.setText(results.toString());

        Log.d(TAG, "功能验证完成: " + validationResult.toString());
    }

    /**
     * 测试类别切换功能
     */
    private void testClassToggleFunction() {
        Log.d(TAG, "=== 开始测试类别切换功能 ===");

        // 1. 测试初始状态
        Set<String> initialClasses = settingsManager.getEnabledClasses();
        Log.d(TAG, "初始启用类别: " + initialClasses.toString());

        // 2. 测试禁用一个类别
        Log.d(TAG, "测试禁用bicycle类别");
        settingsManager.setClassEnabled("bicycle", false);
        Set<String> afterDisable = settingsManager.getEnabledClasses();
        Log.d(TAG, "禁用bicycle后的类别: " + afterDisable.toString());

        // 3. 测试启用一个类别
        Log.d(TAG, "测试启用motorcycle类别");
        settingsManager.setClassEnabled("motorcycle", true);
        Set<String> afterEnable = settingsManager.getEnabledClasses();
        Log.d(TAG, "启用motorcycle后的类别: " + afterEnable.toString());

        // 4. 测试多个类别切换
        Log.d(TAG, "测试多个类别切换");
        settingsManager.setClassEnabled("bus", true);
        settingsManager.setClassEnabled("truck", true);
        settingsManager.setClassEnabled("car", false);
        Set<String> afterMultiple = settingsManager.getEnabledClasses();
        Log.d(TAG, "多个切换后的类别: " + afterMultiple.toString());

        // 5. 验证过滤器是否正确应用设置
        Log.d(TAG, "测试过滤器应用");
        DetectionResultFilter filter = new DetectionResultFilter(this);

        // 创建测试检测结果 (classId, confidence, x1, y1, x2, y2, className)
        java.util.List<DetectionResultFilter.DetectionResult> testResults = new java.util.ArrayList<>();
        testResults.add(new DetectionResultFilter.DetectionResult(0, 0.8f, 100f, 100f, 200f, 200f, "person"));
        testResults.add(new DetectionResultFilter.DetectionResult(2, 0.7f, 300f, 300f, 400f, 400f, "car"));
        testResults.add(new DetectionResultFilter.DetectionResult(1, 0.6f, 500f, 500f, 600f, 600f, "bicycle"));
        testResults.add(new DetectionResultFilter.DetectionResult(3, 0.9f, 700f, 700f, 800f, 800f, "motorcycle"));
        testResults.add(new DetectionResultFilter.DetectionResult(5, 0.75f, 900f, 900f, 1000f, 1000f, "bus"));

        java.util.List<DetectionResultFilter.DetectionResult> filteredResults = filter.filterResults(testResults);
        Log.d(TAG, "原始检测结果数量: " + testResults.size());
        Log.d(TAG, "过滤后检测结果数量: " + filteredResults.size());

        for (DetectionResultFilter.DetectionResult result : filteredResults) {
            Log.d(TAG, "过滤后保留: " + result.className + " (置信度: " + result.confidence + ")");
        }

        Log.d(TAG, "=== 类别切换功能测试完成 ===");
    }
}
