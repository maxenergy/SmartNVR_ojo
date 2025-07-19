# InspireFace集成架构设计方案

## 🎯 **项目目标**

将InspireFace SDK集成到现有YOLOv5/YOLOv8n目标检测系统中，实现：
- 目标检测 → 人员筛选 → 人脸识别 → 属性分析的级联流程
- 实时统计人数、性别分布、年龄分布
- 保持现有功能完全不变，模块化扩展

## 🏗️ **系统架构图**

```
┌─────────────────────────────────────────────────────────────────┐
│                        应用层 (Java)                              │
├─────────────────────────────────────────────────────────────────┤
│  MainActivity                                                   │
│  ├── 目标检测控制                                                 │
│  ├── 人脸识别控制                                                 │
│  └── 统计数据显示                                                 │
├─────────────────────────────────────────────────────────────────┤
│                      JNI接口层                                   │
├─────────────────────────────────────────────────────────────────┤
│                      C++核心层                                   │
│  ┌─────────────────┐  ┌─────────────────┐  ┌─────────────────┐   │
│  │  统一推理管理器   │  │  人脸分析管理器   │  │  统计数据管理器   │   │
│  │ InferenceManager│  │ FaceAnalysisManager│ │ StatisticsManager│   │
│  │                 │  │                 │  │                 │   │
│  │ ├─YOLOv5Engine  │  │ ├─InspireFace   │  │ ├─人数统计       │   │
│  │ ├─YOLOv8Engine  │  │ │  Integration   │  │ ├─性别统计       │   │
│  │ └─级联控制器     │  │ ├─人脸检测       │  │ ├─年龄统计       │   │
│  │                 │  │ ├─属性分析       │  │ └─实时更新       │   │
│  │                 │  │ └─结果缓存       │  │                 │   │
│  └─────────────────┘  └─────────────────┘  └─────────────────┘   │
├─────────────────────────────────────────────────────────────────┤
│                      模型层                                      │
│  ┌─────────────────┐  ┌─────────────────┐                       │
│  │   YOLO模型      │  │  InspireFace    │                       │
│  │                 │  │     模型        │                       │
│  │ ├─yolov5s.rknn  │  │                 │                       │
│  │ └─yolov8n.rknn  │  │ └─Gundam_RK3588 │                       │
│  └─────────────────┘  └─────────────────┘                       │
└─────────────────────────────────────────────────────────────────┘
```

## 📊 **数据流程图**

```
输入图像
    ↓
┌─────────────────┐
│   YOLO检测      │ ← InferenceManager
│ (YOLOv5/YOLOv8n)│
└─────────────────┘
    ↓
┌─────────────────┐
│   结果筛选      │ ← 筛选"person"类别
│ (Person Filter) │
└─────────────────┘
    ↓
┌─────────────────┐
│   人脸区域提取   │ ← 提取人员边界框
│ (ROI Extraction)│
└─────────────────┘
    ↓
┌─────────────────┐
│  InspireFace    │ ← FaceAnalysisManager
│    人脸识别     │
└─────────────────┘
    ↓
┌─────────────────┐
│   属性分析      │ ← 性别、年龄、种族
│ (Attribute)     │
└─────────────────┘
    ↓
┌─────────────────┐
│   统计更新      │ ← StatisticsManager
│ (Statistics)    │
└─────────────────┘
    ↓
┌─────────────────┐
│   结果输出      │ ← 显示统计信息
│ (Output)        │
└─────────────────┘
```

## 🧩 **核心模块设计**

### **1. 人脸分析管理器 (FaceAnalysisManager)**

```cpp
class FaceAnalysisManager {
private:
    bool m_initialized;
    void* m_inspireFaceSession;
    std::mutex m_mutex;
    
    // 配置参数
    struct FaceConfig {
        bool enableGenderDetection = true;
        bool enableAgeDetection = true;
        bool enableRaceDetection = false;
        float faceDetectionThreshold = 0.5f;
        int maxFacesPerPerson = 1;
    } m_config;

public:
    // 初始化和释放
    bool initialize(const std::string& modelPath);
    void release();
    
    // 核心功能
    bool analyzePersonRegions(const cv::Mat& image, 
                             const std::vector<Detection>& personDetections,
                             std::vector<FaceAnalysisResult>& results);
    
    // 配置管理
    void setConfig(const FaceConfig& config);
    FaceConfig getConfig() const;
    
    // 状态查询
    bool isInitialized() const { return m_initialized; }
};
```

### **2. 人脸分析结果结构**

```cpp
struct FaceAnalysisResult {
    // 关联的人员检测
    int personId;
    Detection personDetection;
    
    // 人脸检测结果
    struct FaceInfo {
        cv::Rect faceRect;          // 人脸边界框
        float confidence;           // 检测置信度
        
        // 属性信息
        struct Attributes {
            int gender;             // 0: 女性, 1: 男性, -1: 未知
            float genderConfidence;
            
            int ageBracket;         // 年龄段 0-8
            float ageConfidence;
            
            int race;               // 种族 0-4, -1: 未知
            float raceConfidence;
            
            bool isValid() const {
                return gender >= 0 && ageBracket >= 0;
            }
        } attributes;
        
        // 关键点 (可选)
        std::vector<cv::Point2f> landmarks;
    };
    
    std::vector<FaceInfo> faces;
    
    // 辅助方法
    bool hasValidFaces() const {
        return !faces.empty() && 
               std::any_of(faces.begin(), faces.end(),
                          [](const FaceInfo& f) { return f.attributes.isValid(); });
    }
    
    FaceInfo getBestFace() const {
        if (faces.empty()) return {};
        return *std::max_element(faces.begin(), faces.end(),
                                [](const FaceInfo& a, const FaceInfo& b) {
                                    return a.confidence < b.confidence;
                                });
    }
};
```

### **3. 统计数据管理器 (StatisticsManager)**

```cpp
class StatisticsManager {
private:
    struct Statistics {
        // 基础统计
        int totalPersonCount = 0;
        int totalFaceCount = 0;
        
        // 性别统计
        int maleCount = 0;
        int femaleCount = 0;
        int unknownGenderCount = 0;
        
        // 年龄统计 (9个年龄段)
        std::array<int, 9> ageBracketCounts = {0};
        
        // 种族统计 (5种)
        std::array<int, 5> raceCounts = {0};
        
        // 时间戳
        std::chrono::steady_clock::time_point lastUpdate;
        
        void reset() {
            totalPersonCount = 0;
            totalFaceCount = 0;
            maleCount = femaleCount = unknownGenderCount = 0;
            ageBracketCounts.fill(0);
            raceCounts.fill(0);
            lastUpdate = std::chrono::steady_clock::now();
        }
    };
    
    Statistics m_currentStats;
    Statistics m_historicalStats;
    std::mutex m_mutex;
    
    // 配置
    bool m_enableHistorical = true;
    std::chrono::seconds m_resetInterval{300}; // 5分钟重置

public:
    // 更新统计
    void updateStatistics(const std::vector<FaceAnalysisResult>& results);
    
    // 获取统计
    Statistics getCurrentStatistics() const;
    Statistics getHistoricalStatistics() const;
    
    // 重置统计
    void resetCurrentStatistics();
    void resetHistoricalStatistics();
    
    // 配置
    void setResetInterval(std::chrono::seconds interval);
    void enableHistoricalStats(bool enable);
};
```

### **4. 扩展的推理管理器**

```cpp
class InferenceManager {
private:
    // 现有成员...
    std::unique_ptr<FaceAnalysisManager> m_faceAnalysisManager;
    std::unique_ptr<StatisticsManager> m_statisticsManager;
    
    // 级联检测配置
    struct CascadeConfig {
        bool enableFaceAnalysis = false;
        bool enableStatistics = false;
        float personConfidenceThreshold = 0.5f;
        int minPersonPixelSize = 50;
        bool enablePersonTracking = false;
    } m_cascadeConfig;

public:
    // 扩展的推理接口
    struct ExtendedInferenceResult {
        InferenceResultGroup objectDetections;  // 原有目标检测结果
        std::vector<FaceAnalysisResult> faceAnalysisResults;  // 人脸分析结果
        StatisticsManager::Statistics statistics;  // 统计数据
        
        bool hasPersonDetections() const {
            return std::any_of(objectDetections.results.begin(),
                              objectDetections.results.end(),
                              [](const InferenceResult& r) {
                                  return r.class_name == "person";
                              });
        }
    };
    
    // 核心推理方法
    int extendedInference(const cv::Mat& input_image, 
                         ExtendedInferenceResult& result);
    
    // 人脸分析管理
    bool initializeFaceAnalysis(const std::string& modelPath);
    void releaseFaceAnalysis();
    bool isFaceAnalysisEnabled() const;
    
    // 配置管理
    void setCascadeConfig(const CascadeConfig& config);
    CascadeConfig getCascadeConfig() const;
    
    // 统计管理
    StatisticsManager* getStatisticsManager() { return m_statisticsManager.get(); }
};
```

## 🔧 **关键实现细节**

### **1. 级联检测流程实现**

```cpp
int InferenceManager::extendedInference(const cv::Mat& input_image, 
                                       ExtendedInferenceResult& result) {
    // 1. 执行原有目标检测
    int ret = inference(input_image, result.objectDetections);
    if (ret != 0) return ret;
    
    // 2. 检查是否启用人脸分析
    if (!m_cascadeConfig.enableFaceAnalysis || !m_faceAnalysisManager->isInitialized()) {
        return 0;
    }
    
    // 3. 筛选人员检测结果
    std::vector<Detection> personDetections;
    for (const auto& detection : result.objectDetections.results) {
        if (detection.class_name == "person" && 
            detection.confidence >= m_cascadeConfig.personConfidenceThreshold) {
            
            // 检查人员区域大小
            float width = detection.x2 - detection.x1;
            float height = detection.y2 - detection.y1;
            if (width >= m_cascadeConfig.minPersonPixelSize && 
                height >= m_cascadeConfig.minPersonPixelSize) {
                personDetections.push_back(detection);
            }
        }
    }
    
    // 4. 执行人脸分析
    if (!personDetections.empty()) {
        bool faceAnalysisSuccess = m_faceAnalysisManager->analyzePersonRegions(
            input_image, personDetections, result.faceAnalysisResults);
        
        if (!faceAnalysisSuccess) {
            LOGW("Face analysis failed, continuing with object detection only");
        }
    }
    
    // 5. 更新统计数据
    if (m_cascadeConfig.enableStatistics && m_statisticsManager) {
        m_statisticsManager->updateStatistics(result.faceAnalysisResults);
        result.statistics = m_statisticsManager->getCurrentStatistics();
    }
    
    return 0;
}
```

### **2. InspireFace集成实现**

```cpp
bool FaceAnalysisManager::analyzePersonRegions(const cv::Mat& image,
                                              const std::vector<Detection>& personDetections,
                                              std::vector<FaceAnalysisResult>& results) {
    std::lock_guard<std::mutex> lock(m_mutex);
    
    if (!m_initialized) return false;
    
    results.clear();
    results.reserve(personDetections.size());
    
    for (size_t i = 0; i < personDetections.size(); ++i) {
        const auto& person = personDetections[i];
        
        // 提取人员区域
        cv::Rect personRect(
            static_cast<int>(person.x1),
            static_cast<int>(person.y1),
            static_cast<int>(person.x2 - person.x1),
            static_cast<int>(person.y2 - person.y1)
        );
        
        // 边界检查和调整
        personRect &= cv::Rect(0, 0, image.cols, image.rows);
        if (personRect.width < 50 || personRect.height < 50) continue;
        
        // 提取ROI
        cv::Mat personROI = image(personRect);
        
        // 执行InspireFace分析
        FaceAnalysisResult faceResult;
        faceResult.personId = static_cast<int>(i);
        faceResult.personDetection = person;
        
        if (analyzePersonROI(personROI, personRect, faceResult)) {
            results.push_back(std::move(faceResult));
        }
    }
    
    return true;
}

bool FaceAnalysisManager::analyzePersonROI(const cv::Mat& personROI,
                                          const cv::Rect& personRect,
                                          FaceAnalysisResult& result) {
    // 调用InspireFace SDK进行人脸检测和属性分析
    // 这里需要调用InspireFace的native方法
    // 具体实现需要通过JNI调用Java层的InspireFace接口
    
    // 伪代码示例：
    // 1. 将cv::Mat转换为InspireFace需要的格式
    // 2. 调用InspireFace.ExecuteFaceTrack()
    // 3. 调用InspireFace.MultipleFacePipelineProcess()
    // 4. 调用InspireFace.GetFaceAttributeResult()
    // 5. 转换结果格式
    
    return true; // 实际实现中返回真实结果
}
```

## 📱 **Java层接口设计**

### **1. 扩展的JNI接口**

```java
public class ExtendedInferenceJNI {
    static {
        System.loadLibrary("myyolov5rtspthreadpool");
    }
    
    // 扩展推理结果类
    public static class ExtendedInferenceResult {
        public InferenceResultGroup objectDetections;
        public FaceAnalysisResult[] faceAnalysisResults;
        public StatisticsData statistics;
    }
    
    public static class FaceAnalysisResult {
        public int personId;
        public Detection personDetection;
        public FaceInfo[] faces;
        
        public static class FaceInfo {
            public float[] faceRect;        // [x, y, width, height]
            public float confidence;
            public FaceAttributes attributes;
            
            public static class FaceAttributes {
                public int gender;          // 0: 女性, 1: 男性, -1: 未知
                public float genderConfidence;
                public int ageBracket;      // 年龄段 0-8
                public float ageConfidence;
                public int race;            // 种族 0-4
                public float raceConfidence;
            }
        }
    }
    
    public static class StatisticsData {
        public int totalPersonCount;
        public int totalFaceCount;
        public int maleCount;
        public int femaleCount;
        public int unknownGenderCount;
        public int[] ageBracketCounts;      // 9个年龄段
        public int[] raceCounts;            // 5种种族
        public long lastUpdateTimestamp;
    }
    
    // Native方法
    public static native boolean initializeFaceAnalysis(String modelPath);
    public static native void releaseFaceAnalysis();
    public static native boolean isFaceAnalysisEnabled();
    
    public static native ExtendedInferenceResult extendedInference(
        int cameraIndex, byte[] imageData, int width, int height);
    
    public static native void setCascadeConfig(boolean enableFaceAnalysis,
                                              boolean enableStatistics,
                                              float personConfidenceThreshold,
                                              int minPersonPixelSize);
    
    public static native StatisticsData getCurrentStatistics();
    public static native void resetStatistics();
}
```

### **2. MainActivity扩展**

```java
public class MainActivity extends AppCompatActivity {
    private static final String TAG = "ExtendedInference";
    
    // InspireFace相关
    private boolean mFaceAnalysisEnabled = false;
    private StatisticsData mLastStatistics;
    
    // UI组件
    private TextView mStatisticsTextView;
    private Switch mFaceAnalysisSwitch;
    
    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        // ... 现有初始化代码 ...
        
        initializeFaceAnalysis();
        setupStatisticsUI();
    }
    
    private void initializeFaceAnalysis() {
        // 初始化InspireFace
        String inspireFacePath = InspireFace.copyResourceFileToApplicationDir(this);
        boolean launched = InspireFace.GlobalLaunch(this, InspireFace.GUNDAM_RK3588);
        
        if (launched) {
            boolean initialized = ExtendedInferenceJNI.initializeFaceAnalysis(inspireFacePath);
            if (initialized) {
                mFaceAnalysisEnabled = true;
                Log.i(TAG, "Face analysis initialized successfully");
                
                // 配置级联检测
                ExtendedInferenceJNI.setCascadeConfig(
                    true,   // enableFaceAnalysis
                    true,   // enableStatistics
                    0.5f,   // personConfidenceThreshold
                    50      // minPersonPixelSize
                );
            } else {
                Log.e(TAG, "Failed to initialize face analysis");
            }
        } else {
            Log.e(TAG, "Failed to launch InspireFace");
        }
    }
    
    private void setupStatisticsUI() {
        mStatisticsTextView = findViewById(R.id.statistics_text);
        mFaceAnalysisSwitch = findViewById(R.id.face_analysis_switch);
        
        mFaceAnalysisSwitch.setChecked(mFaceAnalysisEnabled);
        mFaceAnalysisSwitch.setOnCheckedChangeListener((buttonView, isChecked) -> {
            ExtendedInferenceJNI.setCascadeConfig(
                isChecked, true, 0.5f, 50
            );
        });
        
        // 启动统计更新定时器
        startStatisticsUpdateTimer();
    }
    
    private void startStatisticsUpdateTimer() {
        Handler handler = new Handler(Looper.getMainLooper());
        Runnable updateRunnable = new Runnable() {
            @Override
            public void run() {
                updateStatisticsDisplay();
                handler.postDelayed(this, 1000); // 每秒更新
            }
        };
        handler.post(updateRunnable);
    }
    
    private void updateStatisticsDisplay() {
        if (!mFaceAnalysisEnabled) return;
        
        StatisticsData stats = ExtendedInferenceJNI.getCurrentStatistics();
        if (stats != null) {
            mLastStatistics = stats;
            
            StringBuilder sb = new StringBuilder();
            sb.append("人员统计:\n");
            sb.append("总人数: ").append(stats.totalPersonCount).append("\n");
            sb.append("检测到人脸: ").append(stats.totalFaceCount).append("\n\n");
            
            sb.append("性别分布:\n");
            sb.append("男性: ").append(stats.maleCount).append("\n");
            sb.append("女性: ").append(stats.femaleCount).append("\n");
            sb.append("未知: ").append(stats.unknownGenderCount).append("\n\n");
            
            sb.append("年龄分布:\n");
            String[] ageLabels = {"0-2岁", "3-9岁", "10-19岁", "20-29岁", "30-39岁", 
                                 "40-49岁", "50-59岁", "60-69岁", "70岁以上"};
            for (int i = 0; i < stats.ageBracketCounts.length; i++) {
                if (stats.ageBracketCounts[i] > 0) {
                    sb.append(ageLabels[i]).append(": ").append(stats.ageBracketCounts[i]).append("\n");
                }
            }
            
            mStatisticsTextView.setText(sb.toString());
        }
    }
    
    // 重置统计按钮点击事件
    public void onResetStatisticsClick(View view) {
        ExtendedInferenceJNI.resetStatistics();
        updateStatisticsDisplay();
    }
}
```

## ⚡ **性能优化策略**

### **1. 计算优化**
- **ROI提取优化**: 只对检测到的人员区域进行人脸分析
- **分辨率自适应**: 根据人员区域大小调整人脸检测分辨率
- **批处理**: 多个人员区域批量处理
- **缓存机制**: 缓存InspireFace会话和配置

### **2. 内存优化**
- **对象池**: 复用FaceAnalysisResult对象
- **智能释放**: 及时释放大型图像数据
- **内存监控**: 监控内存使用情况，防止泄漏

### **3. 线程优化**
- **异步处理**: 人脸分析在后台线程执行
- **线程池**: 使用线程池管理并发任务
- **优先级调度**: 目标检测优先级高于人脸分析

### **4. 配置优化**
```cpp
// 性能配置建议
struct PerformanceConfig {
    // 人脸检测配置
    float faceDetectionThreshold = 0.6f;    // 提高阈值减少误检
    int maxFacesPerPerson = 1;               // 限制每人最多检测1张人脸
    int minFacePixelSize = 30;               // 最小人脸尺寸
    
    // 处理频率控制
    int faceAnalysisInterval = 3;            // 每3帧进行一次人脸分析
    int statisticsUpdateInterval = 1000;     // 统计数据更新间隔(ms)
    
    // 资源限制
    int maxConcurrentFaceAnalysis = 2;       // 最大并发人脸分析数
    int maxPersonRegionsPerFrame = 10;       // 每帧最大处理人员数
};
```

## 🚀 **集成实施步骤**

### **阶段1: 基础架构搭建**
1. 创建FaceAnalysisManager类框架
2. 扩展InferenceManager接口
3. 设计数据结构和配置类
4. 实现基础的级联检测流程

### **阶段2: InspireFace集成**
1. 集成InspireFace SDK到项目
2. 实现JNI桥接层
3. 完成人脸检测和属性分析功能
4. 测试基础功能

### **阶段3: 统计系统实现**
1. 实现StatisticsManager
2. 设计统计数据结构
3. 实现实时统计更新
4. 完成Java层统计显示

### **阶段4: 性能优化**
1. 实现性能监控
2. 优化内存使用
3. 调整处理频率
4. 压力测试和调优

### **阶段5: 用户界面完善**
1. 设计统计显示界面
2. 添加配置控制选项
3. 实现数据导出功能
4. 完善错误处理和用户提示

这个架构设计确保了：
- ✅ 完全保持现有功能不变
- ✅ 模块化设计，易于维护
- ✅ 高性能的级联检测流程
- ✅ 完整的错误处理机制
- ✅ 灵活的配置和扩展能力

您希望我详细展开哪个部分的实现细节？
