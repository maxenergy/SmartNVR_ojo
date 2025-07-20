#!/bin/bash

# Test script to verify face detection fix
# This script will deploy the app and check if real face detection is working

echo "=== Face Detection Fix Verification Test ==="
echo "Date: $(date)"
echo ""

# Check if device is connected
echo "1. Checking device connection..."
adb devices | grep -q "device$"
if [ $? -ne 0 ]; then
    echo "❌ No Android device connected"
    exit 1
fi
echo "✅ Device connected"

# Install the APK
echo ""
echo "2. Installing updated APK..."
adb install -r app/build/outputs/apk/debug/app-debug.apk
if [ $? -ne 0 ]; then
    echo "❌ Failed to install APK"
    exit 1
fi
echo "✅ APK installed successfully"

# Clear app data to ensure fresh start
echo ""
echo "3. Clearing app data..."
adb shell pm clear com.wulala.myyolov5rtspthreadpool
echo "✅ App data cleared"

# Start the app
echo ""
echo "4. Starting MainActivity..."
adb shell am start -n com.wulala.myyolov5rtspthreadpool/.MainActivity
sleep 3
echo "✅ MainActivity started"

# Monitor logs for face detection activity
echo ""
echo "5. Monitoring face detection logs..."
echo "Looking for face detection activity in logcat..."
echo "Press Ctrl+C to stop monitoring"
echo ""

# Filter logs for face detection related messages
adb logcat -c  # Clear existing logs
adb logcat | grep -E "(Face|人脸|InspireFace|performFaceAnalysis|FaceDetectionBox)" --line-buffered | while read line; do
    timestamp=$(date '+%H:%M:%S')
    echo "[$timestamp] $line"
done
