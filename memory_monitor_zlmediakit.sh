#!/bin/bash
# ZLMediaKit版本内存监控脚本

PACKAGE_NAME="com.wulala.myyolov5rtspthreadpool"
LOG_FILE="zlmediakit_memory_$(date +%Y%m%d_%H%M%S).log"
CSV_FILE="zlmediakit_memory_$(date +%Y%m%d_%H%M%S).csv"

echo "=== ZLMediaKit版本内存泄漏调试 ===" | tee $LOG_FILE
echo "开始时间: $(date)" | tee -a $LOG_FILE
echo "监控包名: $PACKAGE_NAME" | tee -a $LOG_FILE
echo "" | tee -a $LOG_FILE

# CSV头部
echo "Timestamp,PID,VmSize_KB,VmRSS_KB,VmRSS_MB,VmHWM_KB,VmData_KB,VmStk_KB,VmExe_KB,VmLib_KB" > $CSV_FILE

# 获取初始PID
PID=$(adb shell pidof $PACKAGE_NAME)
if [ -z "$PID" ]; then
    echo "❌ 错误: 应用未运行" | tee -a $LOG_FILE
    exit 1
fi

echo "应用PID: $PID" | tee -a $LOG_FILE
echo "开始内存监控..." | tee -a $LOG_FILE
echo "" | tee -a $LOG_FILE

# 获取初始内存信息
INITIAL_MEMORY=$(adb shell cat /proc/$PID/status | grep VmRSS | awk '{print $2}')
INITIAL_MEMORY_MB=$((INITIAL_MEMORY / 1024))
echo "初始内存使用: ${INITIAL_MEMORY_MB}MB" | tee -a $LOG_FILE

echo "时间戳                    PID     VmRSS(MB)  增长(MB)  状态" | tee -a $LOG_FILE
echo "=========================================================" | tee -a $LOG_FILE

SAMPLE_COUNT=0
TOTAL_MEMORY=0
MAX_MEMORY=0
MIN_MEMORY=999999

while true; do
    CURRENT_TIME=$(date '+%Y-%m-%d %H:%M:%S')
    CURRENT_PID=$(adb shell pidof $PACKAGE_NAME)
    
    if [ -z "$CURRENT_PID" ]; then
        echo "❌ [$CURRENT_TIME] 应用崩溃或停止运行!" | tee -a $LOG_FILE
        echo "$CURRENT_TIME,CRASHED,0,0,0,0,0,0,0,0" >> $CSV_FILE
        break
    fi
    
    # 获取详细内存信息
    MEMORY_INFO=$(adb shell cat /proc/$CURRENT_PID/status | grep -E "VmSize|VmRSS|VmHWM|VmData|VmStk|VmExe|VmLib")
    
    VMSIZE_KB=$(echo "$MEMORY_INFO" | grep VmSize | awk '{print $2}')
    VMRSS_KB=$(echo "$MEMORY_INFO" | grep VmRSS | awk '{print $2}')
    VMHWM_KB=$(echo "$MEMORY_INFO" | grep VmHWM | awk '{print $2}')
    VMDATA_KB=$(echo "$MEMORY_INFO" | grep VmData | awk '{print $2}')
    VMSTK_KB=$(echo "$MEMORY_INFO" | grep VmStk | awk '{print $2}')
    VMEXE_KB=$(echo "$MEMORY_INFO" | grep VmExe | awk '{print $2}')
    VMLIB_KB=$(echo "$MEMORY_INFO" | grep VmLib | awk '{print $2}')
    
    VMRSS_MB=$((VMRSS_KB / 1024))
    GROWTH_MB=$((VMRSS_MB - INITIAL_MEMORY_MB))
    
    # 统计信息
    SAMPLE_COUNT=$((SAMPLE_COUNT + 1))
    TOTAL_MEMORY=$((TOTAL_MEMORY + VMRSS_MB))
    
    if [ $VMRSS_MB -gt $MAX_MEMORY ]; then
        MAX_MEMORY=$VMRSS_MB
    fi
    
    if [ $VMRSS_MB -lt $MIN_MEMORY ]; then
        MIN_MEMORY=$VMRSS_MB
    fi
    
    # 状态判断
    STATUS="NORMAL"
    if [ $GROWTH_MB -gt 200 ]; then
        STATUS="CRITICAL"
        echo "🚨 [$CURRENT_TIME] 严重内存泄漏: 增长 ${GROWTH_MB}MB" | tee -a $LOG_FILE
    elif [ $GROWTH_MB -gt 100 ]; then
        STATUS="WARNING"
        echo "⚠️  [$CURRENT_TIME] 内存增长警告: 增长 ${GROWTH_MB}MB" | tee -a $LOG_FILE
    elif [ $GROWTH_MB -gt 50 ]; then
        STATUS="ATTENTION"
    fi
    
    # 记录日志
    printf "%-20s %7s %10s %9s  %s\n" "$CURRENT_TIME" "$CURRENT_PID" "${VMRSS_MB}MB" "+${GROWTH_MB}MB" "$STATUS" | tee -a $LOG_FILE
    
    # 记录CSV
    echo "$CURRENT_TIME,$CURRENT_PID,$VMSIZE_KB,$VMRSS_KB,$VMRSS_MB,$VMHWM_KB,$VMDATA_KB,$VMSTK_KB,$VMEXE_KB,$VMLIB_KB" >> $CSV_FILE
    
    # 每10次采样显示统计信息
    if [ $((SAMPLE_COUNT % 10)) -eq 0 ]; then
        AVG_MEMORY=$((TOTAL_MEMORY / SAMPLE_COUNT))
        MEMORY_VARIANCE=$((MAX_MEMORY - MIN_MEMORY))
        echo "" | tee -a $LOG_FILE
        echo "--- 统计信息 (样本数: $SAMPLE_COUNT) ---" | tee -a $LOG_FILE
        echo "平均内存: ${AVG_MEMORY}MB" | tee -a $LOG_FILE
        echo "最大内存: ${MAX_MEMORY}MB" | tee -a $LOG_FILE
        echo "最小内存: ${MIN_MEMORY}MB" | tee -a $LOG_FILE
        echo "内存波动: ${MEMORY_VARIANCE}MB" | tee -a $LOG_FILE
        echo "最大增长: $((MAX_MEMORY - INITIAL_MEMORY_MB))MB" | tee -a $LOG_FILE
        echo "" | tee -a $LOG_FILE
    fi
    
    sleep 10  # 每10秒检查一次
done

echo "" | tee -a $LOG_FILE
echo "=== 监控结束 ===" | tee -a $LOG_FILE
echo "结束时间: $(date)" | tee -a $LOG_FILE
echo "详细数据已保存到: $CSV_FILE" | tee -a $LOG_FILE
