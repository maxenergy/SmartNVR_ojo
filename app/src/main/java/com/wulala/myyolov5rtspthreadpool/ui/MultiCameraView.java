package com.wulala.myyolov5rtspthreadpool.ui;

import android.content.Context;
import android.util.AttributeSet;
import android.view.SurfaceHolder;
import android.view.SurfaceView;
import android.view.ViewGroup;
import android.widget.GridLayout;
import android.widget.TextView;

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
    
    public interface OnSurfaceChangeListener {
        void onSurfaceCreated(int cameraIndex, SurfaceHolder holder);
        void onSurfaceChanged(int cameraIndex, SurfaceHolder holder, int format, int width, int height);
        void onSurfaceDestroyed(int cameraIndex, SurfaceHolder holder);
    }
    
    private static class CameraViewHolder {
        SurfaceView surfaceView;
        TextView titleView;
        String cameraName;
        boolean isActive = false;
        
        CameraViewHolder(Context context, String name) {
            this.cameraName = name;
            this.surfaceView = new SurfaceView(context);
            this.titleView = new TextView(context);
            this.titleView.setText(name);
            this.titleView.setTextColor(0xFFFFFFFF);
            this.titleView.setBackgroundColor(0x80000000);
            this.titleView.setPadding(8, 4, 8, 4);
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
            
            // 添加到容器
            container.addView(holder.surfaceView);
            container.addView(holder.titleView);
            
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
}