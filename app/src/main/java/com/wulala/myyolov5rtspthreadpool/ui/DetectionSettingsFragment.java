package com.wulala.myyolov5rtspthreadpool.ui;

import android.os.Bundle;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.widget.Button;
import android.widget.CompoundButton;
import android.widget.SeekBar;
import android.widget.Switch;
import android.widget.TextView;
import android.widget.Toast;

import androidx.annotation.NonNull;
import androidx.annotation.Nullable;
import androidx.fragment.app.Fragment;
import androidx.navigation.Navigation;
import androidx.recyclerview.widget.LinearLayoutManager;
import androidx.recyclerview.widget.RecyclerView;

import com.wulala.myyolov5rtspthreadpool.DetectionSettingsManager;
import com.wulala.myyolov5rtspthreadpool.R;
import com.wulala.myyolov5rtspthreadpool.ui.adapters.DetectionClassAdapter;

import java.util.ArrayList;
import java.util.List;
import java.util.Set;

/**
 * 目标检测设置界面
 * 允许用户选择要检测的目标类别和调整检测参数
 */
public class DetectionSettingsFragment extends Fragment {
    private static final String TAG = "DetectionSettingsFragment";
    
    private DetectionSettingsManager settingsManager;
    private DetectionClassAdapter adapter;
    
    private Switch switchDetectionEnabled;
    private SeekBar seekBarConfidence;
    private TextView textConfidenceValue;
    private RecyclerView recyclerViewClasses;
    private Button btnBack;
    private Button btnReset;
    private TextView textStatusInfo;
    
    @Override
    public View onCreateView(LayoutInflater inflater, ViewGroup container,
                             Bundle savedInstanceState) {
        android.util.Log.d(TAG, "onCreateView 被调用");
        View view = inflater.inflate(R.layout.fragment_detection_settings, container, false);
        android.util.Log.d(TAG, "布局加载完成");
        return view;
    }
    
    @Override
    public void onViewCreated(@NonNull View view, @Nullable Bundle savedInstanceState) {
        super.onViewCreated(view, savedInstanceState);

        android.util.Log.d(TAG, "onViewCreated 被调用");

        // 初始化设置管理器
        settingsManager = new DetectionSettingsManager(getContext());
        android.util.Log.d(TAG, "设置管理器初始化完成");

        // 初始化视图
        initViews(view);
        setupListeners();
        loadSettings();

        android.util.Log.d(TAG, "Fragment初始化完成");
    }
    
    private void initViews(View view) {
        android.util.Log.d(TAG, "initViews 开始");

        switchDetectionEnabled = view.findViewById(R.id.switch_detection_enabled);
        seekBarConfidence = view.findViewById(R.id.seekbar_confidence);
        textConfidenceValue = view.findViewById(R.id.text_confidence_value);
        recyclerViewClasses = view.findViewById(R.id.recyclerview_classes);

        android.util.Log.d(TAG, "RecyclerView找到: " + (recyclerViewClasses != null));
        btnBack = view.findViewById(R.id.btn_back);
        btnReset = view.findViewById(R.id.btn_reset);
        textStatusInfo = view.findViewById(R.id.text_status_info);
        
        // 设置RecyclerView
        android.util.Log.d(TAG, "设置RecyclerView布局管理器");
        recyclerViewClasses.setLayoutManager(new LinearLayoutManager(getContext()));

        // 创建适配器
        android.util.Log.d(TAG, "创建类别项目列表");
        List<DetectionClassItem> classItems = createClassItems();
        android.util.Log.d(TAG, "类别项目数量: " + classItems.size());

        adapter = new DetectionClassAdapter(classItems, new DetectionClassAdapter.OnClassToggleListener() {
            @Override
            public void onClassToggled(String className, boolean enabled) {
                android.util.Log.d(TAG, "类别切换回调: " + className + " -> " + enabled);
                settingsManager.setClassEnabled(className, enabled);
                updateStatusInfo(); // 更新状态信息
            }
        });

        android.util.Log.d(TAG, "设置适配器到RecyclerView");
        recyclerViewClasses.setAdapter(adapter);
        android.util.Log.d(TAG, "RecyclerView设置完成");
    }
    
    private void setupListeners() {
        // 返回按钮监听器
        btnBack.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View v) {
                Navigation.findNavController(v).navigateUp();
            }
        });

        // 重置按钮监听器
        btnReset.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View v) {
                settingsManager.resetToDefaults();
                loadSettings();
                Toast.makeText(getContext(), "设置已重置为默认值", Toast.LENGTH_SHORT).show();
            }
        });

        // 检测开关监听器
        switchDetectionEnabled.setOnCheckedChangeListener(new CompoundButton.OnCheckedChangeListener() {
            @Override
            public void onCheckedChanged(CompoundButton buttonView, boolean isChecked) {
                settingsManager.setDetectionEnabled(isChecked);
                updateUIState(isChecked);
                updateStatusInfo();
                showSettingsSavedToast();
            }
        });
        
        // 置信度滑块监听器
        seekBarConfidence.setOnSeekBarChangeListener(new SeekBar.OnSeekBarChangeListener() {
            @Override
            public void onProgressChanged(SeekBar seekBar, int progress, boolean fromUser) {
                if (fromUser) {
                    float confidence = progress / 100.0f;
                    settingsManager.setConfidenceThreshold(confidence);
                    updateConfidenceDisplay(confidence);
                    updateStatusInfo();
                    showSettingsSavedToast();
                }
            }
            
            @Override
            public void onStartTrackingTouch(SeekBar seekBar) {}
            
            @Override
            public void onStopTrackingTouch(SeekBar seekBar) {}
        });
    }
    
    private void loadSettings() {
        // 加载检测开关状态
        boolean detectionEnabled = settingsManager.isDetectionEnabled();
        switchDetectionEnabled.setChecked(detectionEnabled);
        updateUIState(detectionEnabled);
        
        // 加载置信度阈值
        float confidence = settingsManager.getConfidenceThreshold();
        seekBarConfidence.setProgress((int) (confidence * 100));
        updateConfidenceDisplay(confidence);
        
        // 加载启用的类别
        Set<String> enabledClasses = settingsManager.getEnabledClasses();
        adapter.updateEnabledClasses(enabledClasses);

        // 更新状态信息
        updateStatusInfo();
    }
    
    private void updateUIState(boolean detectionEnabled) {
        seekBarConfidence.setEnabled(detectionEnabled);
        recyclerViewClasses.setEnabled(detectionEnabled);
        
        // 更新RecyclerView中所有项目的启用状态
        for (int i = 0; i < recyclerViewClasses.getChildCount(); i++) {
            View child = recyclerViewClasses.getChildAt(i);
            if (child != null) {
                child.setEnabled(detectionEnabled);
                child.setAlpha(detectionEnabled ? 1.0f : 0.5f);
            }
        }
    }
    
    private void updateConfidenceDisplay(float confidence) {
        textConfidenceValue.setText(String.format("%.2f", confidence));
    }
    
    private List<DetectionClassItem> createClassItems() {
        List<DetectionClassItem> items = new ArrayList<>();
        Set<String> enabledClasses = settingsManager.getEnabledClasses();
        
        // 优先显示常用类别
        String[] priorityClasses = {"person", "car", "bicycle", "motorcycle", "bus", "truck", 
                                   "chair", "couch", "tv", "laptop", "bottle", "cup"};
        
        // 添加优先类别
        for (String className : priorityClasses) {
            boolean enabled = enabledClasses.contains(className);
            String displayName = DetectionSettingsManager.getDisplayName(className);
            items.add(new DetectionClassItem(className, displayName, enabled, true));
        }
        
        // 添加其他类别
        for (String className : DetectionSettingsManager.ALL_YOLO_CLASSES) {
            boolean alreadyAdded = false;
            for (String priority : priorityClasses) {
                if (priority.equals(className)) {
                    alreadyAdded = true;
                    break;
                }
            }
            
            if (!alreadyAdded) {
                boolean enabled = enabledClasses.contains(className);
                String displayName = DetectionSettingsManager.getDisplayName(className);
                items.add(new DetectionClassItem(className, displayName, enabled, false));
            }
        }
        
        return items;
    }

    /**
     * 更新状态信息显示
     */
    private void updateStatusInfo() {
        boolean detectionEnabled = settingsManager.isDetectionEnabled();
        float confidence = settingsManager.getConfidenceThreshold();
        Set<String> enabledClasses = settingsManager.getEnabledClasses();

        String statusText = String.format("%s | 置信度: %.2f | 启用类别: %d个",
                detectionEnabled ? "✅ 检测已启用" : "❌ 检测已禁用",
                confidence,
                enabledClasses.size());

        textStatusInfo.setText(statusText);

        // 根据状态设置背景颜色
        if (detectionEnabled && enabledClasses.size() > 0) {
            textStatusInfo.setBackgroundColor(0xFFE8F5E8); // 浅绿色
        } else {
            textStatusInfo.setBackgroundColor(0xFFFFF0F0); // 浅红色
        }
    }

    /**
     * 显示设置已保存的提示
     */
    private void showSettingsSavedToast() {
        Toast.makeText(getContext(), "设置已保存", Toast.LENGTH_SHORT).show();
    }

    /**
     * 检测类别项目数据类
     */
    public static class DetectionClassItem {
        public final String className;
        public final String displayName;
        public boolean enabled;
        public final boolean isPriority;
        
        public DetectionClassItem(String className, String displayName, boolean enabled, boolean isPriority) {
            this.className = className;
            this.displayName = displayName;
            this.enabled = enabled;
            this.isPriority = isPriority;
        }
    }
}
