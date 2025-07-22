# Phase 1 优化计划 (1-2周开发周期)

## 🎯 优化目标

基于当前稳定的基础实现，Phase 1 优化旨在完善核心功能，提升用户体验，为后续高级功能奠定更坚实的基础。

## 📋 优化任务清单

### 🔧 高优先级任务 (第1周)

#### 1. 完善JNI接口实现
**任务描述**: 实现完整的getCurrentStatistics() JNI方法，替换当前的简化实现
**预估工时**: 2-3天
**技术要点**:
```cpp
// 在native-lib.cpp中添加JNI方法
extern "C" JNIEXPORT jobject JNICALL
Java_com_wulala_myyolov5rtspthreadpool_DirectInspireFaceTest_getCurrentStatistics(
    JNIEnv *env, jclass clazz) {
    
    // 创建BatchStatisticsResult对象
    jclass resultClass = env->FindClass("com/wulala/myyolov5rtspthreadpool/BatchStatisticsResult");
    jobject result = env->NewObject(resultClass, 
                                   env->GetMethodID(resultClass, "<init>", "()V"));
    
    // 从各个摄像头收集统计数据
    // 设置字段值
    // 返回结果对象
}
```

**验收标准**:
- [ ] JNI方法正常调用，无崩溃
- [ ] 统计数据正确传递到Java层
- [ ] UI界面显示实时统计信息

#### 2. 增强人员跟踪算法
**任务描述**: 实现基于IoU的多目标跟踪，支持人员ID分配和生命周期管理
**预估工时**: 3-4天
**技术要点**:
```cpp
class PersonTracker {
private:
    struct TrackedPerson {
        int id;
        cv::Rect lastBox;
        std::chrono::steady_clock::time_point lastSeen;
        int consecutiveMisses;
        bool isActive;
    };
    
    std::vector<TrackedPerson> trackedPersons;
    int nextPersonId = 1;
    
public:
    std::vector<Detection> updateTracking(const std::vector<Detection>& detections);
    float calculateIoU(const cv::Rect& box1, const cv::Rect& box2);
    void cleanupInactivePersons();
};
```

**验收标准**:
- [ ] 人员ID在多帧间保持一致
- [ ] 支持人员进入/离开场景
- [ ] 跟踪精度>85%（手动验证）

#### 3. 优化统计数据结构
**任务描述**: 扩展统计功能，添加区域统计和时间序列数据
**预估工时**: 2天
**技术要点**:
```cpp
struct EnhancedPersonStatistics {
    // 基础统计
    int camera_id;
    int current_person_count;
    int total_person_count;
    
    // 区域统计
    int enter_count;
    int exit_count;
    
    // 时间序列
    std::vector<int> hourly_counts;
    std::chrono::steady_clock::time_point last_reset;
    
    // 性能指标
    double avg_detection_time;
    double avg_tracking_time;
};
```

**验收标准**:
- [ ] 支持进入/离开计数
- [ ] 时间序列数据正确记录
- [ ] 性能指标准确计算

### 🔧 中优先级任务 (第2周)

#### 4. 添加配置界面
**任务描述**: 实现运行时参数调整界面，支持检测阈值、跟踪参数等配置
**预估工时**: 2-3天
**技术要点**:
```java
public class SettingsActivity extends AppCompatActivity {
    // 检测参数配置
    private SeekBar confidenceThresholdSeekBar;
    private SeekBar movementThresholdSeekBar;
    
    // 统计参数配置
    private SeekBar logIntervalSeekBar;
    private SeekBar cleanupIntervalSeekBar;
    
    // 保存配置到SharedPreferences
    // 通过JNI传递配置到C++层
}
```

**验收标准**:
- [ ] 配置界面友好易用
- [ ] 参数实时生效
- [ ] 配置持久化保存

#### 5. 实现数据导出功能
**任务描述**: 支持统计数据导出为CSV/JSON格式，便于分析
**预估工时**: 1-2天
**技术要点**:
```java
public class DataExporter {
    public void exportToCSV(List<PersonStatistics> data, String filePath);
    public void exportToJSON(List<PersonStatistics> data, String filePath);
    public void scheduleAutoExport(int intervalHours);
}
```

**验收标准**:
- [ ] 支持CSV和JSON格式导出
- [ ] 支持定时自动导出
- [ ] 导出文件格式正确

#### 6. 优化性能监控
**任务描述**: 添加详细的性能指标收集和监控界面
**预估工时**: 2天
**技术要点**:
```cpp
class PerformanceMonitor {
private:
    struct PerformanceMetrics {
        double avg_detection_time;
        double avg_tracking_time;
        double memory_usage_mb;
        double cpu_usage_percent;
        int frames_processed;
        int frames_skipped;
    };
    
public:
    void recordDetectionTime(double time_ms);
    void recordTrackingTime(double time_ms);
    PerformanceMetrics getCurrentMetrics();
};
```

**验收标准**:
- [ ] 实时性能指标显示
- [ ] 性能历史趋势图
- [ ] 性能警告和建议

### 🔧 低优先级任务 (时间允许时)

#### 7. 添加区域设置功能
**任务描述**: 支持用户自定义检测区域，提高检测精度
**预估工时**: 2-3天

#### 8. 实现简单的人脸检测
**任务描述**: 基于OpenCV实现基础人脸检测，为后续人脸识别做准备
**预估工时**: 3-4天

#### 9. 优化内存管理
**任务描述**: 实现更智能的内存管理策略，减少内存峰值
**预估工时**: 2天

## 🛠️ 技术实现细节

### JNI接口完善
```cpp
// 统计数据收集器
class StatisticsCollector {
private:
    std::mutex stats_mutex;
    std::map<int, EnhancedPersonStatistics> camera_stats;
    
public:
    void updateCameraStats(int camera_id, const EnhancedPersonStatistics& stats);
    BatchStatisticsResult collectAllStats();
    void resetStats();
};

// 全局统计收集器实例
static StatisticsCollector g_stats_collector;

// JNI实现
extern "C" JNIEXPORT jobject JNICALL
Java_com_wulala_myyolov5rtspthreadpool_DirectInspireFaceTest_getCurrentStatistics(
    JNIEnv *env, jclass clazz) {
    
    BatchStatisticsResult stats = g_stats_collector.collectAllStats();
    
    // 创建Java对象并设置字段
    jclass resultClass = env->FindClass("com/wulala/myyolov5rtspthreadpool/BatchStatisticsResult");
    jobject result = env->NewObject(resultClass, env->GetMethodID(resultClass, "<init>", "()V"));
    
    // 设置基础字段
    env->SetBooleanField(result, env->GetFieldID(resultClass, "success", "Z"), stats.success);
    env->SetIntField(result, env->GetFieldID(resultClass, "personCount", "I"), stats.personCount);
    env->SetIntField(result, env->GetFieldID(resultClass, "totalFaceCount", "I"), stats.totalFaceCount);
    
    return result;
}
```

### 增强跟踪算法
```cpp
std::vector<Detection> PersonTracker::updateTracking(const std::vector<Detection>& detections) {
    std::vector<Detection> trackedDetections;
    
    // 1. 计算IoU矩阵
    std::vector<std::vector<float>> iouMatrix(trackedPersons.size(), 
                                             std::vector<float>(detections.size(), 0.0f));
    
    for (size_t i = 0; i < trackedPersons.size(); i++) {
        for (size_t j = 0; j < detections.size(); j++) {
            iouMatrix[i][j] = calculateIoU(trackedPersons[i].lastBox, detections[j].box);
        }
    }
    
    // 2. 匈牙利算法匹配（简化版本）
    std::vector<int> assignments = hungarianAssignment(iouMatrix);
    
    // 3. 更新已匹配的跟踪目标
    for (size_t i = 0; i < assignments.size(); i++) {
        if (assignments[i] >= 0) {
            trackedPersons[i].lastBox = detections[assignments[i]].box;
            trackedPersons[i].lastSeen = std::chrono::steady_clock::now();
            trackedPersons[i].consecutiveMisses = 0;
            
            Detection trackedDetection = detections[assignments[i]];
            trackedDetection.class_id = trackedPersons[i].id;  // 设置跟踪ID
            trackedDetections.push_back(trackedDetection);
        } else {
            trackedPersons[i].consecutiveMisses++;
        }
    }
    
    // 4. 为未匹配的检测创建新跟踪目标
    for (size_t j = 0; j < detections.size(); j++) {
        bool matched = false;
        for (int assignment : assignments) {
            if (assignment == static_cast<int>(j)) {
                matched = true;
                break;
            }
        }
        
        if (!matched) {
            TrackedPerson newPerson;
            newPerson.id = nextPersonId++;
            newPerson.lastBox = detections[j].box;
            newPerson.lastSeen = std::chrono::steady_clock::now();
            newPerson.consecutiveMisses = 0;
            newPerson.isActive = true;
            
            trackedPersons.push_back(newPerson);
            
            Detection trackedDetection = detections[j];
            trackedDetection.class_id = newPerson.id;
            trackedDetections.push_back(trackedDetection);
        }
    }
    
    // 5. 清理不活跃的跟踪目标
    cleanupInactivePersons();
    
    return trackedDetections;
}

float PersonTracker::calculateIoU(const cv::Rect& box1, const cv::Rect& box2) {
    cv::Rect intersection = box1 & box2;
    float intersectionArea = intersection.area();
    float unionArea = box1.area() + box2.area() - intersectionArea;
    
    return unionArea > 0 ? intersectionArea / unionArea : 0.0f;
}
```

### 配置管理系统
```cpp
class ConfigManager {
private:
    struct DetectionConfig {
        float confidence_threshold = 0.5f;
        float movement_threshold = 10.0f;
        int log_interval = 50;
        int cleanup_interval = 200;
        bool enable_tracking = true;
        bool enable_face_detection = false;
    };
    
    DetectionConfig config;
    std::mutex config_mutex;
    
public:
    void updateConfig(const DetectionConfig& new_config);
    DetectionConfig getConfig() const;
    void saveConfigToFile(const std::string& filepath);
    void loadConfigFromFile(const std::string& filepath);
};

// JNI配置接口
extern "C" JNIEXPORT void JNICALL
Java_com_wulala_myyolov5rtspthreadpool_SettingsActivity_updateDetectionConfig(
    JNIEnv *env, jclass clazz, jfloat confidence, jfloat movement, jint logInterval) {
    
    DetectionConfig config;
    config.confidence_threshold = confidence;
    config.movement_threshold = movement;
    config.log_interval = logInterval;
    
    g_config_manager.updateConfig(config);
}
```

## 📊 开发计划时间表

### 第1周 (5个工作日)
```
周一: JNI接口设计和基础实现
周二: JNI接口完善和测试
周三: 跟踪算法设计和IoU计算实现
周四: 跟踪算法完善和匹配逻辑
周五: 统计数据结构扩展和测试
```

### 第2周 (5个工作日)
```
周一: 配置界面设计和布局
周二: 配置界面功能实现
周三: 数据导出功能实现
周四: 性能监控系统实现
周五: 集成测试和文档更新
```

## 🎯 验收标准

### 功能验收
- [ ] 所有高优先级任务100%完成
- [ ] 中优先级任务至少80%完成
- [ ] 新功能通过单元测试和集成测试
- [ ] 性能指标不低于当前版本

### 质量验收
- [ ] 代码覆盖率>80%
- [ ] 无内存泄漏
- [ ] 无崩溃和ANR
- [ ] 用户界面友好易用

### 性能验收
- [ ] 检测延迟<100ms
- [ ] 跟踪精度>85%
- [ ] 内存使用增加<20%
- [ ] CPU使用增加<10%

## 🚀 Phase 2 预览

Phase 1完成后，Phase 2将重点关注：
1. **InspireFace集成**: 解决链接问题，实现真正的人脸识别
2. **高级分析功能**: 年龄、性别、情绪识别
3. **云端集成**: 数据同步和远程监控
4. **AI能力增强**: 行为识别和异常检测

---

*Phase 1优化计划版本: v1.0*  
*计划制定时间: 2025-07-22*  
*预计完成时间: 2025-08-05*
