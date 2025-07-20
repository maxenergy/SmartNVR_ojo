package com.wulala.myyolov5rtspthreadpool;

import android.content.Context;
import android.content.res.AssetManager;
import android.util.Log;

import java.io.File;

/**
 * InspireFace管理器
 * 负责InspireFace模块的初始化和管理
 */
public class InspireFaceManager {
    
    private static final String TAG = "InspireFaceManager";
    
    private static InspireFaceManager instance;
    private boolean initialized = false;
    private Context context;
    
    private InspireFaceManager() {
        // 私有构造函数，实现单例模式
    }
    
    /**
     * 获取单例实例
     */
    public static synchronized InspireFaceManager getInstance() {
        if (instance == null) {
            instance = new InspireFaceManager();
        }
        return instance;
    }
    
    /**
     * 初始化InspireFace模块
     * @param context Android上下文
     * @return true成功，false失败
     */
    public boolean initialize(Context context) {
        if (initialized) {
            Log.w(TAG, "InspireFace already initialized");
            return true;
        }
        
        this.context = context.getApplicationContext();
        
        Log.i(TAG, "Initializing InspireFace module");
        
        try {
            // 获取AssetManager
            AssetManager assetManager = context.getAssets();
            
            // 获取应用内部数据路径
            String internalDataPath = context.getFilesDir().getAbsolutePath();
            
            Log.d(TAG, "Internal data path: " + internalDataPath);

            // 直接使用DirectInspireFaceTest的成功方法
            Log.d(TAG, "Using direct InspireFace initialization method...");
            int result = DirectInspireFaceTest.testInspireFaceInit(assetManager, internalDataPath);
            
            if (result == 0) {
                initialized = true;
                Log.i(TAG, "InspireFace module initialized successfully");
                return true;
            } else {
                Log.e(TAG, "Failed to initialize InspireFace module, error code: " + result);
                return false;
            }
            
        } catch (Exception e) {
            Log.e(TAG, "Exception during InspireFace initialization", e);
            return false;
        }
    }
    
    /**
     * 检查是否已初始化
     */
    public boolean isInitialized() {
        return initialized;
    }
    
    /**
     * 获取模型文件大小信息
     */
    public String getModelInfo() {
        if (!initialized) {
            return "InspireFace not initialized";
        }
        
        try {
            // 计算模型文件总大小
            File modelDir = new File(context.getFilesDir(), "inspireface");
            if (modelDir.exists()) {
                long totalSize = calculateDirectorySize(modelDir);
                double sizeMB = totalSize / (1024.0 * 1024.0);
                return String.format("InspireFace models: %.1f MB", sizeMB);
            } else {
                return "Model directory not found";
            }
        } catch (Exception e) {
            Log.e(TAG, "Error getting model info", e);
            return "Error getting model info";
        }
    }
    
    /**
     * 获取初始化状态信息
     */
    public String getStatusInfo() {
        StringBuilder sb = new StringBuilder();
        sb.append("InspireFace Status:\n");
        sb.append("- Initialized: ").append(initialized).append("\n");
        
        if (initialized && context != null) {
            sb.append("- Data Path: ").append(context.getFilesDir().getAbsolutePath()).append("\n");
            sb.append("- ").append(getModelInfo()).append("\n");
        }
        
        return sb.toString();
    }
    
    /**
     * 重新初始化InspireFace模块
     */
    public boolean reinitialize() {
        Log.i(TAG, "Reinitializing InspireFace module");
        initialized = false;
        
        if (context != null) {
            return initialize(context);
        } else {
            Log.e(TAG, "Context is null, cannot reinitialize");
            return false;
        }
    }
    
    /**
     * 清理资源
     */
    public void cleanup() {
        Log.i(TAG, "Cleaning up InspireFace resources");
        initialized = false;
        context = null;
    }
    
    /**
     * 验证模型文件是否存在
     */
    public boolean validateModelFiles() {
        if (!initialized || context == null) {
            return false;
        }
        
        try {
            File modelDir = new File(context.getFilesDir(), "inspireface");
            if (!modelDir.exists()) {
                Log.w(TAG, "Model directory does not exist");
                return false;
            }
            
            // 检查关键模型文件
            String[] criticalFiles = {
                "__inspire__",  // 配置文件
                "_00_scrfd_2_5g_bnkps_shape320x320_rk3588.rknn",  // 人脸检测
                "_08_fairface_model_rk3588.rknn"  // 人脸属性
            };
            
            for (String fileName : criticalFiles) {
                File file = new File(modelDir, fileName);
                if (!file.exists()) {
                    Log.w(TAG, "Critical model file missing: " + fileName);
                    return false;
                }
            }
            
            Log.d(TAG, "Model file validation passed");
            return true;
            
        } catch (Exception e) {
            Log.e(TAG, "Error validating model files", e);
            return false;
        }
    }
    
    /**
     * 获取详细的模型文件信息
     */
    public String getDetailedModelInfo() {
        if (!initialized || context == null) {
            return "InspireFace not initialized";
        }
        
        StringBuilder sb = new StringBuilder();
        sb.append("InspireFace Model Files:\n");
        
        try {
            File modelDir = new File(context.getFilesDir(), "inspireface");
            if (modelDir.exists()) {
                File[] files = modelDir.listFiles();
                if (files != null) {
                    long totalSize = 0;
                    for (File file : files) {
                        if (file.isFile()) {
                            long size = file.length();
                            totalSize += size;
                            double sizeMB = size / (1024.0 * 1024.0);
                            sb.append(String.format("- %s: %.1f MB\n", file.getName(), sizeMB));
                        }
                    }
                    double totalSizeMB = totalSize / (1024.0 * 1024.0);
                    sb.append(String.format("Total: %.1f MB", totalSizeMB));
                } else {
                    sb.append("No files found in model directory");
                }
            } else {
                sb.append("Model directory not found");
            }
        } catch (Exception e) {
            sb.append("Error reading model directory: ").append(e.getMessage());
        }
        
        return sb.toString();
    }
    
    /**
     * 计算目录大小
     */
    private long calculateDirectorySize(File directory) {
        long size = 0;
        if (directory.exists()) {
            File[] files = directory.listFiles();
            if (files != null) {
                for (File file : files) {
                    if (file.isFile()) {
                        size += file.length();
                    } else if (file.isDirectory()) {
                        size += calculateDirectorySize(file);
                    }
                }
            }
        }
        return size;
    }
}
