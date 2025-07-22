# YOLOv5 RTSP多线程池应用 - 部署指南

## 📋 部署概览

本指南提供YOLOv5 RTSP多线程池应用（含人员统计功能）的完整部署说明，包括环境准备、编译构建、安装配置和运行验证。

## 🔧 环境要求

### 开发环境
```bash
操作系统: Ubuntu 18.04+ / macOS 10.15+ / Windows 10+
Android Studio: 4.2+
Android SDK: API Level 31+
Android NDK: 27.0.12077973
CMake: 3.22.1+
Gradle: 7.0+
```

### 目标设备
```bash
硬件平台: RK3588 / RK3566 / RK3568
操作系统: Android 12+
内存: 4GB+ (推荐8GB+)
存储: 16GB+ 可用空间
网络: 支持RTSP流接收
```

### 依赖库版本
```bash
OpenCV: 4.5.0+
RKNN Runtime: 1.4.0+
ZLMediaKit: 最新版本
FFmpeg: 4.4+
```

## 📦 获取源码

### 1. 克隆仓库
```bash
git clone <repository-url>
cd yolov5rtspthreadpool
```

### 2. 切换到功能分支
```bash
git checkout feature/enhanced-face-recognition
```

### 3. 验证代码完整性
```bash
# 检查关键文件是否存在
ls -la app/src/main/cpp/include/face_analysis_manager.h
ls -la app/src/main/cpp/src/face_analysis_manager.cpp
ls -la app/src/main/cpp/types/person_detection_types.h
```

## 🏗️ 编译构建

### 1. 环境配置
```bash
# 设置Android SDK路径
export ANDROID_HOME=/path/to/android-sdk
export PATH=$PATH:$ANDROID_HOME/tools:$ANDROID_HOME/platform-tools

# 设置NDK路径
export ANDROID_NDK_HOME=$ANDROID_HOME/ndk/27.0.12077973
```

### 2. 依赖检查
```bash
# 检查Gradle版本
./gradlew --version

# 检查Android SDK
adb version

# 检查CMake
cmake --version
```

### 3. 编译应用
```bash
# 清理之前的构建
./gradlew clean

# 编译Debug版本
./gradlew assembleDebug

# 编译Release版本（可选）
./gradlew assembleRelease
```

### 4. 验证编译结果
```bash
# 检查APK文件
ls -la app/build/outputs/apk/debug/app-debug.apk

# 检查native库
ls -la app/src/main/cpp/libs_export/arm64-v8a/libmyyolov5rtspthreadpool.so
```

## 📱 设备准备

### 1. 设备连接
```bash
# USB连接（推荐用于调试）
adb devices

# 网络连接（推荐用于部署）
adb connect <device-ip>:5555
```

### 2. 设备配置
```bash
# 启用开发者选项
# 设置 -> 关于设备 -> 连续点击版本号7次

# 启用USB调试
# 设置 -> 开发者选项 -> USB调试

# 允许未知来源安装
# 设置 -> 安全 -> 未知来源
```

### 3. 权限配置
```bash
# 检查设备权限
adb shell pm list permissions | grep CAMERA
adb shell pm list permissions | grep INTERNET
adb shell pm list permissions | grep WRITE_EXTERNAL_STORAGE
```

## 🚀 应用安装

### 1. 安装APK
```bash
# 安装Debug版本
./gradlew installDebug

# 或手动安装
adb install app/build/outputs/apk/debug/app-debug.apk
```

### 2. 验证安装
```bash
# 检查应用是否安装
adb shell pm list packages | grep myyolov5rtspthreadpool

# 检查应用信息
adb shell dumpsys package com.wulala.myyolov5rtspthreadpool
```

### 3. 权限授予
```bash
# 授予摄像头权限
adb shell pm grant com.wulala.myyolov5rtspthreadpool android.permission.CAMERA

# 授予存储权限
adb shell pm grant com.wulala.myyolov5rtspthreadpool android.permission.WRITE_EXTERNAL_STORAGE

# 授予网络权限（通常自动授予）
adb shell pm grant com.wulala.myyolov5rtspthreadpool android.permission.INTERNET
```

## ⚙️ 配置设置

### 1. RTSP流配置
```bash
# 默认RTSP流地址（需要根据实际情况修改）
Camera 1: rtsp://192.168.1.100:554/stream1
Camera 2: rtsp://192.168.1.101:554/stream1
Camera 3: rtsp://192.168.1.102:554/stream1
Camera 4: rtsp://192.168.1.103:554/stream1
```

### 2. 模型文件配置
```bash
# 确保YOLO模型文件存在
ls -la app/src/main/assets/yolov5s.rknn
ls -la app/src/main/assets/yolov8n.rknn

# 检查模型文件大小（应该>1MB）
du -h app/src/main/assets/*.rknn
```

### 3. 应用参数配置
```cpp
// 在ZLPlayer.cpp中可调整的参数
#define PERSON_DETECTION_CONFIDENCE_THRESHOLD 0.5f  // 人员检测置信度阈值
#define MOVEMENT_DETECTION_THRESHOLD 10.0f          // 移动检测阈值（像素）
#define STATISTICS_LOG_INTERVAL 50                  // 统计日志输出间隔
#define MEMORY_CLEANUP_INTERVAL 200                 // 内存清理间隔
```

## 🔍 运行验证

### 1. 启动应用
```bash
# 启动应用
adb shell am start -n com.wulala.myyolov5rtspthreadpool/.MainActivity

# 检查应用进程
adb shell ps | grep myyolov5rtspthreadpool
```

### 2. 功能验证
```bash
# 监控人员检测日志
adb logcat -v time | grep -E "(📍|🔍|📊)"

# 监控性能日志
adb logcat -v time | grep -E "(Camera.*Frame|inference)"

# 监控错误日志
adb logcat -v time | grep -E "(ERROR|FATAL|CRASH)"
```

### 3. 性能监控
```bash
# 监控内存使用
adb shell dumpsys meminfo com.wulala.myyolov5rtspthreadpool

# 监控CPU使用
adb shell top | grep myyolov5rtspthreadpool

# 监控GPU使用（如果支持）
adb shell cat /sys/class/devfreq/fde60000.gpu/cur_freq
```

## 📊 验证标准

### 功能验证清单
- [ ] 应用正常启动，无崩溃
- [ ] 4路RTSP视频流正常显示
- [ ] 人员检测日志正常输出
- [ ] 位置跟踪数据正确记录
- [ ] 统计数据正常汇总
- [ ] 移动状态检测工作正常

### 性能验证清单
- [ ] 内存使用 < 1.5GB
- [ ] CPU使用率 < 80%
- [ ] 检测延迟 < 100ms
- [ ] 应用响应正常，无ANR
- [ ] 长时间运行稳定（>30分钟）

## 🐛 常见问题

### 编译问题
```bash
# 问题：NDK版本不匹配
解决：确保使用NDK 27.0.12077973版本

# 问题：CMake版本过低
解决：升级到CMake 3.22.1+

# 问题：链接错误
解决：检查CMakeLists.txt中注释的源文件是否正确
```

### 运行问题
```bash
# 问题：应用崩溃
解决：检查logcat中的FATAL错误，通常是权限或JNI问题

# 问题：RTSP流无法连接
解决：检查网络连接和RTSP服务器状态

# 问题：检测功能不工作
解决：检查模型文件是否正确加载
```

### 性能问题
```bash
# 问题：内存使用过高
解决：调整MEMORY_CLEANUP_INTERVAL参数

# 问题：检测延迟过高
解决：降低输入分辨率或调整推理池大小

# 问题：CPU使用率过高
解决：调整帧跳过率或降低检测频率
```

## 🔧 高级配置

### 1. 自定义RTSP源
```java
// 在MainActivity.java中修改RTSP地址
private static final String[] RTSP_URLS = {
    "rtsp://your-camera-1-ip:554/stream",
    "rtsp://your-camera-2-ip:554/stream",
    "rtsp://your-camera-3-ip:554/stream",
    "rtsp://your-camera-4-ip:554/stream"
};
```

### 2. 调整检测参数
```cpp
// 在ZLPlayer.cpp中调整参数
static const float CONFIDENCE_THRESHOLD = 0.6f;  // 提高置信度阈值
static const int MOVEMENT_THRESHOLD = 15;         // 调整移动检测敏感度
static const int LOG_INTERVAL = 20;               // 调整日志输出频率
```

### 3. 性能优化
```cpp
// 调整推理池大小
#define MAX_INFERENCE_POOL_SIZE 5

// 调整帧跳过率
#define FRAME_SKIP_RATE 2

// 调整内存清理频率
#define CLEANUP_INTERVAL 100
```

## 📞 技术支持

### 日志收集
```bash
# 收集完整日志
adb logcat -v time > app_logs.txt

# 收集崩溃日志
adb logcat -v time | grep -E "(FATAL|AndroidRuntime)" > crash_logs.txt

# 收集性能日志
adb shell dumpsys meminfo com.wulala.myyolov5rtspthreadpool > memory_info.txt
```

### 问题报告
提交问题时请包含：
1. 设备型号和Android版本
2. 应用版本和Git提交号
3. 完整的错误日志
4. 重现步骤
5. 预期行为和实际行为

---

*部署指南版本: v1.0*  
*最后更新: 2025-07-22*  
*适用版本: feature/enhanced-face-recognition*
