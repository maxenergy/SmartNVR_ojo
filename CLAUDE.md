# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Project Overview

This is an Android project that implements YOLOv5 object detection with RTSP streaming capabilities optimized for Rockchip RK3588 hardware. The project combines:

- **YOLOv5 inference** using RKNN (Rockchip Neural Network) runtime
- **RTSP streaming** via ZLMediaKit for real-time video processing
- **Hardware acceleration** using Rockchip's MPP (Media Process Platform) and RGA (2D Graphics Acceleration)
- **Thread pool architecture** for efficient parallel processing
- **Android JNI integration** for native C++ implementation

## Build Commands

### Initial Setup
```bash
# Build FFmpeg for Android arm64-v8a
./scripts/build_ffmpeg.sh

# Build ZLMediaKit for Android arm64-v8a  
./scripts/build_zlmediakit.sh
```

### Android Build
```bash
# Build the Android project
./gradlew assembleDebug

# Install on connected device
./gradlew installDebug

# Clean build
./gradlew clean

# Run memory monitoring for debugging
./memory_monitor_zlmediakit.sh
```

### Testing and Debugging
```bash
# Monitor application memory usage
./memory_monitor_zlmediakit.sh

# Check for memory leaks in logs
adb logcat | grep -E "(memory|leak|oom)"

# Debug native crashes
adb logcat | grep -E "(FATAL|SIGSEGV|tombstone)"
```

## Architecture

### Core Components

1. **ZLPlayer (`src/ZLPlayer.cpp`)**: Main player class that orchestrates RTSP streaming and video processing
   - Manages MPP decoder for H.264/H.265 video streams
   - Coordinates with YOLOv5 thread pool for object detection
   - Handles frame callbacks and display queue

2. **YOLOv5 Thread Pool (`task/yolov5_thread_pool.h/cpp`)**: 
   - Manages multiple YOLOv5 inference instances (up to 20 threads)
   - Processes frames in parallel using thread pool pattern
   - Handles frame queuing and result synchronization

3. **RKNN Engine (`engine/rknn_engine.h/cpp`)**: 
   - Wrapper for Rockchip RKNN runtime
   - Loads quantized YOLOv5 models (.rknn files)
   - Provides inference interface for neural network operations

4. **MPP Decoder (`rkmedia/utils/mpp_decoder.h/cpp`)**:
   - Hardware-accelerated video decoding using Rockchip MPP
   - Supports H.264/H.265 formats
   - Integrates with RGA for format conversion

### Key Dependencies

- **3rdparty/ffmpeg/**: FFmpeg libraries for media handling (built from scripts/build_ffmpeg.sh)
- **3rdparty/zlmediakit/**: ZLMediaKit for RTSP/streaming protocols (built from scripts/build_zlmediakit.sh)
- **rknn/**: Rockchip RKNN runtime for neural network inference
- **opencv/**: OpenCV 4.8 for computer vision operations
- **mpp/**: Rockchip Media Process Platform for hardware acceleration
- **rga/**: Rockchip 2D Graphics Acceleration library

### CMake Configuration

The project uses CMake with specific settings for Android arm64-v8a:
- **NDK Version**: 27.0.12077973
- **Minimum API**: 31 (Android 12)
- **C++ Standard**: C++11
- **Target Architecture**: arm64-v8a only

Key CMake variables (app/src/main/cpp/CMakeLists.txt):
```cmake
set(THIRD_PARTY /home/rogers/source/rockchip/yolov5rtspthreadpool/3rdparty)
set(FFMPEG ${THIRD_PARTY}/ffmpeg)
set(ZLMEDIAKIT ${THIRD_PARTY}/zlmediakit)
```

### Data Flow

1. RTSP stream → ZLMediaKit → MPP Decoder → Frame Data
2. Frame Data → YOLOv5 Thread Pool → RKNN Engine → Detection Results
3. Detection Results → OpenCV Drawing → Display Queue → Android Surface

## Development Notes

### Model Files
- YOLOv5 models should be in RKNN format (.rknn files)
- Models are loaded from Android assets via JNI
- Model quantization is handled by RKNN toolkit

### Thread Management
- Maximum 22 YOLOv5 inference threads (configurable in `yolov5_thread_pool.h`)
- Separate threads for RTSP processing and display rendering
- Thread-safe queues for frame and result management

### Hardware Optimization
- RGA used for efficient format conversion and scaling
- MPP provides hardware-accelerated video decoding
- RKNN runtime optimized for Rockchip NPU acceleration

### JNI Interface
- `native-lib.cpp` provides Android integration points
- Asset manager integration for model file loading
- Native window handling for video display

### Memory Management
Recent commits indicate ongoing memory optimization work:
- ZLMediaKit version fixes for memory leaks
- Memory monitoring scripts available (`memory_monitor_zlmediakit.sh`)
- CSV logging for memory usage tracking

## Common Issues

### Build Issues
- Ensure Android NDK 27.0.12077973 is installed
- Check CMake toolchain file paths in CMakeLists.txt
- Verify ZLMediaKit path configuration (line 72 in CMakeLists.txt: `${ZLMEDIAKIT}/lib/libmk_api.so`)
- Run dependency build scripts first: `./scripts/build_ffmpeg.sh` and `./scripts/build_zlmediakit.sh`

### Runtime Issues
- RKNN model compatibility with target hardware
- Thread pool sizing based on available CPU cores
- Memory management for large video frames
- Application crashes may be monitored via memory monitoring script

### Performance Optimization
- Adjust thread pool size based on target device capabilities
- Consider frame skip strategies for real-time performance
- Monitor memory usage with continuous video processing using provided monitoring tools

### Memory Debugging
Use the provided memory monitoring script:
```bash
./memory_monitor_zlmediakit.sh
```
This will generate CSV logs with detailed memory usage statistics.