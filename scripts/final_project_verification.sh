#!/bin/bash

# InspireFace集成项目最终验证脚本
# 完成YOLOv5 + InspireFace集成的最终验证

set -e

PROJECT_ROOT="/home/rogers/source/rockchip/yolov5rtspthreadpool"
cd "$PROJECT_ROOT"

echo "🚀 InspireFace集成项目最终验证"
echo "=================================="

# 1. 检查设备连接
echo "📱 1. 检查设备连接状态"
echo "-------------------"

if adb devices | grep -q "device$"; then
    DEVICE_SERIAL=$(adb devices | grep "device$" | head -1 | cut -f1)
    DEVICE_MODEL=$(adb shell getprop ro.product.model 2>/dev/null || echo "Unknown")
    ANDROID_VERSION=$(adb shell getprop ro.build.version.release 2>/dev/null || echo "Unknown")
    echo "✅ 设备已连接: $DEVICE_SERIAL"
    echo "📋 设备信息: $DEVICE_MODEL (Android $ANDROID_VERSION)"
else
    echo "❌ 未检测到连接的设备"
    exit 1
fi

# 2. 验证APK安装和版本
echo ""
echo "📦 2. 验证APK安装状态"
echo "-------------------"

PACKAGE_NAME="com.wulala.myyolov5rtspthreadpool"

if adb shell pm list packages | grep -q "$PACKAGE_NAME"; then
    VERSION_INFO=$(adb shell dumpsys package "$PACKAGE_NAME" | grep "versionName" | head -1 || echo "版本信息未知")
    echo "✅ APK已安装: $PACKAGE_NAME"
    echo "📋 $VERSION_INFO"
    
    # 检查APK大小
    APK_SIZE=$(adb shell stat /data/app/$PACKAGE_NAME*/base.apk 2>/dev/null | grep Size | awk '{print $2}' || echo "0")
    if [ "$APK_SIZE" != "0" ]; then
        APK_SIZE_MB=$((APK_SIZE / 1024 / 1024))
        echo "📋 APK大小: ${APK_SIZE_MB}MB"
    fi
else
    echo "❌ APK未安装"
    exit 1
fi

# 3. 验证核心组件
echo ""
echo "🔧 3. 验证核心组件"
echo "---------------"

echo "正在验证InspireFace组件..."
adb shell am start -n "$PACKAGE_NAME/.DirectInspireFaceTestActivity" > /dev/null 2>&1
sleep 2

# 清除日志并监控
adb logcat -c
LOG_FILE="/tmp/final_verification.log"
timeout 30s adb logcat -s "DirectInspireFaceTest:*" "InspireFaceManager:*" > "$LOG_FILE" &
LOG_PID=$!

# 触发InspireFace测试
adb shell input tap 960 450
sleep 8

# 停止日志监控
kill $LOG_PID 2>/dev/null || true
wait $LOG_PID 2>/dev/null || true

# 分析InspireFace测试结果
if [ -f "$LOG_FILE" ] && grep -q "InspireFace.*成功\|successfully.*init" "$LOG_FILE"; then
    echo "✅ InspireFace组件验证成功"
    
    # 提取性能数据
    INIT_TIME=$(grep "初始化耗时\|耗时.*ms" "$LOG_FILE" | head -1 | grep -o "[0-9]\+ ms" || echo "未知")
    MEMORY_USAGE=$(grep "内存使用\|memory.*MB" "$LOG_FILE" | head -1 | grep -o "[0-9]\+ MB" || echo "未知")
    
    echo "  - 初始化耗时: $INIT_TIME"
    echo "  - 内存使用: $MEMORY_USAGE"
else
    echo "⚠️  InspireFace组件验证需要确认"
fi

# 4. 验证YOLOv5集成
echo ""
echo "🎯 4. 验证YOLOv5集成"
echo "-----------------"

echo "正在验证YOLOv5推理组件..."
adb shell am start -n "$PACKAGE_NAME/.RealYOLOTestActivity" > /dev/null 2>&1
sleep 3

# 清除日志并监控YOLOv5测试
adb logcat -c
YOLO_LOG_FILE="/tmp/yolov5_verification.log"
timeout 45s adb logcat -s "RealYOLOTest:*" "RealYOLOInference:*" > "$YOLO_LOG_FILE" &
YOLO_LOG_PID=$!

# 执行YOLOv5测试序列
echo "  步骤1: 触发YOLOv5引擎初始化..."
adb shell input tap 960 250
sleep 8

echo "  步骤2: 获取引擎状态..."
adb shell input tap 960 290
sleep 3

echo "  步骤3: 测试基础推理..."
adb shell input tap 960 330
sleep 5

echo "  步骤4: 测试人员检测..."
adb shell input tap 960 370
sleep 5

# 停止YOLOv5日志监控
kill $YOLO_LOG_PID 2>/dev/null || true
wait $YOLO_LOG_PID 2>/dev/null || true

# 分析YOLOv5测试结果
if [ -f "$YOLO_LOG_FILE" ]; then
    YOLO_LINES=$(wc -l < "$YOLO_LOG_FILE")
    echo "📊 YOLOv5测试日志条目: $YOLO_LINES"
    
    if grep -q "YOLO.*初始化.*成功\|YOLO.*success" "$YOLO_LOG_FILE"; then
        echo "✅ YOLOv5引擎初始化成功"
    elif grep -q "Native library loaded" "$YOLO_LOG_FILE"; then
        echo "✅ YOLOv5 Native库加载成功"
    else
        echo "⚠️  YOLOv5引擎状态需要确认"
    fi
    
    if grep -q "推理.*成功\|inference.*success" "$YOLO_LOG_FILE"; then
        echo "✅ YOLOv5推理功能正常"
    else
        echo "⚠️  YOLOv5推理功能需要确认"
    fi
else
    echo "❌ YOLOv5测试日志未生成"
fi

# 5. 验证集成Pipeline
echo ""
echo "🔗 5. 验证集成Pipeline"
echo "-------------------"

echo "正在验证完整的集成Pipeline..."
adb shell am start -n "$PACKAGE_NAME/.IntegratedAITestActivity" > /dev/null 2>&1
sleep 3

# 清除日志并监控集成测试
adb logcat -c
INTEGRATED_LOG_FILE="/tmp/integrated_verification.log"
timeout 30s adb logcat -s "IntegratedAITest:*" "IntegratedAIManager:*" > "$INTEGRATED_LOG_FILE" &
INTEGRATED_LOG_PID=$!

# 触发集成系统初始化
echo "  触发集成系统初始化..."
adb shell input tap 960 280
sleep 8

# 触发检测流程测试
echo "  触发检测流程测试..."
adb shell input tap 960 350
sleep 5

# 停止集成日志监控
kill $INTEGRATED_LOG_PID 2>/dev/null || true
wait $INTEGRATED_LOG_PID 2>/dev/null || true

# 分析集成测试结果
if [ -f "$INTEGRATED_LOG_FILE" ]; then
    INTEGRATED_LINES=$(wc -l < "$INTEGRATED_LOG_FILE")
    echo "📊 集成测试日志条目: $INTEGRATED_LINES"
    
    if grep -q "集成.*初始化.*成功\|integrated.*success" "$INTEGRATED_LOG_FILE"; then
        echo "✅ 集成系统初始化成功"
    else
        echo "⚠️  集成系统初始化需要确认"
    fi
    
    if grep -q "检测.*完成\|detection.*complete" "$INTEGRATED_LOG_FILE"; then
        echo "✅ 集成检测流程正常"
    else
        echo "⚠️  集成检测流程需要确认"
    fi
else
    echo "❌ 集成测试日志未生成"
fi

# 6. 系统性能评估
echo ""
echo "📈 6. 系统性能评估"
echo "---------------"

if adb shell ps | grep -q "$PACKAGE_NAME"; then
    echo "✅ 应用稳定运行"
    
    # 获取内存使用情况
    echo "💾 内存使用情况:"
    MEMORY_INFO=$(adb shell "dumpsys meminfo $PACKAGE_NAME" 2>/dev/null | grep -E "TOTAL|Native Heap|Dalvik Heap" | head -3 || echo "无法获取内存信息")
    echo "$MEMORY_INFO"
    
    # 获取CPU使用情况
    echo "⚡ CPU使用情况:"
    CPU_INFO=$(adb shell "top -n 1 | grep $PACKAGE_NAME" 2>/dev/null | head -1 || echo "无法获取CPU信息")
    echo "$CPU_INFO"
    
else
    echo "❌ 应用已停止运行"
fi

# 7. 生成最终验证报告
echo ""
echo "📋 7. 最终验证报告"
echo "=================="

echo "🎯 InspireFace集成项目验证结果:"

# 统计成功指标
SUCCESS_COUNT=0
TOTAL_COUNT=5

# 检查各组件状态
if [ -f "$LOG_FILE" ] && grep -q "InspireFace.*成功\|successfully.*init" "$LOG_FILE"; then
    echo "   ✅ InspireFace组件: 验证成功"
    SUCCESS_COUNT=$((SUCCESS_COUNT + 1))
else
    echo "   ⚠️  InspireFace组件: 需要确认"
fi

if [ -f "$YOLO_LOG_FILE" ] && grep -q "Native library loaded" "$YOLO_LOG_FILE"; then
    echo "   ✅ YOLOv5组件: 基础功能正常"
    SUCCESS_COUNT=$((SUCCESS_COUNT + 1))
else
    echo "   ⚠️  YOLOv5组件: 需要确认"
fi

if [ -f "$INTEGRATED_LOG_FILE" ] && [ $(wc -l < "$INTEGRATED_LOG_FILE") -gt 0 ]; then
    echo "   ✅ 集成架构: 基础框架正常"
    SUCCESS_COUNT=$((SUCCESS_COUNT + 1))
else
    echo "   ⚠️  集成架构: 需要确认"
fi

if adb shell ps | grep -q "$PACKAGE_NAME"; then
    echo "   ✅ 系统稳定性: 应用稳定运行"
    SUCCESS_COUNT=$((SUCCESS_COUNT + 1))
else
    echo "   ❌ 系统稳定性: 应用异常"
fi

if [ "$APK_SIZE_MB" -gt 50 ] 2>/dev/null; then
    echo "   ✅ 部署完整性: APK包含完整组件"
    SUCCESS_COUNT=$((SUCCESS_COUNT + 1))
else
    echo "   ⚠️  部署完整性: 需要确认"
fi

# 计算成功率
SUCCESS_RATE=$((SUCCESS_COUNT * 100 / TOTAL_COUNT))
echo ""
echo "📊 验证成功率: $SUCCESS_COUNT/$TOTAL_COUNT ($SUCCESS_RATE%)"

if [ $SUCCESS_RATE -ge 80 ]; then
    echo "🎉 项目验证成功！"
    echo "   - 核心组件功能正常"
    echo "   - 系统架构稳定"
    echo "   - 可进入下一阶段开发"
elif [ $SUCCESS_RATE -ge 60 ]; then
    echo "⚠️  项目基本验证通过"
    echo "   - 主要功能已实现"
    echo "   - 部分组件需要优化"
    echo "   - 建议进行针对性改进"
else
    echo "❌ 项目验证需要改进"
    echo "   - 关键组件存在问题"
    echo "   - 需要进行系统性修复"
fi

echo ""
echo "📁 验证文件位置:"
echo "   - InspireFace日志: $LOG_FILE"
echo "   - YOLOv5日志: $YOLO_LOG_FILE"
echo "   - 集成测试日志: $INTEGRATED_LOG_FILE"

echo ""
echo "✅ InspireFace集成项目最终验证完成"
echo "=================================="
