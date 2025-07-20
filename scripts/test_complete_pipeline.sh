#!/bin/bash

# 完整Pipeline测试脚本
# 测试"YOLO人员检测 → InspireFace人脸分析 → 统计显示"的完整流程

set -e

PROJECT_ROOT="/home/rogers/source/rockchip/yolov5rtspthreadpool"
cd "$PROJECT_ROOT"

echo "🚀 完整Pipeline测试开始"
echo "================================"

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

# 2. 启动完整Pipeline测试Activity
echo "🎯 2. 启动完整Pipeline测试"
echo "------------------------"

PACKAGE_NAME="com.wulala.myyolov5rtspthreadpool"

echo "正在启动完整Pipeline测试Activity..."
adb shell am start -n "$PACKAGE_NAME/.CompletePipelineTestActivity" > /dev/null 2>&1

# 等待应用启动
sleep 3

echo "✅ 完整Pipeline测试Activity已启动"

# 3. 监控应用日志
echo "📊 3. 监控Pipeline测试日志"
echo "------------------------"

echo "开始监控完整Pipeline测试日志..."

# 清除旧日志
adb logcat -c

# 启动日志监控
echo "正在监控以下标签的日志:"
echo "  - CompletePipelineTest"
echo "  - IntegratedAIManager" 
echo "  - InspireFaceManager"

# 监控日志并保存到文件
LOG_FILE="/tmp/complete_pipeline_test.log"
timeout 120s adb logcat -s "CompletePipelineTest:*" "IntegratedAIManager:*" "InspireFaceManager:*" > "$LOG_FILE" &
LOG_PID=$!

# 4. 执行完整的Pipeline测试流程
echo "🔧 4. 执行完整Pipeline测试流程"
echo "----------------------------"

echo "步骤1: 触发系统初始化..."
adb shell input tap 960 280
sleep 5

echo "步骤2: 触发Pipeline配置..."
adb shell input tap 960 320
sleep 3

echo "步骤3: 触发完整流程测试..."
adb shell input tap 960 360
sleep 10

echo "步骤4: 触发统计数据显示..."
adb shell input tap 960 400
sleep 5

echo "步骤5: 进行多轮测试验证..."
for i in {1..3}; do
    echo "  第 $i 轮测试..."
    adb shell input tap 960 360  # 重复触发完整流程测试
    sleep 8
done

# 5. 等待测试完成并收集日志
echo "⏱️ 5. 等待测试完成"
echo "----------------"

echo "等待Pipeline测试完成（最多120秒）..."
wait $LOG_PID 2>/dev/null || true

# 6. 分析测试结果
echo "📋 6. 分析Pipeline测试结果"
echo "------------------------"

if [ -f "$LOG_FILE" ]; then
    echo "✅ 日志文件已生成: $LOG_FILE"
    
    # 统计日志条目
    TOTAL_LINES=$(wc -l < "$LOG_FILE")
    echo "📊 总日志条目: $TOTAL_LINES"
    
    if [ $TOTAL_LINES -gt 0 ]; then
        echo ""
        echo "🔍 关键Pipeline测试结果:"
        echo "======================="
        
        # 显示初始化相关日志
        echo "📌 系统初始化:"
        grep -i "初始化\|initialize\|init.*成功\|init.*失败" "$LOG_FILE" | head -10 || echo "  未找到初始化日志"
        
        echo ""
        echo "📌 Pipeline配置:"
        grep -i "pipeline\|配置\|config" "$LOG_FILE" | head -5 || echo "  未找到配置日志"
        
        echo ""
        echo "📌 检测结果:"
        grep -i "检测\|detect.*人\|detect.*face\|人员\|人脸" "$LOG_FILE" | head -10 || echo "  未找到检测日志"
        
        echo ""
        echo "📌 统计数据:"
        grep -i "统计\|累计\|总.*人\|男性\|女性\|比例" "$LOG_FILE" | head -10 || echo "  未找到统计日志"
        
        echo ""
        echo "📌 性能数据:"
        grep -i "耗时\|ms\|内存\|memory\|性能" "$LOG_FILE" | head -5 || echo "  未找到性能日志"
        
        echo ""
        echo "📌 错误信息:"
        grep -i "错误\|error\|失败\|failed\|异常\|exception" "$LOG_FILE" || echo "  ✅ 未发现错误"
        
        # 统计关键指标
        echo ""
        echo "📈 Pipeline测试统计:"
        echo "=================="
        
        INIT_SUCCESS=$(grep -c "初始化成功\|initialized successfully" "$LOG_FILE" 2>/dev/null || echo "0")
        DETECTION_COUNT=$(grep -c "检测.*人\|检测到.*人" "$LOG_FILE" 2>/dev/null || echo "0")
        FACE_ANALYSIS_COUNT=$(grep -c "人脸分析\|face.*analysis" "$LOG_FILE" 2>/dev/null || echo "0")
        
        echo "  - 成功初始化次数: $INIT_SUCCESS"
        echo "  - 人员检测次数: $DETECTION_COUNT"
        echo "  - 人脸分析次数: $FACE_ANALYSIS_COUNT"
        
        # 提取统计数据
        TOTAL_PERSONS=$(grep "累计检测人员" "$LOG_FILE" | tail -1 | grep -o "[0-9]\+" | head -1 || echo "0")
        TOTAL_FACES=$(grep "累计检测人脸" "$LOG_FILE" | tail -1 | grep -o "[0-9]\+" | head -1 || echo "0")
        
        if [ "$TOTAL_PERSONS" != "0" ]; then
            echo "  - 累计检测人员: $TOTAL_PERSONS 人"
            echo "  - 累计检测人脸: $TOTAL_FACES 个"
        fi
        
    else
        echo "❌ 日志文件为空，可能没有捕获到相关日志"
    fi
else
    echo "❌ 日志文件未生成"
fi

# 7. 检查应用状态和性能
echo ""
echo "📱 7. 检查应用状态和性能"
echo "----------------------"

if adb shell ps | grep -q "$PACKAGE_NAME"; then
    echo "✅ 应用仍在运行"
    
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

# 8. 生成Pipeline测试报告
echo ""
echo "📋 8. Pipeline测试报告总结"
echo "========================="

echo "🎯 完整Pipeline测试结果:"
if [ -f "$LOG_FILE" ] && [ $(wc -l < "$LOG_FILE") -gt 0 ]; then
    echo "   ✅ 成功捕获到Pipeline测试日志"
    
    # 检查关键成功指标
    if grep -q "系统初始化.*成功\|initialized.*successfully" "$LOG_FILE"; then
        echo "   ✅ 系统初始化成功"
    else
        echo "   ⚠️  系统初始化状态未确认"
    fi
    
    if grep -q "Pipeline.*配置\|配置.*完成" "$LOG_FILE"; then
        echo "   ✅ Pipeline配置成功"
    else
        echo "   ⚠️  Pipeline配置状态未确认"
    fi
    
    if grep -q "检测.*人\|人员.*检测" "$LOG_FILE"; then
        echo "   ✅ 人员检测功能正常"
    else
        echo "   ⚠️  人员检测功能未确认"
    fi
    
    if grep -q "人脸分析\|face.*analysis" "$LOG_FILE"; then
        echo "   ✅ 人脸分析功能正常"
    else
        echo "   ⚠️  人脸分析功能未确认"
    fi
    
    if grep -q "统计.*数据\|累计.*人" "$LOG_FILE"; then
        echo "   ✅ 统计功能正常"
    else
        echo "   ⚠️  统计功能未确认"
    fi
    
else
    echo "   ❌ 未能捕获到有效的Pipeline测试日志"
fi

echo ""
echo "🚀 下一步建议:"
if [ -f "$LOG_FILE" ] && grep -q "成功\|success" "$LOG_FILE"; then
    echo "   1. 集成到主界面UI显示"
    echo "   2. 优化检测性能和准确性"
    echo "   3. 添加实时视频流处理"
    echo "   4. 进行长时间稳定性测试"
    echo "   5. 准备生产环境部署"
else
    echo "   1. 检查Pipeline各组件的集成状态"
    echo "   2. 验证YOLO和InspireFace的协同工作"
    echo "   3. 分析具体的错误信息和性能瓶颈"
    echo "   4. 优化数据传递和处理流程"
fi

echo ""
echo "📁 测试文件位置:"
echo "   - 日志文件: $LOG_FILE"
echo "   - 可使用 'cat $LOG_FILE' 查看完整日志"

echo ""
echo "✅ 完整Pipeline测试完成"
echo "======================"
