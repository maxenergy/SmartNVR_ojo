package com.wulala.myyolov5rtspthreadpool;

import android.app.Activity;
import android.graphics.Bitmap;
import android.os.Bundle;
import android.os.Handler;
import android.os.Looper;
import android.util.Log;
import android.view.SurfaceHolder;
import android.view.SurfaceView;
import android.view.View;
import android.widget.Button;
import android.widget.ImageView;
import android.widget.LinearLayout;
import android.widget.TextView;
import android.widget.Toast;

/**
 * 实时视频流处理Activity
 * 显示RTSP视频流并进行实时AI分析
 */
public class RealTimeVideoActivity extends Activity implements RealTimeVideoProcessor.ProcessingCallback {
    
    private static final String TAG = "RealTimeVideoActivity";
    private static final String RTSP_URL = "rtsp://192.168.31.22:8554/unicast";
    
    // UI组件
    private SurfaceView surfaceView;
    private ImageView processedImageView;
    private TextView statusTextView;
    private TextView statisticsTextView;
    private TextView performanceTextView;
    private Button startButton;
    private Button stopButton;
    private Button resetButton;
    private LinearLayout controlPanel;
    
    // 核心组件
    private RealTimeVideoProcessor videoProcessor;
    private RTSPVideoReceiver videoReceiver;
    private Handler mainHandler;
    
    // 状态管理
    private boolean isStreaming = false;
    private boolean isProcessing = false;
    
    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        
        mainHandler = new Handler(Looper.getMainLooper());
        
        createLayout();
        initializeComponents();
        setupEventListeners();
        
        Log.i(TAG, "实时视频流Activity已启动");
        updateStatus("就绪 - 点击开始按钮启动视频流");
    }
    
    private void createLayout() {
        LinearLayout mainLayout = new LinearLayout(this);
        mainLayout.setOrientation(LinearLayout.VERTICAL);
        mainLayout.setPadding(10, 10, 10, 10);
        
        // 标题
        TextView titleView = new TextView(this);
        titleView.setText("实时视频流AI分析");
        titleView.setTextSize(20);
        titleView.setPadding(0, 0, 0, 10);
        mainLayout.addView(titleView);
        
        // 视频显示区域
        createVideoDisplayArea(mainLayout);
        
        // 控制面板
        createControlPanel(mainLayout);
        
        // 统计信息面板
        createStatisticsPanel(mainLayout);
        
        // 性能监控面板
        createPerformancePanel(mainLayout);
        
        setContentView(mainLayout);
    }
    
    private void createVideoDisplayArea(LinearLayout parent) {
        LinearLayout videoLayout = new LinearLayout(this);
        videoLayout.setOrientation(LinearLayout.HORIZONTAL);
        videoLayout.setPadding(5, 5, 5, 5);

        // 原始视频流区域
        LinearLayout originalVideoLayout = new LinearLayout(this);
        originalVideoLayout.setOrientation(LinearLayout.VERTICAL);
        originalVideoLayout.setPadding(5, 0, 5, 0);

        TextView originalLabel = new TextView(this);
        originalLabel.setText("📹 原始RTSP视频流");
        originalLabel.setTextSize(16);
        originalLabel.setTextColor(0xFF2196F3); // 蓝色
        originalLabel.setPadding(0, 0, 0, 8);
        originalVideoLayout.addView(originalLabel);

        surfaceView = new SurfaceView(this);
        LinearLayout.LayoutParams surfaceParams = new LinearLayout.LayoutParams(420, 320);
        surfaceParams.setMargins(2, 2, 2, 2);
        surfaceView.setLayoutParams(surfaceParams);
        surfaceView.setBackgroundColor(0xFF000000); // 黑色背景
        originalVideoLayout.addView(surfaceView);

        videoLayout.addView(originalVideoLayout);

        // AI分析结果区域
        LinearLayout processedVideoLayout = new LinearLayout(this);
        processedVideoLayout.setOrientation(LinearLayout.VERTICAL);
        processedVideoLayout.setPadding(5, 0, 5, 0);

        TextView processedLabel = new TextView(this);
        processedLabel.setText("🤖 AI分析结果 (YOLOv5 + InspireFace)");
        processedLabel.setTextSize(16);
        processedLabel.setTextColor(0xFF4CAF50); // 绿色
        processedLabel.setPadding(0, 0, 0, 8);
        processedVideoLayout.addView(processedLabel);

        processedImageView = new ImageView(this);
        LinearLayout.LayoutParams imageParams = new LinearLayout.LayoutParams(420, 320);
        imageParams.setMargins(2, 2, 2, 2);
        processedImageView.setLayoutParams(imageParams);
        processedImageView.setScaleType(ImageView.ScaleType.FIT_CENTER);
        processedImageView.setBackgroundColor(0xFF1A1A1A); // 深灰色背景
        processedVideoLayout.addView(processedImageView);

        videoLayout.addView(processedVideoLayout);

        parent.addView(videoLayout);

        // 添加分隔线
        View separator = new View(this);
        separator.setBackgroundColor(0xFFCCCCCC);
        LinearLayout.LayoutParams sepParams = new LinearLayout.LayoutParams(
            LinearLayout.LayoutParams.MATCH_PARENT, 2);
        sepParams.setMargins(0, 10, 0, 10);
        separator.setLayoutParams(sepParams);
        parent.addView(separator);
    }
    
    private void createControlPanel(LinearLayout parent) {
        controlPanel = new LinearLayout(this);
        controlPanel.setOrientation(LinearLayout.HORIZONTAL);
        controlPanel.setPadding(0, 10, 0, 10);
        
        startButton = new Button(this);
        startButton.setText("开始视频流");
        controlPanel.addView(startButton);
        
        stopButton = new Button(this);
        stopButton.setText("停止视频流");
        stopButton.setEnabled(false);
        controlPanel.addView(stopButton);
        
        resetButton = new Button(this);
        resetButton.setText("重置统计");
        controlPanel.addView(resetButton);
        
        parent.addView(controlPanel);
        
        // 状态显示
        statusTextView = new TextView(this);
        statusTextView.setText("状态: 未连接");
        statusTextView.setTextSize(14);
        statusTextView.setPadding(0, 5, 0, 5);
        parent.addView(statusTextView);
    }
    
    private void createStatisticsPanel(LinearLayout parent) {
        TextView statsLabel = new TextView(this);
        statsLabel.setText("📊 实时AI分析统计");
        statsLabel.setTextSize(18);
        statsLabel.setTextColor(0xFF673AB7); // 紫色
        statsLabel.setPadding(0, 15, 0, 8);
        statsLabel.getPaint().setFakeBoldText(true);
        parent.addView(statsLabel);

        statisticsTextView = new TextView(this);
        statisticsTextView.setText("🔍 等待AI分析数据...\n" +
                                  "👥 当前人数: 0\n" +
                                  "👨 男性: 0  👩 女性: 0\n" +
                                  "📈 累计检测: 0 人次\n" +
                                  "🎂 年龄分布: 暂无数据");
        statisticsTextView.setTextSize(14);
        statisticsTextView.setPadding(15, 10, 15, 10);
        statisticsTextView.setBackgroundColor(0xFFF3E5F5); // 淡紫色背景
        statisticsTextView.setLineSpacing(4, 1.0f);
        parent.addView(statisticsTextView);
    }
    
    private void createPerformancePanel(LinearLayout parent) {
        TextView perfLabel = new TextView(this);
        perfLabel.setText("⚡ 系统性能监控");
        perfLabel.setTextSize(18);
        perfLabel.setTextColor(0xFFFF5722); // 橙色
        perfLabel.setPadding(0, 15, 0, 8);
        perfLabel.getPaint().setFakeBoldText(true);
        parent.addView(perfLabel);

        performanceTextView = new TextView(this);
        performanceTextView.setText("🎯 FPS: 0.0\n" +
                                   "⏱️ 处理耗时: 0ms\n" +
                                   "📊 平均耗时: 0ms\n" +
                                   "🔢 总帧数: 0\n" +
                                   "💾 内存使用: 监控中...");
        performanceTextView.setTextSize(14);
        performanceTextView.setPadding(15, 10, 15, 10);
        performanceTextView.setBackgroundColor(0xFFFFF3E0); // 淡橙色背景
        performanceTextView.setLineSpacing(4, 1.0f);
        parent.addView(performanceTextView);
    }
    
    private void initializeComponents() {
        // 初始化视频处理器
        videoProcessor = RealTimeVideoProcessor.getInstance();
        
        // 初始化RTSP接收器
        videoReceiver = new RTSPVideoReceiver();
    }
    
    private void setupEventListeners() {
        startButton.setOnClickListener(v -> startVideoStream());
        stopButton.setOnClickListener(v -> stopVideoStream());
        resetButton.setOnClickListener(v -> resetStatistics());
        
        // 设置SurfaceView回调
        surfaceView.getHolder().addCallback(new SurfaceHolder.Callback() {
            @Override
            public void surfaceCreated(SurfaceHolder holder) {
                Log.d(TAG, "Surface创建完成");
            }
            
            @Override
            public void surfaceChanged(SurfaceHolder holder, int format, int width, int height) {
                Log.d(TAG, "Surface尺寸变化: " + width + "x" + height);
            }
            
            @Override
            public void surfaceDestroyed(SurfaceHolder holder) {
                Log.d(TAG, "Surface销毁");
            }
        });
    }
    
    private void startVideoStream() {
        Log.i(TAG, "开始启动视频流...");
        updateStatus("正在初始化AI组件...");
        
        new Thread(() -> {
            try {
                // 初始化视频处理器
                boolean initSuccess = videoProcessor.initialize(this, this);
                if (!initSuccess) {
                    mainHandler.post(() -> {
                        updateStatus("AI组件初始化失败");
                        showToast("AI组件初始化失败，请检查模型文件");
                    });
                    return;
                }
                
                mainHandler.post(() -> updateStatus("正在连接RTSP视频流..."));
                
                // 启动RTSP接收器
                boolean streamSuccess = videoReceiver.startStream(RTSP_URL, surfaceView.getHolder(), 
                    new RTSPVideoReceiver.FrameCallback() {
                        @Override
                        public void onFrameReceived(Bitmap frame) {
                            // 处理接收到的帧
                            if (isProcessing && videoProcessor.isInitialized()) {
                                videoProcessor.processFrame(frame);
                            }
                        }
                        
                        @Override
                        public void onError(String error) {
                            mainHandler.post(() -> {
                                updateStatus("视频流错误: " + error);
                                showToast("视频流连接失败: " + error);
                            });
                        }
                    });
                
                if (streamSuccess) {
                    mainHandler.post(() -> {
                        isStreaming = true;
                        isProcessing = true;
                        startButton.setEnabled(false);
                        stopButton.setEnabled(true);
                        updateStatus("视频流已启动 - 正在进行AI分析");
                        showToast("视频流启动成功");
                    });
                } else {
                    mainHandler.post(() -> {
                        updateStatus("视频流启动失败");
                        showToast("无法连接到RTSP视频流");
                    });
                }
                
            } catch (Exception e) {
                Log.e(TAG, "启动视频流异常", e);
                mainHandler.post(() -> {
                    updateStatus("启动异常: " + e.getMessage());
                    showToast("启动失败: " + e.getMessage());
                });
            }
        }).start();
    }
    
    private void stopVideoStream() {
        Log.i(TAG, "停止视频流...");
        
        isStreaming = false;
        isProcessing = false;
        
        // 停止RTSP接收器
        if (videoReceiver != null) {
            videoReceiver.stopStream();
        }
        
        startButton.setEnabled(true);
        stopButton.setEnabled(false);
        
        updateStatus("视频流已停止");
        showToast("视频流已停止");
    }
    
    private void resetStatistics() {
        if (videoProcessor != null) {
            videoProcessor.resetStatistics();
            updateStatisticsDisplay(videoProcessor.getStatistics());
            showToast("统计数据已重置");
        }
    }
    
    private void updateStatus(String status) {
        if (statusTextView != null) {
            statusTextView.setText("状态: " + status);
        }
        Log.i(TAG, "状态更新: " + status);
    }
    
    private void showToast(String message) {
        Toast.makeText(this, message, Toast.LENGTH_SHORT).show();
    }
    
    // RealTimeVideoProcessor.ProcessingCallback 实现
    
    @Override
    public void onFrameProcessed(RealTimeVideoProcessor.ProcessingResult result) {
        mainHandler.post(() -> {
            if (result.success && result.annotatedFrame != null) {
                // 显示处理后的帧
                processedImageView.setImageBitmap(result.annotatedFrame);
            }
            
            // 更新性能信息
            updatePerformanceDisplay(result);
        });
    }
    
    @Override
    public void onStatisticsUpdated(RealTimeVideoProcessor.StatisticsData statistics) {
        mainHandler.post(() -> updateStatisticsDisplay(statistics));
    }
    
    @Override
    public void onError(String error) {
        mainHandler.post(() -> {
            updateStatus("处理错误: " + error);
            Log.e(TAG, "处理错误: " + error);
        });
    }
    
    private void updateStatisticsDisplay(RealTimeVideoProcessor.StatisticsData stats) {
        if (statisticsTextView != null) {
            String statsText = String.format(
                "🔍 AI分析状态: %s\n" +
                "👥 当前人数: %d 人\n" +
                "👨 男性: %d  👩 女性: %d\n" +
                "📈 累计检测: %d 人次\n" +
                "🎂 年龄分布: %s",
                isProcessing ? "正在分析" : "等待数据",
                stats.currentPersonCount,
                stats.totalMaleCount,
                stats.totalFemaleCount,
                stats.totalPersonCount,
                stats.getAgeDistributionSummary().isEmpty() ? "暂无数据" : stats.getAgeDistributionSummary()
            );
            statisticsTextView.setText(statsText);
        }
    }
    
    private void updatePerformanceDisplay(RealTimeVideoProcessor.ProcessingResult result) {
        if (performanceTextView != null) {
            RealTimeVideoProcessor.StatisticsData stats = videoProcessor.getStatistics();

            // 获取内存使用情况
            Runtime runtime = Runtime.getRuntime();
            long usedMemory = (runtime.totalMemory() - runtime.freeMemory()) / (1024 * 1024);
            long maxMemory = runtime.maxMemory() / (1024 * 1024);

            // 性能状态评估
            String performanceStatus = getPerformanceStatus(result.fps, result.processingTime);

            String perfText = String.format(
                "🎯 FPS: %.1f %s\n" +
                "⏱️ 处理耗时: %dms\n" +
                "📊 平均耗时: %dms\n" +
                "🔢 总帧数: %d\n" +
                "💾 内存: %dMB/%dMB",
                result.fps,
                performanceStatus,
                result.processingTime,
                stats.averageProcessingTime,
                stats.totalFrameCount,
                usedMemory,
                maxMemory
            );
            performanceTextView.setText(perfText);
        }
    }

    /**
     * 获取性能状态描述
     */
    private String getPerformanceStatus(float fps, long processingTime) {
        if (fps >= 15.0f && processingTime <= 50) {
            return "🟢"; // 绿色 - 优秀
        } else if (fps >= 10.0f && processingTime <= 100) {
            return "🟡"; // 黄色 - 良好
        } else {
            return "🔴"; // 红色 - 需要优化
        }
    }
    
    @Override
    protected void onPause() {
        super.onPause();
        if (isStreaming) {
            stopVideoStream();
        }
    }
    
    @Override
    protected void onDestroy() {
        super.onDestroy();
        
        // 清理资源
        if (videoReceiver != null) {
            videoReceiver.cleanup();
        }
        
        if (videoProcessor != null) {
            videoProcessor.cleanup();
        }
        
        Log.i(TAG, "实时视频流Activity已销毁");
    }
}
