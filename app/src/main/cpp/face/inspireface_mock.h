/**
 * @file inspireface_mock.h
 * @brief InspireFace模拟实现 - 用于兼容性问题解决前的临时方案
 * @author AI Assistant
 * @date 2025-07-22
 */

#ifndef INSPIREFACE_MOCK_H
#define INSPIREFACE_MOCK_H

#include <opencv2/opencv.hpp>
#include <string>
#include <vector>
#include <memory>
#include <android/asset_manager.h>

/**
 * @brief InspireFace模拟会话管理器
 * 
 * 提供与真实InspireFace相同的API，但使用简化的算法实现
 */
class MockInspireFaceSession {
public:
    MockInspireFaceSession();
    ~MockInspireFaceSession();

    /**
     * 初始化模拟会话
     * @param assetManager Android资产管理器
     * @param internalDataPath 应用内部数据路径
     * @param enableFaceAttribute 是否启用人脸属性分析
     * @return 是否成功
     */
    bool initialize(AAssetManager* assetManager, const std::string& internalDataPath,
                   bool enableFaceAttribute = true);
    
    /**
     * 释放会话资源
     */
    void release();
    
    /**
     * 检查会话是否已初始化
     */
    bool isInitialized() const { return m_initialized; }
    
    /**
     * 获取原始会话句柄（模拟）
     */
    void* getSessionHandle() const { return (void*)this; }

private:
    bool m_initialized;
    std::string m_dataPath;
    cv::CascadeClassifier m_faceCascade; // 使用OpenCV的人脸检测器
};

/**
 * @brief InspireFace模拟图像处理器
 */
class MockInspireFaceImageProcessor {
public:
    MockInspireFaceImageProcessor();
    ~MockInspireFaceImageProcessor();

    /**
     * 从Mat创建图像流
     */
    bool createImageStreamFromMat(const cv::Mat& image, void** imageStream);
    
    /**
     * 释放图像流
     */
    void releaseImageStream(void* imageStream);

private:
    // 简单的图像数据包装器
    struct ImageStreamData {
        cv::Mat image;
        bool valid;
    };
};

/**
 * @brief 模拟人脸检测结果
 */
struct MockFaceDetectionResult {
    cv::Rect faceRect;
    float confidence;
    int faceId;
};

/**
 * @brief 模拟人脸属性结果
 */
struct MockFaceAttributeResult {
    int gender;      // 0: 男性, 1: 女性
    int ageBracket;  // 年龄段 0-8
    float confidence;
};

/**
 * @brief InspireFace模拟检测器
 */
class MockInspireFaceDetector {
public:
    MockInspireFaceDetector();
    ~MockInspireFaceDetector();

    /**
     * 初始化检测器
     */
    bool initialize(MockInspireFaceSession* session);
    
    /**
     * 检测和分析人脸
     */
    bool detectAndAnalyze(void* imageStream, 
                         std::vector<MockFaceDetectionResult>& faceResults,
                         std::vector<MockFaceAttributeResult>& attributeResults);

private:
    MockInspireFaceSession* m_session;
    bool m_initialized;
    cv::CascadeClassifier m_faceCascade;
    
    // 简化的年龄性别估计
    int estimateGender(const cv::Mat& faceImage);
    int estimateAge(const cv::Mat& faceImage);
};

/**
 * @brief InspireFace模拟工具函数
 */
namespace MockInspireFaceUtils {
    /**
     * 初始化模拟库
     */
    bool initializeLibrary();
    
    /**
     * 释放模拟库
     */
    void releaseLibrary();
}

#endif // INSPIREFACE_MOCK_H
