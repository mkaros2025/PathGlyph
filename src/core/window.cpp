#include "window.h"

#include <GLFW/glfw3.h>
#include <glad/glad.h>  // 确保 GLAD 在 GLFW 之前包含

#include <iostream>

namespace PathGlyph {

Window::Window(int width, int height, const char* title)
    : m_keyCallback(nullptr), m_mouseCallback(nullptr) {
  if (!init()) {
    throw std::runtime_error("Failed to initialize GLFW window");
  }

  // 初始化窗口句柄 GLFW 窗口句柄
  m_window = glfwCreateWindow(width, height, title, nullptr, nullptr);
  if (!m_window) {
    glfwTerminate();
    throw std::runtime_error("Failed to create GLFW window");
  }

  // 将 m_window 的 OpenGL 上下文设为当前线程的上下文
  glfwMakeContextCurrent(m_window);

  // glfwGetProcAddress 的返回值是 OpenGL 函数的地址（函数指针）
  // 强制转换为 GLADloadproc 类型后
  // gladLoadGLLoader 将函数指针绑定至函数
  // 动态加载当前 OpenGL 上下文支持的函数指针，是初始化渲染管线的重要步骤
  if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
    glfwDestroyWindow(m_window);
    glfwTerminate();
    throw std::runtime_error("Failed to initialize GLAD");
  }

  // 打印 OpenGL 版本信息
  std::cout << "OpenGL Version: " << glGetString(GL_VERSION) << std::endl;
  // 打印当前上下文支持的 GLSL（OpenGL Shading Language）最高版本
  std::cout << "GLSL Version: " << glGetString(GL_SHADING_LANGUAGE_VERSION)
            << std::endl;
  // 打印当前图形渲染硬件详细信息
  std::cout << "Renderer: " << glGetString(GL_RENDERER) << std::endl;

  // 将自定义数据（this）绑定到窗口对象（m_window）
  // 设置用户指针以便在回调中访问Window实例
  glfwSetWindowUserPointer(m_window, this);

  // 注册回调
  glfwSetKeyCallback(m_window, keyCallback);
  glfwSetCursorPosCallback(m_window, cursorPosCallback);

  // 设置 vsync
  // 垂直同步！
  glfwSwapInterval(1);
}

bool Window::init() {
  // 初始化 GLFW
  if (!glfwInit()) {
    std::cerr << "Failed to initialize GLFW" << std::endl;
    return false;
  }

  // 设置 OpenGL 版本为 4.2 以及配置
  glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
  glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 2);
  glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

// 对 macOS 的特殊处理
#ifdef __APPLE__
  glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
#endif

// 设置调试输出
#ifdef _DEBUG
  glfwWindowHint(GLFW_OPENGL_DEBUG_CONTEXT, GLFW_TRUE);
#endif

  // ImGui 初始化
  IMGUI_CHECKVERSION();
  ImGui::CreateContext();
  ImGui_ImplGlfw_InitForOpenGL(m_window, true);
  ImGui_ImplOpenGL3_Init("#version 420");

  // 设置深色主题
  ImGui::StyleColorsLight();

  return true;
}

// 析构函数
Window::~Window() {
  if (m_window) {
    glfwDestroyWindow(m_window);
  }
  glfwTerminate();
}

// 检查用户是否关闭窗口，需要在主循环中持续运行
bool Window::shouldClose() const { return glfwWindowShouldClose(m_window); }

// OpenGL 使用双缓冲机制来避免屏幕撕裂（tearing）：
// 前缓冲区：当前显示在屏幕上的图像。
// 后缓冲区：正在渲染的图像。
// 交换前缓冲区和后缓冲区
// OpenGL 渲染管线中显示渲染结果的关键步骤
void Window::swapBuffers() const { glfwSwapBuffers(m_window); }

// 处理所有挂起的窗口事件，例如键盘输入、鼠标移动、窗口大小调整等
// 需要在主渲染循环中每帧调用一次，以确保应用程序能够响应用户输入和窗口事件
void Window::pollEvents() const {
  glfwPollEvents();
}

// 设置键盘回调函数
void Window::setKeyCallback(KeyCallback cb) { m_keyCallback = std::move(cb); }

void Window::setMouseCallback(MouseCallback cb) {
  m_mouseCallback = std::move(cb);
}

// 获取窗口 size
void Window::getSize(int& width, int& height) const {
  glfwGetWindowSize(m_window, &width, &height);
}

void Window::getCursorPos(double& x, double& y) const {
  glfwGetCursorPos(m_window, &x, &y);
}

void Window::keyCallback(GLFWwindow* window, int key, int scancode, int action,
                         int mods) {
  Window* win = static_cast<Window*>(glfwGetWindowUserPointer(window));
  if (win && win->m_keyCallback) {
    win->m_keyCallback(key, action);
  }
}

void Window::cursorPosCallback(GLFWwindow* window, double xpos, double ypos) {
  Window* win = static_cast<Window*>(glfwGetWindowUserPointer(window));
  if (win && win->m_mouseCallback) {
    win->m_mouseCallback(xpos, ypos);
  }
}

void Window::close() { glfwSetWindowShouldClose(m_window, GLFW_TRUE); }

void Window::DrawPanel() {
  // 设置面板初始位置和尺寸
  ImGuiIO& io = ImGui::GetIO();
  ImGui::SetNextWindowPos(ImVec2(io.DisplaySize.x - 320, 20), ImGuiCond_Always);
  ImGui::SetNextWindowSize(ImVec2(300, io.DisplaySize.y - 40), ImGuiCond_Always);

  // 样式配置
  ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4(0.1f, 0.1f, 0.1f, 0.9f)); // 深色半透明背景
  ImGui::Begin("Control Panel", nullptr, 
      ImGuiWindowFlags_NoMove | 
      ImGuiWindowFlags_NoResize |
      ImGuiWindowFlags_NoCollapse);

  // 主功能区域
  DrawModeSwitch();       // 模式切换
  ImGui::Separator();
  DrawEditControls();     // 编辑模式控件
  ImGui::Separator();
  DrawPathControls();     // 路径生成
  ImGui::Separator();
  DrawPerformanceStats(); // 性能监控

  ImGui::End();
  ImGui::PopStyleColor();
}

void Window::DrawModeSwitch() {
  // 模式切换按钮组
  if (ImGui::RadioButton("View Mode", !m_editMode)) {
      m_editMode = false;
  }
  ImGui::SameLine();
  if (ImGui::RadioButton("Edit Mode", m_editMode)) {
      m_editMode = true;
  }

  // 模式提示文本
  ImGui::TextColored(ImVec4(1,0.5,0,1), 
      m_editMode ? "Editing Enabled" : "Viewing Mode");
}

void Window::DrawEditControls() {
  if (!m_editMode) return;

  // 编辑类型选择
  const char* editTypes[] = { "Start", "End", "Wall" };
  ImGui::Combo("Edit Type", &m_selectedEditType, editTypes, IM_ARRAYSIZE(editTypes));

  // 当前选择坐标显示
  if (m_selectedTile.x >= 0) {
      ImGui::Text("Selected: (%d, %d)", 
          m_selectedTile.x, m_selectedTile.y);
  }

  // 操作说明
  ImGui::BulletText("Left Click: Set");
  ImGui::BulletText("Right Click: Clear");
}

void Window::DrawPathControls() {
  if (m_editMode) {
      ImGui::TextDisabled("Exit Edit Mode to generate path");
      return;
  }

  // 起点/终点预览
  auto& start = maze->GetStart();
  auto& end = maze->GetEnd();
  ImGui::Text("Current Start: (%d, %d)", start.x, start.y);
  ImGui::Text("Current End: (%d, %d)", end.x, end.y);

  // 生成按钮
  if (ImGui::Button("Find Optimal Path", ImVec2(-1, 0))) {
      auto path = pathfinder->AStar(start, end);
      renderer->VisualizePath(path);
  }

  // 路径存在性检测
  if (maze->HasValidPath()) {
      ImGui::TextColored(ImVec4(0,1,0,1), "Path Exists");
  } else {
      ImGui::TextColored(ImVec4(1,0,0,1), "No Valid Path!");
  }
}


}  // namespace PathGlyph




