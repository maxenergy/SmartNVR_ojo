#!/bin/bash

# YOLOv8n配置验证脚本
# 验证模型文件和配置是否正确匹配

set -e

# 颜色定义
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m'

log_info() { echo -e "${BLUE}[INFO]${NC} $1"; }
log_success() { echo -e "${GREEN}[SUCCESS]${NC} $1"; }
log_warning() { echo -e "${YELLOW}[WARNING]${NC} $1"; }
log_error() { echo -e "${RED}[ERROR]${NC} $1"; }

# 验证本地模型文件
verify_local_files() {
    log_info "验证本地模型文件..."
    
    if [ -f "best_rk3588.rknn" ]; then
        SIZE=$(stat -c%s "best_rk3588.rknn" 2>/dev/null || echo "0")
        log_success "✓ 模型文件存在: best_rk3588.rknn (${SIZE} bytes)"
    else
        log_error "✗ 模型文件不存在: best_rk3588.rknn"
        return 1
    fi
    
    if [ -f "best_labels.txt" ]; then
        LINES=$(wc -l < "best_labels.txt" 2>/dev/null || echo "0")
        log_success "✓ 标签文件存在: best_labels.txt (${LINES} 行)"
        
        log_info "标签内容:"
        cat "best_labels.txt" | while read line; do
            if [ -n "$line" ]; then
                echo "  - $line"
            fi
        done
    else
        log_error "✗ 标签文件不存在: best_labels.txt"
        return 1
    fi
}

# 验证代码配置
verify_code_config() {
    log_info "验证代码配置..."
    
    # 检查model_config.h
    if grep -q "best_rk3588.rknn" app/src/main/cpp/types/model_config.h; then
        log_success "✓ model_config.h 已更新模型路径"
    else
        log_warning "⚠ model_config.h 可能未更新模型路径"
    fi
    
    if grep -q "num_classes = 1" app/src/main/cpp/types/model_config.h; then
        log_success "✓ model_config.h 已设置为单类别检测"
    else
        log_warning "⚠ model_config.h 类别数量可能不正确"
    fi
    
    # 检查yolov8_postprocess.h
    if grep -q "YOLOV8_OBJ_CLASS_NUM.*1" app/src/main/cpp/process/yolov8_postprocess.h; then
        log_success "✓ yolov8_postprocess.h 已设置为单类别"
    else
        log_warning "⚠ yolov8_postprocess.h 类别数量可能不正确"
    fi
    
    # 检查yolov8_postprocess.cpp
    if grep -q '"person"' app/src/main/cpp/process/yolov8_postprocess.cpp; then
        log_success "✓ yolov8_postprocess.cpp 已设置person类别"
    else
        log_warning "⚠ yolov8_postprocess.cpp 类别名称可能不正确"
    fi
}

# 验证设备上的文件
verify_device_files() {
    log_info "验证设备上的文件..."
    
    if ! command -v adb &> /dev/null; then
        log_warning "ADB未找到，跳过设备验证"
        return 0
    fi
    
    DEVICE_COUNT=$(adb devices | grep -v "List of devices" | grep -c "device$" || true)
    if [ "$DEVICE_COUNT" -eq 0 ]; then
        log_warning "未找到连接的设备，跳过设备验证"
        return 0
    fi
    
    APP_DATA_DIR="/data/data/com.wulala.myyolov5rtspthreadpool/files"
    
    # 检查模型文件
    if adb shell "test -f $APP_DATA_DIR/best_rk3588.rknn" 2>/dev/null; then
        REMOTE_SIZE=$(adb shell "stat -c%s $APP_DATA_DIR/best_rk3588.rknn" 2>/dev/null || echo "0")
        log_success "✓ 设备上模型文件存在 (${REMOTE_SIZE} bytes)"
    else
        log_warning "⚠ 设备上模型文件不存在，需要部署"
    fi
    
    # 检查标签文件
    if adb shell "test -f $APP_DATA_DIR/best_labels.txt" 2>/dev/null; then
        log_success "✓ 设备上标签文件存在"
    else
        log_warning "⚠ 设备上标签文件不存在，需要部署"
    fi
}

# 验证编译状态
verify_build_status() {
    log_info "验证编译状态..."
    
    if [ -f "app/build/outputs/apk/debug/app-debug.apk" ]; then
        APK_SIZE=$(stat -c%s "app/build/outputs/apk/debug/app-debug.apk" 2>/dev/null || echo "0")
        log_success "✓ APK文件存在 (${APK_SIZE} bytes)"
        
        # 检查APK中的native库
        if unzip -l "app/build/outputs/apk/debug/app-debug.apk" | grep -q "libmyyolov5rtspthreadpool.so"; then
            log_success "✓ Native库已包含在APK中"
        else
            log_error "✗ Native库未找到在APK中"
        fi
    else
        log_warning "⚠ APK文件不存在，需要重新编译"
    fi
}

# 生成配置报告
generate_config_report() {
    log_info "生成配置验证报告..."
    
    REPORT_FILE="yolov8n_config_verification_$(date +%Y%m%d_%H%M%S).txt"
    
    cat > "$REPORT_FILE" << EOF
YOLOv8n配置验证报告
==================

验证时间: $(date)

本地文件检查:
$([ -f "best_rk3588.rknn" ] && echo "✓ best_rk3588.rknn 存在" || echo "✗ best_rk3588.rknn 不存在")
$([ -f "best_labels.txt" ] && echo "✓ best_labels.txt 存在" || echo "✗ best_labels.txt 不存在")

代码配置检查:
$(grep -q "best_rk3588.rknn" app/src/main/cpp/types/model_config.h && echo "✓ 模型路径已更新" || echo "✗ 模型路径未更新")
$(grep -q "num_classes = 1" app/src/main/cpp/types/model_config.h && echo "✓ 类别数量正确" || echo "✗ 类别数量不正确")
$(grep -q "YOLOV8_OBJ_CLASS_NUM.*1" app/src/main/cpp/process/yolov8_postprocess.h && echo "✓ 后处理类别数量正确" || echo "✗ 后处理类别数量不正确")

编译状态:
$([ -f "app/build/outputs/apk/debug/app-debug.apk" ] && echo "✓ APK已编译" || echo "✗ APK未编译")

模型配置:
- 模型文件: best_rk3588.rknn
- 类别数量: 1 (person)
- 输入尺寸: 640x640x3
- 置信度阈值: 0.5
- NMS阈值: 0.6

建议操作:
1. 如果本地文件缺失，请确保模型文件在当前目录
2. 如果代码配置不正确，请重新运行集成脚本
3. 如果APK未编译，请运行 ./gradlew assembleDebug
4. 使用 ./scripts/deploy_yolov8n_model.sh 部署模型文件

EOF
    
    log_success "配置验证报告已生成: $REPORT_FILE"
}

# 主函数
main() {
    log_info "开始YOLOv8n配置验证..."
    echo "========================================"
    
    verify_local_files
    verify_code_config
    verify_device_files
    verify_build_status
    
    echo "========================================"
    log_success "YOLOv8n配置验证完成！"
    
    generate_config_report
}

# 脚本入口
if [[ "${BASH_SOURCE[0]}" == "${0}" ]]; then
    main "$@"
fi
