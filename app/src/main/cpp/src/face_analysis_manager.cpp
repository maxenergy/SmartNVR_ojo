#include "../include/face_analysis_manager.h"
#include <android/log.h>
#include <algorithm>
#include <cmath>

#define TAG "FaceAnalysisManager"
#define LOGD(...) __android_log_print(ANDROID_LOG_DEBUG, TAG, __VA_ARGS__)
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR, TAG, __VA_ARGS__)
#define LOGW(...) __android_log_print(ANDROID_LOG_WARN, TAG, __VA_ARGS__)

FaceAnalysisManager::FaceAnalysisManager() : initialized_(false) {
    LOGD("FaceAnalysisManager constructor");
}

FaceAnalysisManager::~FaceAnalysisManager() {
    release();
    LOGD("FaceAnalysisManager destructor");
}

int FaceAnalysisManager::initialize() {
    try {
        // 加载OpenCV人脸检测器
        // 注意：在实际应用中，需要将haarcascade_frontalface_alt.xml文件放到assets目录
        // 这里使用简化的初始化
        
        // TODO: 加载实际的人脸检测模型
        // std::string cascade_path = "/path/to/haarcascade_frontalface_alt.xml";
        // if (!face_cascade_.load(cascade_path)) {
        //     LOGE("Failed to load face cascade classifier");
        //     return -1;
        // }
        
        // 清空已知人员数据库
        known_persons_.clear();
        
        initialized_ = true;
        LOGD("Face analysis manager initialized successfully");
        return 0;
        
    } catch (const std::exception& e) {
        LOGE("Failed to initialize face analysis manager: %s", e.what());
        return -1;
    }
}

void FaceAnalysisManager::release() {
    if (initialized_) {
        known_persons_.clear();
        initialized_ = false;
        LOGD("Face analysis manager released");
    }
}

FaceAnalysisResult FaceAnalysisManager::analyzeFace(const cv::Mat& image) {
    FaceAnalysisResult result;
    result.face_detected = false;
    result.confidence = 0.0f;
    result.age = 0;
    result.gender = 0;
    result.person_id = -1;
    
    if (!initialized_ || image.empty()) {
        return result;
    }
    
    try {
        // 检测人脸
        std::vector<cv::Rect> faces;
        int face_count = detectFaces(image, faces);
        
        if (face_count > 0) {
            // 使用第一个检测到的人脸
            cv::Rect face_rect = faces[0];
            result.face_detected = true;
            result.confidence = 0.8f; // 简化的置信度
            
            // 设置人脸边界框
            result.face_box.x = face_rect.x;
            result.face_box.y = face_rect.y;
            result.face_box.width = face_rect.width;
            result.face_box.height = face_rect.height;
            
            // 提取人脸区域
            cv::Mat face_image = image(face_rect);
            
            // 估计年龄和性别
            result.age = estimateAge(face_image);
            result.gender = recognizeGender(face_image);
            
            // 提取人脸特征
            result.face_features = extractFaceFeatures(face_image);
            
            // 人脸识别
            result.person_id = recognizePerson(result.face_features);
            
            LOGD("Face analyzed: age=%d, gender=%s, person_id=%d", 
                 result.age, result.gender == 0 ? "male" : "female", result.person_id);
        }
        
    } catch (const std::exception& e) {
        LOGE("Face analysis exception: %s", e.what());
    }
    
    return result;
}

int FaceAnalysisManager::detectFaces(const cv::Mat& image, std::vector<cv::Rect>& faces) {
    faces.clear();
    
    if (!initialized_ || image.empty()) {
        return 0;
    }
    
    try {
        // 简化的人脸检测实现
        // 在实际应用中，这里应该使用训练好的人脸检测模型
        
        // 转换为灰度图
        cv::Mat gray;
        if (image.channels() == 3) {
            cv::cvtColor(image, gray, cv::COLOR_BGR2GRAY);
        } else {
            gray = image.clone();
        }
        
        // TODO: 使用实际的人脸检测算法
        // face_cascade_.detectMultiScale(gray, faces, 1.1, 3, 0, cv::Size(30, 30));
        
        // 简化实现：假设在图像中心区域有一个人脸
        if (image.cols > 100 && image.rows > 100) {
            int face_size = std::min(image.cols, image.rows) / 3;
            int x = (image.cols - face_size) / 2;
            int y = (image.rows - face_size) / 2;
            faces.push_back(cv::Rect(x, y, face_size, face_size));
        }
        
        return faces.size();
        
    } catch (const std::exception& e) {
        LOGE("Face detection exception: %s", e.what());
        return 0;
    }
}

std::vector<float> FaceAnalysisManager::extractFaceFeatures(const cv::Mat& face_image) {
    std::vector<float> features;
    
    if (!initialized_ || face_image.empty()) {
        return features;
    }
    
    try {
        // 简化的特征提取实现
        // 在实际应用中，这里应该使用深度学习模型提取人脸特征
        
        cv::Mat processed = preprocessFaceImage(face_image);
        
        // 简化实现：使用图像的统计特征作为人脸特征
        cv::Scalar mean, stddev;
        cv::meanStdDev(processed, mean, stddev);
        
        features.resize(128); // 假设特征向量长度为128
        for (int i = 0; i < 128; i++) {
            features[i] = static_cast<float>(mean[0] + stddev[0] * sin(i * 0.1));
        }
        
        LOGD("Extracted face features: %zu dimensions", features.size());
        
    } catch (const std::exception& e) {
        LOGE("Feature extraction exception: %s", e.what());
    }
    
    return features;
}

int FaceAnalysisManager::estimateAge(const cv::Mat& face_image) {
    if (!initialized_ || face_image.empty()) {
        return 25; // 默认年龄
    }
    
    try {
        // 简化的年龄估计实现
        // 在实际应用中，这里应该使用训练好的年龄估计模型
        
        cv::Mat processed = preprocessFaceImage(face_image);
        cv::Scalar mean = cv::mean(processed);
        
        // 简化算法：基于图像亮度估计年龄
        int estimated_age = static_cast<int>(20 + mean[0] / 5);
        estimated_age = std::max(1, std::min(100, estimated_age));
        
        return estimated_age;
        
    } catch (const std::exception& e) {
        LOGE("Age estimation exception: %s", e.what());
        return 25;
    }
}

int FaceAnalysisManager::recognizeGender(const cv::Mat& face_image) {
    if (!initialized_ || face_image.empty()) {
        return 0; // 默认男性
    }
    
    try {
        // 简化的性别识别实现
        // 在实际应用中，这里应该使用训练好的性别识别模型
        
        cv::Mat processed = preprocessFaceImage(face_image);
        cv::Scalar mean = cv::mean(processed);
        
        // 简化算法：基于图像特征判断性别
        return (static_cast<int>(mean[0]) % 2);
        
    } catch (const std::exception& e) {
        LOGE("Gender recognition exception: %s", e.what());
        return 0;
    }
}

int FaceAnalysisManager::recognizePerson(const std::vector<float>& face_features) {
    if (!initialized_ || face_features.empty() || known_persons_.empty()) {
        return -1; // 未识别
    }
    
    try {
        float best_similarity = 0.0f;
        int best_person_id = -1;
        const float threshold = 0.7f; // 相似度阈值
        
        // 与已知人员进行匹配
        for (const auto& person : known_persons_) {
            float similarity = calculateSimilarity(face_features, person.features);
            if (similarity > best_similarity && similarity > threshold) {
                best_similarity = similarity;
                best_person_id = person.id;
            }
        }
        
        if (best_person_id != -1) {
            LOGD("Person recognized: ID=%d, similarity=%.2f", best_person_id, best_similarity);
        }
        
        return best_person_id;
        
    } catch (const std::exception& e) {
        LOGE("Person recognition exception: %s", e.what());
        return -1;
    }
}

void FaceAnalysisManager::addKnownPerson(int person_id, const std::vector<float>& face_features, const std::string& name) {
    if (!initialized_ || face_features.empty()) {
        return;
    }
    
    try {
        KnownPerson person;
        person.id = person_id;
        person.name = name;
        person.features = face_features;
        
        known_persons_.push_back(person);
        
        LOGD("Added known person: ID=%d, name=%s", person_id, name.c_str());
        
    } catch (const std::exception& e) {
        LOGE("Add known person exception: %s", e.what());
    }
}

float FaceAnalysisManager::calculateSimilarity(const std::vector<float>& features1, const std::vector<float>& features2) {
    if (features1.size() != features2.size() || features1.empty()) {
        return 0.0f;
    }
    
    try {
        // 计算余弦相似度
        float dot_product = 0.0f;
        float norm1 = 0.0f;
        float norm2 = 0.0f;
        
        for (size_t i = 0; i < features1.size(); i++) {
            dot_product += features1[i] * features2[i];
            norm1 += features1[i] * features1[i];
            norm2 += features2[i] * features2[i];
        }
        
        if (norm1 == 0.0f || norm2 == 0.0f) {
            return 0.0f;
        }
        
        return dot_product / (sqrt(norm1) * sqrt(norm2));
        
    } catch (const std::exception& e) {
        LOGE("Similarity calculation exception: %s", e.what());
        return 0.0f;
    }
}

cv::Mat FaceAnalysisManager::preprocessFaceImage(const cv::Mat& face_image) {
    cv::Mat processed;
    
    try {
        // 调整大小到标准尺寸
        cv::resize(face_image, processed, cv::Size(112, 112));
        
        // 转换为灰度图
        if (processed.channels() == 3) {
            cv::cvtColor(processed, processed, cv::COLOR_BGR2GRAY);
        }
        
        // 直方图均衡化
        cv::equalizeHist(processed, processed);
        
        // 归一化
        processed.convertTo(processed, CV_32F, 1.0/255.0);
        
    } catch (const std::exception& e) {
        LOGE("Face image preprocessing exception: %s", e.what());
        processed = face_image.clone();
    }
    
    return processed;
}
