package com.wulala.myyolov5rtspthreadpool.entities;

/**
 * 人脸属性类
 * 包含性别、年龄、种族等属性信息
 */
public class FaceAttributes {
    
    // 性别常量
    public static final int GENDER_UNKNOWN = -1;
    public static final int GENDER_MALE = 0;
    public static final int GENDER_FEMALE = 1;
    
    // 年龄段常量
    public static final int AGE_UNKNOWN = -1;
    public static final int AGE_0_9 = 0;      // 0-9岁
    public static final int AGE_10_19 = 1;    // 10-19岁
    public static final int AGE_20_29 = 2;    // 20-29岁
    public static final int AGE_30_39 = 3;    // 30-39岁
    public static final int AGE_40_49 = 4;    // 40-49岁
    public static final int AGE_50_59 = 5;    // 50-59岁
    public static final int AGE_60_69 = 6;    // 60-69岁
    public static final int AGE_70_PLUS = 7;  // 70岁以上
    
    // 种族常量
    public static final int RACE_UNKNOWN = -1;
    public static final int RACE_BLACK = 0;
    public static final int RACE_ASIAN = 1;
    public static final int RACE_LATINO = 2;
    public static final int RACE_WHITE = 3;
    
    /**
     * 性别 (0: 男性, 1: 女性, -1: 未知)
     */
    private int gender;
    
    /**
     * 性别置信度 (0.0 - 1.0)
     */
    private float genderConfidence;
    
    /**
     * 年龄段 (0-7对应不同年龄段, -1: 未知)
     */
    private int ageBracket;
    
    /**
     * 年龄置信度 (0.0 - 1.0)
     */
    private float ageConfidence;
    
    /**
     * 种族 (0: 黑人, 1: 亚洲人, 2: 拉丁裔, 3: 白人, -1: 未知)
     */
    private int race;
    
    /**
     * 种族置信度 (0.0 - 1.0)
     */
    private float raceConfidence;
    
    public FaceAttributes() {
        this.gender = GENDER_UNKNOWN;
        this.genderConfidence = 0.0f;
        this.ageBracket = AGE_UNKNOWN;
        this.ageConfidence = 0.0f;
        this.race = RACE_UNKNOWN;
        this.raceConfidence = 0.0f;
    }
    
    public FaceAttributes(int gender, float genderConfidence, 
                         int ageBracket, float ageConfidence,
                         int race, float raceConfidence) {
        this.gender = gender;
        this.genderConfidence = genderConfidence;
        this.ageBracket = ageBracket;
        this.ageConfidence = ageConfidence;
        this.race = race;
        this.raceConfidence = raceConfidence;
    }
    
    // Getters and Setters
    public int getGender() {
        return gender;
    }
    
    public void setGender(int gender) {
        this.gender = gender;
    }
    
    public float getGenderConfidence() {
        return genderConfidence;
    }
    
    public void setGenderConfidence(float genderConfidence) {
        this.genderConfidence = Math.max(0.0f, Math.min(1.0f, genderConfidence));
    }
    
    public int getAgeBracket() {
        return ageBracket;
    }
    
    public void setAgeBracket(int ageBracket) {
        this.ageBracket = ageBracket;
    }
    
    public float getAgeConfidence() {
        return ageConfidence;
    }
    
    public void setAgeConfidence(float ageConfidence) {
        this.ageConfidence = Math.max(0.0f, Math.min(1.0f, ageConfidence));
    }
    
    public int getRace() {
        return race;
    }
    
    public void setRace(int race) {
        this.race = race;
    }
    
    public float getRaceConfidence() {
        return raceConfidence;
    }
    
    public void setRaceConfidence(float raceConfidence) {
        this.raceConfidence = Math.max(0.0f, Math.min(1.0f, raceConfidence));
    }
    
    // 便利方法
    
    /**
     * 获取性别字符串
     */
    public String getGenderString() {
        switch (gender) {
            case GENDER_MALE: return "男性";
            case GENDER_FEMALE: return "女性";
            default: return "未知";
        }
    }
    
    /**
     * 获取年龄段字符串
     */
    public String getAgeBracketString() {
        switch (ageBracket) {
            case AGE_0_9: return "0-9岁";
            case AGE_10_19: return "10-19岁";
            case AGE_20_29: return "20-29岁";
            case AGE_30_39: return "30-39岁";
            case AGE_40_49: return "40-49岁";
            case AGE_50_59: return "50-59岁";
            case AGE_60_69: return "60-69岁";
            case AGE_70_PLUS: return "70岁以上";
            default: return "未知年龄";
        }
    }
    
    /**
     * 获取种族字符串
     */
    public String getRaceString() {
        switch (race) {
            case RACE_BLACK: return "黑人";
            case RACE_ASIAN: return "亚洲人";
            case RACE_LATINO: return "拉丁裔";
            case RACE_WHITE: return "白人";
            default: return "未知种族";
        }
    }
    
    /**
     * 检查是否有有效的属性数据
     */
    public boolean isValid() {
        return gender != GENDER_UNKNOWN || 
               ageBracket != AGE_UNKNOWN || 
               race != RACE_UNKNOWN;
    }
    
    /**
     * 检查性别是否有效
     */
    public boolean hasValidGender() {
        return gender != GENDER_UNKNOWN && genderConfidence > 0.0f;
    }
    
    /**
     * 检查年龄是否有效
     */
    public boolean hasValidAge() {
        return ageBracket != AGE_UNKNOWN && ageConfidence > 0.0f;
    }
    
    /**
     * 检查种族是否有效
     */
    public boolean hasValidRace() {
        return race != RACE_UNKNOWN && raceConfidence > 0.0f;
    }
    
    /**
     * 获取属性摘要字符串
     */
    public String getSummary() {
        StringBuilder sb = new StringBuilder();
        
        if (hasValidGender()) {
            sb.append(getGenderString());
        }
        
        if (hasValidAge()) {
            if (sb.length() > 0) sb.append(", ");
            sb.append(getAgeBracketString());
        }
        
        if (hasValidRace()) {
            if (sb.length() > 0) sb.append(", ");
            sb.append(getRaceString());
        }
        
        return sb.length() > 0 ? sb.toString() : "无属性信息";
    }
    
    /**
     * 获取详细属性字符串（包含置信度）
     */
    public String getDetailedSummary() {
        StringBuilder sb = new StringBuilder();
        
        if (hasValidGender()) {
            sb.append(getGenderString())
              .append("(").append(String.format("%.1f%%", genderConfidence * 100)).append(")");
        }
        
        if (hasValidAge()) {
            if (sb.length() > 0) sb.append(", ");
            sb.append(getAgeBracketString())
              .append("(").append(String.format("%.1f%%", ageConfidence * 100)).append(")");
        }
        
        if (hasValidRace()) {
            if (sb.length() > 0) sb.append(", ");
            sb.append(getRaceString())
              .append("(").append(String.format("%.1f%%", raceConfidence * 100)).append(")");
        }
        
        return sb.length() > 0 ? sb.toString() : "无属性信息";
    }
    
    @Override
    public String toString() {
        return "FaceAttributes{" +
                "gender=" + getGenderString() + "(" + genderConfidence + ")" +
                ", ageBracket=" + getAgeBracketString() + "(" + ageConfidence + ")" +
                ", race=" + getRaceString() + "(" + raceConfidence + ")" +
                '}';
    }
}
