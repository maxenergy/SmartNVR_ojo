# 故障排除指南和常见问题解答

## 🔧 快速诊断

### 应用状态检查
```bash
# 检查应用是否运行
adb shell ps | grep myyolov5rtspthreadpool

# 检查应用版本
adb shell dumpsys package com.wulala.myyolov5rtspthreadpool | grep versionName

# 检查权限状态
adb shell dumpsys package com.wulala.myyolov5rtspthreadpool | grep permission
```

### 系统资源检查
```bash
# 检查内存使用
adb shell cat /proc/meminfo | grep -E "(MemTotal|MemAvailable)"

# 检查存储空间
adb shell df -h /data

# 检查CPU负载
adb shell cat /proc/loadavg
```

## ❌ 编译问题

### Q1: 编译时出现"undefined symbol"错误
```
错误信息: ld.lld: error: undefined symbol: FaceAnalysisManager::initialize
```
**解决方案**:
1. 检查CMakeLists.txt中是否正确注释了problematic源文件
2. 确保以下文件被注释：
   ```cmake
   # engine/extended_inference_manager.cpp
   # jni/direct_inspireface_test_jni.cpp
   # jni/extended_inference_jni.cpp
   ```
3. 清理并重新编译：
   ```bash
   ./gradlew clean
   ./gradlew assembleDebug
   ```

### Q2: NDK版本不匹配错误
```
错误信息: NDK version mismatch
```
**解决方案**:
1. 确保使用正确的NDK版本：
   ```bash
   export ANDROID_NDK_HOME=$ANDROID_HOME/ndk/27.0.12077973
   ```
2. 在Android Studio中设置正确的NDK路径
3. 清理项目并重新编译

### Q3: CMake配置错误
```
错误信息: CMake Error: Could not find CMAKE_MAKE_PROGRAM
```
**解决方案**:
1. 安装或更新CMake到3.22.1+版本
2. 在Android Studio中配置CMake路径
3. 检查环境变量设置

## 🚫 运行时崩溃

### Q4: 应用启动时崩溃
```
错误信息: java.lang.UnsatisfiedLinkError: No implementation found for...
```
**解决方案**:
1. 检查是否调用了不存在的JNI方法
2. 确认MultiCameraView.java中使用了简化的统计实现：
   ```java
   // 确保使用这个方法而不是JNI调用
   lastStatistics = createSimplifiedStatistics();
   ```
3. 重新安装应用：
   ```bash
   adb uninstall com.wulala.myyolov5rtspthreadpool
   ./gradlew installDebug
   ```

### Q5: 内存不足导致崩溃
```
错误信息: OutOfMemoryError或lowmemorykiller
```
**解决方案**:
1. 调整内存清理频率：
   ```cpp
   // 在ZLPlayer.cpp中减少清理间隔
   if (++processCounter % 100 == 0) {  // 原来是200
       cleanupPersonTrackingData();
   }
   ```
2. 降低视频分辨率或帧率
3. 减少同时处理的摄像头数量

### Q6: RTSP连接失败导致崩溃
```
错误信息: Connection refused或Network unreachable
```
**解决方案**:
1. 检查RTSP服务器状态：
   ```bash
   ping <rtsp-server-ip>
   telnet <rtsp-server-ip> 554
   ```
2. 验证RTSP URL格式：
   ```
   正确格式: rtsp://192.168.1.100:554/stream1
   ```
3. 检查网络权限和防火墙设置

## 🐛 功能异常

### Q7: 人员检测不工作
```
现象: 视频正常播放，但没有检测日志输出
```
**解决方案**:
1. 检查YOLO模型文件：
   ```bash
   adb shell ls -la /android_asset/yolov5s.rknn
   ```
2. 验证检测置信度阈值：
   ```cpp
   // 在ZLPlayer.cpp中降低阈值
   if (detection.className == "person" && detection.confidence > 0.3) {  // 原来是0.5
   ```
3. 检查推理池状态：
   ```bash
   adb logcat | grep "inference pool"
   ```

### Q8: 统计数据不准确
```
现象: 检测到人员但统计数据异常
```
**解决方案**:
1. 检查统计逻辑：
   ```cpp
   // 确保计数逻辑正确
   for (const auto& detection : detections) {
       if (detection.className == "person" && detection.confidence > 0.5) {
           personDetections.push_back(detection);
           personCount++;  // 确保这行存在
       }
   }
   ```
2. 验证日志输出：
   ```bash
   adb logcat | grep "📊.*累计统计"
   ```

### Q9: 移动检测不敏感
```
现象: 人员明显移动但显示为静止状态
```
**解决方案**:
1. 调整移动检测阈值：
   ```cpp
   // 在ZLPlayer.cpp中降低阈值
   isMoving = distance > 5.0f;  // 原来是10.0f
   ```
2. 检查坐标计算：
   ```cpp
   // 确保中心点计算正确
   cv::Point2f currentCenter(
       person.box.x + person.box.width / 2.0f,
       person.box.y + person.box.height / 2.0f
   );
   ```

## ⚡ 性能问题

### Q10: 应用运行缓慢
```
现象: 视频卡顿，检测延迟高
```
**解决方案**:
1. 调整帧跳过率：
   ```cpp
   // 增加跳帧频率
   #define FRAME_SKIP_RATE 3  // 原来是2
   ```
2. 减少日志输出频率：
   ```cpp
   // 减少日志输出
   if (++logCounter % 20 == 0) {  // 原来是10
   ```
3. 优化推理池大小：
   ```cpp
   #define MAX_INFERENCE_POOL_SIZE 3  // 原来是5
   ```

### Q11: 内存使用持续增长
```
现象: 应用运行时间越长，内存使用越高
```
**解决方案**:
1. 增加清理频率：
   ```cpp
   if (++processCounter % 50 == 0) {  // 原来是200
       cleanupPersonTrackingData();
   }
   ```
2. 检查内存泄漏：
   ```bash
   # 监控内存使用变化
   while true; do
       adb shell dumpsys meminfo com.wulala.myyolov5rtspthreadpool | grep "TOTAL"
       sleep 30
   done
   ```

### Q12: CPU使用率过高
```
现象: 设备发热，CPU使用率>90%
```
**解决方案**:
1. 降低检测频率：
   ```cpp
   // 只在特定条件下进行检测
   static int detectionCounter = 0;
   if (++detectionCounter % 3 == 0) {  // 每3帧检测一次
       processPersonDetectionAndFaceAnalysis(...);
   }
   ```
2. 调整线程优先级
3. 考虑使用GPU加速

## 🔍 调试技巧

### 日志分析
```bash
# 过滤关键日志
adb logcat | grep -E "(📍|🔍|📊|ERROR|FATAL)"

# 监控特定摄像头
adb logcat | grep "Camera 0"

# 监控内存清理
adb logcat | grep "🧹.*清理"

# 监控异常
adb logcat | grep -E "(Exception|Error.*Camera)"
```

### 性能分析
```bash
# 实时监控内存
watch -n 5 'adb shell dumpsys meminfo com.wulala.myyolov5rtspthreadpool | grep -E "(TOTAL|Native)"'

# 监控CPU使用
adb shell top -p $(adb shell pidof com.wulala.myyolov5rtspthreadpool)

# 监控网络连接
adb shell netstat | grep 554  # RTSP端口
```

### 代码调试
```cpp
// 添加调试日志
LOGD("🔧 DEBUG: personCount=%d, detections.size()=%zu", personCount, detections.size());

// 添加性能计时
auto start = std::chrono::steady_clock::now();
// ... 代码逻辑 ...
auto end = std::chrono::steady_clock::now();
auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
LOGD("⏱️ 处理耗时: %ldms", duration.count());
```

## 🆘 紧急恢复

### 应用完全无法启动
1. 卸载并重新安装：
   ```bash
   adb uninstall com.wulala.myyolov5rtspthreadpool
   ./gradlew clean
   ./gradlew installDebug
   ```

2. 重置设备权限：
   ```bash
   adb shell pm reset-permissions com.wulala.myyolov5rtspthreadpool
   ```

3. 清除应用数据：
   ```bash
   adb shell pm clear com.wulala.myyolov5rtspthreadpool
   ```

### 设备无响应
1. 重启ADB服务：
   ```bash
   adb kill-server
   adb start-server
   ```

2. 重启设备：
   ```bash
   adb reboot
   ```

### 回滚到稳定版本
```bash
# 切换到主分支
git checkout main

# 重新编译安装
./gradlew clean
./gradlew installDebug
```

## 📞 获取帮助

### 收集诊断信息
```bash
# 生成完整诊断报告
echo "=== 设备信息 ===" > diagnosis.txt
adb shell getprop ro.build.version.release >> diagnosis.txt
adb shell getprop ro.product.model >> diagnosis.txt

echo "=== 应用信息 ===" >> diagnosis.txt
adb shell dumpsys package com.wulala.myyolov5rtspthreadpool | grep -E "(versionName|versionCode)" >> diagnosis.txt

echo "=== 内存信息 ===" >> diagnosis.txt
adb shell dumpsys meminfo com.wulala.myyolov5rtspthreadpool >> diagnosis.txt

echo "=== 最近日志 ===" >> diagnosis.txt
adb logcat -d | tail -100 >> diagnosis.txt
```

### 联系支持
提交问题时请包含：
1. 问题详细描述
2. 重现步骤
3. 设备型号和系统版本
4. 应用版本信息
5. 完整的日志文件
6. 诊断报告

---

*故障排除指南版本: v1.0*  
*最后更新: 2025-07-22*  
*覆盖版本: feature/enhanced-face-recognition*
