#pragma once

#include <GLFW/glfw3.h>
#include <memory>
#include <string>
#include <vector>
#include <array>
#include "common/types.h"
#include "maze/maze.h"
#include "graphics/renderer.h"
#include "ui/ImGuiWindow.h"
#include "gui/imgui_impl_glfw.h"
#include "gui/imgui_impl_opengl3.h"
#include "core/simulation.h"

namespace PathGlyph {

class Application {
public:
    // 构造和析构
    Application(int width, int height, const char* title);
    ~Application();
    
    // 主循环
    void run();

private:
    // ===== 核心组件 =====
    std::shared_ptr<Maze> m_maze;              // 迷宫数据
    std::unique_ptr<Renderer> m_renderer;      // 渲染器
    std::shared_ptr<ImGuiWindow> m_uiWindow;   // UI窗口
    std::shared_ptr<EditState> m_editState;    // 编辑状态
    std::shared_ptr<Simulation> m_simulation;  // 仿真系统
    
    // ===== 窗口相关 =====
    GLFWwindow* m_window = nullptr;
    int m_windowWidth;
    int m_windowHeight;
    float m_sidePanelWidth = 300.0f;
    float m_gridSize = 1.0f;
    
    // ===== 交互状态 =====
    // 鼠标状态
    bool m_mouseButtons[3] = {false, false, false};
    bool m_rightMouseDragging = false;
    bool m_leftMouseDragging = false;  // 新增：判断左键是否处于拖动状态
    double m_lastX = 0.0;
    double m_lastY = 0.0;
    double m_currentMouseX = 0.0;  // 当前鼠标X坐标
    double m_currentMouseY = 0.0;  // 当前鼠标Y坐标
    double m_leftMouseDownTime = 0.0; // 左键按下的时间
    
    // ===== 初始化方法 =====
    bool initWindow();
    void setupCallbacks();
    
    // ===== 事件处理 =====
    // 鼠标和输入处理
    void handleMouseClick(double x, double y);
    void handleCursorPos(double xpos, double ypos);
    void handleScroll(double xoffset, double yoffset);
    
    // 坐标转换
    Point screenToGrid(double screenX, double screenY);
    
    // ===== 静态回调函数 =====
    static void framebufferSizeCallback(GLFWwindow* window, int width, int height);
    static void mouseButtonCallback(GLFWwindow* window, int button, int action, int mods);
    static void cursorPosCallback(GLFWwindow* window, double xpos, double ypos);
    static void scrollCallback(GLFWwindow* window, double xoffset, double yoffset);
    
    // 辅助函数
    static Application* getAppPtr(GLFWwindow* window);
};

} // namespace PathGlyph 