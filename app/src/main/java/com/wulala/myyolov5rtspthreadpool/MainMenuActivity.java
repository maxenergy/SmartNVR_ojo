package com.wulala.myyolov5rtspthreadpool;

import android.app.Activity;
import android.content.Intent;
import android.os.Bundle;
import android.util.Log;
import android.widget.Button;
import android.widget.LinearLayout;
import android.widget.TextView;

/**
 * 主菜单Activity
 * 提供访问所有功能模块的入口
 */
public class MainMenuActivity extends Activity {
    
    private static final String TAG = "MainMenuActivity";
    
    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        
        createLayout();
        
        Log.i(TAG, "主菜单Activity已启动");
    }
    
    private void createLayout() {
        LinearLayout mainLayout = new LinearLayout(this);
        mainLayout.setOrientation(LinearLayout.VERTICAL);
        mainLayout.setPadding(20, 20, 20, 20);
        
        // 标题
        TextView titleView = new TextView(this);
        titleView.setText("InspireFace集成项目");
        titleView.setTextSize(24);
        titleView.setPadding(0, 0, 0, 20);
        mainLayout.addView(titleView);
        
        // 副标题
        TextView subtitleView = new TextView(this);
        subtitleView.setText("YOLOv5 + InspireFace 实时AI分析系统");
        subtitleView.setTextSize(16);
        subtitleView.setPadding(0, 0, 0, 30);
        mainLayout.addView(subtitleView);
        
        // 实时视频流AI分析
        Button realTimeVideoButton = new Button(this);
        realTimeVideoButton.setText("🎥 实时视频流AI分析");
        realTimeVideoButton.setTextSize(18);
        realTimeVideoButton.setPadding(20, 20, 20, 20);
        realTimeVideoButton.setOnClickListener(v -> {
            Intent intent = new Intent(this, RealTimeVideoActivity.class);
            startActivity(intent);
        });
        mainLayout.addView(realTimeVideoButton);
        
        // 分隔线
        addSeparator(mainLayout);
        
        // 组件测试区域
        TextView testSectionTitle = new TextView(this);
        testSectionTitle.setText("组件测试");
        testSectionTitle.setTextSize(18);
        testSectionTitle.setPadding(0, 10, 0, 10);
        mainLayout.addView(testSectionTitle);
        
        // InspireFace测试
        Button inspireFaceTestButton = new Button(this);
        inspireFaceTestButton.setText("👤 InspireFace人脸分析测试");
        inspireFaceTestButton.setOnClickListener(v -> {
            Intent intent = new Intent(this, DirectInspireFaceTestActivity.class);
            startActivity(intent);
        });
        mainLayout.addView(inspireFaceTestButton);
        
        // YOLOv5测试
        Button yoloTestButton = new Button(this);
        yoloTestButton.setText("🎯 YOLOv5推理测试");
        yoloTestButton.setOnClickListener(v -> {
            Intent intent = new Intent(this, RealYOLOTestActivity.class);
            startActivity(intent);
        });
        mainLayout.addView(yoloTestButton);
        
        // 集成AI测试
        Button integratedTestButton = new Button(this);
        integratedTestButton.setText("🤖 集成AI系统测试");
        integratedTestButton.setOnClickListener(v -> {
            Intent intent = new Intent(this, IntegratedAITestActivity.class);
            startActivity(intent);
        });
        mainLayout.addView(integratedTestButton);
        
        // 完整Pipeline测试
        Button pipelineTestButton = new Button(this);
        pipelineTestButton.setText("🔄 完整Pipeline测试");
        pipelineTestButton.setOnClickListener(v -> {
            Intent intent = new Intent(this, CompletePipelineTestActivity.class);
            startActivity(intent);
        });
        mainLayout.addView(pipelineTestButton);
        
        // 分隔线
        addSeparator(mainLayout);
        
        // 系统信息
        TextView systemInfoTitle = new TextView(this);
        systemInfoTitle.setText("系统信息");
        systemInfoTitle.setTextSize(18);
        systemInfoTitle.setPadding(0, 10, 0, 10);
        mainLayout.addView(systemInfoTitle);
        
        TextView systemInfoView = new TextView(this);
        systemInfoView.setText(getSystemInfo());
        systemInfoView.setTextSize(12);
        systemInfoView.setPadding(10, 5, 10, 5);
        systemInfoView.setBackgroundColor(0xFFF0F0F0);
        mainLayout.addView(systemInfoView);
        
        setContentView(mainLayout);
    }
    
    private void addSeparator(LinearLayout parent) {
        TextView separator = new TextView(this);
        separator.setText("────────────────────────────────");
        separator.setTextSize(12);
        separator.setPadding(0, 10, 0, 10);
        parent.addView(separator);
    }
    
    private String getSystemInfo() {
        StringBuilder info = new StringBuilder();
        
        // 设备信息
        info.append("设备型号: ").append(android.os.Build.MODEL).append("\n");
        info.append("Android版本: ").append(android.os.Build.VERSION.RELEASE).append("\n");
        info.append("API级别: ").append(android.os.Build.VERSION.SDK_INT).append("\n");
        info.append("CPU架构: ").append(android.os.Build.SUPPORTED_ABIS[0]).append("\n");
        
        // 内存信息
        Runtime runtime = Runtime.getRuntime();
        long maxMemory = runtime.maxMemory() / (1024 * 1024);
        long totalMemory = runtime.totalMemory() / (1024 * 1024);
        long freeMemory = runtime.freeMemory() / (1024 * 1024);
        long usedMemory = totalMemory - freeMemory;
        
        info.append("最大内存: ").append(maxMemory).append("MB\n");
        info.append("已分配: ").append(totalMemory).append("MB\n");
        info.append("已使用: ").append(usedMemory).append("MB\n");
        info.append("可用: ").append(freeMemory).append("MB");
        
        return info.toString();
    }
}
