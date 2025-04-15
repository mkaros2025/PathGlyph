#pragma once

#include <imgui.h>
#include <memory>
#include <string>
#include <functional>
#include <GLFW/glfw3.h>

#include "common/types.h"
#include "core/simulation.h"

namespace PathGlyph {

class ImGuiWindow {
public:
    ImGuiWindow(GLFWwindow* window, std::shared_ptr<EditState> state, std::shared_ptr<Simulation> simulation);
    ~ImGuiWindow();

    // 初始化 ImGui
    bool initialize();
    
    // 渲染循环
    void beginFrame();
    void endFrame();
    
    // 绘制控制面板
    void drawControlPanel();
    
    // 处理输入
    void handleInput();

private:
    GLFWwindow* window_;
    float sidePanelWidth_ = 250.0f;  // 侧边栏宽度
    std::shared_ptr<EditState> currentState_;  // 当前编辑状态
    std::shared_ptr<Simulation> simulation_;
};

} // namespace PathGlyph 