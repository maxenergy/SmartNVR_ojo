#!/bin/bash

# YOLOv8n自定义模型部署脚本
# 用于将转换好的YOLOv8n模型文件部署到Android设备

set -e

# 颜色定义
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

# 日志函数
log_info() {
    echo -e "${BLUE}[INFO]${NC} $1"
}

log_success() {
    echo -e "${GREEN}[SUCCESS]${NC} $1"
}

log_warning() {
    echo -e "${YELLOW}[WARNING]${NC} $1"
}

log_error() {
    echo -e "${RED}[ERROR]${NC} $1"
}

# 配置变量
PACKAGE_NAME="com.wulala.myyolov5rtspthreadpool"
APP_DATA_DIR="/data/data/${PACKAGE_NAME}/files"
MODEL_FILE="best_rk3588.rknn"
LABELS_FILE="best_labels.txt"

# 检查设备连接
check_device() {
    log_info "检查设备连接..."
    
    if ! command -v adb &> /dev/null; then
        log_error "ADB未找到，请安装Android SDK"
        exit 1
    fi
    
    DEVICE_COUNT=$(adb devices | grep -v "List of devices" | grep -c "device$" || true)
    
    if [ "$DEVICE_COUNT" -eq 0 ]; then
        log_error "未找到连接的Android设备"
        exit 1
    elif [ "$DEVICE_COUNT" -gt 1 ]; then
        log_warning "发现多个设备，使用第一个设备"
    fi
    
    DEVICE_MODEL=$(adb shell getprop ro.product.model 2>/dev/null || echo "Unknown")
    log_success "设备已连接: $DEVICE_MODEL"
}

# 检查模型文件
check_model_files() {
    log_info "检查模型文件..."
    
    if [ ! -f "$MODEL_FILE" ]; then
        log_error "模型文件不存在: $MODEL_FILE"
        log_info "请确保 $MODEL_FILE 文件在当前目录中"
        exit 1
    fi
    
    if [ ! -f "$LABELS_FILE" ]; then
        log_error "标签文件不存在: $LABELS_FILE"
        log_info "请确保 $LABELS_FILE 文件在当前目录中"
        exit 1
    fi
    
    MODEL_SIZE=$(stat -c%s "$MODEL_FILE" 2>/dev/null || echo "0")
    LABELS_SIZE=$(stat -c%s "$LABELS_FILE" 2>/dev/null || echo "0")
    
    log_success "模型文件: $MODEL_FILE (${MODEL_SIZE} bytes)"
    log_success "标签文件: $LABELS_FILE (${LABELS_SIZE} bytes)"
}

# 检查应用是否安装
check_app_installed() {
    log_info "检查应用安装状态..."
    
    if adb shell pm list packages | grep -q "$PACKAGE_NAME"; then
        log_success "应用已安装: $PACKAGE_NAME"
        
        # 获取应用版本
        VERSION=$(adb shell dumpsys package "$PACKAGE_NAME" | grep "versionName" | head -1 | cut -d'=' -f2 || echo "Unknown")
        log_info "应用版本: $VERSION"
    else
        log_error "应用未安装: $PACKAGE_NAME"
        log_info "请先安装APK文件"
        exit 1
    fi
}

# 创建应用数据目录
create_app_directory() {
    log_info "创建应用数据目录..."
    
    # 检查目录是否存在
    if adb shell "test -d $APP_DATA_DIR" 2>/dev/null; then
        log_success "应用数据目录已存在: $APP_DATA_DIR"
    else
        log_info "创建应用数据目录: $APP_DATA_DIR"
        adb shell "mkdir -p $APP_DATA_DIR" || {
            log_error "无法创建目录，可能需要root权限"
            exit 1
        }
    fi
}

# 部署模型文件
deploy_model_file() {
    log_info "部署YOLOv8n模型文件..."
    
    # 推送模型文件
    log_info "推送模型文件: $MODEL_FILE"
    if adb push "$MODEL_FILE" "$APP_DATA_DIR/" > /dev/null 2>&1; then
        log_success "模型文件部署成功"
    else
        log_error "模型文件部署失败"
        exit 1
    fi
    
    # 设置文件权限
    adb shell "chmod 644 $APP_DATA_DIR/$MODEL_FILE" 2>/dev/null || true
    
    # 验证文件
    REMOTE_SIZE=$(adb shell "stat -c%s $APP_DATA_DIR/$MODEL_FILE" 2>/dev/null || echo "0")
    LOCAL_SIZE=$(stat -c%s "$MODEL_FILE" 2>/dev/null || echo "0")
    
    if [ "$REMOTE_SIZE" = "$LOCAL_SIZE" ]; then
        log_success "模型文件验证成功 (${REMOTE_SIZE} bytes)"
    else
        log_error "模型文件验证失败 (本地: ${LOCAL_SIZE}, 远程: ${REMOTE_SIZE})"
        exit 1
    fi
}

# 部署标签文件
deploy_labels_file() {
    log_info "部署标签文件..."
    
    # 推送标签文件
    log_info "推送标签文件: $LABELS_FILE"
    if adb push "$LABELS_FILE" "$APP_DATA_DIR/" > /dev/null 2>&1; then
        log_success "标签文件部署成功"
    else
        log_error "标签文件部署失败"
        exit 1
    fi
    
    # 设置文件权限
    adb shell "chmod 644 $APP_DATA_DIR/$LABELS_FILE" 2>/dev/null || true
    
    # 显示标签内容
    log_info "标签文件内容:"
    adb shell "cat $APP_DATA_DIR/$LABELS_FILE" 2>/dev/null | while read line; do
        if [ -n "$line" ]; then
            echo "  - $line"
        fi
    done
}

# 验证部署结果
verify_deployment() {
    log_info "验证部署结果..."
    
    # 列出应用数据目录中的文件
    log_info "应用数据目录文件列表:"
    adb shell "ls -la $APP_DATA_DIR/" 2>/dev/null | grep -E "\\.rknn$|\\.txt$" | while read line; do
        echo "  $line"
    done
    
    # 检查模型文件
    if adb shell "test -f $APP_DATA_DIR/$MODEL_FILE" 2>/dev/null; then
        log_success "✓ YOLOv8n模型文件已部署"
    else
        log_error "✗ YOLOv8n模型文件未找到"
    fi
    
    # 检查标签文件
    if adb shell "test -f $APP_DATA_DIR/$LABELS_FILE" 2>/dev/null; then
        log_success "✓ 标签文件已部署"
    else
        log_error "✗ 标签文件未找到"
    fi
}

# 重启应用
restart_app() {
    log_info "重启应用以加载新模型..."
    
    # 停止应用
    adb shell am force-stop "$PACKAGE_NAME" 2>/dev/null || true
    sleep 2
    
    # 启动应用
    adb shell am start -n "${PACKAGE_NAME}/.MainActivity" 2>/dev/null || true
    
    log_success "应用已重启"
}

# 生成部署报告
generate_report() {
    log_info "生成部署报告..."
    
    REPORT_FILE="yolov8n_deployment_report_$(date +%Y%m%d_%H%M%S).txt"
    
    cat > "$REPORT_FILE" << EOF
YOLOv8n自定义模型部署报告
========================

部署时间: $(date)
设备信息: $(adb shell getprop ro.product.model 2>/dev/null || echo "Unknown")
Android版本: $(adb shell getprop ro.build.version.release 2>/dev/null || echo "Unknown")

部署文件:
- 模型文件: $MODEL_FILE ($(stat -c%s "$MODEL_FILE" 2>/dev/null || echo "0") bytes)
- 标签文件: $LABELS_FILE ($(stat -c%s "$LABELS_FILE" 2>/dev/null || echo "0") bytes)

部署路径: $APP_DATA_DIR/

模型配置:
- 类别数量: 1 (person)
- 输入尺寸: 640x640x3
- 置信度阈值: 0.5
- NMS阈值: 0.6

部署状态: 成功

下一步:
1. 启动应用测试YOLOv8n功能
2. 使用Java接口切换到YOLOv8n模型
3. 验证人员检测功能

EOF
    
    log_success "部署报告已生成: $REPORT_FILE"
}

# 主函数
main() {
    log_info "开始YOLOv8n自定义模型部署..."
    echo "========================================"
    
    # 执行部署步骤
    check_device
    check_model_files
    check_app_installed
    create_app_directory
    deploy_model_file
    deploy_labels_file
    verify_deployment
    restart_app
    
    echo "========================================"
    log_success "YOLOv8n自定义模型部署完成！"
    
    generate_report
    
    echo ""
    log_info "使用方法:"
    echo "  1. 启动应用"
    echo "  2. 调用 setInferenceModel(cameraIndex, MODEL_YOLOV8N)"
    echo "  3. 验证人员检测功能"
}

# 脚本入口
if [[ "${BASH_SOURCE[0]}" == "${0}" ]]; then
    main "$@"
fi
