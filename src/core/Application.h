#pragma once

#include <glad/glad.h>
#include <GLFW/glfw3.h>

#include <functional>
#include <memory>
#include <string>

#include "gui/imgui_impl_glfw.h"
#include "gui/imgui_impl_opengl3.h"
#include "maze/maze.h"
#include "renderer/renderer.h"

namespace PathGlyph {

// 编辑模式枚举
enum class EditMode {
    VIEW,      // 查看模式
    EDIT       // 编辑模式
};

// 编辑对象类型
enum class EditObjectType {
    START_POINT,   // 起点
    END_POINT,     // 终点
    OBSTACLE       // 障碍物
};

class Application {
public:
    Application(int width = 1000, int height = 600, const char* title = "PathGlyph");
    ~Application();
    
    // 禁用拷贝
    Application(const Application&) = delete;
    Application& operator=(const Application&) = delete;
    
    // 运行应用
    void run();
    
private:
    // 初始化函数
    bool initWindow();
    void initImGui();
    void setupCallbacks();
    
    // 窗口管理
    GLFWwindow* m_window = nullptr;
    int m_windowWidth;
    int m_windowHeight;
    int m_gridSize = 10;
    
    // 核心组件
    std::unique_ptr<Maze> m_maze;
    std::unique_ptr<Renderer> m_renderer;
    
    // 编辑状态
    EditMode m_editMode = EditMode::VIEW;
    EditObjectType m_editObjectType = EditObjectType::START_POINT;
    bool m_generatePath = false;
    float m_sidePanelWidth = 250.0f;
    
    // UI相关函数
    void drawPanel();
    void drawModeSwitch();
    void drawEditControls();
    void drawPathControls();
    void processUIState();
    
    // 事件处理
    void handleMouseClick(int button, int action, double xpos, double ypos);
    void handleEdit(int x, int y, EditObjectType type);
    void handlePathfinding();
    void handleClearPath();
    
    // 坐标转换
    Point screenToGrid(int screenX, int screenY);
    
    // 静态回调函数
    static void framebufferSizeCallback(GLFWwindow* window, int width, int height);
    static void mouseButtonCallback(GLFWwindow* window, int button, int action, int mods);
    static void cursorPosCallback(GLFWwindow* window, double xpos, double ypos);
    static void scrollCallback(GLFWwindow* window, double xoffset, double yoffset);
    static void keyCallback(GLFWwindow* window, int key, int scancode, int action, int mods);
    static void charCallback(GLFWwindow* window, unsigned int codepoint);
    
    // 鼠标拖动和滚轮事件处理函数
    void handleCursorPos(double xpos, double ypos);
    void handleScroll(double xoffset, double yoffset);
    
    // 鼠标拖动状态
    bool m_rightMouseDragging = false;  // 使用右键拖动
    bool m_mouseButtons[3] = {false, false, false}; // 存储鼠标按钮状态
    double m_lastX, m_lastY;
    
    // GLFW回调的用户指针
    static Application* getAppPtr(GLFWwindow* window) {
        return static_cast<Application*>(glfwGetWindowUserPointer(window));
    }
};

} // namespace PathGlyph 