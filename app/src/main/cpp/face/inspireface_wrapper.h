#ifndef INSPIREFACE_WRAPPER_H
#define INSPIREFACE_WRAPPER_H

#include <opencv2/opencv.hpp>
#include <string>
#include <vector>

// InspireFace C API包装器
// 为了简化集成，我们创建一个C++包装器来封装InspireFace的C API

/**
 * @brief InspireFace会话管理器
 * 
 * 封装InspireFace的会话创建、配置和销毁
 */
class InspireFaceSession {
public:
    InspireFaceSession();
    ~InspireFaceSession();
    
    /**
     * 初始化InspireFace会话
     * @param modelPath 模型文件路径
     * @param enableFaceAttribute 是否启用人脸属性分析
     * @return 是否成功
     */
    bool initialize(const std::string& modelPath, bool enableFaceAttribute = true);
    
    /**
     * 释放会话资源
     */
    void release();
    
    /**
     * 检查会话是否已初始化
     */
    bool isInitialized() const { return m_initialized; }
    
    /**
     * 获取原始会话句柄（用于直接调用InspireFace API）
     */
    void* getSessionHandle() const { return m_session; }

private:
    void* m_session;        // HFSession句柄
    bool m_initialized;
    std::string m_modelPath;
};

/**
 * @brief 人脸检测结果
 */
struct FaceDetectionResult {
    cv::Rect faceRect;          // 人脸边界框
    float confidence;           // 检测置信度
    int trackId;               // 跟踪ID
    void* faceToken;           // InspireFace的人脸token（HFFaceBasicToken）
    
    FaceDetectionResult() 
        : confidence(0.0f), trackId(-1), faceToken(nullptr) {}
};

/**
 * @brief 人脸属性结果
 */
struct FaceAttributeResult {
    int gender;                // 性别：0=女性，1=男性
    float genderConfidence;    // 性别置信度
    int ageBracket;           // 年龄段：0-8对应不同年龄段
    float ageConfidence;      // 年龄置信度
    int race;                 // 种族：0-4对应不同种族
    float raceConfidence;     // 种族置信度
    
    FaceAttributeResult() 
        : gender(-1), genderConfidence(0.0f)
        , ageBracket(-1), ageConfidence(0.0f)
        , race(-1), raceConfidence(0.0f) {}
        
    bool isValid() const {
        return gender >= 0 && ageBracket >= 0;
    }
    
    std::string getGenderString() const {
        if (gender == 0) return "女性";
        else if (gender == 1) return "男性";
        else return "未知";
    }
    
    std::string getAgeBracketString() const {
        const char* ageLabels[] = {
            "0-2岁", "3-9岁", "10-19岁", "20-29岁", "30-39岁",
            "40-49岁", "50-59岁", "60-69岁", "70岁以上"
        };
        if (ageBracket >= 0 && ageBracket < 9) {
            return ageLabels[ageBracket];
        }
        return "未知";
    }
    
    std::string getRaceString() const {
        const char* raceLabels[] = {
            "黑人", "亚洲人", "拉丁裔", "白人", "其他"
        };
        if (race >= 0 && race < 5) {
            return raceLabels[race];
        }
        return "未知";
    }
};

/**
 * @brief InspireFace图像处理器
 * 
 * 封装图像格式转换和处理功能
 */
class InspireFaceImageProcessor {
public:
    InspireFaceImageProcessor();
    ~InspireFaceImageProcessor();
    
    /**
     * 从OpenCV Mat创建InspireFace图像流
     * @param image OpenCV图像
     * @param imageStream 输出的图像流句柄
     * @return 是否成功
     */
    bool createImageStreamFromMat(const cv::Mat& image, void** imageStream);
    
    /**
     * 释放图像流
     * @param imageStream 图像流句柄
     */
    void releaseImageStream(void* imageStream);
    
    /**
     * 从ROI区域创建图像流
     * @param image 原始图像
     * @param roi ROI区域
     * @param imageStream 输出的图像流句柄
     * @return 是否成功
     */
    bool createImageStreamFromROI(const cv::Mat& image, const cv::Rect& roi, void** imageStream);

private:
    void* m_imageBitmap;    // 临时图像位图句柄
};

/**
 * @brief InspireFace检测器
 * 
 * 封装人脸检测和属性分析功能
 */
class InspireFaceDetector {
public:
    InspireFaceDetector();
    ~InspireFaceDetector();
    
    /**
     * 初始化检测器
     * @param session InspireFace会话
     * @return 是否成功
     */
    bool initialize(InspireFaceSession* session);
    
    /**
     * 执行人脸检测
     * @param imageStream 图像流句柄
     * @param results 检测结果
     * @return 是否成功
     */
    bool detectFaces(void* imageStream, std::vector<FaceDetectionResult>& results);
    
    /**
     * 分析人脸属性
     * @param imageStream 图像流句柄
     * @param faceResults 人脸检测结果
     * @param attributeResults 属性分析结果
     * @return 是否成功
     */
    bool analyzeFaceAttributes(void* imageStream, 
                              const std::vector<FaceDetectionResult>& faceResults,
                              std::vector<FaceAttributeResult>& attributeResults);
    
    /**
     * 一步完成人脸检测和属性分析
     * @param imageStream 图像流句柄
     * @param faceResults 人脸检测结果
     * @param attributeResults 属性分析结果
     * @return 是否成功
     */
    bool detectAndAnalyze(void* imageStream,
                         std::vector<FaceDetectionResult>& faceResults,
                         std::vector<FaceAttributeResult>& attributeResults);

private:
    InspireFaceSession* m_session;
    bool m_initialized;
    
    // 内部辅助方法
    bool convertMultipleFaceData(void* multipleFaceData, std::vector<FaceDetectionResult>& results);
    bool convertAttributeResults(void* attributeData, std::vector<FaceAttributeResult>& results);
};

/**
 * @brief InspireFace工具函数
 */
namespace InspireFaceUtils {
    
    /**
     * 初始化InspireFace库
     * @return 是否成功
     */
    bool initializeLibrary();
    
    /**
     * 释放InspireFace库
     */
    void releaseLibrary();
    
    /**
     * 检查模型文件是否存在
     * @param modelPath 模型路径
     * @return 是否存在
     */
    bool checkModelFiles(const std::string& modelPath);
    
    /**
     * 获取InspireFace版本信息
     * @return 版本字符串
     */
    std::string getVersion();
    
    /**
     * 设置日志级别
     * @param level 日志级别
     */
    void setLogLevel(int level);
    
    /**
     * 转换OpenCV Rect到InspireFace Rect
     */
    void convertRect(const cv::Rect& cvRect, void* hfRect);
    
    /**
     * 转换InspireFace Rect到OpenCV Rect
     */
    cv::Rect convertRect(const void* hfRect);
    
    /**
     * 检查InspireFace结果码
     * @param result 结果码
     * @param operation 操作描述
     * @return 是否成功
     */
    bool checkResult(long result, const std::string& operation);
}

// 错误码定义
#define ISF_SUCCESS 0
#define ISF_ERROR_INVALID_PARAM -1
#define ISF_ERROR_INIT_FAILED -2
#define ISF_ERROR_NOT_INITIALIZED -3
#define ISF_ERROR_DETECTION_FAILED -4
#define ISF_ERROR_ATTRIBUTE_FAILED -5
#define ISF_ERROR_IMAGE_PROCESSING_FAILED -6

#endif // INSPIREFACE_WRAPPER_H
