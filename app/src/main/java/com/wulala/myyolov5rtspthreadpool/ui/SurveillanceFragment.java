package com.wulala.myyolov5rtspthreadpool.ui;

import android.os.Build;
import android.os.Bundle;
import android.view.LayoutInflater;
import android.view.SurfaceHolder;
import android.view.SurfaceView;
import android.view.View;
import android.view.ViewGroup;
import android.view.Window;
import android.view.WindowManager;
import android.widget.LinearLayout;

import androidx.core.view.WindowCompat;
import androidx.core.view.WindowInsetsCompat;
import androidx.core.view.WindowInsetsControllerCompat;
import androidx.fragment.app.Fragment;

import com.wulala.myyolov5rtspthreadpool.MainActivity;
import com.wulala.myyolov5rtspthreadpool.Settings;
import com.wulala.myyolov5rtspthreadpool.databinding.FragmentSurveillanceBinding;
import com.wulala.myyolov5rtspthreadpool.entities.Camera;

import java.util.ArrayList;
import java.util.List;

/**
 * SurveillanceFragment for YOLOv5 RTSP streaming with multi-camera support
 * Support 1x1, 1x2, 2x2, 3x3, 4x4 grid layouts for multiple cameras
 */
public class SurveillanceFragment extends Fragment implements MultiCameraView.OnSurfaceChangeListener {

    private static final String TAG = "SurveillanceFragment";

    private FragmentSurveillanceBinding binding;
    private boolean fullscreenMode = false;
    private MultiCameraView multiCameraView;
    private MainActivity mainActivity;
    private List<Camera> cameras;

    @Override
    public View onCreateView(
            LayoutInflater inflater, ViewGroup container,
            Bundle savedInstanceState
    ) {
        binding = FragmentSurveillanceBinding.inflate(inflater, container, false);
        mainActivity = (MainActivity) getActivity();

        // 创建多摄像头视图
        multiCameraView = new MultiCameraView(getContext());
        multiCameraView.setLayoutParams(new LinearLayout.LayoutParams(
                LinearLayout.LayoutParams.MATCH_PARENT,
                LinearLayout.LayoutParams.MATCH_PARENT
        ));

        // 设置Surface回调监听器
        multiCameraView.setOnSurfaceChangeListener(this);

        // 添加多摄像头视图到容器
        binding.gridRowContainer.addView(multiCameraView);

        // 加载摄像头配置并设置视图
        loadCamerasAndSetupViews();

        return binding.getRoot();
    }

    private void loadCamerasAndSetupViews() {
        // 从设置中加载摄像头列表
        Settings settings = Settings.fromDisk(getContext());
        cameras = settings.getCameras();
        
        // 创建摄像头名称列表
        List<String> cameraNames = new ArrayList<>();
        for (Camera camera : cameras) {
            cameraNames.add(camera.getName());
        }
        
        // 如果没有摄像头，添加默认摄像头
        if (cameraNames.isEmpty()) {
            cameraNames.add("默认摄像头");
        }
        
        android.util.Log.d(TAG, "Loading " + cameraNames.size() + " cameras");
        
        // 设置多摄像头视图
        multiCameraView.setupCameras(cameraNames);
        
        // 通知MainActivity摄像头数量变化
        if (mainActivity != null) {
            mainActivity.onCameraCountChanged(cameras.size());
        }
    }

    @Override
    public void onResume() {
        super.onResume();

        leanbackMode(true);
        fullscreenMode = false;

        // Register for back pressed events
        mainActivity.setOnBackButtonPressedListener(new OnBackButtonPressedListener() {
            @Override
            public boolean onBackPressed() {
                if(fullscreenMode) {
                    fullscreenMode = false;
                    exitFullscreen();
                    return true;
                }
                return false;
            }
        });
    }

    @Override
    public void onPause() {
        super.onPause();
        leanbackMode(false);
    }

    @Override
    public void onSurfaceCreated(int cameraIndex, SurfaceHolder holder) {
        android.util.Log.d(TAG, "Surface created for camera " + cameraIndex);
        // 通知native代码surface已准备就绪
        if (mainActivity != null) {
            mainActivity.setNativeSurface(cameraIndex, holder.getSurface());
        }
        
        // 设置摄像头为活跃状态
        multiCameraView.setCameraActive(cameraIndex, true);
    }

    @Override
    public void onSurfaceChanged(int cameraIndex, SurfaceHolder holder, int format, int width, int height) {
        android.util.Log.d(TAG, "Surface changed for camera " + cameraIndex + 
                           ": " + width + "x" + height);
        // 更新native代码中的surface
        if (mainActivity != null) {
            mainActivity.setNativeSurface(cameraIndex, holder.getSurface());
        }
    }

    @Override
    public void onSurfaceDestroyed(int cameraIndex, SurfaceHolder holder) {
        android.util.Log.d(TAG, "Surface destroyed for camera " + cameraIndex);
        // 释放native代码中的surface
        if (mainActivity != null) {
            mainActivity.setNativeSurface(cameraIndex, null);
        }
        
        // 设置摄像头为非活跃状态
        multiCameraView.setCameraActive(cameraIndex, false);
    }

    /**
     * Goes fullscreen ignoring the device screen insets (camera etc)
     */
    private void leanbackMode(boolean leanback) {
        // 暂时简化全屏模式，避免影响FAB触摸事件
        if (leanback) {
            android.util.Log.d(TAG, "Enabling leanback mode");
            // 只隐藏状态栏，保留导航栏以确保FAB可以响应
            requireActivity().getWindow().setFlags(
                WindowManager.LayoutParams.FLAG_FULLSCREEN,
                WindowManager.LayoutParams.FLAG_FULLSCREEN
            );
        } else {
            android.util.Log.d(TAG, "Disabling leanback mode");
            requireActivity().getWindow().clearFlags(WindowManager.LayoutParams.FLAG_FULLSCREEN);
        }
    }

    private void enterFullscreen() {
        // Hide any additional UI elements for fullscreen viewing
        // The video surface already fills the container
        fullscreenMode = true;
        android.util.Log.d(TAG, "Entered fullscreen mode");
    }

    private void exitFullscreen() {
        // Show any hidden UI elements
        // The video surface maintains its layout
        fullscreenMode = false;
        android.util.Log.d(TAG, "Exited fullscreen mode");
    }
    
    /**
     * 刷新摄像头视图（当设置改变时调用）
     */
    public void refreshCameraViews() {
        loadCamerasAndSetupViews();
    }
    
    /**
     * 获取指定摄像头的SurfaceView
     */
    public SurfaceView getCameraSurfaceView(int cameraIndex) {
        return multiCameraView.getSurfaceView(cameraIndex);
    }

    /**
     * 获取MultiCameraView实例
     */
    public MultiCameraView getMultiCameraView() {
        return multiCameraView;
    }
}