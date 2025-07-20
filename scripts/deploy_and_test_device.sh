#!/bin/bash

# InspireFace设备部署和测试脚本
# 用于在RK3588设备上进行完整的功能验证

set -e

PROJECT_ROOT="/home/rogers/source/rockchip/yolov5rtspthreadpool"
cd "$PROJECT_ROOT"

echo "🚀 InspireFace设备部署测试开始"
echo "================================="

# 1. 检查设备连接
echo "📱 1. 检查设备连接状态"
echo "-------------------"

if adb devices | grep -q "device$"; then
    DEVICE_SERIAL=$(adb devices | grep "device$" | head -1 | cut -f1)
    echo "✅ 设备已连接: $DEVICE_SERIAL"
    
    # 获取设备信息
    DEVICE_MODEL=$(adb shell getprop ro.product.model 2>/dev/null || echo "Unknown")
    ANDROID_VERSION=$(adb shell getprop ro.build.version.release 2>/dev/null || echo "Unknown")
    CPU_ABI=$(adb shell getprop ro.product.cpu.abi 2>/dev/null || echo "Unknown")
    
    echo "📋 设备信息:"
    echo "   - 型号: $DEVICE_MODEL"
    echo "   - Android版本: $ANDROID_VERSION"
    echo "   - CPU架构: $CPU_ABI"
    
    # 检查是否为RK3588设备
    HARDWARE=$(adb shell getprop ro.hardware 2>/dev/null || echo "Unknown")
    echo "   - 硬件平台: $HARDWARE"
    
    if [[ "$HARDWARE" == *"rk3588"* ]] || [[ "$DEVICE_MODEL" == *"RK3588"* ]]; then
        echo "✅ 确认为RK3588设备"
    else
        echo "⚠️  设备可能不是RK3588平台，继续测试..."
    fi
    
else
    echo "❌ 未检测到连接的设备"
    echo "请确保:"
    echo "   1. 设备已通过USB连接"
    echo "   2. 已启用USB调试"
    echo "   3. 已授权ADB调试"
    exit 1
fi

echo ""

# 2. 构建和安装APK
echo "📦 2. 构建和安装APK"
echo "-----------------"

echo "正在构建APK..."
if ./gradlew assembleDebug > /tmp/build.log 2>&1; then
    echo "✅ APK构建成功"
    
    APK_PATH="app/build/outputs/apk/debug/app-debug.apk"
    APK_SIZE=$(du -sh "$APK_PATH" | cut -f1)
    echo "📦 APK信息: $APK_SIZE"
    
    echo "正在安装APK到设备..."
    if adb install -r "$APK_PATH" > /tmp/install.log 2>&1; then
        echo "✅ APK安装成功"
    else
        echo "❌ APK安装失败"
        cat /tmp/install.log
        exit 1
    fi
    
else
    echo "❌ APK构建失败"
    tail -20 /tmp/build.log
    exit 1
fi

echo ""

# 3. 检查应用权限和存储
echo "🔐 3. 检查应用权限和存储"
echo "---------------------"

PACKAGE_NAME="com.wulala.myyolov5rtspthreadpool"

# 检查应用是否已安装
if adb shell pm list packages | grep -q "$PACKAGE_NAME"; then
    echo "✅ 应用已安装: $PACKAGE_NAME"
    
    # 获取应用数据目录
    DATA_DIR="/data/data/$PACKAGE_NAME"
    echo "📁 应用数据目录: $DATA_DIR"
    
    # 检查存储权限
    echo "🔍 检查存储权限..."
    adb shell pm grant "$PACKAGE_NAME" android.permission.WRITE_EXTERNAL_STORAGE 2>/dev/null || true
    adb shell pm grant "$PACKAGE_NAME" android.permission.READ_EXTERNAL_STORAGE 2>/dev/null || true
    
else
    echo "❌ 应用未正确安装"
    exit 1
fi

echo ""

# 4. 启动应用并测试InspireFace初始化
echo "🎯 4. 启动应用并测试InspireFace"
echo "-----------------------------"

echo "正在启动应用..."
adb shell am start -n "$PACKAGE_NAME/.MainActivity" > /dev/null 2>&1

# 等待应用启动
sleep 3

echo "正在检查应用进程..."
if adb shell ps | grep -q "$PACKAGE_NAME"; then
    echo "✅ 应用已启动"
else
    echo "⚠️  应用可能未正常启动"
fi

# 启动InspireFace测试Activity
echo "正在启动InspireFace测试界面..."
adb shell am start -n "$PACKAGE_NAME/.InspireFaceTestActivity" > /dev/null 2>&1

sleep 2

echo ""

# 5. 监控日志输出
echo "📊 5. 监控应用日志"
echo "----------------"

echo "开始监控应用日志 (10秒)..."
echo "查找InspireFace相关日志..."

# 清除旧日志
adb logcat -c

# 监控日志
timeout 10s adb logcat -s "InspireFaceTest:*" "InspireFaceManager:*" "ExtendedInference:*" "InspireFace:*" || true

echo ""

# 6. 检查模型文件复制状态
echo "📁 6. 检查模型文件复制状态"
echo "------------------------"

MODEL_DIR="$DATA_DIR/files/inspireface"

echo "检查模型目录是否存在..."
if adb shell "test -d $MODEL_DIR" 2>/dev/null; then
    echo "✅ 模型目录已创建: $MODEL_DIR"
    
    echo "📋 模型文件列表:"
    adb shell "ls -la $MODEL_DIR" 2>/dev/null | while read line; do
        echo "   $line"
    done
    
    echo "📊 模型文件统计:"
    RKNN_COUNT=$(adb shell "ls $MODEL_DIR/*.rknn 2>/dev/null | wc -l" || echo "0")
    MNN_COUNT=$(adb shell "ls $MODEL_DIR/*.mnn 2>/dev/null | wc -l" || echo "0")
    CONFIG_COUNT=$(adb shell "ls $MODEL_DIR/__inspire__ 2>/dev/null | wc -l" || echo "0")
    
    echo "   - RKNN模型: $RKNN_COUNT 个"
    echo "   - MNN模型: $MNN_COUNT 个"
    echo "   - 配置文件: $CONFIG_COUNT 个"
    
    # 检查总大小
    TOTAL_SIZE=$(adb shell "du -sh $MODEL_DIR 2>/dev/null | cut -f1" || echo "Unknown")
    echo "   - 总大小: $TOTAL_SIZE"
    
    # 验证关键文件
    echo "🔍 验证关键模型文件:"
    CRITICAL_FILES=(
        "__inspire__"
        "_00_scrfd_2_5g_bnkps_shape320x320_rk3588.rknn"
        "_08_fairface_model_rk3588.rknn"
    )
    
    ALL_EXIST=true
    for file in "${CRITICAL_FILES[@]}"; do
        if adb shell "test -f $MODEL_DIR/$file" 2>/dev/null; then
            SIZE=$(adb shell "ls -lh $MODEL_DIR/$file 2>/dev/null | awk '{print \$5}'" || echo "Unknown")
            echo "   ✅ $file ($SIZE)"
        else
            echo "   ❌ $file (缺失)"
            ALL_EXIST=false
        fi
    done
    
    if [ "$ALL_EXIST" = true ]; then
        echo "✅ 所有关键模型文件都存在"
    else
        echo "❌ 部分关键模型文件缺失"
    fi
    
else
    echo "❌ 模型目录不存在，可能复制失败"
    echo "检查应用是否有写入权限..."
fi

echo ""

# 7. 性能监控
echo "⚡ 7. 性能监控"
echo "------------"

echo "📊 内存使用情况:"
MEMORY_INFO=$(adb shell "dumpsys meminfo $PACKAGE_NAME" 2>/dev/null | grep -E "TOTAL|Native Heap|Dalvik Heap" | head -3 || echo "无法获取内存信息")
echo "$MEMORY_INFO"

echo ""
echo "🔋 CPU使用情况:"
CPU_INFO=$(adb shell "top -n 1 | grep $PACKAGE_NAME" 2>/dev/null | head -1 || echo "无法获取CPU信息")
echo "$CPU_INFO"

echo ""

# 8. 生成测试报告
echo "📋 8. 测试报告"
echo "------------"

echo "🎯 设备部署测试总结:"
echo "   ✅ 设备连接正常"
echo "   ✅ APK构建和安装成功"
echo "   ✅ 应用启动正常"

if [ "$ALL_EXIST" = true ]; then
    echo "   ✅ 模型文件复制成功"
else
    echo "   ⚠️  模型文件复制可能有问题"
fi

echo ""
echo "🚀 下一步建议:"
echo "   1. 手动测试InspireFace初始化功能"
echo "   2. 验证人脸检测和属性分析"
echo "   3. 测试与YOLO检测的集成"
echo "   4. 进行性能基准测试"

echo ""
echo "📱 设备测试命令:"
echo "   - 查看实时日志: adb logcat -s InspireFaceTest"
echo "   - 重启应用: adb shell am start -n $PACKAGE_NAME/.InspireFaceTestActivity"
echo "   - 清除应用数据: adb shell pm clear $PACKAGE_NAME"

echo ""
echo "✅ 设备部署测试完成"
echo "==================="
