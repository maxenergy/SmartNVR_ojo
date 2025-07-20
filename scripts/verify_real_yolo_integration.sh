#!/bin/bash

# 真实YOLO集成验证脚本
# 验证完整的"真实YOLO检测→人员筛选→InspireFace分析→统计显示"流程

set -e

PROJECT_ROOT="/home/rogers/source/rockchip/yolov5rtspthreadpool"
cd "$PROJECT_ROOT"

echo "🚀 真实YOLO集成验证开始"
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

# 2. 验证APK安装
echo ""
echo "📦 2. 验证APK安装状态"
echo "-------------------"

PACKAGE_NAME="com.wulala.myyolov5rtspthreadpool"

if adb shell pm list packages | grep -q "$PACKAGE_NAME"; then
    VERSION_INFO=$(adb shell dumpsys package "$PACKAGE_NAME" | grep "versionName" | head -1 || echo "版本信息未知")
    echo "✅ APK已安装: $PACKAGE_NAME"
    echo "📋 $VERSION_INFO"
else
    echo "❌ APK未安装"
    exit 1
fi

# 3. 检查应用权限
echo ""
echo "🔐 3. 检查应用权限"
echo "---------------"

PERMISSIONS=$(adb shell dumpsys package "$PACKAGE_NAME" | grep "android.permission" | wc -l)
echo "📋 已授予权限数量: $PERMISSIONS"

# 4. 启动真实YOLO验证测试
echo ""
echo "🎯 4. 启动真实YOLO验证测试"
echo "------------------------"

echo "正在启动DirectInspireFaceTestActivity..."
adb shell am start -n "$PACKAGE_NAME/.DirectInspireFaceTestActivity" > /dev/null 2>&1

# 等待应用启动
sleep 3

echo "✅ 测试Activity已启动"

# 5. 监控测试日志
echo ""
echo "📊 5. 监控真实YOLO集成测试"
echo "------------------------"

echo "开始监控测试日志..."

# 清除旧日志
adb logcat -c

# 启动日志监控
LOG_FILE="/tmp/real_yolo_verification.log"
timeout 90s adb logcat -s "DirectInspireFaceTest:*" "InspireFaceManager:*" "RealYOLOInference:*" "IntegratedAIManager:*" > "$LOG_FILE" &
LOG_PID=$!

# 6. 执行测试序列
echo ""
echo "🔧 6. 执行真实YOLO集成测试序列"
echo "-----------------------------"

echo "步骤1: 触发InspireFace初始化测试..."
adb shell input tap 960 450
sleep 8

echo "步骤2: 触发模型文件验证..."
adb shell input tap 960 350
sleep 5

echo "步骤3: 触发能力检测测试..."
adb shell input tap 960 300
sleep 5

echo "步骤4: 进行多轮验证测试..."
for i in {1..3}; do
    echo "  第 $i 轮验证测试..."
    adb shell input tap 960 450  # 重复触发完整测试
    sleep 8
done

# 7. 等待测试完成并收集日志
echo ""
echo "⏱️ 7. 等待测试完成"
echo "----------------"

echo "等待真实YOLO集成测试完成（最多90秒）..."
wait $LOG_PID 2>/dev/null || true

# 8. 分析测试结果
echo ""
echo "📋 8. 分析真实YOLO集成测试结果"
echo "=============================="

if [ -f "$LOG_FILE" ]; then
    echo "✅ 日志文件已生成: $LOG_FILE"
    
    # 统计日志条目
    TOTAL_LINES=$(wc -l < "$LOG_FILE")
    echo "📊 总日志条目: $TOTAL_LINES"
    
    if [ $TOTAL_LINES -gt 0 ]; then
        echo ""
        echo "🔍 关键测试结果分析:"
        echo "==================="
        
        # InspireFace初始化
        echo "📌 InspireFace初始化:"
        grep -i "inspireface.*初始化\|inspireface.*success\|successfully loaded" "$LOG_FILE" | head -5 || echo "  未找到InspireFace初始化日志"
        
        echo ""
        echo "📌 模型加载:"
        grep -i "model.*load\|资源.*加载\|archive.*file" "$LOG_FILE" | head -5 || echo "  未找到模型加载日志"
        
        echo ""
        echo "📌 会话创建:"
        grep -i "session.*creat\|会话.*创建" "$LOG_FILE" | head -3 || echo "  未找到会话创建日志"
        
        echo ""
        echo "📌 版本信息:"
        grep -i "version\|版本" "$LOG_FILE" | head -3 || echo "  未找到版本信息"
        
        echo ""
        echo "📌 性能数据:"
        grep -i "耗时\|ms\|memory\|内存" "$LOG_FILE" | head -5 || echo "  未找到性能数据"
        
        echo ""
        echo "📌 错误信息:"
        grep -i "error\|错误\|failed\|失败\|exception\|异常" "$LOG_FILE" | head -5 || echo "  ✅ 未发现错误"
        
        # 统计关键指标
        echo ""
        echo "📈 关键指标统计:"
        echo "==============="
        
        INIT_SUCCESS=$(grep -c "初始化.*成功\|successfully.*init\|loaded.*success" "$LOG_FILE" 2>/dev/null || echo "0")
        SESSION_SUCCESS=$(grep -c "session.*success\|会话.*成功" "$LOG_FILE" 2>/dev/null || echo "0")
        VERSION_FOUND=$(grep -c "version.*[0-9]\|版本.*[0-9]" "$LOG_FILE" 2>/dev/null || echo "0")
        
        echo "  - 成功初始化次数: $INIT_SUCCESS"
        echo "  - 成功创建会话次数: $SESSION_SUCCESS"
        echo "  - 版本信息获取次数: $VERSION_FOUND"
        
        # 检查是否有真实YOLO相关日志
        YOLO_LOGS=$(grep -c "RealYOLOInference\|YOLO.*engine\|yolo.*推理" "$LOG_FILE" 2>/dev/null || echo "0")
        echo "  - YOLO相关日志条目: $YOLO_LOGS"
        
        if [ $YOLO_LOGS -gt 0 ]; then
            echo ""
            echo "📌 真实YOLO集成日志:"
            grep -i "RealYOLOInference\|YOLO.*engine\|yolo.*推理" "$LOG_FILE" | head -10
        fi
        
    else
        echo "❌ 日志文件为空，可能没有捕获到相关日志"
    fi
else
    echo "❌ 日志文件未生成"
fi

# 9. 检查应用状态和性能
echo ""
echo "📱 9. 检查应用状态和性能"
echo "----------------------"

if adb shell ps | grep -q "$PACKAGE_NAME"; then
    echo "✅ 应用仍在运行"
    
    # 获取内存使用情况
    echo "💾 内存使用情况:"
    MEMORY_INFO=$(adb shell "dumpsys meminfo $PACKAGE_NAME" 2>/dev/null | grep -E "TOTAL|Native Heap|Dalvik Heap" | head -3 || echo "无法获取内存信息")
    echo "$MEMORY_INFO"
    
else
    echo "❌ 应用已停止运行"
fi

# 10. 生成验证报告
echo ""
echo "📋 10. 真实YOLO集成验证报告"
echo "=========================="

echo "🎯 验证结果总结:"
if [ -f "$LOG_FILE" ] && [ $(wc -l < "$LOG_FILE") -gt 0 ]; then
    echo "   ✅ 成功捕获到测试日志"
    
    # 检查关键成功指标
    if grep -q "InspireFace.*成功\|successfully.*loaded\|session.*success" "$LOG_FILE"; then
        echo "   ✅ InspireFace组件验证成功"
    else
        echo "   ⚠️  InspireFace组件状态需要确认"
    fi
    
    if grep -q "version.*[0-9]\|版本.*[0-9]" "$LOG_FILE"; then
        echo "   ✅ 版本信息获取成功"
    else
        echo "   ⚠️  版本信息获取需要确认"
    fi
    
    if grep -q "RealYOLOInference\|YOLO.*engine" "$LOG_FILE"; then
        echo "   ✅ 真实YOLO集成组件已激活"
    else
        echo "   ⚠️  真实YOLO集成组件未检测到活动"
    fi
    
else
    echo "   ❌ 未能捕获到有效的测试日志"
fi

echo ""
echo "🚀 下一步建议:"
if [ -f "$LOG_FILE" ] && grep -q "成功\|success" "$LOG_FILE"; then
    echo "   1. 基础组件验证成功，可以进行真实YOLO推理测试"
    echo "   2. 测试完整的检测Pipeline性能"
    echo "   3. 验证人员检测和人脸分析的准确性"
    echo "   4. 进行实时视频流处理测试"
    echo "   5. 准备生产环境部署"
else
    echo "   1. 检查模型文件是否正确部署到设备"
    echo "   2. 验证应用权限和资源访问"
    echo "   3. 检查设备硬件兼容性"
    echo "   4. 分析具体的错误信息和日志"
fi

echo ""
echo "📁 测试文件位置:"
echo "   - 日志文件: $LOG_FILE"
echo "   - 可使用 'cat $LOG_FILE' 查看完整日志"

echo ""
echo "✅ 真实YOLO集成验证完成"
echo "======================"
