#ifndef FACE_ANALYSIS_MANAGER_H
#define FACE_ANALYSIS_MANAGER_H

#include <opencv2/opencv.hpp>
#include <vector>
#include <memory>
#include <string>
#include "person_detection_types.h"
#include "model_config.h"  // 包含InferenceResult定义

// 🔧 Phase 2: 前向声明（使用真实InspireFace）
struct AAssetManager;
class InspireFaceSession;
class InspireFaceImageProcessor;
class InspireFaceDetector;

// 🔧 Phase 2: 前向声明和临时结构定义
struct FaceAnalysisConfig {
    bool enableGenderDetection = true;
    bool enableAgeDetection = true;
    bool enableRaceDetection = false;
    float faceDetectionThreshold = 0.5f;
    int maxFacesPerPerson = 1;
};

// 🔧 Phase 2: 前向声明，实际定义在FaceAnalysisManager类内部
struct PersonDetection;
struct SimpleFaceAnalysisResult;

/**
 * @brief 人脸分析管理器
 * 负责人脸检测、特征提取和身份识别
 */
class FaceAnalysisManager {
public:
    // 🔧 Phase 2: 简化的人脸分析接口结构（用于JNI调用）
    struct PersonDetection {
        float x1, y1, x2, y2;
        float confidence;
    };

    struct SimpleFaceAnalysisResult {
        bool success = false;
        std::string errorMessage;
        int faceCount = 0;
        int maleCount = 0;
        int femaleCount = 0;
        int ageGroups[9] = {0}; // 年龄组分布

        struct Face {
            float x1, y1, x2, y2;
            float confidence;
            int gender; // 0: 男性, 1: 女性
            int age;    // 年龄段 0-8
        };
        std::vector<Face> faces;
    };

    FaceAnalysisManager();
    ~FaceAnalysisManager();

    /**
     * @brief 初始化人脸分析管理器
     * @return 0成功，-1失败
     */
    int initialize();

    /**
     * @brief 🔧 Phase 2: 使用模型路径初始化人脸分析管理器
     * @param modelPath 模型文件路径
     * @return true成功，false失败
     */
    bool initialize(const std::string& modelPath);

    /**
     * @brief 🔧 Phase 2: 使用InspireFace初始化人脸分析管理器
     * @param assetManager Android资产管理器
     * @param internalDataPath 内部数据路径
     * @return true成功，false失败
     */
    bool initializeInspireFace(AAssetManager* assetManager, const std::string& internalDataPath);

    /**
     * @brief 🔧 Phase 2: 延迟初始化InspireFace（避免与RTSP/MPP冲突）
     */
    bool performDelayedInspireFaceInitialization();
    bool initializeInspireFaceLibraryStep();
    bool createInspireFaceComponentsStep();
    bool initializeInspireFaceSessionStep();

    /**
     * @brief 释放资源
     */
    void release();

    /**
     * @brief 分析人脸
     * @param image 输入图像
     * @return 人脸分析结果
     */
    FaceAnalysisResult analyzeFace(const cv::Mat& image);

    /**
     * @brief 🔧 Phase 2: 分析人员区域中的人脸
     * @param image 输入图像
     * @param personDetections 人员检测结果
     * @param results 输出人脸分析结果
     * @return true成功，false失败
     */
    bool analyzePersonRegions(const cv::Mat& image,
                             const std::vector<InferenceResult>& personDetections,
                             std::vector<FaceAnalysisResult>& results);

    /**
     * @brief 检测人脸
     * @param image 输入图像
     * @param faces 输出人脸检测结果
     * @return 检测到的人脸数量
     */
    int detectFaces(const cv::Mat& image, std::vector<cv::Rect>& faces);

    /**
     * @brief 提取人脸特征
     * @param face_image 人脸图像
     * @return 人脸特征向量
     */
    std::vector<float> extractFaceFeatures(const cv::Mat& face_image);

    /**
     * @brief 估计年龄
     * @param face_image 人脸图像
     * @return 估计年龄
     */
    int estimateAge(const cv::Mat& face_image);

    /**
     * @brief 识别性别
     * @param face_image 人脸图像
     * @return 0=男，1=女
     */
    int recognizeGender(const cv::Mat& face_image);

    /**
     * @brief 人脸识别
     * @param face_features 人脸特征
     * @return 识别的人员ID，-1表示未识别
     */
    int recognizePerson(const std::vector<float>& face_features);

    /**
     * @brief 添加已知人员
     * @param person_id 人员ID
     * @param face_features 人脸特征
     * @param name 人员姓名
     */
    void addKnownPerson(int person_id, const std::vector<float>& face_features, const std::string& name);

    /**
     * @brief 检查是否已初始化
     * @return true已初始化，false未初始化
     */
    bool isInitialized() const { return initialized_; }

    /**
     * @brief 🔧 Phase 2: 简化的人脸分析接口（用于JNI调用）
     * @param image 输入图像
     * @param personDetections 人员检测结果
     * @param result 输出人脸分析结果
     * @return true成功，false失败
     */
    bool analyzeFaces(const cv::Mat& image,
                     const std::vector<PersonDetection>& personDetections,
                     SimpleFaceAnalysisResult& result);

    /**
     * @brief 🔧 Phase 2: 测试InspireFace集成功能
     * @return true成功，false失败
     */
    bool testInspireFaceIntegration();

private:
    bool initialized_;
    
    // OpenCV人脸检测器
    cv::CascadeClassifier face_cascade_;
    
    // 人脸特征数据库
    struct KnownPerson {
        int id;
        std::string name;
        std::vector<float> features;
    };
    std::vector<KnownPerson> known_persons_;

    // 🔧 Phase 2: InspireFace组件（使用真实InspireFace）
    std::unique_ptr<InspireFaceSession> inspireface_session_;
    std::unique_ptr<InspireFaceImageProcessor> image_processor_;
    std::unique_ptr<InspireFaceDetector> face_detector_;
    bool inspireface_initialized_;

    // 🔧 Phase 2: 保存InspireFace初始化参数（用于延迟初始化）
    AAssetManager* m_assetManager;
    std::string m_internalDataPath;

    /**
     * @brief 计算特征向量相似度
     * @param features1 特征向量1
     * @param features2 特征向量2
     * @return 相似度分数
     */
    float calculateSimilarity(const std::vector<float>& features1, const std::vector<float>& features2);
    
    /**
     * @brief 预处理人脸图像
     * @param face_image 原始人脸图像
     * @return 预处理后的图像
     */
    cv::Mat preprocessFaceImage(const cv::Mat& face_image);

    /**
     * @brief 🔧 Phase 2: 设置人脸分析配置
     * @param config 配置参数
     */
    void setConfig(const FaceAnalysisConfig& config);

    /**
     * @brief 🔧 Phase 2: 获取人脸分析配置
     * @return 当前配置参数
     */
    FaceAnalysisConfig getConfig() const;

    /**
     * @brief 🔧 Phase 2: 清理InspireFace组件
     */
    void cleanupInspireFaceComponents();
};

#endif // FACE_ANALYSIS_MANAGER_H
