#!/bin/bash

# 集成AI系统测试脚本
# 通过ADB命令直接测试集成功能

set -e

PROJECT_ROOT="/home/rogers/source/rockchip/yolov5rtspthreadpool"
cd "$PROJECT_ROOT"

echo "🚀 集成AI系统测试开始"
echo "========================"

# 1. 检查设备连接
echo "📱 1. 检查设备连接状态"
echo "-------------------"

if adb devices | grep -q "device$"; then
    DEVICE_SERIAL=$(adb devices | grep "device$" | head -1 | cut -f1)
    echo "✅ 设备已连接: $DEVICE_SERIAL"
else
    echo "❌ 未检测到连接的设备"
    exit 1
fi

# 2. 启动集成AI测试Activity
echo "🎯 2. 启动集成AI测试"
echo "-----------------"

PACKAGE_NAME="com.wulala.myyolov5rtspthreadpool"

echo "正在启动集成AI测试Activity..."
adb shell am start -n "$PACKAGE_NAME/.IntegratedAITestActivity" > /dev/null 2>&1

# 等待应用启动
sleep 3

echo "✅ 集成AI测试Activity已启动"

# 3. 监控应用日志
echo "📊 3. 监控集成系统日志"
echo "--------------------"

echo "开始监控集成AI系统日志..."

# 清除旧日志
adb logcat -c

# 启动日志监控
echo "正在监控以下标签的日志:"
echo "  - IntegratedAITest"
echo "  - IntegratedAIManager" 
echo "  - InspireFaceManager"
echo "  - DirectInspireFaceTest"

# 监控日志并保存到文件
LOG_FILE="/tmp/integrated_ai_test.log"
timeout 60s adb logcat -s "IntegratedAITest:*" "IntegratedAIManager:*" "InspireFaceManager:*" "DirectInspireFaceTest:*" > "$LOG_FILE" &
LOG_PID=$!

# 4. 手动触发测试（多种方法）
echo "🔧 4. 触发集成系统测试"
echo "-------------------"

echo "方法1: 尝试触发初始化按钮（坐标1）..."
adb shell input tap 960 280
sleep 2

echo "方法2: 尝试触发初始化按钮（坐标2）..."
adb shell input tap 500 280
sleep 2

echo "方法3: 尝试触发初始化按钮（坐标3）..."
adb shell input tap 960 250
sleep 2

echo "方法4: 使用键盘导航..."
adb shell input keyevent KEYCODE_TAB
sleep 1
adb shell input keyevent KEYCODE_ENTER
sleep 2

echo "方法5: 直接启动DirectInspireFaceTest作为对比..."
adb shell am start -n "$PACKAGE_NAME/.DirectInspireFaceTestActivity" > /dev/null 2>&1
sleep 2
adb shell input tap 960 450  # 触发完整初始化测试
sleep 5

# 5. 等待测试完成并收集日志
echo "⏱️ 5. 等待测试完成"
echo "----------------"

echo "等待测试完成（最多60秒）..."
wait $LOG_PID 2>/dev/null || true

# 6. 分析测试结果
echo "📋 6. 分析测试结果"
echo "---------------"

if [ -f "$LOG_FILE" ]; then
    echo "✅ 日志文件已生成: $LOG_FILE"
    
    # 统计日志条目
    TOTAL_LINES=$(wc -l < "$LOG_FILE")
    echo "📊 总日志条目: $TOTAL_LINES"
    
    if [ $TOTAL_LINES -gt 0 ]; then
        echo ""
        echo "🔍 关键日志内容:"
        echo "==============="
        
        # 显示初始化相关日志
        echo "📌 初始化相关:"
        grep -i "初始化\|initialize\|init" "$LOG_FILE" || echo "  未找到初始化日志"
        
        echo ""
        echo "📌 成功相关:"
        grep -i "成功\|success\|✅" "$LOG_FILE" || echo "  未找到成功日志"
        
        echo ""
        echo "📌 错误相关:"
        grep -i "失败\|error\|❌\|exception" "$LOG_FILE" || echo "  未找到错误日志"
        
        echo ""
        echo "📌 InspireFace相关:"
        grep -i "inspireface\|face" "$LOG_FILE" || echo "  未找到InspireFace日志"
        
        echo ""
        echo "📌 完整日志内容:"
        echo "==============="
        cat "$LOG_FILE"
        
    else
        echo "❌ 日志文件为空，可能没有捕获到相关日志"
    fi
else
    echo "❌ 日志文件未生成"
fi

# 7. 检查应用状态
echo ""
echo "📱 7. 检查应用状态"
echo "---------------"

if adb shell ps | grep -q "$PACKAGE_NAME"; then
    echo "✅ 应用仍在运行"
    
    # 获取内存使用情况
    MEMORY_INFO=$(adb shell "dumpsys meminfo $PACKAGE_NAME" 2>/dev/null | grep -E "TOTAL|Native Heap|Dalvik Heap" | head -3 || echo "无法获取内存信息")
    echo "💾 内存使用情况:"
    echo "$MEMORY_INFO"
else
    echo "❌ 应用已停止运行"
fi

# 8. 生成测试报告
echo ""
echo "📋 8. 测试报告总结"
echo "=================="

echo "🎯 集成AI系统测试结果:"
if [ -f "$LOG_FILE" ] && [ $(wc -l < "$LOG_FILE") -gt 0 ]; then
    echo "   ✅ 成功捕获到应用日志"
    
    # 检查关键成功指标
    if grep -q "InspireFace.*成功\|InspireFace.*success" "$LOG_FILE"; then
        echo "   ✅ InspireFace初始化成功"
    else
        echo "   ⚠️  InspireFace初始化状态未确认"
    fi
    
    if grep -q "集成.*成功\|integrated.*success" "$LOG_FILE"; then
        echo "   ✅ 集成系统初始化成功"
    else
        echo "   ⚠️  集成系统初始化状态未确认"
    fi
    
else
    echo "   ❌ 未能捕获到有效的应用日志"
fi

echo ""
echo "🚀 下一步建议:"
if [ -f "$LOG_FILE" ] && grep -q "成功\|success" "$LOG_FILE"; then
    echo "   1. 继续进行性能基准测试"
    echo "   2. 测试完整的检测pipeline"
    echo "   3. 集成到主界面UI"
    echo "   4. 进行稳定性测试"
else
    echo "   1. 检查UI坐标和按钮触发问题"
    echo "   2. 验证应用权限和资源访问"
    echo "   3. 检查InspireFace模型文件状态"
    echo "   4. 分析具体的错误信息"
fi

echo ""
echo "📁 测试文件位置:"
echo "   - 日志文件: $LOG_FILE"
echo "   - 可使用 'cat $LOG_FILE' 查看完整日志"

echo ""
echo "✅ 集成AI系统测试完成"
echo "===================="
