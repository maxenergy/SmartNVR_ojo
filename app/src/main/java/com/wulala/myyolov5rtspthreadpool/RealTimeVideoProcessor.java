package com.wulala.myyolov5rtspthreadpool;

import android.content.Context;
import android.graphics.Bitmap;
import android.graphics.Canvas;
import android.graphics.Color;
import android.graphics.Paint;
import android.graphics.Rect;
import android.graphics.RectF;
import android.util.Log;

import com.wulala.myyolov5rtspthreadpool.entities.FaceAttributes;

import java.util.ArrayList;
import java.util.List;
import java.util.concurrent.atomic.AtomicBoolean;
import java.util.concurrent.atomic.AtomicInteger;
import java.util.concurrent.atomic.AtomicLong;

/**
 * 实时视频处理器
 * 集成YOLOv5目标检测和InspireFace人脸分析的完整Pipeline
 */
public class RealTimeVideoProcessor {
    
    private static final String TAG = "RealTimeVideoProcessor";
    
    // 单例实例
    private static RealTimeVideoProcessor instance;
    
    // 处理状态
    private final AtomicBoolean isProcessing = new AtomicBoolean(false);
    private final AtomicBoolean isInitialized = new AtomicBoolean(false);
    
    // 统计数据
    private final AtomicInteger currentPersonCount = new AtomicInteger(0);
    private final AtomicLong totalPersonCount = new AtomicLong(0);
    private final AtomicInteger totalMaleCount = new AtomicInteger(0);
    private final AtomicInteger totalFemaleCount = new AtomicInteger(0);
    private final AtomicLong totalFrameCount = new AtomicLong(0);
    private final AtomicLong totalProcessingTime = new AtomicLong(0);
    
    // 年龄分布统计
    private final AtomicInteger[] ageDistribution = new AtomicInteger[8];
    
    // 性能监控
    private long lastFrameTime = 0;
    private float currentFPS = 0.0f;
    private long lastProcessingTime = 0;
    
    // AI组件
    private IntegratedAIManager aiManager;
    private Context context;
    
    // 处理配置
    private static final int MAX_PROCESSING_TIME_MS = 100; // 最大处理时间
    private static final float MIN_PERSON_CONFIDENCE = 0.5f;
    private static final float MIN_PERSON_SIZE = 50.0f;
    
    // 回调接口
    public interface ProcessingCallback {
        void onFrameProcessed(ProcessingResult result);
        void onStatisticsUpdated(StatisticsData statistics);
        void onError(String error);
    }
    
    private ProcessingCallback callback;
    
    private RealTimeVideoProcessor() {
        // 初始化年龄分布数组
        for (int i = 0; i < ageDistribution.length; i++) {
            ageDistribution[i] = new AtomicInteger(0);
        }
    }
    
    /**
     * 获取单例实例
     */
    public static synchronized RealTimeVideoProcessor getInstance() {
        if (instance == null) {
            instance = new RealTimeVideoProcessor();
        }
        return instance;
    }
    
    /**
     * 初始化处理器
     */
    public boolean initialize(Context context, ProcessingCallback callback) {
        this.context = context.getApplicationContext();
        this.callback = callback;
        
        Log.i(TAG, "正在初始化实时视频处理器...");
        
        try {
            // 初始化集成AI管理器
            aiManager = IntegratedAIManager.getInstance();
            boolean success = aiManager.initialize(context);
            
            if (success) {
                isInitialized.set(true);
                Log.i(TAG, "✅ 实时视频处理器初始化成功");
                return true;
            } else {
                Log.e(TAG, "❌ AI管理器初始化失败");
                return false;
            }
            
        } catch (Exception e) {
            Log.e(TAG, "初始化异常", e);
            return false;
        }
    }
    
    /**
     * 处理视频帧
     */
    public void processFrame(Bitmap frame) {
        if (!isInitialized.get()) {
            Log.w(TAG, "处理器未初始化");
            return;
        }
        
        if (isProcessing.get()) {
            Log.d(TAG, "上一帧仍在处理中，跳过当前帧");
            return;
        }
        
        // 异步处理帧
        new Thread(() -> processFrameInternal(frame)).start();
    }
    
    /**
     * 内部帧处理方法
     */
    private void processFrameInternal(Bitmap frame) {
        if (!isProcessing.compareAndSet(false, true)) {
            return;
        }
        
        long startTime = System.currentTimeMillis();
        ProcessingResult result = new ProcessingResult();
        
        try {
            // 更新帧计数和FPS
            updateFPS();
            totalFrameCount.incrementAndGet();
            
            // 转换Bitmap为字节数组
            byte[] imageData = bitmapToByteArray(frame);
            if (imageData == null) {
                result.success = false;
                result.errorMessage = "图像数据转换失败";
                return;
            }
            
            // 执行AI检测
            IntegratedAIManager.AIDetectionResult aiResult = aiManager.performDetection(
                imageData, frame.getWidth(), frame.getHeight());
            
            if (aiResult.success) {
                result.success = true;
                result.detectedPersons = aiResult.detectedPersons;
                result.detectedFaces = aiResult.detectedFaces;
                result.personDetections = aiResult.personDetections;
                
                // 更新统计数据
                updateStatistics(aiResult);
                
                // 创建带检测框的结果图像
                result.annotatedFrame = createAnnotatedFrame(frame, aiResult);
                
                Log.d(TAG, "帧处理成功: " + aiResult.detectedPersons + " 人员, " + 
                      aiResult.detectedFaces + " 人脸");
                
            } else {
                result.success = false;
                result.errorMessage = aiResult.errorMessage;
                Log.w(TAG, "AI检测失败: " + aiResult.errorMessage);
            }
            
        } catch (Exception e) {
            result.success = false;
            result.errorMessage = e.getMessage();
            Log.e(TAG, "帧处理异常", e);
        } finally {
            long endTime = System.currentTimeMillis();
            lastProcessingTime = endTime - startTime;
            totalProcessingTime.addAndGet(lastProcessingTime);
            
            result.processingTime = lastProcessingTime;
            result.fps = currentFPS;
            
            // 回调结果
            if (callback != null) {
                callback.onFrameProcessed(result);
                callback.onStatisticsUpdated(getStatistics());
            }
            
            isProcessing.set(false);
        }
    }
    
    /**
     * 更新FPS计算
     */
    private void updateFPS() {
        long currentTime = System.currentTimeMillis();
        if (lastFrameTime > 0) {
            float deltaTime = (currentTime - lastFrameTime) / 1000.0f;
            if (deltaTime > 0) {
                currentFPS = 1.0f / deltaTime;
            }
        }
        lastFrameTime = currentTime;
    }
    
    /**
     * 更新统计数据
     */
    private void updateStatistics(IntegratedAIManager.AIDetectionResult result) {
        // 更新当前人数
        currentPersonCount.set(result.detectedPersons);
        
        // 更新累计人数（只有当检测到新人员时）
        if (result.detectedPersons > 0) {
            totalPersonCount.addAndGet(result.detectedPersons);
        }
        
        // 更新性别统计
        totalMaleCount.addAndGet(result.maleCount);
        totalFemaleCount.addAndGet(result.femaleCount);
        
        // 更新年龄分布
        if (result.ageGroups != null) {
            for (int i = 0; i < Math.min(result.ageGroups.length, ageDistribution.length); i++) {
                ageDistribution[i].addAndGet(result.ageGroups[i]);
            }
        }
    }
    
    /**
     * 创建带注释的帧图像
     */
    private Bitmap createAnnotatedFrame(Bitmap originalFrame, IntegratedAIManager.AIDetectionResult result) {
        Bitmap annotatedFrame = originalFrame.copy(Bitmap.Config.ARGB_8888, true);
        Canvas canvas = new Canvas(annotatedFrame);

        // 设置绘制样式
        Paint personBoxPaint = new Paint();
        personBoxPaint.setColor(Color.GREEN);
        personBoxPaint.setStyle(Paint.Style.STROKE);
        personBoxPaint.setStrokeWidth(4.0f);
        personBoxPaint.setAntiAlias(true);

        Paint faceBoxPaint = new Paint();
        faceBoxPaint.setColor(Color.YELLOW);
        faceBoxPaint.setStyle(Paint.Style.STROKE);
        faceBoxPaint.setStrokeWidth(2.0f);
        faceBoxPaint.setAntiAlias(true);

        Paint maleTextPaint = new Paint();
        maleTextPaint.setColor(Color.CYAN);
        maleTextPaint.setTextSize(20);
        maleTextPaint.setAntiAlias(true);
        maleTextPaint.setFakeBoldText(true);

        Paint femaleTextPaint = new Paint();
        femaleTextPaint.setColor(Color.MAGENTA);
        femaleTextPaint.setTextSize(20);
        femaleTextPaint.setAntiAlias(true);
        femaleTextPaint.setFakeBoldText(true);

        Paint unknownTextPaint = new Paint();
        unknownTextPaint.setColor(Color.WHITE);
        unknownTextPaint.setTextSize(20);
        unknownTextPaint.setAntiAlias(true);
        unknownTextPaint.setFakeBoldText(true);

        Paint backgroundPaint = new Paint();
        backgroundPaint.setColor(Color.BLACK);
        backgroundPaint.setAlpha(160);

        Paint shadowPaint = new Paint();
        shadowPaint.setColor(Color.BLACK);
        shadowPaint.setAlpha(100);

        // 绘制YOLOv5人员检测框
        if (result.personDetections != null && result.personDetections.size() > 0) {
            for (int i = 0; i < result.personDetections.size(); i++) {
                RealYOLOInference.DetectionResult person = result.personDetections.get(i);

                // 绘制人员检测框（绿色）
                RectF personRect = new RectF(person.x1, person.y1, person.x2, person.y2);
                canvas.drawRect(personRect, personBoxPaint);

                // 绘制人员标签
                String personLabel = String.format("Person #%d (%.1f%%)", i + 1, person.confidence * 100);
                drawLabelWithBackground(canvas, personLabel, person.x1, person.y1 - 8,
                                      unknownTextPaint, backgroundPaint, shadowPaint);
            }
        }

        // 绘制真实的人脸检测结果
        if (result.faceDetections != null && result.faceDetections.size() > 0) {
            for (int i = 0; i < result.faceDetections.size(); i++) {
                IntegratedAIManager.FaceDetectionBox face = result.faceDetections.get(i);

                // 绘制真实的人脸检测框
                RectF faceRect = new RectF(face.x1, face.y1, face.x2, face.y2);
                canvas.drawRect(faceRect, faceBoxPaint);

                // 使用真实的人脸属性数据
                String genderLabel = face.getGenderString();
                String ageLabel = face.getAgeGroup();

                String combinedLabel = String.format("Face #%d: %s, %s", i + 1, genderLabel, ageLabel);

                // 根据性别选择文字颜色
                Paint textPaint = (face.gender == 1) ? maleTextPaint : femaleTextPaint;

                // 绘制人脸属性标签
                float labelY = face.y2 + 25;
                drawLabelWithBackground(canvas, combinedLabel, face.x1,
                                      labelY, textPaint, backgroundPaint, shadowPaint);

                // 绘制真实置信度信息
                String confidenceLabel = String.format("置信度: %.1f%%", face.confidence * 100);
                drawLabelWithBackground(canvas, confidenceLabel, face.x1,
                                      labelY + 25, textPaint, backgroundPaint, shadowPaint);
            }
        } else if (result.personDetections != null && result.personDetections.size() > 0 && result.detectedFaces > 0) {
            // 如果没有真实的人脸检测框数据，回退到基于人员检测框的模拟显示
            Log.w(TAG, "使用模拟人脸检测框 - 缺少真实人脸检测数据");

            for (int i = 0; i < Math.min(result.personDetections.size(), result.detectedFaces); i++) {
                RealYOLOInference.DetectionResult person = result.personDetections.get(i);

                // 在人员检测框内绘制模拟人脸框
                float faceWidth = (person.x2 - person.x1) * 0.6f;
                float faceHeight = (person.y2 - person.y1) * 0.4f;
                float faceLeft = person.x1 + (person.x2 - person.x1 - faceWidth) / 2;
                float faceTop = person.y1 + (person.y2 - person.y1) * 0.1f;

                RectF faceRect = new RectF(faceLeft, faceTop, faceLeft + faceWidth, faceTop + faceHeight);
                canvas.drawRect(faceRect, faceBoxPaint);

                // 模拟人脸属性（基于索引生成不同属性）
                String genderLabel = (i % 2 == 0) ? "男性" : "女性";
                String[] ageLabels = {"20-29岁", "30-39岁", "40-49岁", "25-35岁"};
                String ageLabel = ageLabels[i % ageLabels.length];

                String combinedLabel = String.format("Face #%d: %s, %s (模拟)", i + 1, genderLabel, ageLabel);

                // 根据性别选择文字颜色
                Paint textPaint = (i % 2 == 0) ? maleTextPaint : femaleTextPaint;

                // 绘制人脸属性标签
                float labelY = faceTop + faceHeight + 25;
                drawLabelWithBackground(canvas, combinedLabel, faceLeft,
                                      labelY, textPaint, backgroundPaint, shadowPaint);

                // 绘制模拟置信度信息
                float confidence = 0.85f + (i * 0.05f);
                String confidenceLabel = String.format("置信度: %.1f%% (模拟)", confidence * 100);
                drawLabelWithBackground(canvas, confidenceLabel, faceLeft,
                                      labelY + 25, textPaint, backgroundPaint, shadowPaint);
            }
        }

        // 绘制统计信息叠加层
        drawEnhancedStatisticsOverlay(canvas, result);

        return annotatedFrame;
    }
    
    /**
     * 绘制带背景和阴影的标签
     */
    private void drawLabelWithBackground(Canvas canvas, String text, float x, float y,
                                       Paint textPaint, Paint backgroundPaint, Paint shadowPaint) {
        if (text == null || text.isEmpty()) return;

        // 计算文本边界
        Rect textBounds = new Rect();
        textPaint.getTextBounds(text, 0, text.length(), textBounds);

        // 添加内边距
        int padding = 8;
        RectF backgroundRect = new RectF(
            x - padding,
            y - textBounds.height() - padding,
            x + textBounds.width() + padding,
            y + padding
        );

        // 绘制阴影
        RectF shadowRect = new RectF(backgroundRect);
        shadowRect.offset(2, 2);
        canvas.drawRect(shadowRect, shadowPaint);

        // 绘制背景
        canvas.drawRect(backgroundRect, backgroundPaint);

        // 绘制文本
        canvas.drawText(text, x, y, textPaint);
    }

    /**
     * 获取性别标签
     */
    private String getGenderLabel(FaceAttributes attributes) {
        if (attributes == null || !attributes.hasValidGender()) {
            return "未知性别";
        }

        switch (attributes.getGender()) {
            case FaceAttributes.GENDER_MALE:
                return "男性";
            case FaceAttributes.GENDER_FEMALE:
                return "女性";
            default:
                return "未知性别";
        }
    }

    /**
     * 获取年龄标签
     */
    private String getAgeLabel(FaceAttributes attributes) {
        if (attributes == null || !attributes.hasValidAge()) {
            return "未知年龄";
        }

        return attributes.getAgeBracketString();
    }

    /**
     * 根据性别获取对应的文字画笔
     */
    private Paint getTextPaintByGender(FaceAttributes attributes, Paint malePaint,
                                     Paint femalePaint, Paint unknownPaint) {
        if (attributes == null || !attributes.hasValidGender()) {
            return unknownPaint;
        }

        switch (attributes.getGender()) {
            case FaceAttributes.GENDER_MALE:
                return malePaint;
            case FaceAttributes.GENDER_FEMALE:
                return femalePaint;
            default:
                return unknownPaint;
        }
    }

    /**
     * 绘制增强的统计信息叠加层
     */
    private void drawEnhancedStatisticsOverlay(Canvas canvas, IntegratedAIManager.AIDetectionResult result) {
        Paint overlayPaint = new Paint();
        overlayPaint.setColor(Color.BLACK);
        overlayPaint.setAlpha(200);

        Paint titlePaint = new Paint();
        titlePaint.setColor(Color.YELLOW);
        titlePaint.setTextSize(22);
        titlePaint.setAntiAlias(true);
        titlePaint.setFakeBoldText(true);

        Paint textPaint = new Paint();
        textPaint.setColor(Color.WHITE);
        textPaint.setTextSize(18);
        textPaint.setAntiAlias(true);

        Paint maleStatsPaint = new Paint();
        maleStatsPaint.setColor(Color.CYAN);
        maleStatsPaint.setTextSize(18);
        maleStatsPaint.setAntiAlias(true);

        Paint femaleStatsPaint = new Paint();
        femaleStatsPaint.setColor(Color.MAGENTA);
        femaleStatsPaint.setTextSize(18);
        femaleStatsPaint.setAntiAlias(true);

        // 绘制统计面板背景
        RectF statsPanel = new RectF(10, 10, 350, 200);
        canvas.drawRect(statsPanel, overlayPaint);

        // 绘制标题
        canvas.drawText("实时AI分析统计", 20, 35, titlePaint);

        // 绘制基础统计
        String[] basicStats = {
            "检测人员: " + result.detectedPersons,
            "识别人脸: " + result.detectedFaces,
            "FPS: " + String.format("%.1f", currentFPS),
            "处理耗时: " + lastProcessingTime + "ms"
        };

        for (int i = 0; i < basicStats.length; i++) {
            canvas.drawText(basicStats[i], 20, 60 + i * 22, textPaint);
        }

        // 绘制性别统计
        canvas.drawText("男性: " + result.maleCount, 20, 148, maleStatsPaint);
        canvas.drawText("女性: " + result.femaleCount, 120, 148, femaleStatsPaint);

        // 绘制年龄分布（如果有数据）
        if (result.ageGroups != null && result.ageGroups.length > 0) {
            String ageDistribution = getAgeDistributionSummary(result.ageGroups);
            if (!ageDistribution.isEmpty()) {
                canvas.drawText("年龄分布: " + ageDistribution, 20, 170, textPaint);
            }
        }
    }

    /**
     * 获取年龄分布摘要
     */
    private String getAgeDistributionSummary(int[] ageGroups) {
        String[] ageLabels = {"0-9", "10-19", "20-29", "30-39", "40-49", "50-59", "60-69", "70+"};

        StringBuilder summary = new StringBuilder();
        int totalCount = 0;

        for (int count : ageGroups) {
            totalCount += count;
        }

        if (totalCount == 0) return "";

        for (int i = 0; i < Math.min(ageGroups.length, ageLabels.length); i++) {
            if (ageGroups[i] > 0) {
                if (summary.length() > 0) summary.append(", ");
                summary.append(ageLabels[i]).append(":").append(ageGroups[i]);
            }
        }

        return summary.toString();
    }
    
    /**
     * 将Bitmap转换为字节数组
     */
    private byte[] bitmapToByteArray(Bitmap bitmap) {
        try {
            int width = bitmap.getWidth();
            int height = bitmap.getHeight();
            int[] pixels = new int[width * height];
            bitmap.getPixels(pixels, 0, width, 0, 0, width, height);
            
            byte[] bytes = new byte[width * height * 3];
            for (int i = 0; i < pixels.length; i++) {
                int pixel = pixels[i];
                bytes[i * 3] = (byte) ((pixel >> 16) & 0xFF); // R
                bytes[i * 3 + 1] = (byte) ((pixel >> 8) & 0xFF); // G
                bytes[i * 3 + 2] = (byte) (pixel & 0xFF); // B
            }
            
            return bytes;
        } catch (Exception e) {
            Log.e(TAG, "Bitmap转换异常", e);
            return null;
        }
    }
    
    /**
     * 获取当前统计数据
     */
    public StatisticsData getStatistics() {
        StatisticsData stats = new StatisticsData();
        stats.currentPersonCount = currentPersonCount.get();
        stats.totalPersonCount = totalPersonCount.get();
        stats.totalMaleCount = totalMaleCount.get();
        stats.totalFemaleCount = totalFemaleCount.get();
        stats.totalFrameCount = totalFrameCount.get();
        stats.averageProcessingTime = totalFrameCount.get() > 0 ? 
            totalProcessingTime.get() / totalFrameCount.get() : 0;
        stats.currentFPS = currentFPS;
        stats.lastProcessingTime = lastProcessingTime;
        
        // 复制年龄分布
        stats.ageDistribution = new int[ageDistribution.length];
        for (int i = 0; i < ageDistribution.length; i++) {
            stats.ageDistribution[i] = ageDistribution[i].get();
        }
        
        return stats;
    }
    
    /**
     * 重置统计数据
     */
    public void resetStatistics() {
        currentPersonCount.set(0);
        totalPersonCount.set(0);
        totalMaleCount.set(0);
        totalFemaleCount.set(0);
        totalFrameCount.set(0);
        totalProcessingTime.set(0);
        
        for (AtomicInteger age : ageDistribution) {
            age.set(0);
        }
        
        Log.i(TAG, "统计数据已重置");
    }
    
    /**
     * 检查是否正在处理
     */
    public boolean isProcessing() {
        return isProcessing.get();
    }
    
    /**
     * 检查是否已初始化
     */
    public boolean isInitialized() {
        return isInitialized.get();
    }
    
    /**
     * 清理资源
     */
    public void cleanup() {
        Log.i(TAG, "清理实时视频处理器资源");
        
        isProcessing.set(false);
        isInitialized.set(false);
        
        if (aiManager != null) {
            aiManager.cleanup();
        }
        
        callback = null;
        context = null;
    }
    
    /**
     * 处理结果类
     */
    public static class ProcessingResult {
        public boolean success = false;
        public int detectedPersons = 0;
        public int detectedFaces = 0;
        public List<RealYOLOInference.DetectionResult> personDetections = new ArrayList<>();
        public Bitmap annotatedFrame = null;
        public long processingTime = 0;
        public float fps = 0.0f;
        public String errorMessage = null;
    }
    
    /**
     * 统计数据类
     */
    public static class StatisticsData {
        public int currentPersonCount = 0;
        public long totalPersonCount = 0;
        public int totalMaleCount = 0;
        public int totalFemaleCount = 0;
        public long totalFrameCount = 0;
        public long averageProcessingTime = 0;
        public float currentFPS = 0.0f;
        public long lastProcessingTime = 0;
        public int[] ageDistribution = new int[8];
        
        public String getGenderRatio() {
            int total = totalMaleCount + totalFemaleCount;
            if (total == 0) return "无数据";
            
            float maleRatio = (float) totalMaleCount / total * 100;
            float femaleRatio = (float) totalFemaleCount / total * 100;
            
            return String.format("男性: %.1f%%, 女性: %.1f%%", maleRatio, femaleRatio);
        }
        
        public String getAgeDistributionSummary() {
            String[] ageLabels = {"0-9岁", "10-19岁", "20-29岁", "30-39岁", 
                                 "40-49岁", "50-59岁", "60-69岁", "70岁以上"};
            
            int total = 0;
            for (int count : ageDistribution) {
                total += count;
            }
            
            if (total == 0) return "无年龄数据";
            
            StringBuilder sb = new StringBuilder();
            for (int i = 0; i < ageDistribution.length; i++) {
                if (ageDistribution[i] > 0) {
                    float ratio = (float) ageDistribution[i] / total * 100;
                    if (sb.length() > 0) sb.append(", ");
                    sb.append(ageLabels[i]).append(": ").append(String.format("%.1f%%", ratio));
                }
            }
            
            return sb.toString();
        }
    }
}
