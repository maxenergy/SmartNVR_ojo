#!/bin/bash

# FFmpeg编译脚本 for Android arm64-v8a (RK3588)
# 作者: Augment Agent
# 日期: 2025-07-14

set -e

# 配置变量
PROJECT_ROOT=$(cd "$(dirname "$0")/.." && pwd)
FFMPEG_VERSION="6.1.1"
FFMPEG_SOURCE_DIR="$PROJECT_ROOT/temp/ffmpeg-$FFMPEG_VERSION"
INSTALL_DIR="$PROJECT_ROOT/3rdparty/ffmpeg"
ANDROID_SDK_ROOT="/home/rogers/Android/Sdk"
ANDROID_NDK_ROOT="$ANDROID_SDK_ROOT/ndk/27.0.12077973"
ANDROID_API_LEVEL=31
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
    for tool in wget tar make; do
        if ! command -v $tool &> /dev/null; then
            log_error "缺少必要工具: $tool"
            exit 1
        fi
    done
    
    log_info "依赖检查完成"
}

# 下载FFmpeg源码
download_ffmpeg() {
    log_info "下载FFmpeg源码..."
    
    mkdir -p "$PROJECT_ROOT/temp"
    cd "$PROJECT_ROOT/temp"
    
    if [ ! -f "ffmpeg-$FFMPEG_VERSION.tar.xz" ]; then
        log_info "下载FFmpeg $FFMPEG_VERSION..."
        wget --no-check-certificate "https://ffmpeg.org/releases/ffmpeg-$FFMPEG_VERSION.tar.xz"
    else
        log_info "FFmpeg源码已存在，跳过下载"
    fi
    
    if [ ! -d "ffmpeg-$FFMPEG_VERSION" ]; then
        log_info "解压FFmpeg源码..."
        tar -xf "ffmpeg-$FFMPEG_VERSION.tar.xz"
    else
        log_info "FFmpeg源码已解压，跳过解压"
    fi
}

# 配置交叉编译环境
setup_cross_compile() {
    log_info "配置交叉编译环境..."
    
    export ANDROID_NDK_HOME="$ANDROID_NDK_ROOT"
    export TOOLCHAIN="$ANDROID_NDK_ROOT/toolchains/llvm/prebuilt/linux-x86_64"
    export TARGET="aarch64-linux-android"
    export API="$ANDROID_API_LEVEL"
    
    export AR="$TOOLCHAIN/bin/llvm-ar"
    export CC="$TOOLCHAIN/bin/$TARGET$API-clang"
    export AS="$CC"
    export CXX="$TOOLCHAIN/bin/$TARGET$API-clang++"
    export LD="$TOOLCHAIN/bin/ld"
    export RANLIB="$TOOLCHAIN/bin/llvm-ranlib"
    export STRIP="$TOOLCHAIN/bin/llvm-strip"
    export NM="$TOOLCHAIN/bin/llvm-nm"
    
    export CFLAGS="-O3 -fPIC"
    export CXXFLAGS="-O3 -fPIC"
    export LDFLAGS=""
    
    log_info "交叉编译环境配置完成"
}

# 编译FFmpeg
compile_ffmpeg() {
    log_info "开始编译FFmpeg..."
    
    cd "$FFMPEG_SOURCE_DIR"
    
    # 清理之前的构建
    if [ -f "Makefile" ]; then
        make clean || true
    fi
    
    log_info "配置FFmpeg构建选项..."
    ./configure \
        --prefix="$INSTALL_DIR" \
        --enable-cross-compile \
        --target-os=android \
        --arch=aarch64 \
        --cpu=armv8-a \
        --cross-prefix="$TOOLCHAIN/bin/llvm-" \
        --cc="$CC" \
        --cxx="$CXX" \
        --enable-shared \
        --disable-static \
        --disable-doc \
        --disable-programs \
        --disable-avdevice \
        --disable-postproc \
        --disable-avfilter \
        --enable-avformat \
        --enable-avcodec \
        --enable-swresample \
        --enable-swscale \
        --enable-protocol=file \
        --enable-protocol=rtmp \
        --enable-protocol=rtsp \
        --enable-protocol=tcp \
        --enable-protocol=udp \
        --enable-protocol=http \
        --enable-protocol=https \
        --enable-demuxer=rtsp \
        --enable-demuxer=flv \
        --enable-demuxer=h264 \
        --enable-demuxer=hevc \
        --enable-decoder=h264 \
        --enable-decoder=hevc \
        --enable-decoder=aac \
        --enable-encoder=h264 \
        --enable-encoder=hevc \
        --enable-encoder=aac \
        --enable-muxer=mp4 \
        --enable-muxer=flv \
        --enable-muxer=rtsp \
        --disable-debug \
        --extra-cflags="$CFLAGS" \
        --extra-cxxflags="$CXXFLAGS" \
        --extra-ldflags="$LDFLAGS"
    
    log_info "开始编译（这可能需要一些时间）..."
    make -j$(nproc)
    
    log_info "安装FFmpeg到3rdparty目录..."
    make install
    
    log_info "FFmpeg编译完成"
}

# 验证编译结果
verify_build() {
    log_info "验证编译结果..."
    
    if [ ! -d "$INSTALL_DIR/lib" ] || [ ! -d "$INSTALL_DIR/include" ]; then
        log_error "编译失败：缺少lib或include目录"
        exit 1
    fi
    
    # 检查关键库文件
    for lib in libavformat libavcodec libavutil libswresample libswscale; do
        if [ ! -f "$INSTALL_DIR/lib/${lib}.so" ]; then
            log_error "编译失败：缺少 ${lib}.so"
            exit 1
        fi
    done
    
    log_info "编译验证成功"
    log_info "FFmpeg库已安装到: $INSTALL_DIR"
}

# 清理临时文件
cleanup() {
    log_info "清理临时文件..."
    rm -rf "$PROJECT_ROOT/temp/ffmpeg-$FFMPEG_VERSION"
    log_info "清理完成"
}

# 主函数
main() {
    log_info "开始编译FFmpeg for Android arm64-v8a"
    log_info "目标设备: RK3588"
    log_info "Android API Level: $ANDROID_API_LEVEL"
    
    check_dependencies
    download_ffmpeg
    setup_cross_compile
    compile_ffmpeg
    verify_build
    
    log_info "FFmpeg编译流程完成！"
    log_warn "注意：如果不需要保留源码，可以运行 'cleanup' 函数清理临时文件"
}

# 如果直接运行脚本
if [[ "${BASH_SOURCE[0]}" == "${0}" ]]; then
    main "$@"
fi
