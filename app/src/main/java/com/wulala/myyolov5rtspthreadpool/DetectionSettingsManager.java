package com.wulala.myyolov5rtspthreadpool;

import android.content.Context;
import android.content.SharedPreferences;
import android.util.Log;

import java.util.ArrayList;
import java.util.Arrays;
import java.util.HashSet;
import java.util.List;
import java.util.Set;

/**
 * 目标检测设置管理器
 * 管理YOLO检测的类别选择和过滤设置
 */
public class DetectionSettingsManager {
    private static final String TAG = "DetectionSettingsManager";
    private static final String PREFS_NAME = "detection_settings";
    private static final String KEY_ENABLED_CLASSES = "enabled_classes";
    private static final String KEY_CONFIDENCE_THRESHOLD = "confidence_threshold";
    private static final String KEY_DETECTION_ENABLED = "detection_enabled";
    
    // COCO数据集的80个类别（与yolov8_postprocess.cpp中的定义保持一致）
    public static final String[] ALL_YOLO_CLASSES = {
        "person", "bicycle", "car", "motorcycle", "airplane", "bus", "train", "truck", "boat", "traffic light",
        "fire hydrant", "stop sign", "parking meter", "bench", "bird", "cat", "dog", "horse", "sheep", "cow",
        "elephant", "bear", "zebra", "giraffe", "backpack", "umbrella", "handbag", "tie", "suitcase", "frisbee",
        "skis", "snowboard", "sports ball", "kite", "baseball bat", "baseball glove", "skateboard", "surfboard",
        "tennis racket", "bottle", "wine glass", "cup", "fork", "knife", "spoon", "bowl", "banana", "apple",
        "sandwich", "orange", "broccoli", "carrot", "hot dog", "pizza", "donut", "cake", "chair", "couch",
        "potted plant", "bed", "dining table", "toilet", "tv", "laptop", "mouse", "remote", "keyboard", "cell phone",
        "microwave", "oven", "toaster", "sink", "refrigerator", "book", "clock", "vase", "scissors", "teddy bear",
        "hair drier", "toothbrush"
    };
    
    // 常用类别的中文显示名称
    public static final String[] CLASS_DISPLAY_NAMES = {
        "人员", "自行车", "汽车", "摩托车", "飞机", "公交车", "火车", "卡车", "船", "交通灯",
        "消防栓", "停车标志", "停车计时器", "长椅", "鸟", "猫", "狗", "马", "羊", "牛",
        "大象", "熊", "斑马", "长颈鹿", "背包", "雨伞", "手提包", "领带", "手提箱", "飞盘",
        "滑雪板", "滑雪板", "运动球", "风筝", "棒球棒", "棒球手套", "滑板", "冲浪板",
        "网球拍", "瓶子", "酒杯", "杯子", "叉子", "刀", "勺子", "碗", "香蕉", "苹果",
        "三明治", "橙子", "西兰花", "胡萝卜", "热狗", "披萨", "甜甜圈", "蛋糕", "椅子", "沙发",
        "盆栽", "床", "餐桌", "厕所", "电视", "笔记本电脑", "鼠标", "遥控器", "键盘", "手机",
        "微波炉", "烤箱", "烤面包机", "水槽", "冰箱", "书", "时钟", "花瓶", "剪刀", "泰迪熊",
        "吹风机", "牙刷"
    };
    
    private Context context;
    private SharedPreferences prefs;
    
    public DetectionSettingsManager(Context context) {
        this.context = context.getApplicationContext();
        this.prefs = this.context.getSharedPreferences(PREFS_NAME, Context.MODE_PRIVATE);
    }
    
    /**
     * 获取启用的检测类别
     */
    public Set<String> getEnabledClasses() {
        Set<String> defaultClasses = new HashSet<>();
        defaultClasses.add("person"); // 默认只启用人员检测
        
        return prefs.getStringSet(KEY_ENABLED_CLASSES, defaultClasses);
    }
    
    /**
     * 设置启用的检测类别
     */
    public void setEnabledClasses(Set<String> enabledClasses) {
        prefs.edit()
             .putStringSet(KEY_ENABLED_CLASSES, enabledClasses)
             .apply();
        
        Log.d(TAG, "已更新启用的检测类别: " + enabledClasses.toString());
    }
    
    /**
     * 检查某个类别是否启用
     */
    public boolean isClassEnabled(String className) {
        return getEnabledClasses().contains(className);
    }
    
    /**
     * 启用或禁用某个类别
     */
    public void setClassEnabled(String className, boolean enabled) {
        Set<String> enabledClasses = new HashSet<>(getEnabledClasses());
        
        if (enabled) {
            enabledClasses.add(className);
        } else {
            enabledClasses.remove(className);
        }
        
        setEnabledClasses(enabledClasses);
    }
    
    /**
     * 获取置信度阈值
     */
    public float getConfidenceThreshold() {
        return prefs.getFloat(KEY_CONFIDENCE_THRESHOLD, 0.5f);
    }
    
    /**
     * 设置置信度阈值
     */
    public void setConfidenceThreshold(float threshold) {
        prefs.edit()
             .putFloat(KEY_CONFIDENCE_THRESHOLD, threshold)
             .apply();
        
        Log.d(TAG, "已更新置信度阈值: " + threshold);
    }
    
    /**
     * 检查目标检测是否启用
     */
    public boolean isDetectionEnabled() {
        return prefs.getBoolean(KEY_DETECTION_ENABLED, true);
    }
    
    /**
     * 设置目标检测启用状态
     */
    public void setDetectionEnabled(boolean enabled) {
        prefs.edit()
             .putBoolean(KEY_DETECTION_ENABLED, enabled)
             .apply();
        
        Log.d(TAG, "目标检测" + (enabled ? "已启用" : "已禁用"));
    }
    
    /**
     * 获取类别的显示名称
     */
    public static String getDisplayName(String className) {
        for (int i = 0; i < ALL_YOLO_CLASSES.length && i < CLASS_DISPLAY_NAMES.length; i++) {
            if (ALL_YOLO_CLASSES[i].equals(className)) {
                return CLASS_DISPLAY_NAMES[i];
            }
        }
        return className; // 如果没找到，返回原始名称
    }
    
    /**
     * 获取类别的索引
     */
    public static int getClassIndex(String className) {
        for (int i = 0; i < ALL_YOLO_CLASSES.length; i++) {
            if (ALL_YOLO_CLASSES[i].equals(className)) {
                return i;
            }
        }
        return -1;
    }
    
    /**
     * 根据索引获取类别名称
     */
    public static String getClassName(int classIndex) {
        if (classIndex >= 0 && classIndex < ALL_YOLO_CLASSES.length) {
            return ALL_YOLO_CLASSES[classIndex];
        }
        return "unknown";
    }
    
    /**
     * 获取所有可用的类别列表
     */
    public static List<String> getAllClasses() {
        return Arrays.asList(ALL_YOLO_CLASSES);
    }
    
    /**
     * 重置为默认设置
     */
    public void resetToDefaults() {
        Set<String> defaultClasses = new HashSet<>();
        defaultClasses.add("person");
        
        prefs.edit()
             .putStringSet(KEY_ENABLED_CLASSES, defaultClasses)
             .putFloat(KEY_CONFIDENCE_THRESHOLD, 0.5f)
             .putBoolean(KEY_DETECTION_ENABLED, true)
             .apply();
        
        Log.d(TAG, "已重置为默认设置");
    }
}
