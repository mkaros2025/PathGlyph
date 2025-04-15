#include "core/application.h"
#include <iostream>
#include <imgui.h>
#include <stdexcept>
#include <chrono>
#include <thread>
#include <cmath>
#include <unistd.h> // 用于getcwd

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
    
    // 创建编辑状态
    m_editState = std::make_shared<EditState>();
    
    // 创建迷宫 - 从JSON文件加载
    m_maze = std::make_shared<Maze>();
    
    // 尝试从默认JSON文件加载迷宫配置
    const std::string MazeFile = "../../../../assets/mazes/default_maze.json";
    if (!m_maze->loadFromJson(MazeFile)) {
        throw std::runtime_error("Failed to load mazefile from " + MazeFile);
    }
    
    // 创建仿真系统
    m_simulation = std::make_shared<Simulation>(m_maze, m_editState);
    
    // 创建UI窗口，传入编辑状态和仿真系统
    m_uiWindow = std::make_shared<ImGuiWindow>(m_window, m_editState, m_simulation);
    if (!m_uiWindow->initialize()) {
        throw std::runtime_error("Failed to initialize ImGui");
    }
    
    // 创建渲染器
    m_renderer = std::make_unique<Renderer>(m_window, m_maze, m_editState);
    
    // 设置回调
    setupCallbacks();
    

    std::cout << "Application initialized successfully." << std::endl;
}

Application::~Application() {
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

    glfwWindowHint(GLFW_SAMPLES, 4); 
    
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
    
    // 设置视口，即用户能看到的那部分窗口，这里就是用户能看到窗口的所有内容
    glViewport(0, 0, m_windowWidth, m_windowHeight);
    
    // 存储this指针用于回调
    glfwSetWindowUserPointer(m_window, this);
    
    return true;
}

void Application::setupCallbacks() {
    // 我们还需要注册这个函数，告诉GLFW我们希望每当窗口调整大小的时候调用这个函数
    glfwSetFramebufferSizeCallback(m_window, framebufferSizeCallback);
    // 鼠标按键回调
    glfwSetMouseButtonCallback(m_window, mouseButtonCallback);
    // 鼠标移动回调
    glfwSetCursorPosCallback(m_window, cursorPosCallback);
    // 鼠标滚轮回调
    glfwSetScrollCallback(m_window, scrollCallback);
}

// 修改渲染窗口大小
void Application::framebufferSizeCallback(GLFWwindow* window, int width, int height) {
    Application* app = getAppPtr(window);
    if (!app) return;

    app->m_windowWidth = width;
    app->m_windowHeight = height;
    glViewport(0, 0, width, height);
    app->m_renderer->handleResize(width, height);
}

// 鼠标点击
void Application::mouseButtonCallback(GLFWwindow* window, int button, int action, int mods) {
    Application* app = getAppPtr(window);
    if (!app) return;

    double xpos = app->m_currentMouseX;
    double ypos = app->m_currentMouseY;
    
    if (button >= 0 && button < 3) {
        if (action == GLFW_PRESS) {
            app->m_mouseButtons[button] = true;
        } else if (action == GLFW_RELEASE) {
            app->m_mouseButtons[button] = false;
        }
    }
    
    bool isOverUI = (xpos < app->m_sidePanelWidth);
    if (isOverUI) {
        return;
    }

    if (button == GLFW_MOUSE_BUTTON_RIGHT) {
        if (action == GLFW_PRESS) {
            app->m_rightMouseDragging = true;
            app->m_lastX = xpos;
            app->m_lastY = ypos;
        } else if (action == GLFW_RELEASE) {
            app->m_rightMouseDragging = false;
        }
    } else if (button == GLFW_MOUSE_BUTTON_LEFT) {
        if (action == GLFW_PRESS) {
            app->m_lastX = xpos;
            app->m_lastY = ypos;
            app->m_leftMouseDownTime = glfwGetTime();
            app->m_leftMouseDragging = false;
        } else if (action == GLFW_RELEASE) {
            if (!app->m_leftMouseDragging) {
                app->handleMouseClick(xpos, ypos);
            }
            app->m_leftMouseDragging = false;
        }
    }
}

void Application::cursorPosCallback(GLFWwindow* window, double xpos, double ypos) {
    Application* app = getAppPtr(window);
    if (!app) return;

    app->m_currentMouseX = xpos;
    app->m_currentMouseY = ypos;
    if (xpos < app->m_sidePanelWidth) return;

    app->handleCursorPos(xpos, ypos);
}

void Application::handleCursorPos(double xpos, double ypos) {
    // 缓存当前鼠标位置
    m_currentMouseX = xpos;
    m_currentMouseY = ypos; 
    // 检查是否在UI区域内，如果在UI区域内直接返回
    if (xpos < m_sidePanelWidth) {
        return;
    }

    // 当鼠标右键按下时进行拖动
    if (m_rightMouseDragging) {
        // 计算移动差值
        double dx = xpos - m_lastX;
        double dy = ypos - m_lastY;
        
        m_renderer->pan(-dx, dy);
    } else if (m_mouseButtons[GLFW_MOUSE_BUTTON_LEFT]) {
        // 计算移动差值
        double dx = xpos - m_lastX;
        double dy = ypos - m_lastY;
        
        // 鼠标移动距离超过了一定值，标记为拖动状态
        double movementThreshold = 3.0; // 移动3像素以上被认为是拖动
        if (!m_leftMouseDragging && (std::abs(dx) > movementThreshold || std::abs(dy) > movementThreshold)) {
            m_leftMouseDragging = true;
        }
        
        if (m_leftMouseDragging) {
            m_renderer->rotate(dx, dy);
        }
    }
    m_lastX = xpos;
    m_lastY = ypos;
}

void Application::scrollCallback(GLFWwindow* window, double xoffset, double yoffset) {
    Application* app = getAppPtr(window);
    if (!app) return;
    
    double xpos = app->m_currentMouseX;
    double ypos = app->m_currentMouseY;
    
    if (xpos < app->m_sidePanelWidth) {
        return;
    }
    
    app->handleScroll(xoffset, yoffset);
}

void Application::handleScroll(double xoffset, double yoffset) {
    const float zoomFactor = 1.1f;
    if (yoffset > 0) {
        m_renderer->zoom(zoomFactor); // 放大
    } else if (yoffset < 0) {
        m_renderer->zoom(1.0f / zoomFactor); // 缩小
    }
}

void Application::handleMouseClick(double x, double y) {
    // 只在编辑模式下处理
    if (m_editState->mode != EditMode::EDIT) {
        return;
    }
    
    // 将屏幕坐标转换为网格坐标
    Point gridPos = screenToGrid(x, y);
    
    // 检查坐标是否有效 - 使用网格坐标检查边界
    if (!m_maze->isInBounds(gridPos)) {
        std::cout << "invalid coordinate" << std::endl;
        return;
    }
    
    // 根据编辑类型执行相应操作
    EditObjectType type = static_cast<EditObjectType>(m_editState->editType);
    switch (type) {
        case EditObjectType::START_POINT:
            // 如果网格上已有起点，先清除
            if (m_maze->isStartPoint(gridPos)) {
                m_maze->clearStart();
            } else {
                m_maze->setStart(gridPos);
            }
            break;
            
        case EditObjectType::END_POINT:
            // 如果网格上已有终点，先清除
            if (m_maze->isEndPoint(gridPos)) {
                m_maze->clearGoal();
            } else {
                m_maze->setGoal(gridPos);
            }
            break;
            
        case EditObjectType::OBSTACLE:
            if (m_editState->obstacleAction == 0) {  // 添加
                if (m_editState->obstacleType == 0) {  // 静态
                    m_maze->addStaticObstacle(gridPos);
                } else {  // 动态
                    // 创建动态障碍物
                    if (m_editState->motionType == 0) {  // 线性运动
                        m_maze->addDynamicObstacle(gridPos, 3.0f, glm::vec2(1.0f, 0.0f));
                    } else {  // 圆周运动
                        Point center = gridPos;  // 默认中心点与当前点相同
                        m_maze->addDynamicObstacle(gridPos, center, 5.0f, 1.0f);
                    }
                }
            } else {  // 删除
                m_maze->removeObstacle(gridPos);
            }
            break;
    }
    
    // 标记几何数据需要更新
    m_renderer->markGeometryForUpdate();
}

// 屏幕坐标到网格坐标
Point Application::screenToGrid(double screenX, double screenY) {
    int gridX, gridY;
    glm::vec2 screenPos(screenX, screenY);
    
    // 使用渲染器的3D投影方法进行坐标转换
    // 获取当前的视图投影矩阵
    if (m_editState) {
        // 从渲染器获取视图投影矩阵
        glm::mat4 viewProj = m_renderer->getViewProjectionMatrix();
        
        // 判断是否点击在UI区域外
        if (screenX < m_sidePanelWidth) {
            std::cout << "点击在UI区域内，不处理" << std::endl;
            return Point(-1, -1);
        }
        
        // 向TileManager传递视图投影矩阵和屏幕坐标
        if (m_renderer->getTileManager()->screenToTileCoordinate(screenPos, gridX, gridY, viewProj)) {
            std::cout << "成功转换为网格坐标: " << gridX << ", " << gridY << std::endl;
            return Point(gridX, gridY);
        }
    }
    
    // 如果转换失败，返回无效坐标
    std::cout << "无法转换为有效网格坐标" << std::endl;
    return Point(-1, -1);
}

Application* Application::getAppPtr(GLFWwindow* window) {
    return static_cast<Application*>(glfwGetWindowUserPointer(window));
}

void Application::run() {
    // 时间相关变量
    float lastTime = glfwGetTime();
    
    // 主循环
    while (!glfwWindowShouldClose(m_window)) {
        // 计算deltaTime
        float currentTime = glfwGetTime();
        float deltaTime = currentTime - lastTime;
        lastTime = currentTime;
        
        // 轮询事件
        glfwPollEvents();
        
        // 处理ImGui输入
        m_uiWindow->handleInput();
        
        m_uiWindow->beginFrame(); // 开始ImGui帧
        m_uiWindow->drawControlPanel(); // 绘制控制面板
        
        if (m_editState->shouldStartSimulation) {
            m_editState->shouldStartSimulation = false;
            m_simulation->start();
        }
        if (m_editState->shouldResetState) {
            m_editState->shouldResetState = false;
            m_simulation->reset();
        }    
        
        // 标记几何体需要更新 - 确保动态障碍物和Agent的位置变化能被渲染出来
        if (m_simulation->isRunning()) {
            m_simulation->update(deltaTime);
            m_renderer->markGeometryForUpdate();
        }
        
        // 清屏
        glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        
        // 让Renderer处理所有地图渲染
        m_renderer->render();
        // 结束ImGui帧
        m_uiWindow->endFrame();
        
        // 交换缓冲区
        glfwSwapBuffers(m_window);
    }
}
}