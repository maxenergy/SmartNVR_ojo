package com.wulala.myyolov5rtspthreadpool.ui;

import android.os.Bundle;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.widget.TextView;

import androidx.annotation.NonNull;
import androidx.annotation.Nullable;
import androidx.fragment.app.Fragment;

import com.wulala.myyolov5rtspthreadpool.R;

/**
 * Info fragment for YOLOv5 RTSP application information
 */
public class InfoFragment extends Fragment {

    @Nullable
    @Override
    public View onCreateView(@NonNull LayoutInflater inflater, @Nullable ViewGroup container, @Nullable Bundle savedInstanceState) {
        TextView textView = new TextView(getContext());
        textView.setText("YOLOv5 RTSP Object Detection\n\n" +
                "This application provides real-time object detection using YOLOv5 model " +
                "with RTSP streaming capabilities optimized for Rockchip RK3588 hardware.\n\n" +
                "Features:\n" +
                "• YOLOv5 inference with RKNN runtime\n" +
                "• RTSP streaming with ZLMediaKit\n" +
                "• Hardware-accelerated video decoding\n" +
                "• Real-time detection box drawing\n" +
                "• Configurable confidence threshold\n\n" +
                "Version: 1.0.0");
        textView.setPadding(64, 64, 64, 64);
        textView.setTextSize(16);
        return textView;
    }
}