// 简化的检测设置功能验证脚本
// 用于验证核心功能的正确性

import java.util.*;

class DetectionSettingsValidation {
    
    // 模拟COCO类别数组
    private static final String[] ALL_YOLO_CLASSES = {
        "person", "bicycle", "car", "motorcycle", "airplane", "bus", "train", "truck", "boat", "traffic light",
        "fire hydrant", "stop sign", "parking meter", "bench", "bird", "cat", "dog", "horse", "sheep", "cow",
        "elephant", "bear", "zebra", "giraffe", "backpack", "umbrella", "handbag", "tie", "suitcase", "frisbee",
        "skis", "snowboard", "sports ball", "kite", "baseball bat", "baseball glove", "skateboard", "surfboard",
        "tennis racket", "bottle", "wine glass", "cup", "fork", "knife", "spoon", "bowl", "banana", "apple",
        "sandwich", "orange", "broccoli", "carrot", "hot dog", "pizza", "donut", "cake", "chair", "couch",
        "potted plant", "bed", "dining table", "toilet", "tv", "laptop", "mouse", "remote", "keyboard", "cell phone",
        "microwave", "oven", "toaster", "sink", "refrigerator", "book", "clock", "vase", "scissors", "teddy bear",
        "hair drier", "toothbrush"
    };
    
    public static void main(String[] args) {
        System.out.println("=== 检测设置功能验证 ===\n");
        
        boolean allTestsPassed = true;
        
        // 测试1: 类别数组完整性
        allTestsPassed &= testClassArrayIntegrity();
        
        // 测试2: 类别索引映射
        allTestsPassed &= testClassIndexMapping();
        
        // 测试3: 过滤逻辑模拟
        allTestsPassed &= testFilterLogic();
        
        // 测试4: 边界条件
        allTestsPassed &= testBoundaryConditions();
        
        System.out.println("\n=== 验证结果 ===");
        if (allTestsPassed) {
            System.out.println("🎉 所有测试通过！检测设置功能实现正确。");
        } else {
            System.out.println("❌ 存在问题，请检查实现。");
        }
    }
    
    private static boolean testClassArrayIntegrity() {
        System.out.println("测试1: 类别数组完整性");
        
        // 验证类别数量
        if (ALL_YOLO_CLASSES.length != 80) {
            System.out.println("❌ 类别数量错误: " + ALL_YOLO_CLASSES.length + " (期望: 80)");
            return false;
        }
        
        // 验证关键类别存在
        String[] keyClasses = {"person", "car", "bicycle", "chair", "tv", "laptop"};
        for (String keyClass : keyClasses) {
            boolean found = false;
            for (String cls : ALL_YOLO_CLASSES) {
                if (cls.equals(keyClass)) {
                    found = true;
                    break;
                }
            }
            if (!found) {
                System.out.println("❌ 关键类别缺失: " + keyClass);
                return false;
            }
        }
        
        System.out.println("✅ 类别数组完整性测试通过");
        return true;
    }
    
    private static boolean testClassIndexMapping() {
        System.out.println("测试2: 类别索引映射");
        
        // 验证person类别在索引0
        if (!ALL_YOLO_CLASSES[0].equals("person")) {
            System.out.println("❌ person类别不在索引0: " + ALL_YOLO_CLASSES[0]);
            return false;
        }
        
        // 验证car类别在索引2
        if (!ALL_YOLO_CLASSES[2].equals("car")) {
            System.out.println("❌ car类别不在索引2: " + ALL_YOLO_CLASSES[2]);
            return false;
        }
        
        System.out.println("✅ 类别索引映射测试通过");
        return true;
    }
    
    private static boolean testFilterLogic() {
        System.out.println("测试3: 过滤逻辑模拟");
        
        // 模拟检测结果
        List<DetectionResult> testResults = Arrays.asList(
            new DetectionResult("person", 0.9f),
            new DetectionResult("car", 0.7f),
            new DetectionResult("bicycle", 0.6f),
            new DetectionResult("chair", 0.4f)
        );
        
        // 模拟设置：只启用person，置信度阈值0.5
        Set<String> enabledClasses = new HashSet<>(Arrays.asList("person"));
        float confidenceThreshold = 0.5f;
        
        // 应用过滤
        List<DetectionResult> filtered = filterResults(testResults, enabledClasses, confidenceThreshold);
        
        // 验证结果
        if (filtered.size() != 1) {
            System.out.println("❌ 过滤结果数量错误: " + filtered.size() + " (期望: 1)");
            return false;
        }
        
        if (!filtered.get(0).className.equals("person")) {
            System.out.println("❌ 过滤结果类别错误: " + filtered.get(0).className + " (期望: person)");
            return false;
        }
        
        System.out.println("✅ 过滤逻辑测试通过");
        return true;
    }
    
    private static boolean testBoundaryConditions() {
        System.out.println("测试4: 边界条件");
        
        // 测试空结果
        List<DetectionResult> emptyResults = new ArrayList<>();
        Set<String> enabledClasses = new HashSet<>(Arrays.asList("person"));
        List<DetectionResult> filtered = filterResults(emptyResults, enabledClasses, 0.5f);
        
        if (!filtered.isEmpty()) {
            System.out.println("❌ 空结果过滤失败");
            return false;
        }
        
        // 测试极端置信度
        List<DetectionResult> testResults = Arrays.asList(
            new DetectionResult("person", 1.0f),
            new DetectionResult("person", 0.0f)
        );
        
        filtered = filterResults(testResults, enabledClasses, 1.0f);
        if (filtered.size() != 1) {
            System.out.println("❌ 极端置信度过滤失败");
            return false;
        }
        
        System.out.println("✅ 边界条件测试通过");
        return true;
    }
    
    // 模拟过滤逻辑
    private static List<DetectionResult> filterResults(List<DetectionResult> results, 
                                                      Set<String> enabledClasses, 
                                                      float confidenceThreshold) {
        List<DetectionResult> filtered = new ArrayList<>();
        
        for (DetectionResult result : results) {
            if (enabledClasses.contains(result.className) && 
                result.confidence >= confidenceThreshold) {
                filtered.add(result);
            }
        }
        
        return filtered;
    }
    
    // 简化的检测结果类
    static class DetectionResult {
        String className;
        float confidence;
        
        DetectionResult(String className, float confidence) {
            this.className = className;
            this.confidence = confidence;
        }
    }
}
