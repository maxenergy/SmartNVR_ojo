#ifndef FACE_ANALYSIS_MANAGER_H
#define FACE_ANALYSIS_MANAGER_H

#include <opencv2/opencv.hpp>
#include <vector>
#include <memory>
#include "person_detection_types.h"

/**
 * @brief 人脸分析管理器
 * 负责人脸检测、特征提取和身份识别
 */
class FaceAnalysisManager {
public:
    FaceAnalysisManager();
    ~FaceAnalysisManager();

    /**
     * @brief 初始化人脸分析管理器
     * @return 0成功，-1失败
     */
    int initialize();

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
};

#endif // FACE_ANALYSIS_MANAGER_H
