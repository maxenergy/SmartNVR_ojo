#ifndef PERSON_DETECTION_TYPES_H
#define PERSON_DETECTION_TYPES_H

#include <chrono>
#include <vector>
#include <opencv2/opencv.hpp>

// 🔧 定义简单的边界框结构
struct BoundingBox {
    int x;
    int y;
    int width;
    int height;

    BoundingBox() : x(0), y(0), width(0), height(0) {}
    BoundingBox(int x_, int y_, int w_, int h_) : x(x_), y(y_), width(w_), height(h_) {}

    // 从cv::Rect转换
    BoundingBox(const cv::Rect& rect) : x(rect.x), y(rect.y), width(rect.width), height(rect.height) {}

    // 转换为cv::Rect
    cv::Rect toRect() const { return cv::Rect(x, y, width, height); }
};

// 🔧 人员统计数据结构
struct PersonStatistics {
    int camera_id;
    int person_count;
    int face_count;
    int male_count;
    int female_count;
    int age_group_0_18;
    int age_group_19_35;
    int age_group_36_60;
    int age_group_60_plus;
    std::chrono::steady_clock::time_point timestamp;
};

// 🔧 人脸分析结果结构
struct FaceAnalysisResult {
    bool face_detected;
    float confidence;
    BoundingBox face_box;
    int age;
    int gender; // 0=男, 1=女
    std::vector<float> face_features; // 人脸特征向量
    int person_id; // 关联的人员ID
};

// 🔧 人员检测数据传输结构（用于JNI）
#define MAX_DETECTIONS 50
#define MAX_FACES 50

struct PersonDetectionData {
    int camera_id;
    int person_count;
    int face_count;
    long long timestamp;
    
    // 人员检测框数据
    BoundingBox person_boxes[MAX_DETECTIONS];
    float person_confidences[MAX_DETECTIONS];
    
    // 人脸数据
    BoundingBox face_boxes[MAX_FACES];
    float face_confidences[MAX_FACES];
    int ages[MAX_FACES];
    int genders[MAX_FACES];
};

// 🔧 人员跟踪数据结构
struct TrackedPerson {
    int track_id;
    BoundingBox last_box;
    std::chrono::steady_clock::time_point last_seen;
    int consecutive_misses;
    bool is_active;
};

// 🔧 区域进入/离开统计
struct AreaStatistics {
    int camera_id;
    int enter_count;
    int exit_count;
    int current_count;
    std::chrono::steady_clock::time_point last_update;
};

#endif // PERSON_DETECTION_TYPES_H
