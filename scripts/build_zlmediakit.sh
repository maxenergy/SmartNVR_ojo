#!/bin/bash

# ZLMediaKit编译脚本 for Android arm64-v8a (RK3588)
# 作者: Augment Agent
# 日期: 2025-07-14

set -e

# 配置变量
PROJECT_ROOT=$(cd "$(dirname "$0")/.." && pwd)
ZLMEDIAKIT_VERSION="master"
ZLMEDIAKIT_SOURCE_DIR="$PROJECT_ROOT/temp/ZLMediaKit"
INSTALL_DIR="$PROJECT_ROOT/3rdparty/zlmediakit"
ANDROID_SDK_ROOT="/home/rogers/Android/Sdk"
ANDROID_NDK_ROOT="$ANDROID_SDK_ROOT/ndk/25.1.8937393"
ANDROID_API_LEVEL=28
ANDROID_ABI="arm64-v8a"
ANDROID_ARCH="aarch64"

# 颜色输出
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
NC='\033[0m' # No Color

log_info() {
    echo -e "${GREEN}[INFO]${NC} $1"
}

log_warn() {
    echo -e "${YELLOW}[WARN]${NC} $1"
}

log_error() {
    echo -e "${RED}[ERROR]${NC} $1"
}

# 检查依赖
check_dependencies() {
    log_info "检查编译依赖..."
    
    if [ ! -d "$ANDROID_NDK_ROOT" ]; then
        log_error "Android NDK未找到: $ANDROID_NDK_ROOT"
        exit 1
    fi
    
    # 检查必要工具
    for tool in git cmake make; do
        if ! command -v $tool &> /dev/null; then
            log_error "缺少必要工具: $tool"
            exit 1
        fi
    done
    
    log_info "依赖检查完成"
}

# 下载ZLMediaKit源码
download_zlmediakit() {
    log_info "下载ZLMediaKit源码..."
    
    mkdir -p "$PROJECT_ROOT/temp"
    cd "$PROJECT_ROOT/temp"
    
    if [ ! -d "ZLMediaKit" ]; then
        log_info "克隆ZLMediaKit仓库..."
        git clone --recursive https://github.com/ZLMediaKit/ZLMediaKit.git
    else
        log_info "ZLMediaKit源码已存在，更新代码..."
        cd ZLMediaKit
        git pull
        git submodule update --init --recursive
        cd ..
    fi
}

# 配置交叉编译环境
setup_cross_compile() {
    log_info "配置交叉编译环境..."
    
    export ANDROID_NDK_HOME="$ANDROID_NDK_ROOT"
    export TOOLCHAIN="$ANDROID_NDK_ROOT/toolchains/llvm/prebuilt/linux-x86_64"
    export TARGET="aarch64-linux-android"
    export API="$ANDROID_API_LEVEL"
    
    export CMAKE_TOOLCHAIN_FILE="$ANDROID_NDK_ROOT/build/cmake/android.toolchain.cmake"
    
    log_info "交叉编译环境配置完成"
}

# 编译ZLMediaKit
compile_zlmediakit() {
    log_info "开始编译ZLMediaKit..."
    
    cd "$ZLMEDIAKIT_SOURCE_DIR"
    
    # 创建构建目录
    BUILD_DIR="build_android_arm64"
    rm -rf "$BUILD_DIR"
    mkdir -p "$BUILD_DIR"
    cd "$BUILD_DIR"
    
    log_info "配置CMake构建选项..."
    cmake .. \
        -DCMAKE_TOOLCHAIN_FILE="$CMAKE_TOOLCHAIN_FILE" \
        -DANDROID_ABI="$ANDROID_ABI" \
        -DANDROID_PLATFORM="android-$ANDROID_API_LEVEL" \
        -DANDROID_NDK="$ANDROID_NDK_ROOT" \
        -DCMAKE_BUILD_TYPE=Release \
        -DCMAKE_INSTALL_PREFIX="$INSTALL_DIR" \
        -DENABLE_TESTS=OFF \
        -DENABLE_API=ON \
        -DENABLE_RTPPROXY=ON \
        -DENABLE_RTSP=ON \
        -DENABLE_RTMP=ON \
        -DENABLE_HLS=ON \
        -DENABLE_MP4=ON \
        -DENABLE_WEBRTC=ON \
        -DENABLE_SRT=ON \
        -DENABLE_FFMPEG=OFF \
        -DENABLE_OPENSSL=OFF \
        -DENABLE_MYSQL=OFF \
        -DENABLE_FAAC=OFF \
        -DENABLE_X264=OFF \
        -DENABLE_SCTP=OFF
    
    log_info "开始编译（这可能需要一些时间）..."
    make -j$(nproc)
    
    log_info "安装ZLMediaKit到3rdparty目录..."
    make install
    
    log_info "ZLMediaKit编译完成"
}

# 创建符合项目需求的目录结构
organize_installation() {
    log_info "整理安装目录结构..."
    
    # 确保目录存在
    mkdir -p "$INSTALL_DIR"/{include,lib,bin}
    
    # 复制头文件
    if [ -d "$ZLMEDIAKIT_SOURCE_DIR/src" ]; then
        cp -r "$ZLMEDIAKIT_SOURCE_DIR/src"/* "$INSTALL_DIR/include/" 2>/dev/null || true
    fi
    
    # 复制API头文件
    if [ -d "$ZLMEDIAKIT_SOURCE_DIR/api/include" ]; then
        cp -r "$ZLMEDIAKIT_SOURCE_DIR/api/include"/* "$INSTALL_DIR/include/" 2>/dev/null || true
    fi
    
    # 复制库文件
    find "$ZLMEDIAKIT_SOURCE_DIR/build_android_arm64" -name "*.so" -exec cp {} "$INSTALL_DIR/lib/" \; 2>/dev/null || true
    find "$ZLMEDIAKIT_SOURCE_DIR/build_android_arm64" -name "*.a" -exec cp {} "$INSTALL_DIR/lib/" \; 2>/dev/null || true
    
    log_info "目录结构整理完成"
}

# 验证编译结果
verify_build() {
    log_info "验证编译结果..."
    
    if [ ! -d "$INSTALL_DIR/lib" ] || [ ! -d "$INSTALL_DIR/include" ]; then
        log_error "编译失败：缺少lib或include目录"
        exit 1
    fi
    
    # 检查关键库文件
    if [ ! -f "$INSTALL_DIR/lib/libmk_api.so" ] && [ ! -f "$INSTALL_DIR/lib/libmk_api.a" ]; then
        log_error "编译失败：缺少 mk_api 库"
        exit 1
    fi
    
    log_info "编译验证成功"
    log_info "ZLMediaKit库已安装到: $INSTALL_DIR"
    
    # 显示安装的文件
    log_info "安装的库文件:"
    ls -la "$INSTALL_DIR/lib/" 2>/dev/null || true
}

# 清理临时文件
cleanup() {
    log_info "清理临时文件..."
    rm -rf "$ZLMEDIAKIT_SOURCE_DIR/build_android_arm64"
    log_info "清理完成"
}

# 主函数
main() {
    log_info "开始编译ZLMediaKit for Android arm64-v8a"
    log_info "目标设备: RK3588"
    log_info "Android API Level: $ANDROID_API_LEVEL"
    
    check_dependencies
    download_zlmediakit
    setup_cross_compile
    compile_zlmediakit
    organize_installation
    verify_build
    
    log_info "ZLMediaKit编译流程完成！"
    log_warn "注意：如果不需要保留构建文件，可以运行 'cleanup' 函数清理临时文件"
}

# 如果直接运行脚本
if [[ "${BASH_SOURCE[0]}" == "${0}" ]]; then
    main "$@"
fi
