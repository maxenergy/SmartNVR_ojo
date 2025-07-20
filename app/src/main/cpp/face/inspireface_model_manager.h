#ifndef INSPIREFACE_MODEL_MANAGER_H
#define INSPIREFACE_MODEL_MANAGER_H

#include <string>
#include <vector>
#include <android/asset_manager.h>

/**
 * InspireFace模型管理器
 * 负责从assets复制模型文件到内部存储，并管理模型路径
 */
class InspireFaceModelManager {
public:
    /**
     * 构造函数
     * @param assetManager Android资产管理器
     * @param internalDataPath 应用内部数据路径
     */
    InspireFaceModelManager(AAssetManager* assetManager, const std::string& internalDataPath);
    
    /**
     * 析构函数
     */
    ~InspireFaceModelManager();
    
    /**
     * 初始化模型管理器
     * 检查并复制所需的模型文件
     * @return 成功返回true，失败返回false
     */
    bool initialize();
    
    /**
     * 获取模型根目录路径
     * @return 模型根目录的完整路径
     */
    std::string getModelRootPath() const;
    
    /**
     * 获取配置文件路径
     * @return 配置文件的完整路径
     */
    std::string getConfigFilePath() const;
    
    /**
     * 检查所有必需的模型文件是否存在
     * @return 所有文件存在返回true，否则返回false
     */
    bool validateModelFiles() const;
    
    /**
     * 获取模型文件列表
     * @return 模型文件名列表
     */
    std::vector<std::string> getModelFileList() const;
    
    /**
     * 获取特定模型文件的完整路径
     * @param modelName 模型文件名
     * @return 模型文件的完整路径
     */
    std::string getModelFilePath(const std::string& modelName) const;
    
    /**
     * 清理模型文件（可选）
     * @return 成功返回true，失败返回false
     */
    bool cleanupModelFiles();
    
    /**
     * 获取模型文件总大小（MB）
     * @return 模型文件总大小
     */
    double getTotalModelSizeMB() const;
    
    /**
     * 检查是否已初始化
     * @return 已初始化返回true，否则返回false
     */
    bool isInitialized() const { return m_initialized; }

private:
    /**
     * 从assets复制单个文件到内部存储
     * @param assetPath assets中的文件路径
     * @param targetPath 目标文件路径
     * @return 成功返回true，失败返回false
     */
    bool copyAssetFile(const std::string& assetPath, const std::string& targetPath);
    
    /**
     * 检查文件是否存在
     * @param filePath 文件路径
     * @return 存在返回true，否则返回false
     */
    bool fileExists(const std::string& filePath) const;
    
    /**
     * 创建目录（如果不存在）
     * @param dirPath 目录路径
     * @return 成功返回true，失败返回false
     */
    bool createDirectory(const std::string& dirPath) const;
    
    /**
     * 获取文件大小
     * @param filePath 文件路径
     * @return 文件大小（字节），失败返回-1
     */
    long getFileSize(const std::string& filePath) const;

private:
    AAssetManager* m_assetManager;
    std::string m_internalDataPath;
    std::string m_modelRootPath;
    std::string m_configFilePath;
    bool m_initialized;
    
    // 模型文件列表
    static const std::vector<std::string> MODEL_FILES;
    static const std::string CONFIG_FILE_NAME;
    static const std::string MODEL_DIR_NAME;
};

#endif // INSPIREFACE_MODEL_MANAGER_H
