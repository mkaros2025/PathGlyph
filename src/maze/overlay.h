#pragma once
#include <glm/glm.hpp>

namespace PathGlyph {

// 简化为配置数据结构
struct OverlayConfig {
    glm::vec3 color{1.0f};     // 颜色
    float height = 0.0f;       // 高度偏移
    float glowIntensity = 0.0f; // 发光强度

    // 预定义配置
    static const OverlayConfig PATH;   // 路径
    static const OverlayConfig START;  // 起点
    static const OverlayConfig GOAL;   // 终点
    static const OverlayConfig CURRENT;// 当前位置
};

// 预定义配置的实现
inline const OverlayConfig OverlayConfig::PATH   = {glm::vec3(0.2f, 0.6f, 1.0f), 0.1f, 0.3f};
inline const OverlayConfig OverlayConfig::START  = {glm::vec3(0.2f, 0.8f, 0.2f), 0.2f, 0.5f};
inline const OverlayConfig OverlayConfig::GOAL   = {glm::vec3(0.8f, 0.8f, 0.2f), 0.2f, 0.5f};
inline const OverlayConfig OverlayConfig::CURRENT = {glm::vec3(0.2f, 0.6f, 0.8f), 0.3f, 0.4f};

} // namespace PathGlyph