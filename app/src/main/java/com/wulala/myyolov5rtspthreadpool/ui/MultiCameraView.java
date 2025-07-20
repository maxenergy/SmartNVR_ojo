package com.wulala.myyolov5rtspthreadpool.ui;

import android.content.Context;
import android.graphics.Bitmap;
import android.graphics.Canvas;
import android.graphics.Color;
import android.graphics.Paint;
import android.graphics.Rect;
import android.graphics.RectF;
import android.util.AttributeSet;
import android.view.SurfaceHolder;
import android.view.SurfaceView;
import android.view.ViewGroup;
import android.widget.GridLayout;
import android.widget.ImageView;
import android.widget.TextView;

import com.wulala.myyolov5rtspthreadpool.DetectionResultFilter;
import com.wulala.myyolov5rtspthreadpool.IntegratedAIManager;
import com.wulala.myyolov5rtspthreadpool.RealYOLOInference;
import com.wulala.myyolov5rtspthreadpool.entities.FaceAttributes;

import java.util.ArrayList;
import java.util.List;

/**
 * 多摄像头分割视图管理器
 * 支持1x1, 1x2, 2x2, 2x3, 3x3, 4x4等分割布局
 */
public class MultiCameraView extends GridLayout {
    private static final String TAG = "MultiCameraView";
    
    private List<CameraViewHolder> cameraViews;
    private OnSurfaceChangeListener surfaceChangeListener;
    private int maxCameras = 16; // 最多支持4x4=16路摄像头
    private DetectionResultFilter detectionFilter; // 检测结果过滤器
    
    public interface OnSurfaceChangeListener {
        void onSurfaceCreated(int cameraIndex, SurfaceHolder holder);
        void onSurfaceChanged(int cameraIndex, SurfaceHolder holder, int format, int width, int height);
        void onSurfaceDestroyed(int cameraIndex, SurfaceHolder holder);
    }
    
    private class CameraViewHolder {
        SurfaceView surfaceView;
        TextView titleView;
        ImageView aiOverlayView;  // AI分析结果叠加层
        TextView statsView;       // 统计信息显示
        String cameraName;
        boolean isActive = false;
        boolean aiAnalysisEnabled = false;

        // AI分析相关数据
        private int currentPersonCount = 0;
        private int totalPersonCount = 0;
        private int maleCount = 0;
        private int femaleCount = 0;
        private List<RealYOLOInference.DetectionResult> lastDetections = new ArrayList<>();
        private List<IntegratedAIManager.FaceDetectionBox> lastFaceDetections = new ArrayList<>();

        CameraViewHolder(Context context, String name) {
            this.cameraName = name;

            // 创建SurfaceView
            this.surfaceView = new SurfaceView(context);

            // 创建标题视图
            this.titleView = new TextView(context);
            this.titleView.setText(name);
            this.titleView.setTextColor(0xFFFFFFFF);
            this.titleView.setBackgroundColor(0x80000000);
            this.titleView.setPadding(8, 4, 8, 4);
            this.titleView.setTextSize(12);

            // 创建AI叠加层
            this.aiOverlayView = new ImageView(context);
            this.aiOverlayView.setScaleType(ImageView.ScaleType.MATRIX);
            this.aiOverlayView.setVisibility(android.view.View.GONE);

            // 创建统计信息视图
            this.statsView = new TextView(context);
            this.statsView.setTextColor(0xFFFFFFFF);
            this.statsView.setBackgroundColor(0xCC000000);
            this.statsView.setPadding(6, 3, 6, 3);
            this.statsView.setTextSize(10);
            this.statsView.setVisibility(android.view.View.GONE);
            updateStatsDisplay();
        }

        /**
         * 启用AI分析显示
         */
        public void enableAIAnalysis(boolean enabled) {
            this.aiAnalysisEnabled = enabled;
            if (enabled) {
                aiOverlayView.setVisibility(android.view.View.VISIBLE);
                statsView.setVisibility(android.view.View.VISIBLE);
            } else {
                aiOverlayView.setVisibility(android.view.View.GONE);
                statsView.setVisibility(android.view.View.GONE);
            }
        }

        /**
         * 更新AI分析结果
         */
        public void updateAIResults(IntegratedAIManager.AIDetectionResult result) {
            if (!aiAnalysisEnabled || result == null) return;

            // 先应用检测过滤器，然后更新统计数据

            // 应用检测过滤器
            List<DetectionResultFilter.DetectionResult> allDetections = new ArrayList<>();
            if (result.personDetections != null) {
                for (RealYOLOInference.DetectionResult detection : result.personDetections) {
                    allDetections.add(DetectionResultFilter.fromRealYOLOResult(detection));
                }
            }

            // 过滤检测结果
            List<DetectionResultFilter.DetectionResult> filteredDetections =
                MultiCameraView.this.detectionFilter.filterResults(allDetections);

            // 转换回RealYOLOInference.DetectionResult格式
            lastDetections = new ArrayList<>();
            for (DetectionResultFilter.DetectionResult filtered : filteredDetections) {
                lastDetections.add(DetectionResultFilter.toRealYOLOResult(filtered));
            }

            // 更新人员计数为过滤后的结果
            currentPersonCount = lastDetections.size();

            // 更新统计数据
            if (currentPersonCount > 0) {
                totalPersonCount += currentPersonCount;
                // 只有检测到人员时才更新人脸数据
                maleCount = result.maleCount;      // 使用当前值，不累积
                femaleCount = result.femaleCount;  // 使用当前值，不累积
            } else {
                // 没有检测到人员时，清零人脸数据
                maleCount = 0;
                femaleCount = 0;
            }

            lastFaceDetections = result.faceDetections != null ?
                new ArrayList<>(result.faceDetections) : new ArrayList<>();

            // 更新显示
            updateStatsDisplay();
            updateOverlayDisplay();
        }

        /**
         * 更新统计信息显示
         */
        private void updateStatsDisplay() {
            String statsText = String.format("👥%d 👨%d 👩%d",
                currentPersonCount, maleCount, femaleCount);
            statsView.setText(statsText);
        }

        /**
         * 更新叠加层显示
         */
        private void updateOverlayDisplay() {
            if (!aiAnalysisEnabled || surfaceView == null) return;

            try {
                // 创建叠加图像
                int width = surfaceView.getWidth();
                int height = surfaceView.getHeight();

                if (width <= 0 || height <= 0) {
                    width = 320;  // 默认尺寸
                    height = 240;
                }

                Bitmap overlayBitmap = createAIOverlayBitmap(width, height);
                if (overlayBitmap != null) {
                    aiOverlayView.setImageBitmap(overlayBitmap);
                }
            } catch (Exception e) {
                android.util.Log.e("MultiCameraView", "更新叠加层失败", e);
            }
        }

        /**
         * 创建AI分析结果叠加图像
         */
        private Bitmap createAIOverlayBitmap(int width, int height) {
            try {
                Bitmap bitmap = Bitmap.createBitmap(width, height, Bitmap.Config.ARGB_8888);
                Canvas canvas = new Canvas(bitmap);

                // 设置绘制样式
                Paint personBoxPaint = new Paint();
                personBoxPaint.setColor(Color.GREEN);
                personBoxPaint.setStyle(Paint.Style.STROKE);
                personBoxPaint.setStrokeWidth(2.0f);
                personBoxPaint.setAntiAlias(true);

                Paint faceBoxPaint = new Paint();
                faceBoxPaint.setColor(Color.YELLOW);
                faceBoxPaint.setStyle(Paint.Style.STROKE);
                faceBoxPaint.setStrokeWidth(1.5f);
                faceBoxPaint.setAntiAlias(true);

                Paint textPaint = new Paint();
                textPaint.setColor(Color.WHITE);
                textPaint.setTextSize(12);
                textPaint.setAntiAlias(true);

                Paint backgroundPaint = new Paint();
                backgroundPaint.setColor(Color.BLACK);
                backgroundPaint.setAlpha(128);

                // 绘制检测框
                if (lastDetections != null && !lastDetections.isEmpty() && currentPersonCount > 0) {
                    for (int i = 0; i < lastDetections.size(); i++) {
                        RealYOLOInference.DetectionResult detection = lastDetections.get(i);

                        // 缩放坐标到当前视图尺寸
                        float scaleX = (float) width / 640.0f;  // 假设原始尺寸为640
                        float scaleY = (float) height / 480.0f; // 假设原始尺寸为480

                        float left = detection.x1 * scaleX;
                        float top = detection.y1 * scaleY;
                        float right = detection.x2 * scaleX;
                        float bottom = detection.y2 * scaleY;

                        // 绘制人员检测框
                        RectF personRect = new RectF(left, top, right, bottom);
                        canvas.drawRect(personRect, personBoxPaint);

                        // 绘制标签
                        String label = String.format("Person %d", i + 1);
                        drawLabelWithBackground(canvas, label, left, top - 5,
                                              textPaint, backgroundPaint);

                    }
                }

                // 绘制真实的人脸检测框（独立于人员检测框）
                if (lastFaceDetections != null && !lastFaceDetections.isEmpty()) {
                    for (int i = 0; i < lastFaceDetections.size(); i++) {
                        IntegratedAIManager.FaceDetectionBox face = lastFaceDetections.get(i);

                        // 缩放人脸检测框坐标到当前视图尺寸
                        float scaleX = (float) width / 640.0f;  // 假设原始尺寸为640
                        float scaleY = (float) height / 480.0f; // 假设原始尺寸为480

                        float faceLeft = face.x1 * scaleX;
                        float faceTop = face.y1 * scaleY;
                        float faceRight = face.x2 * scaleX;
                        float faceBottom = face.y2 * scaleY;

                        // 绘制真实的人脸检测框
                        RectF faceRect = new RectF(faceLeft, faceTop, faceRight, faceBottom);
                        canvas.drawRect(faceRect, faceBoxPaint);

                        // 绘制真实的人脸属性标签
                        String genderAge = String.format("%s, %s", face.getGenderString(), face.getAgeGroup());
                        drawLabelWithBackground(canvas, genderAge, faceLeft,
                                              faceBottom + 15, textPaint, backgroundPaint);

                        // 绘制置信度信息
                        String confidenceLabel = String.format("%.1f%%", face.confidence * 100);
                        drawLabelWithBackground(canvas, confidenceLabel, faceLeft,
                                              faceBottom + 35, textPaint, backgroundPaint);
                    }
                } else if (currentPersonCount > 0 && (maleCount > 0 || femaleCount > 0)) {
                    // 如果没有真实的人脸检测框，回退到模拟显示
                    android.util.Log.w(TAG, "使用模拟人脸检测框 - 缺少真实人脸检测数据");

                    if (lastDetections != null && !lastDetections.isEmpty()) {
                        for (int i = 0; i < Math.min(lastDetections.size(), maleCount + femaleCount); i++) {
                            RealYOLOInference.DetectionResult detection = lastDetections.get(i);

                            // 缩放坐标到当前视图尺寸
                            float scaleX = (float) width / 640.0f;
                            float scaleY = (float) height / 480.0f;

                            float left = detection.x1 * scaleX;
                            float top = detection.y1 * scaleY;
                            float right = detection.x2 * scaleX;
                            float bottom = detection.y2 * scaleY;

                            // 绘制模拟人脸框（在人员框内）
                            float faceWidth = (right - left) * 0.6f;
                            float faceHeight = (bottom - top) * 0.4f;
                            float faceLeft = left + (right - left - faceWidth) / 2;
                            float faceTop = top + (bottom - top) * 0.1f;

                            RectF faceRect = new RectF(faceLeft, faceTop,
                                                      faceLeft + faceWidth, faceTop + faceHeight);
                            canvas.drawRect(faceRect, faceBoxPaint);

                            // 绘制性别年龄标签（基于实际的人脸分析结果）
                            String genderAge;
                            if (i < maleCount) {
                                genderAge = "男性, 25-35岁 (模拟)";
                            } else if (i < maleCount + femaleCount) {
                                genderAge = "女性, 20-30岁 (模拟)";
                            } else {
                                genderAge = "分析中...";
                            }
                            drawLabelWithBackground(canvas, genderAge, faceLeft,
                                                  faceTop + faceHeight + 15, textPaint, backgroundPaint);
                        }
                    }
                }

                return bitmap;
            } catch (Exception e) {
                android.util.Log.e("MultiCameraView", "创建叠加图像失败", e);
                return null;
            }
        }

        /**
         * 绘制带背景的标签
         */
        private void drawLabelWithBackground(Canvas canvas, String text, float x, float y,
                                           Paint textPaint, Paint backgroundPaint) {
            if (text == null || text.isEmpty()) return;

            Rect textBounds = new Rect();
            textPaint.getTextBounds(text, 0, text.length(), textBounds);

            int padding = 3;
            RectF backgroundRect = new RectF(
                x - padding,
                y - textBounds.height() - padding,
                x + textBounds.width() + padding,
                y + padding
            );

            canvas.drawRect(backgroundRect, backgroundPaint);
            canvas.drawText(text, x, y, textPaint);
        }
    }
    
    public MultiCameraView(Context context) {
        super(context);
        init();
    }
    
    public MultiCameraView(Context context, AttributeSet attrs) {
        super(context, attrs);
        init();
    }
    
    public MultiCameraView(Context context, AttributeSet attrs, int defStyleAttr) {
        super(context, attrs, defStyleAttr);
        init();
    }
    
    private void init() {
        cameraViews = new ArrayList<>();
        setColumnCount(1);
        setRowCount(1);
        detectionFilter = new DetectionResultFilter(getContext());
    }
    
    public void setOnSurfaceChangeListener(OnSurfaceChangeListener listener) {
        this.surfaceChangeListener = listener;
    }
    
    /**
     * 设置摄像头数量并重新布局
     */
    public void setupCameras(List<String> cameraNames) {
        // 清除现有视图
        removeAllViews();
        cameraViews.clear();
        
        int cameraCount = Math.min(cameraNames.size(), maxCameras);
        if (cameraCount == 0) return;
        
        // 计算网格布局
        GridLayout.LayoutParams gridParams = calculateGridLayout(cameraCount);
        
        // 创建摄像头视图
        for (int i = 0; i < cameraCount; i++) {
            String cameraName = i < cameraNames.size() ? cameraNames.get(i) : "Camera " + (i + 1);
            CameraViewHolder holder = new CameraViewHolder(getContext(), cameraName);
            
            // 创建容器
            ViewGroup container = new android.widget.FrameLayout(getContext());
            
            // 设置SurfaceView布局参数
            android.widget.FrameLayout.LayoutParams surfaceParams = 
                new android.widget.FrameLayout.LayoutParams(
                    ViewGroup.LayoutParams.MATCH_PARENT,
                    ViewGroup.LayoutParams.MATCH_PARENT);
            holder.surfaceView.setLayoutParams(surfaceParams);
            
            // 设置标题布局参数
            android.widget.FrameLayout.LayoutParams titleParams =
                new android.widget.FrameLayout.LayoutParams(
                    ViewGroup.LayoutParams.WRAP_CONTENT,
                    ViewGroup.LayoutParams.WRAP_CONTENT);
            titleParams.gravity = android.view.Gravity.TOP | android.view.Gravity.LEFT;
            titleParams.leftMargin = 8;
            titleParams.topMargin = 8;
            holder.titleView.setLayoutParams(titleParams);

            // 设置AI叠加层布局参数
            android.widget.FrameLayout.LayoutParams overlayParams =
                new android.widget.FrameLayout.LayoutParams(
                    ViewGroup.LayoutParams.MATCH_PARENT,
                    ViewGroup.LayoutParams.MATCH_PARENT);
            holder.aiOverlayView.setLayoutParams(overlayParams);

            // 设置统计信息布局参数
            android.widget.FrameLayout.LayoutParams statsParams =
                new android.widget.FrameLayout.LayoutParams(
                    ViewGroup.LayoutParams.WRAP_CONTENT,
                    ViewGroup.LayoutParams.WRAP_CONTENT);
            statsParams.gravity = android.view.Gravity.TOP | android.view.Gravity.RIGHT;
            statsParams.rightMargin = 8;
            statsParams.topMargin = 8;
            holder.statsView.setLayoutParams(statsParams);

            // 添加到容器（按层次顺序）
            container.addView(holder.surfaceView);      // 底层：视频流
            container.addView(holder.aiOverlayView);    // 中层：AI叠加
            container.addView(holder.titleView);        // 顶层：标题
            container.addView(holder.statsView);        // 顶层：统计信息
            
            // 设置网格布局参数
            GridLayout.LayoutParams layoutParams = new GridLayout.LayoutParams();
            layoutParams.width = 0;
            layoutParams.height = 0;
            layoutParams.columnSpec = GridLayout.spec(i % getColumnCount(), 1f);
            layoutParams.rowSpec = GridLayout.spec(i / getColumnCount(), 1f);
            layoutParams.setMargins(2, 2, 2, 2);
            container.setLayoutParams(layoutParams);
            
            // 设置Surface回调
            final int cameraIndex = i;
            holder.surfaceView.getHolder().addCallback(new SurfaceHolder.Callback() {
                @Override
                public void surfaceCreated(SurfaceHolder holder) {
                    if (surfaceChangeListener != null) {
                        surfaceChangeListener.onSurfaceCreated(cameraIndex, holder);
                    }
                }
                
                @Override
                public void surfaceChanged(SurfaceHolder holder, int format, int width, int height) {
                    if (surfaceChangeListener != null) {
                        surfaceChangeListener.onSurfaceChanged(cameraIndex, holder, format, width, height);
                    }
                }
                
                @Override
                public void surfaceDestroyed(SurfaceHolder holder) {
                    if (surfaceChangeListener != null) {
                        surfaceChangeListener.onSurfaceDestroyed(cameraIndex, holder);
                    }
                }
            });
            
            // 添加点击事件用于切换焦点
            container.setOnClickListener(v -> {
                // TODO: 实现焦点切换，将选中的摄像头放大显示
                android.util.Log.d(TAG, "Camera " + cameraIndex + " clicked: " + cameraName);
            });
            
            addView(container);
            cameraViews.add(holder);
        }
        
        android.util.Log.d(TAG, "Setup " + cameraCount + " cameras in " + 
                          getRowCount() + "x" + getColumnCount() + " grid");
    }
    
    /**
     * 计算最优网格布局
     */
    private GridLayout.LayoutParams calculateGridLayout(int cameraCount) {
        int rows, cols;
        
        switch (cameraCount) {
            case 1:
                rows = 1; cols = 1;
                break;
            case 2:
                rows = 1; cols = 2;  // 1x2横向排列
                break;
            case 3:
            case 4:
                rows = 2; cols = 2;  // 2x2
                break;
            case 5:
            case 6:
                rows = 2; cols = 3;  // 2x3
                break;
            case 7:
            case 8:
            case 9:
                rows = 3; cols = 3;  // 3x3
                break;
            default:
                rows = 4; cols = 4;  // 4x4，最多16路
                break;
        }
        
        setRowCount(rows);
        setColumnCount(cols);
        
        return new GridLayout.LayoutParams();
    }
    
    /**
     * 获取指定索引的SurfaceView
     */
    public SurfaceView getSurfaceView(int index) {
        if (index >= 0 && index < cameraViews.size()) {
            return cameraViews.get(index).surfaceView;
        }
        return null;
    }
    
    /**
     * 设置摄像头标题
     */
    public void setCameraTitle(int index, String title) {
        if (index >= 0 && index < cameraViews.size()) {
            cameraViews.get(index).titleView.setText(title);
            cameraViews.get(index).cameraName = title;
        }
    }
    
    /**
     * 设置摄像头活跃状态
     */
    public void setCameraActive(int index, boolean active) {
        if (index >= 0 && index < cameraViews.size()) {
            CameraViewHolder holder = cameraViews.get(index);
            holder.isActive = active;

            // 更新视觉状态
            if (active) {
                holder.titleView.setBackgroundColor(0x8000FF00); // 绿色背景表示活跃
            } else {
                holder.titleView.setBackgroundColor(0x80FF0000); // 红色背景表示断开
            }
        }
    }

    /**
     * 启用指定摄像头的AI分析显示
     */
    public void enableAIAnalysis(int cameraIndex, boolean enabled) {
        if (cameraIndex >= 0 && cameraIndex < cameraViews.size()) {
            cameraViews.get(cameraIndex).enableAIAnalysis(enabled);
            android.util.Log.d(TAG, "Camera " + cameraIndex + " AI analysis " +
                              (enabled ? "enabled" : "disabled"));
        }
    }

    /**
     * 启用所有摄像头的AI分析显示
     */
    public void enableAllAIAnalysis(boolean enabled) {
        for (int i = 0; i < cameraViews.size(); i++) {
            enableAIAnalysis(i, enabled);
        }
        android.util.Log.d(TAG, "All cameras AI analysis " + (enabled ? "enabled" : "disabled"));
    }

    /**
     * 更新指定摄像头的AI分析结果
     */
    public void updateAIResults(int cameraIndex, IntegratedAIManager.AIDetectionResult result) {
        if (cameraIndex >= 0 && cameraIndex < cameraViews.size()) {
            cameraViews.get(cameraIndex).updateAIResults(result);
        }
    }

    /**
     * 模拟AI分析结果更新（用于测试）
     * 注意：此方法已被修改，只有在真实检测到人员时才会显示人脸识别信息
     */
    public void simulateAIResults(int cameraIndex) {
        if (cameraIndex >= 0 && cameraIndex < cameraViews.size()) {
            // 创建模拟的AI检测结果
            IntegratedAIManager.AIDetectionResult mockResult = new IntegratedAIManager.AIDetectionResult();
            mockResult.success = true;

            // 随机决定是否检测到人员（模拟真实场景）
            boolean hasPersons = Math.random() > 0.3; // 70%概率检测到人员

            if (hasPersons) {
                mockResult.detectedPersons = 1 + (int)(Math.random() * 2); // 1-2个人
                mockResult.detectedObjects = mockResult.detectedPersons + (int)(Math.random() * 3); // 可能有其他物体

                // 只有检测到人员时才进行人脸分析
                mockResult.detectedFaces = mockResult.detectedPersons;
                mockResult.maleCount = (int)(Math.random() * (mockResult.detectedPersons + 1));
                mockResult.femaleCount = mockResult.detectedPersons - mockResult.maleCount;
                mockResult.faceAnalysisSuccess = true;

                // 创建模拟检测框
                mockResult.personDetections = new ArrayList<>();
                for (int i = 0; i < mockResult.detectedPersons; i++) {
                    float x1 = 50 + i * 100;
                    float y1 = 50 + i * 50;
                    float x2 = x1 + 80;
                    float y2 = y1 + 120;
                    float confidence = 0.8f + (float)(Math.random() * 0.2f);

                    RealYOLOInference.DetectionResult detection = new RealYOLOInference.DetectionResult(
                        0, confidence, x1, y1, x2, y2, "person");
                    mockResult.personDetections.add(detection);
                }

                android.util.Log.d(TAG, "Simulated AI results for camera " + cameraIndex +
                                  ": " + mockResult.detectedPersons + " persons with face analysis");
            } else {
                // 没有检测到人员，确保所有人脸相关数据为0
                mockResult.detectedPersons = 0;
                mockResult.detectedObjects = (int)(Math.random() * 3); // 可能有其他物体
                mockResult.detectedFaces = 0;
                mockResult.maleCount = 0;
                mockResult.femaleCount = 0;
                mockResult.faceAnalysisSuccess = false;
                mockResult.personDetections = new ArrayList<>();
                mockResult.ageGroups = new int[9]; // 确保年龄组数据也为0

                android.util.Log.d(TAG, "Simulated AI results for camera " + cameraIndex +
                                  ": 0 persons, no face analysis");
            }

            updateAIResults(cameraIndex, mockResult);
        }
    }

    /**
     * 获取摄像头数量
     */
    public int getCameraCount() {
        return cameraViews.size();
    }

    /**
     * 检查指定摄像头是否启用了AI分析
     */
    public boolean isAIAnalysisEnabled(int cameraIndex) {
        if (cameraIndex >= 0 && cameraIndex < cameraViews.size()) {
            return cameraViews.get(cameraIndex).aiAnalysisEnabled;
        }
        return false;
    }
}