#include "inspireface_model_manager.h"
#include "log4c.h"
#include <fstream>
#include <sys/stat.h>
#include <unistd.h>
#include <android/asset_manager.h>

// 静态成员定义
const std::string InspireFaceModelManager::CONFIG_FILE_NAME = "__inspire__";
const std::string InspireFaceModelManager::MODEL_DIR_NAME = "inspireface";

const std::vector<std::string> InspireFaceModelManager::MODEL_FILES = {
    "_00_scrfd_2_5g_bnkps_shape160x160_rk3588.rknn",
    "_00_scrfd_2_5g_bnkps_shape320x320_rk3588.rknn", 
    "_00_scrfd_2_5g_bnkps_shape640x640_rk3588.rknn",
    "_01_hyplmkv2_0.25_112x_rk3588.rknn",
    "_03_r18_Glint360K_fixed_rk3588.rknn",
    "_04_rnet_rk3588.rknn",
    "_05_facemask_mb_025_cut_rk3588.rknn",
    "_06_msafa27_rk3588.rknn",
    "_07_pose-quality_rk3588.rknn",
    "_08_fairface_model_rk3588.rknn",
    "_09_blink_crop.mnn",
    "_10_emotion_rk3588.rknn"
};

InspireFaceModelManager::InspireFaceModelManager(AAssetManager* assetManager, 
                                                 const std::string& internalDataPath)
    : m_assetManager(assetManager)
    , m_internalDataPath(internalDataPath)
    , m_initialized(false) {
    
    m_modelRootPath = m_internalDataPath + "/" + MODEL_DIR_NAME;
    m_configFilePath = m_modelRootPath + "/" + CONFIG_FILE_NAME;
    
    LOGI("InspireFaceModelManager created with path: %s", m_modelRootPath.c_str());
}

InspireFaceModelManager::~InspireFaceModelManager() {
    LOGD("InspireFaceModelManager destroyed");
}

bool InspireFaceModelManager::initialize() {
    if (m_initialized) {
        LOGW("InspireFaceModelManager already initialized");
        return true;
    }
    
    if (!m_assetManager) {
        LOGE("Asset manager is null");
        return false;
    }
    
    LOGI("Initializing InspireFace model manager");
    
    // 创建模型目录
    if (!createDirectory(m_modelRootPath)) {
        LOGE("Failed to create model directory: %s", m_modelRootPath.c_str());
        return false;
    }
    
    // 复制配置文件
    std::string assetConfigPath = MODEL_DIR_NAME + "/" + CONFIG_FILE_NAME;
    if (!copyAssetFile(assetConfigPath, m_configFilePath)) {
        LOGE("Failed to copy config file");
        return false;
    }
    
    // 复制所有模型文件
    int successCount = 0;
    for (const auto& modelFile : MODEL_FILES) {
        std::string assetPath = MODEL_DIR_NAME + "/" + modelFile;
        std::string targetPath = m_modelRootPath + "/" + modelFile;
        
        if (copyAssetFile(assetPath, targetPath)) {
            successCount++;
            LOGD("Successfully copied model file: %s", modelFile.c_str());
        } else {
            LOGW("Failed to copy model file: %s", modelFile.c_str());
        }
    }
    
    LOGI("Copied %d/%zu model files", successCount, MODEL_FILES.size());
    
    // 验证模型文件
    if (!validateModelFiles()) {
        LOGE("Model file validation failed");
        return false;
    }
    
    m_initialized = true;
    LOGI("InspireFace model manager initialized successfully");
    LOGI("Model root path: %s", m_modelRootPath.c_str());
    LOGI("Total model size: %.1f MB", getTotalModelSizeMB());
    
    return true;
}

std::string InspireFaceModelManager::getModelRootPath() const {
    return m_modelRootPath;
}

std::string InspireFaceModelManager::getConfigFilePath() const {
    return m_configFilePath;
}

bool InspireFaceModelManager::validateModelFiles() const {
    // 检查配置文件
    if (!fileExists(m_configFilePath)) {
        LOGE("Config file not found: %s", m_configFilePath.c_str());
        return false;
    }
    
    // 检查关键模型文件（至少需要人脸检测和属性分析模型）
    std::vector<std::string> criticalFiles = {
        "_00_scrfd_2_5g_bnkps_shape320x320_rk3588.rknn", // 人脸检测
        "_08_fairface_model_rk3588.rknn"                  // 人脸属性
    };
    
    for (const auto& file : criticalFiles) {
        std::string filePath = m_modelRootPath + "/" + file;
        if (!fileExists(filePath)) {
            LOGE("Critical model file not found: %s", filePath.c_str());
            return false;
        }
    }
    
    LOGD("Model file validation passed");
    return true;
}

std::vector<std::string> InspireFaceModelManager::getModelFileList() const {
    return MODEL_FILES;
}

std::string InspireFaceModelManager::getModelFilePath(const std::string& modelName) const {
    return m_modelRootPath + "/" + modelName;
}

bool InspireFaceModelManager::cleanupModelFiles() {
    LOGI("Cleaning up model files");
    
    // 删除所有模型文件
    for (const auto& modelFile : MODEL_FILES) {
        std::string filePath = m_modelRootPath + "/" + modelFile;
        if (fileExists(filePath)) {
            if (unlink(filePath.c_str()) == 0) {
                LOGD("Deleted model file: %s", modelFile.c_str());
            } else {
                LOGW("Failed to delete model file: %s", modelFile.c_str());
            }
        }
    }
    
    // 删除配置文件
    if (fileExists(m_configFilePath)) {
        unlink(m_configFilePath.c_str());
    }
    
    // 删除目录
    rmdir(m_modelRootPath.c_str());
    
    return true;
}

double InspireFaceModelManager::getTotalModelSizeMB() const {
    long totalSize = 0;
    
    // 计算配置文件大小
    totalSize += getFileSize(m_configFilePath);
    
    // 计算所有模型文件大小
    for (const auto& modelFile : MODEL_FILES) {
        std::string filePath = m_modelRootPath + "/" + modelFile;
        long fileSize = getFileSize(filePath);
        if (fileSize > 0) {
            totalSize += fileSize;
        }
    }
    
    return totalSize / (1024.0 * 1024.0); // 转换为MB
}

bool InspireFaceModelManager::copyAssetFile(const std::string& assetPath, 
                                            const std::string& targetPath) {
    if (!m_assetManager) {
        LOGE("Asset manager is null");
        return false;
    }
    
    // 如果目标文件已存在且大小合理，跳过复制
    if (fileExists(targetPath)) {
        long fileSize = getFileSize(targetPath);
        if (fileSize > 1024) { // 至少1KB
            LOGD("Target file already exists, skipping: %s", targetPath.c_str());
            return true;
        }
    }
    
    LOGD("Copying asset file: %s -> %s", assetPath.c_str(), targetPath.c_str());
    
    AAsset* asset = AAssetManager_open(m_assetManager, assetPath.c_str(), AASSET_MODE_BUFFER);
    if (!asset) {
        LOGE("Failed to open asset: %s", assetPath.c_str());
        return false;
    }
    
    off_t assetLength = AAsset_getLength(asset);
    const void* assetBuffer = AAsset_getBuffer(asset);
    
    if (!assetBuffer || assetLength <= 0) {
        LOGE("Failed to get asset buffer: %s", assetPath.c_str());
        AAsset_close(asset);
        return false;
    }
    
    std::ofstream outFile(targetPath, std::ios::binary);
    if (!outFile.is_open()) {
        LOGE("Failed to create target file: %s", targetPath.c_str());
        AAsset_close(asset);
        return false;
    }
    
    outFile.write(static_cast<const char*>(assetBuffer), assetLength);
    outFile.close();
    
    AAsset_close(asset);
    
    // 验证复制结果
    if (!fileExists(targetPath)) {
        LOGE("File copy verification failed: %s", targetPath.c_str());
        return false;
    }
    
    long copiedSize = getFileSize(targetPath);
    if (copiedSize != assetLength) {
        LOGE("File size mismatch: expected %ld, got %ld", assetLength, copiedSize);
        return false;
    }
    
    LOGD("Successfully copied %ld bytes: %s", assetLength, targetPath.c_str());
    return true;
}

bool InspireFaceModelManager::fileExists(const std::string& filePath) const {
    struct stat buffer;
    return (stat(filePath.c_str(), &buffer) == 0);
}

bool InspireFaceModelManager::createDirectory(const std::string& dirPath) const {
    struct stat st = {0};
    
    if (stat(dirPath.c_str(), &st) == -1) {
        if (mkdir(dirPath.c_str(), 0755) != 0) {
            LOGE("Failed to create directory: %s", dirPath.c_str());
            return false;
        }
        LOGD("Created directory: %s", dirPath.c_str());
    }
    
    return true;
}

long InspireFaceModelManager::getFileSize(const std::string& filePath) const {
    struct stat st;
    if (stat(filePath.c_str(), &st) == 0) {
        return st.st_size;
    }
    return -1;
}
