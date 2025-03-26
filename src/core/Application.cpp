#include "core/Application.h"
#include <iostream>
#include <imgui.h>
#include "../common/Point.h"

namespace PathGlyph {

Application::Application(int width, int height, const char* title)
    : m_windowWidth(width), m_windowHeight(height)
{
    // 初始化窗口
    if (!initWindow()) {
        throw std::runtime_error("Failed to initialize window");
    }
    
    // 设置窗口标题
    glfwSetWindowTitle(m_window, title);
    
    // 初始化ImGui
    initImGui();
    
    // 创建迷宫 - 从JSON文件加载
    m_maze = std::make_unique<Maze>();
    
    // 尝试从默认JSON文件加载迷宫配置
    const std::string defaultMazeFile = "../../../../assets/mazes/default_maze.json";
    if (!m_maze->loadFromJson(defaultMazeFile)) {
        std::cerr << "警告: 无法从 " << defaultMazeFile << " 加载迷宫配置，使用默认设置" << std::endl;
        
        // 如果加载失败，使用默认值
        m_maze = std::make_unique<Maze>(15, 15);
        m_maze->setStart(2, 2);
        m_maze->setGoal(12, 12);
    } else {
        std::cout << "成功从 " << defaultMazeFile << " 加载迷宫配置" << std::endl;
    }
    
    // 创建渲染器（此处假设渲染器构造函数已修改为接受GLFWwindow*）
    m_renderer = std::make_unique<Renderer>(m_window);
    // 设置迷宫
    m_renderer->setMaze(m_maze.get());
    
    // 设置回调
    setupCallbacks();
    
    std::cout << "Application initialized successfully." << std::endl;
}

Application::~Application() {
    // 清理ImGui
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();
    
    // 清理GLFW
    if (m_window) {
        glfwDestroyWindow(m_window);
    }
    glfwTerminate();
}

bool Application::initWindow() {
    // 初始化GLFW
    if (!glfwInit()) {
        std::cerr << "Failed to initialize GLFW" << std::endl;
        return false;
    }
    
    // 配置GLFW，主版本号+次版本号，表示4.2
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 2);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    
    // 创建窗口
    m_window = glfwCreateWindow(m_windowWidth, m_windowHeight, "PathGlyph", nullptr, nullptr);
    if (!m_window) {
        std::cerr << "Failed to create GLFW window" << std::endl;
        glfwTerminate();
        return false;
    }
    
    // 设置当前上下文
    glfwMakeContextCurrent(m_window);
    
    // 加载OpenGL函数指针
    if (!gladLoadGLLoader(reinterpret_cast<GLADloadproc>(glfwGetProcAddress))) {
        std::cerr << "Failed to initialize GLAD" << std::endl;
        return false;
    }
    
    // 输出OpenGL版本信息
    std::cout << "OpenGL Info:" << std::endl;
    std::cout << "  Vendor: " << glGetString(GL_VENDOR) << std::endl;
    std::cout << "  Renderer: " << glGetString(GL_RENDERER) << std::endl;
    std::cout << "  Version: " << glGetString(GL_VERSION) << std::endl;
    std::cout << "  GLSL Version: " << glGetString(GL_SHADING_LANGUAGE_VERSION) << std::endl;
    
    // 设置视口，即用户能看到的那部分窗口，这里就是用户能看到窗口的所有内容
    glViewport(0, 0, m_windowWidth, m_windowHeight);
    
    // 存储this指针用于回调
    glfwSetWindowUserPointer(m_window, this);
    
    return true;
}

void Application::initImGui() {
    // 创建ImGui上下文
    IMGUI_CHECKVERSION();  // 检查ImGui版本兼容性
    ImGui::CreateContext(); // 创建ImGui上下文
    ImGuiIO& io = ImGui::GetIO(); // 获取ImGui输入/输出配置
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard; // 启用键盘导航
    
    // 设置ImGui风格
    ImGui::StyleColorsDark(); // 使用深色主题
    
    // 初始化ImGui与GLFW和OpenGL的绑定
    // 关键：这里设置为false，不会自动注册 GLFW 的鼠标、键盘等输入事件回调
    if (!ImGui_ImplGlfw_InitForOpenGL(m_window, false)) {
        throw std::runtime_error("Failed to initialize ImGui GLFW backend");
    }
    
    if (!ImGui_ImplOpenGL3_Init("#version 420 core")) {
        throw std::runtime_error("Failed to initialize ImGui OpenGL3 backend");
    }
    
    std::cout << "ImGui initialized successfully." << std::endl;
}

void Application::setupCallbacks() {
    // 窗口大小改变回调
    glfwSetFramebufferSizeCallback(m_window, framebufferSizeCallback);
    
    // 鼠标按键回调
    glfwSetMouseButtonCallback(m_window, mouseButtonCallback);
    
    // 鼠标移动回调
    glfwSetCursorPosCallback(m_window, cursorPosCallback);
    
    // 鼠标滚轮回调
    glfwSetScrollCallback(m_window, scrollCallback);
    
    // 键盘回调
    glfwSetKeyCallback(m_window, keyCallback);
    
    // 字符输入回调
    glfwSetCharCallback(m_window, charCallback);
}

void Application::framebufferSizeCallback(GLFWwindow* window, int width, int height) {
    Application* app = getAppPtr(window);
    if (app) {
        app->m_windowWidth = width;
        app->m_windowHeight = height;
        glViewport(0, 0, width, height);
        
        // 通知渲染器窗口大小改变
        if (app->m_renderer) {
            app->m_renderer->handleResize(width, height);
        }
    }
}

void Application::mouseButtonCallback(GLFWwindow* window, int button, int action, int mods) {
    Application* app = getAppPtr(window);
    if (app) {
        // 获取鼠标位置
        double xpos, ypos;
        glfwGetCursorPos(window, &xpos, &ypos);
        
        // 更新ImGui鼠标状态，即左右中键
        if (button >= 0 && button < 3) {
            if (action == GLFW_PRESS) {
                app->m_mouseButtons[button] = true;
            }
        }
        
        // 检查是否在UI区域内
        bool isOverUI = (xpos < app->m_sidePanelWidth);
        
        // 只有当不在UI区域时才处理地图交互
        if (!isOverUI) {
            app->handleMouseClick(button, action, xpos, ypos);
        }
    }
}

void Application::cursorPosCallback(GLFWwindow* window, double xpos, double ypos) {
    Application* app = getAppPtr(window);
    if (app) {
        // 检查是否在UI区域内
        bool isOverUI = (xpos < app->m_sidePanelWidth);
        
        // 只有当不在UI区域时才处理平移视图
        if (!isOverUI) {
            app->handleCursorPos(xpos, ypos);
        }
    }
}

void Application::handleCursorPos(double xpos, double ypos) {
    // 当鼠标右键按下时进行拖动
    if (m_rightMouseDragging) {
        // 计算移动差值
        double dx = xpos - m_lastX;
        double dy = ypos - m_lastY;
        
        // 平移视图
        m_renderer->pan(dx, -dy);
        
        // 更新上次鼠标位置
        m_lastX = xpos;
        m_lastY = ypos;
    }
}

void Application::scrollCallback(GLFWwindow* window, double xoffset, double yoffset) {
    Application* app = getAppPtr(window);
    if (app) {
        // 获取鼠标位置
        double xpos, ypos;
        glfwGetCursorPos(window, &xpos, &ypos);
        
        // 检查是否在UI区域内
        bool isOverUI = (xpos < app->m_sidePanelWidth);
        
        // 只有当不在UI区域时才处理地图滚轮事件
        if (!isOverUI) {
            app->handleScroll(xoffset, yoffset);
        }
    }
}

void Application::handleScroll(double xoffset, double yoffset) {
    // 使用yoffset进行缩放，通常上滚为正，下滚为负
    const float zoomFactor = 1.1f;
    if (yoffset > 0) {
        m_renderer->zoom(zoomFactor); // 放大
    } else if (yoffset < 0) {
        m_renderer->zoom(1.0f / zoomFactor); // 缩小
    }
}

void Application::handleMouseClick(int button, int action, double xpos, double ypos) {    
    // 右键处理拖动平移
    if (button == GLFW_MOUSE_BUTTON_RIGHT) {
        if (action == GLFW_PRESS) {
            m_rightMouseDragging = true;  // 使用新变量名
            m_lastX = xpos;
            m_lastY = ypos;
        } else if (action == GLFW_RELEASE) {
            m_rightMouseDragging = false;
        }
        return;  // 右键只用于拖动，不处理其他操作
    }
    
    // 左键处理编辑操作
    // 仅处理鼠标左键点击
    if (button != GLFW_MOUSE_BUTTON_LEFT || action != GLFW_PRESS) {
        return;
    }
    
    // 如果在编辑模式下
    if (m_editMode == EditMode::EDIT) {
        handleEdit(static_cast<int>(xpos), static_cast<int>(ypos), m_editObjectType);
    }
}

void Application::handleEdit(int screenX, int screenY, EditObjectType type) {
    // 将屏幕坐标转换为网格坐标
    Point gridPos = screenToGrid(screenX, screenY);
    
    // 检查坐标是否有效
    if (gridPos.x < 0 || gridPos.x >= m_maze->getWidth() || 
        gridPos.y < 0 || gridPos.y >= m_maze->getHeight()) {
        return;
    }
    
    // 根据编辑类型执行相应操作
    switch (type) {
        case EditObjectType::START_POINT:
            // 如果网格上已有起点，先清除
            if (m_maze->isStartPoint(gridPos.x, gridPos.y)) {
                m_maze->clearStart();
            } else {
                m_maze->setStart(gridPos.x, gridPos.y);
            }
            break;
            
        case EditObjectType::END_POINT:
            // 如果网格上已有终点，先清除
            if (m_maze->isEndPoint(gridPos.x, gridPos.y)) {
                m_maze->clearGoal();
            } else {
                m_maze->setGoal(gridPos.x, gridPos.y);
            }
            break;
            
        case EditObjectType::OBSTACLE:
            // 如果网格上已有障碍物，先清除，否则添加
            if (m_maze->isObstacle(gridPos.x, gridPos.y)) {
                m_maze->removeObstacle(gridPos.x, gridPos.y);
            } else {
                m_maze->addObstacle(gridPos.x, gridPos.y);
            }
            break;
    }
    
    // 通知渲染器更新地图几何
    m_renderer->setMaze(m_maze.get());
    
    // 清除现有路径
    m_maze->clearPath();
}

void Application::handlePathfinding() {
    // 检查起点和终点是否已设置
    const Point& start = m_maze->getStart();
    const Point& goal = m_maze->getGoal();
    
    if (start.x < 0 || start.y < 0 || goal.x < 0 || goal.y < 0) {
        std::cout << "Cannot find path: Start or goal point not set." << std::endl;
        return;
    }
    
    // 使用A*算法查找路径
    std::vector<Point> path = m_maze->findPathAStar();
    
    if (path.empty()) {
        std::cout << "No valid path found." << std::endl;
    } else {
        std::cout << "Path found with " << path.size() << " steps." << std::endl;
        
        // 添加以下行，确保渲染器更新
        m_renderer->setMaze(m_maze.get());
        
        // 确保路径显示配置开启
        m_renderer->getConfig().showPath = true;
        
        std::cout << "Path rendering updated." << std::endl;
    }
}

void Application::handleClearPath() {
    m_maze->clearPath();
    
    // 通知渲染器更新
    m_renderer->setMaze(m_maze.get());
    
    std::cout << "Path cleared and renderer updated." << std::endl;
}

Point Application::screenToGrid(int screenX, int screenY) {
    // 获取当前视图变换
    const auto& config = m_renderer->getConfig();
    
    // 打印调试信息
    std::cout << "屏幕到网格坐标转换:" << std::endl;
    std::cout << "  屏幕坐标: (" << screenX << ", " << screenY << ")" << std::endl;
    std::cout << "  相机偏移: (" << config.cameraOffset.x << ", " << config.cameraOffset.y << ")" << std::endl;
    std::cout << "  缩放级别: " << config.zoomLevel << std::endl;
    std::cout << "  网格大小: " << m_gridSize << std::endl;
    std::cout << "  迷宫大小: " << m_maze->getWidth() << "x" << m_maze->getHeight() << std::endl;
    
    // 1. 考虑UI区域的偏移
    float adjustedScreenX = screenX - m_sidePanelWidth;
    
    // 2. 将屏幕坐标转换为世界坐标时考虑窗口中心点作为原点
    float centerX = (m_windowWidth - m_sidePanelWidth) / 2.0f;
    float centerY = m_windowHeight / 2.0f;
    
    // 3. 相对于中心点的坐标
    float relativeX = adjustedScreenX - centerX;
    float relativeY = screenY - centerY;
    
    // 4. 应用缩放和相机偏移
    float worldX = relativeX / config.zoomLevel - config.cameraOffset.x;
    float worldY = relativeY / config.zoomLevel - config.cameraOffset.y;
    
    std::cout << "  世界坐标: (" << worldX << ", " << worldY << ")" << std::endl;
    
    // 5. 将世界坐标转换为网格坐标 - 注意我们需要除以网格大小
    int gridX = static_cast<int>(floor(worldX / m_gridSize + 0.5f));
    int gridY = static_cast<int>(floor(worldY / m_gridSize + 0.5f));
    
    std::cout << "  网格坐标(原始): (" << gridX << ", " << gridY << ")" << std::endl;
    
    // 确保网格坐标在迷宫范围内
    gridX = std::max(0, std::min(gridX, m_maze->getWidth() - 1));
    gridY = std::max(0, std::min(gridY, m_maze->getHeight() - 1));
    
    std::cout << "  网格坐标(修正): (" << gridX << ", " << gridY << ")" << std::endl;
    
    return Point(gridX, gridY);
}

void Application::drawPanel() {
    // 设置侧边面板位置和大小
    ImGui::SetNextWindowPos(ImVec2(0, 0), ImGuiCond_Always);
    ImGui::SetNextWindowSize(ImVec2(m_sidePanelWidth, static_cast<float>(m_windowHeight)), ImGuiCond_Always);
    
    // 创建面板窗口
    ImGui::Begin("Controls", nullptr, ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoCollapse);
    
    // 添加面板内容
    ImGui::Text("PathGlyph");
    ImGui::Separator();
    
    // 绘制模式切换控件
    drawModeSwitch();
    ImGui::Separator();
    
    // 根据当前模式绘制相应控件
    if (m_editMode == EditMode::EDIT) {
        drawEditControls();
    }
    
    // 绘制路径控制按钮
    drawPathControls();
    
    ImGui::End();
}

void Application::drawModeSwitch() {
    ImGui::Text("Mode:");
    
    // 视图/编辑模式切换
    int mode = static_cast<int>(m_editMode);
    
    if (ImGui::RadioButton("View", mode == static_cast<int>(EditMode::VIEW))) {
        m_editMode = EditMode::VIEW;
    }
    ImGui::SameLine();
    if (ImGui::RadioButton("Edit", mode == static_cast<int>(EditMode::EDIT))) {
        m_editMode = EditMode::EDIT;
    }
}

void Application::drawEditControls() {
    ImGui::Text("Edit Tools:");
    
    // 编辑工具选择
    int objectType = static_cast<int>(m_editObjectType);
    
    if (ImGui::RadioButton("Start Point", objectType == static_cast<int>(EditObjectType::START_POINT))) {
        m_editObjectType = EditObjectType::START_POINT;
    }
    if (ImGui::RadioButton("End Point", objectType == static_cast<int>(EditObjectType::END_POINT))) {
        m_editObjectType = EditObjectType::END_POINT;
    }
    if (ImGui::RadioButton("Obstacle", objectType == static_cast<int>(EditObjectType::OBSTACLE))) {
        m_editObjectType = EditObjectType::OBSTACLE;
    }
    
    ImGui::Text("Click on the grid to place/remove objects");
}

void Application::drawPathControls() {
    ImGui::Separator();
    ImGui::Text("Path Controls:");
    
    // 寻路按钮
    // ImVec2是ImGui中表示二维向量（x和y坐标）的结构体
    // 高度参数为0，表示使用ImGui的默认按钮高度
    if (ImGui::Button("Find Path", ImVec2(ImGui::GetContentRegionAvail().x, 0))) {
        m_generatePath = true;
    }
    
    // 清除路径按钮
    if (ImGui::Button("Clear Path", ImVec2(ImGui::GetContentRegionAvail().x, 0))) {
        handleClearPath();
    }
}

// 新增：处理UI状态变化
void Application::processUIState() {
    // 处理寻路请求
    if (m_generatePath) {
        handlePathfinding();
        m_generatePath = false;
    }
}

void Application::run() {
    // 主循环
    while (!glfwWindowShouldClose(m_window)) {
        // 轮询事件
        glfwPollEvents();
        
        // 开始ImGui帧
        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();
        
        // 手动处理ImGui输入
        ImGuiIO& io = ImGui::GetIO();
        
        // 更新鼠标位置
        double mouseX, mouseY;
        glfwGetCursorPos(m_window, &mouseX, &mouseY);
        io.MousePos = ImVec2((float)mouseX, (float)mouseY);
        
        // 更新鼠标按钮状态
        for (int i = 0; i < 3; i++) {
            io.MouseDown[i] = m_mouseButtons[i] || glfwGetMouseButton(m_window, i) != 0;
            m_mouseButtons[i] = false;
        }
        
        // 处理ImGui UI
        drawPanel();
        
        // 处理UI状态变化
        processUIState();
        
        // 清屏
        glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        
        // 让Renderer处理所有地图渲染
        m_renderer->render();
        
        // 渲染ImGui UI层
        ImGui::Render();
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
        
        // 交换缓冲区
        glfwSwapBuffers(m_window);
    }
}

void Application::keyCallback(GLFWwindow* window, int key, int scancode, int action, int mods) {
    Application* app = getAppPtr(window);
    if (app) {
        // 先让ImGui处理键盘事件
        ImGui_ImplGlfw_KeyCallback(window, key, scancode, action, mods);
        
        // 获取ImGui IO以检查是否要捕获键盘
        ImGuiIO& io = ImGui::GetIO();
        if (io.WantCaptureKeyboard)
            return;
            
        // 处理应用程序自己的逻辑
        if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS) {
            glfwSetWindowShouldClose(window, GLFW_TRUE);
        }
    }
}

void Application::charCallback(GLFWwindow* window, unsigned int codepoint) {
    Application* app = getAppPtr(window);
    if (app) {
        // 直接使用ImGui的字符回调
        ImGui_ImplGlfw_CharCallback(window, codepoint);
    }
}

} // namespace PathGlyph 