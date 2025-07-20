package com.wulala.myyolov5rtspthreadpool.entities;

import android.graphics.Rect;

/**
 * 人脸分析结果类
 * 包含人脸检测和属性分析的完整信息
 */
public class FaceAnalysisResult {
    
    /**
     * 人脸边界框
     */
    private Rect faceRect;
    
    /**
     * 人脸检测置信度 (0.0 - 1.0)
     */
    private float confidence;
    
    /**
     * 跟踪ID
     */
    private int trackId;
    
    /**
     * 人脸属性
     */
    private FaceAttributes attributes;
    
    /**
     * 人脸在原图中的相对位置（相对于检测到的人员区域）
     */
    private Rect relativeRect;
    
    /**
     * 是否有效的人脸检测结果
     */
    private boolean valid;
    
    public FaceAnalysisResult() {
        this.faceRect = new Rect();
        this.confidence = 0.0f;
        this.trackId = -1;
        this.attributes = new FaceAttributes();
        this.relativeRect = new Rect();
        this.valid = false;
    }
    
    public FaceAnalysisResult(Rect faceRect, float confidence, int trackId) {
        this.faceRect = faceRect != null ? new Rect(faceRect) : new Rect();
        this.confidence = confidence;
        this.trackId = trackId;
        this.attributes = new FaceAttributes();
        this.relativeRect = new Rect();
        this.valid = true;
    }
    
    // Getters and Setters
    public Rect getFaceRect() {
        return faceRect;
    }
    
    public void setFaceRect(Rect faceRect) {
        this.faceRect = faceRect != null ? new Rect(faceRect) : new Rect();
    }
    
    public float getConfidence() {
        return confidence;
    }
    
    public void setConfidence(float confidence) {
        this.confidence = Math.max(0.0f, Math.min(1.0f, confidence));
    }
    
    public int getTrackId() {
        return trackId;
    }
    
    public void setTrackId(int trackId) {
        this.trackId = trackId;
    }
    
    public FaceAttributes getAttributes() {
        return attributes;
    }
    
    public void setAttributes(FaceAttributes attributes) {
        this.attributes = attributes != null ? attributes : new FaceAttributes();
    }
    
    public Rect getRelativeRect() {
        return relativeRect;
    }
    
    public void setRelativeRect(Rect relativeRect) {
        this.relativeRect = relativeRect != null ? new Rect(relativeRect) : new Rect();
    }
    
    public boolean isValid() {
        return valid;
    }
    
    public void setValid(boolean valid) {
        this.valid = valid;
    }
    
    // 便利方法
    
    /**
     * 获取人脸区域的宽度
     */
    public int getWidth() {
        return faceRect.width();
    }
    
    /**
     * 获取人脸区域的高度
     */
    public int getHeight() {
        return faceRect.height();
    }
    
    /**
     * 获取人脸区域的面积
     */
    public int getArea() {
        return getWidth() * getHeight();
    }
    
    /**
     * 获取人脸中心点X坐标
     */
    public int getCenterX() {
        return faceRect.centerX();
    }
    
    /**
     * 获取人脸中心点Y坐标
     */
    public int getCenterY() {
        return faceRect.centerY();
    }
    
    /**
     * 检查是否有有效的属性分析结果
     */
    public boolean hasValidAttributes() {
        return attributes != null && attributes.isValid();
    }
    
    /**
     * 获取置信度百分比字符串
     */
    public String getConfidencePercentage() {
        return String.format("%.1f%%", confidence * 100);
    }
    
    /**
     * 获取人脸描述字符串
     */
    public String getDescription() {
        StringBuilder sb = new StringBuilder();
        sb.append("人脸 #").append(trackId);
        sb.append(" (").append(getConfidencePercentage()).append(")");
        
        if (hasValidAttributes()) {
            sb.append(" - ").append(attributes.getGenderString());
            sb.append(", ").append(attributes.getAgeBracketString());
        }
        
        return sb.toString();
    }
    
    /**
     * 获取详细信息字符串
     */
    public String getDetailedInfo() {
        StringBuilder sb = new StringBuilder();
        sb.append("人脸ID: ").append(trackId).append("\n");
        sb.append("位置: [").append(faceRect.left).append(",").append(faceRect.top)
          .append(",").append(faceRect.right).append(",").append(faceRect.bottom).append("]\n");
        sb.append("大小: ").append(getWidth()).append("x").append(getHeight()).append("\n");
        sb.append("置信度: ").append(getConfidencePercentage()).append("\n");
        
        if (hasValidAttributes()) {
            sb.append("属性:\n");
            sb.append("  性别: ").append(attributes.getGenderString())
              .append(" (").append(String.format("%.1f%%", attributes.getGenderConfidence() * 100)).append(")\n");
            sb.append("  年龄: ").append(attributes.getAgeBracketString())
              .append(" (").append(String.format("%.1f%%", attributes.getAgeConfidence() * 100)).append(")\n");
            sb.append("  种族: ").append(attributes.getRaceString())
              .append(" (").append(String.format("%.1f%%", attributes.getRaceConfidence() * 100)).append(")\n");
        }
        
        return sb.toString();
    }
    
    @Override
    public String toString() {
        return "FaceAnalysisResult{" +
                "faceRect=" + faceRect +
                ", confidence=" + confidence +
                ", trackId=" + trackId +
                ", attributes=" + attributes +
                ", valid=" + valid +
                '}';
    }
}
