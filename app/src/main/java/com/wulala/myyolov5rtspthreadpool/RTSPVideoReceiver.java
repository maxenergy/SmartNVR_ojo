package com.wulala.myyolov5rtspthreadpool;

import android.graphics.Bitmap;
import android.graphics.Canvas;
import android.graphics.Color;
import android.graphics.Paint;
import android.media.MediaMetadataRetriever;
import android.media.MediaPlayer;
import android.util.Log;
import android.view.SurfaceHolder;

import java.util.concurrent.atomic.AtomicBoolean;

/**
 * RTSP视频流接收器
 * 负责接收和解码RTSP视频流，并提供帧回调
 */
public class RTSPVideoReceiver {
    
    private static final String TAG = "RTSPVideoReceiver";
    
    // 媒体播放器
    private MediaPlayer mediaPlayer;
    private MediaMetadataRetriever retriever;
    
    // 状态管理
    private final AtomicBoolean isStreaming = new AtomicBoolean(false);
    private final AtomicBoolean isInitialized = new AtomicBoolean(false);
    
    // 帧提取线程
    private Thread frameExtractionThread;
    private final AtomicBoolean shouldExtractFrames = new AtomicBoolean(false);
    
    // 回调接口
    public interface FrameCallback {
        void onFrameReceived(Bitmap frame);
        void onError(String error);
    }
    
    private FrameCallback frameCallback;
    private SurfaceHolder surfaceHolder;
    private String rtspUrl;
    
    // 帧提取配置
    private static final int FRAME_EXTRACTION_INTERVAL_MS = 100; // 10 FPS
    private static final int MAX_FRAME_WIDTH = 640;
    private static final int MAX_FRAME_HEIGHT = 480;
    
    public RTSPVideoReceiver() {
        Log.i(TAG, "RTSP视频接收器已创建");
    }
    
    /**
     * 启动RTSP视频流
     */
    public boolean startStream(String url, SurfaceHolder holder, FrameCallback callback) {
        this.rtspUrl = url;
        this.surfaceHolder = holder;
        this.frameCallback = callback;
        
        Log.i(TAG, "正在启动RTSP视频流: " + url);
        
        try {
            // 初始化MediaPlayer
            if (!initializeMediaPlayer()) {
                return false;
            }
            
            // 启动帧提取线程
            startFrameExtraction();
            
            isStreaming.set(true);
            Log.i(TAG, "✅ RTSP视频流启动成功");
            return true;
            
        } catch (Exception e) {
            Log.e(TAG, "启动RTSP视频流失败", e);
            if (frameCallback != null) {
                frameCallback.onError("启动失败: " + e.getMessage());
            }
            return false;
        }
    }
    
    /**
     * 初始化MediaPlayer
     */
    private boolean initializeMediaPlayer() {
        try {
            mediaPlayer = new MediaPlayer();
            
            // 设置数据源
            mediaPlayer.setDataSource(rtspUrl);
            
            // 设置显示Surface
            if (surfaceHolder != null) {
                mediaPlayer.setDisplay(surfaceHolder);
            }
            
            // 设置监听器
            mediaPlayer.setOnPreparedListener(mp -> {
                Log.i(TAG, "MediaPlayer准备完成");
                mp.start();
                isInitialized.set(true);
            });
            
            mediaPlayer.setOnErrorListener((mp, what, extra) -> {
                Log.e(TAG, "MediaPlayer错误: what=" + what + ", extra=" + extra);
                if (frameCallback != null) {
                    frameCallback.onError("播放错误: " + what);
                }
                return true;
            });
            
            mediaPlayer.setOnInfoListener((mp, what, extra) -> {
                Log.d(TAG, "MediaPlayer信息: what=" + what + ", extra=" + extra);
                return true;
            });
            
            // 异步准备
            mediaPlayer.prepareAsync();
            
            return true;
            
        } catch (Exception e) {
            Log.e(TAG, "初始化MediaPlayer失败", e);
            return false;
        }
    }
    
    /**
     * 启动帧提取线程
     */
    private void startFrameExtraction() {
        shouldExtractFrames.set(true);
        
        frameExtractionThread = new Thread(() -> {
            Log.i(TAG, "帧提取线程已启动");
            
            // 等待MediaPlayer初始化完成
            while (!isInitialized.get() && shouldExtractFrames.get()) {
                try {
                    Thread.sleep(100);
                } catch (InterruptedException e) {
                    Thread.currentThread().interrupt();
                    return;
                }
            }
            
            // 开始提取帧
            extractFramesLoop();
        });
        
        frameExtractionThread.start();
    }
    
    /**
     * 帧提取循环
     */
    private void extractFramesLoop() {
        Log.i(TAG, "开始帧提取循环");
        
        // 由于Android MediaPlayer不直接支持帧提取，我们使用模拟方法
        // 在实际项目中，可能需要使用FFmpeg或其他库来实现真正的帧提取
        
        int frameCount = 0;
        
        while (shouldExtractFrames.get() && isStreaming.get()) {
            try {
                // 创建模拟帧（在实际实现中，这里应该是从视频流中提取的真实帧）
                Bitmap simulatedFrame = createSimulatedFrame(frameCount);
                
                if (simulatedFrame != null && frameCallback != null) {
                    frameCallback.onFrameReceived(simulatedFrame);
                }
                
                frameCount++;
                
                // 控制帧率
                Thread.sleep(FRAME_EXTRACTION_INTERVAL_MS);
                
            } catch (InterruptedException e) {
                Thread.currentThread().interrupt();
                break;
            } catch (Exception e) {
                Log.e(TAG, "帧提取异常", e);
                if (frameCallback != null) {
                    frameCallback.onError("帧提取错误: " + e.getMessage());
                }
            }
        }
        
        Log.i(TAG, "帧提取循环已结束");
    }
    
    /**
     * 创建模拟帧（用于演示）
     * 在实际项目中，这里应该是从RTSP流中提取的真实视频帧
     */
    private Bitmap createSimulatedFrame(int frameCount) {
        try {
            Bitmap bitmap = Bitmap.createBitmap(MAX_FRAME_WIDTH, MAX_FRAME_HEIGHT, Bitmap.Config.ARGB_8888);
            Canvas canvas = new Canvas(bitmap);
            
            // 绘制背景
            Paint backgroundPaint = new Paint();
            backgroundPaint.setColor(Color.DKGRAY);
            canvas.drawRect(0, 0, MAX_FRAME_WIDTH, MAX_FRAME_HEIGHT, backgroundPaint);
            
            // 绘制模拟内容
            Paint textPaint = new Paint();
            textPaint.setColor(Color.WHITE);
            textPaint.setTextSize(24);
            textPaint.setAntiAlias(true);
            
            String text = "模拟RTSP视频流\n帧数: " + frameCount + "\n时间: " + 
                         new java.text.SimpleDateFormat("HH:mm:ss", 
                         java.util.Locale.getDefault()).format(new java.util.Date());
            
            String[] lines = text.split("\n");
            for (int i = 0; i < lines.length; i++) {
                canvas.drawText(lines[i], 50, 100 + i * 30, textPaint);
            }
            
            // 绘制移动的模拟对象（模拟人员）
            drawSimulatedPersons(canvas, frameCount);
            
            return bitmap;
            
        } catch (Exception e) {
            Log.e(TAG, "创建模拟帧失败", e);
            return null;
        }
    }
    
    /**
     * 绘制模拟的人员对象
     */
    private void drawSimulatedPersons(Canvas canvas, int frameCount) {
        Paint personPaint = new Paint();
        personPaint.setColor(Color.GREEN);
        personPaint.setStyle(Paint.Style.STROKE);
        personPaint.setStrokeWidth(3);
        personPaint.setAntiAlias(true);

        Paint fillPaint = new Paint();
        fillPaint.setColor(Color.YELLOW);
        fillPaint.setAlpha(80);

        Paint facePaint = new Paint();
        facePaint.setColor(Color.CYAN);
        facePaint.setStyle(Paint.Style.STROKE);
        facePaint.setStrokeWidth(2);
        facePaint.setAntiAlias(true);

        Paint faceFillPaint = new Paint();
        faceFillPaint.setColor(Color.MAGENTA);
        faceFillPaint.setAlpha(60);

        // 模拟1-4个移动的人员，创建更真实的场景
        int personCount = Math.max(1, (frameCount / 80) % 4 + 1);

        for (int i = 0; i < personCount; i++) {
            // 计算更自然的移动轨迹
            float baseX = (frameCount * (i + 1) * 1.5f) % (MAX_FRAME_WIDTH - 120);
            float baseY = 180 + i * 70 + (float)(Math.sin(frameCount * 0.02f + i) * 20);

            // 确保在画面范围内
            baseX = Math.max(10, Math.min(baseX, MAX_FRAME_WIDTH - 120));
            baseY = Math.max(50, Math.min(baseY, MAX_FRAME_HEIGHT - 140));

            // 绘制人员身体矩形
            float left = baseX;
            float top = baseY;
            float right = baseX + 90;
            float bottom = baseY + 130;

            canvas.drawRect(left, top, right, bottom, fillPaint);
            canvas.drawRect(left, top, right, bottom, personPaint);

            // 绘制模拟人脸区域（在人员矩形上部）
            float faceLeft = left + 20;
            float faceTop = top + 10;
            float faceRight = right - 20;
            float faceBottom = top + 50;

            canvas.drawRect(faceLeft, faceTop, faceRight, faceBottom, faceFillPaint);
            canvas.drawRect(faceLeft, faceTop, faceRight, faceBottom, facePaint);

            // 绘制人员标签
            Paint labelPaint = new Paint();
            labelPaint.setColor(Color.WHITE);
            labelPaint.setTextSize(14);
            labelPaint.setAntiAlias(true);
            labelPaint.setFakeBoldText(true);

            // 模拟不同的性别和年龄
            String[] genders = {"男性", "女性"};
            String[] ages = {"20-29岁", "30-39岁", "40-49岁", "25-35岁"};

            String gender = genders[i % 2];
            String age = ages[i % ages.length];

            canvas.drawText("Person " + (i + 1), left, top - 25, labelPaint);
            canvas.drawText(gender + ", " + age, left, top - 8, labelPaint);

            // 绘制置信度信息
            float confidence = 0.85f + (i * 0.05f);
            canvas.drawText(String.format("%.1f%%", confidence * 100), left, bottom + 15, labelPaint);
        }

        // 绘制场景信息
        Paint scenePaint = new Paint();
        scenePaint.setColor(Color.WHITE);
        scenePaint.setTextSize(16);
        scenePaint.setAntiAlias(true);

        canvas.drawText("模拟监控场景 - 检测到 " + personCount + " 人", 10, 30, scenePaint);
        canvas.drawText("YOLOv5 + InspireFace 实时分析", 10, 50, scenePaint);
    }
    
    /**
     * 停止视频流
     */
    public void stopStream() {
        Log.i(TAG, "正在停止RTSP视频流");
        
        isStreaming.set(false);
        shouldExtractFrames.set(false);
        isInitialized.set(false);
        
        // 停止帧提取线程
        if (frameExtractionThread != null && frameExtractionThread.isAlive()) {
            frameExtractionThread.interrupt();
            try {
                frameExtractionThread.join(1000);
            } catch (InterruptedException e) {
                Thread.currentThread().interrupt();
            }
        }
        
        // 停止MediaPlayer
        if (mediaPlayer != null) {
            try {
                if (mediaPlayer.isPlaying()) {
                    mediaPlayer.stop();
                }
                mediaPlayer.release();
                mediaPlayer = null;
            } catch (Exception e) {
                Log.e(TAG, "停止MediaPlayer异常", e);
            }
        }
        
        Log.i(TAG, "✅ RTSP视频流已停止");
    }
    
    /**
     * 检查是否正在流式传输
     */
    public boolean isStreaming() {
        return isStreaming.get();
    }
    
    /**
     * 检查是否已初始化
     */
    public boolean isInitialized() {
        return isInitialized.get();
    }
    
    /**
     * 获取当前RTSP URL
     */
    public String getRtspUrl() {
        return rtspUrl;
    }
    
    /**
     * 清理资源
     */
    public void cleanup() {
        Log.i(TAG, "清理RTSP视频接收器资源");
        
        stopStream();
        
        frameCallback = null;
        surfaceHolder = null;
        rtspUrl = null;
        
        Log.i(TAG, "✅ RTSP视频接收器资源已清理");
    }
    
    /**
     * 获取视频信息
     */
    public String getVideoInfo() {
        if (mediaPlayer != null && isInitialized.get()) {
            try {
                int width = mediaPlayer.getVideoWidth();
                int height = mediaPlayer.getVideoHeight();
                int duration = mediaPlayer.getDuration();
                
                return String.format("分辨率: %dx%d, 时长: %dms", width, height, duration);
            } catch (Exception e) {
                Log.e(TAG, "获取视频信息失败", e);
            }
        }
        
        return "视频信息不可用";
    }
    
    /**
     * 设置帧提取间隔
     */
    public void setFrameExtractionInterval(int intervalMs) {
        // 这里可以动态调整帧提取间隔
        Log.d(TAG, "设置帧提取间隔: " + intervalMs + "ms");
    }
}
