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
import com.wulala.myyolov5rtspthreadpool.databinding.FragmentSurveillanceBinding;

/**
 * SurveillanceFragment for YOLOv5 RTSP streaming with object detection
 * Adapted from ojo project UI structure to integrate native video display
 */
public class SurveillanceFragment extends Fragment implements SurfaceHolder.Callback {

    private static final String TAG = "SurveillanceFragment";

    private FragmentSurveillanceBinding binding;
    private boolean fullscreenMode = false;
    private SurfaceView videoSurface;
    private MainActivity mainActivity;

    @Override
    public View onCreateView(
            LayoutInflater inflater, ViewGroup container,
            Bundle savedInstanceState
    ) {
        binding = FragmentSurveillanceBinding.inflate(inflater, container, false);
        mainActivity = (MainActivity) getActivity();

        // Create SurfaceView for native video rendering
        videoSurface = new SurfaceView(getContext());
        videoSurface.setLayoutParams(new LinearLayout.LayoutParams(
                LinearLayout.LayoutParams.MATCH_PARENT,
                LinearLayout.LayoutParams.MATCH_PARENT
        ));

        // Set up surface holder callback
        videoSurface.getHolder().addCallback(this);

        // Add video surface to the container
        binding.gridRowContainer.addView(videoSurface);

        // Set click listener for fullscreen toggle
        videoSurface.setOnClickListener(v -> {
            fullscreenMode = !fullscreenMode;
            if (fullscreenMode) {
                enterFullscreen();
            } else {
                exitFullscreen();
            }
        });

        return binding.getRoot();
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
    public void surfaceCreated(SurfaceHolder holder) {
        // Notify native code that surface is ready
        if (mainActivity != null) {
            mainActivity.setNativeSurface(holder.getSurface());
        }
    }

    @Override
    public void surfaceChanged(SurfaceHolder holder, int format, int width, int height) {
        // Update surface in native code
        if (mainActivity != null) {
            mainActivity.setNativeSurface(holder.getSurface());
        }
    }

    @Override
    public void surfaceDestroyed(SurfaceHolder holder) {
        // Release surface in native code
        if (mainActivity != null) {
            mainActivity.setNativeSurface(null);
        }
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
    }

    private void exitFullscreen() {
        // Show any hidden UI elements
        // The video surface maintains its layout
    }
}