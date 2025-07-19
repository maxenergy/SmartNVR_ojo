package com.wulala.myyolov5rtspthreadpool.entities;

/**
 * 目标检测结果类
 * 用于表示YOLO模型的检测结果
 */
public class Detection {
    public String className;    // 类别名称
    public float confidence;    // 置信度
    public float x1, y1;       // 左上角坐标
    public float x2, y2;       // 右下角坐标
    
    public Detection() {
        this.className = "";
        this.confidence = 0.0f;
        this.x1 = this.y1 = this.x2 = this.y2 = 0.0f;
    }
    
    public Detection(String className, float confidence, float x1, float y1, float x2, float y2) {
        this.className = className;
        this.confidence = confidence;
        this.x1 = x1;
        this.y1 = y1;
        this.x2 = x2;
        this.y2 = y2;
    }
    
    /**
     * 获取边界框宽度
     */
    public float getWidth() {
        return x2 - x1;
    }
    
    /**
     * 获取边界框高度
     */
    public float getHeight() {
        return y2 - y1;
    }
    
    /**
     * 获取边界框面积
     */
    public float getArea() {
        return getWidth() * getHeight();
    }
    
    /**
     * 检查是否为有效检测结果
     */
    public boolean isValid() {
        return className != null && !className.isEmpty() && 
               confidence > 0 && getWidth() > 0 && getHeight() > 0;
    }
    
    @Override
    public String toString() {
        return String.format("Detection{class='%s', conf=%.2f, bbox=[%.1f,%.1f,%.1f,%.1f]}", 
                           className, confidence, x1, y1, x2, y2);
    }
}
