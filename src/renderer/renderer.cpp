#include "renderer.h"
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <iostream>
#include <vector>
#include <filesystem>

// 添加命名空间别名
namespace fs = std::filesystem;

#if !defined(_WIN32)
#include <unistd.h>
#endif

namespace PathGlyph {

Renderer::Renderer(GLFWwindow* window)
    : window_(window) {
    // 获取初始窗口尺寸
    int width, height;
    glfwGetFramebufferSize(window_, &width, &height);
    viewportWidth_ = width;
    viewportHeight_ = height;
    
    // 在构造函数中自动初始化

    if (!initialize()) {
        std::cerr << "错误：渲染器初始化失败" << std::endl;
        throw std::runtime_error("渲染器初始化失败");
    }
}

bool Renderer::initialize() {
    // 加载通用着色器
    if (!loadShaders()) {
        std::cerr << "Failed to load shaders" << std::endl;
        return false;
    }
    
    // 启用深度测试
    enableDepthTest(true);
    
    // 计算投影矩阵
    updateMatrices();
    
    return true;
}

bool Renderer::loadShaders() {
    std::cout << "开始加载通用着色器..." << std::endl;
    
    // 指定确切的着色器文件路径
    fs::path vertexShaderPath = "../../../../assets/shaders/tile.vert";
    fs::path fragmentShaderPath = "../../../../assets/shaders/tile.frag";
    
    // 检查文件是否存在
    if (!fs::exists(vertexShaderPath)) {
        std::cerr << "错误：找不到顶点着色器文件: " << fs::absolute(vertexShaderPath).string() << std::endl;
        return false;
    }
    
    if (!fs::exists(fragmentShaderPath)) {
        std::cerr << "错误：找不到片段着色器文件: " << fs::absolute(fragmentShaderPath).string() << std::endl;
        return false;
    }
    
    try {
        universalShader_ = std::make_unique<Shader>(vertexShaderPath.string(), fragmentShaderPath.string());
        std::cout << "通用着色器加载成功，ID: " << universalShader_->getID() << std::endl;
        return true;
    } catch (const std::exception& e) {
        std::cerr << "着色器加载错误: " << e.what() << std::endl;
        return false;
    }
}

void Renderer::setMaze(const Maze* maze) {
    maze_ = maze;
    needsUpdateMaze_ = true;
}

void Renderer::render() {
    if (!maze_) return;
    
    // 清除屏幕和深度缓冲
    glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    
    // 检查着色器是否有效
    if (!universalShader_) {
        std::cerr << "无法渲染：着色器未加载" << std::endl;
        return;
    }
    
    // 更新变换矩阵
    updateMatrices();
    
    // 根据需要更新几何体数据
    if (needsUpdateMaze_) {
        updateMazeGeometry();
        needsUpdateMaze_ = false;
    }
    
    // 设置线框模式
    enableWireframe(config_.wireframeMode);
    
    // 设置着色器的uniform变量
    universalShader_->use();
    universalShader_->setMat4("projection", projectionMatrix_);
    universalShader_->setMat4("view", viewMatrix_);
    
    // 使用TileBatch渲染所有内容
    tileBatch_.render(universalShader_->getID());
    
    // 恢复线框模式
    enableWireframe(false);
}

void Renderer::zoom(float factor) {
    config_.zoomLevel *= factor;
    
    // 限制缩放范围
    config_.zoomLevel = std::max(0.01f, std::min(config_.zoomLevel, 10.0f));
    
    // 更新变换矩阵
    updateMatrices();
}

void Renderer::pan(float dx, float dy) {
    // 计算与缩放比例相关的平移量
    float scaledDx = dx / config_.zoomLevel;
    float scaledDy = dy / config_.zoomLevel;
    
    // 更新相机偏移
    config_.cameraOffset.x += scaledDx;
    config_.cameraOffset.y += scaledDy;
    
    // 更新变换矩阵
    updateMatrices();
}

void Renderer::resetView() {
    config_.zoomLevel = 1.0f;
    config_.cameraOffset = glm::vec2(0.0f);
    updateMatrices();
}

void Renderer::handleResize(int width, int height) {
    viewportWidth_ = width;
    viewportHeight_ = height;
    glViewport(0, 0, width, height);
    updateMatrices();
}

void Renderer::enableWireframe(bool enable) {
    if (enable) {
        glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
    } else {
        glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
    }
}

void Renderer::enableDepthTest(bool enable) {
    if (enable) {
        glEnable(GL_DEPTH_TEST);
    } else {
        glDisable(GL_DEPTH_TEST);
    }
}

void Renderer::enableBlending(bool enable) {
    if (enable) {
        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    } else {
        glDisable(GL_BLEND);
    }
}

void Renderer::updateMatrices() {
    // 创建投影矩阵
    float aspect = static_cast<float>(viewportWidth_) / static_cast<float>(viewportHeight_);
    projectionMatrix_ = glm::ortho(-aspect * 500.0f, aspect * 500.0f, -500.0f, 500.0f, -1000.0f, 1000.0f);
    
    // 创建视图矩阵
    viewMatrix_ = glm::mat4(1.0f);
    
    // 应用缩放
    viewMatrix_ = glm::scale(viewMatrix_, glm::vec3(config_.zoomLevel, config_.zoomLevel, 1.0f));
    
    // 应用平移
    viewMatrix_ = glm::translate(viewMatrix_, glm::vec3(config_.cameraOffset.x, config_.cameraOffset.y, 0.0f));
}

// 更新迷宫的几何渲染
void Renderer::updateMazeGeometry() {
    if (!maze_) return;
    
    // 清空批次
    tileBatch_.clear();
    
    // 获取尺寸
    int width = maze_->getWidth();
    int height = maze_->getHeight();
    
    // 1. 添加基础图块
    for (int y = 0; y < height; ++y) {
        for (int x = 0; x < width; ++x) {
            // 确定图块类型
            TileType tileType = maze_->isObstacle(x, y) ? TileType::Wall : TileType::Ground;
            
            // 创建图块并添加到批次
            Tile tile(x, y, tileType);
            
            // 添加到批次
            tileBatch_.addTile(tile);
        }
    }
    
    // 2. 添加路径（如果存在）
    if (maze_->isPathFound() && config_.showPath) {
        const auto& path = maze_->getPath();
        
        for (size_t i = 0; i < path.size() - 1; ++i) {
            // 当前点和下一点
            const Point& current = path[i];
            const Point& next = path[i + 1];
            
            // 将逻辑坐标转换为等轴测坐标
            glm::vec3 start(current.x + 0.5f, current.y + 0.5f, 0.1f);
            glm::vec3 end(next.x + 0.5f, next.y + 0.5f, 0.1f);
            
            // 添加路径段
            tileBatch_.addPathSegment(start, end, glm::vec3(0.2f, 0.6f, 1.0f), 5.0f, 0.05f);
        }
    }
    
    // 3. 添加起点和终点
    const Point& start = maze_->getStart();
    if (start.x >= 0 && start.y >= 0) {
        tileBatch_.addAgent(glm::vec2(start.x + 0.5f, start.y + 0.5f), 
                           glm::vec3(0.2f, 0.8f, 0.2f), // 绿色
                           Tile::TILE_WIDTH * 0.6f, 0.3f);
    }
    
    const Point& goal = maze_->getGoal();
    if (goal.x >= 0 && goal.y >= 0) {
        tileBatch_.addAgent(glm::vec2(goal.x + 0.5f, goal.y + 0.5f),
                          glm::vec3(0.8f, 0.8f, 0.2f), // 黄色
                          Tile::TILE_WIDTH * 0.6f, 0.3f);
    }
    
    // 4. 添加当前位置
    const Point& current = maze_->getCurrentPosition();
    if (current.x >= 0 && current.y >= 0) {
        tileBatch_.addAgent(glm::vec2(current.x + 0.5f, current.y + 0.5f),
                          glm::vec3(0.2f, 0.6f, 0.8f), // 蓝色
                          Tile::TILE_WIDTH * 0.4f, 0.4f);
    }
    
    // 5. 添加障碍物
    if (config_.showObstacles) {
        const auto& obstacles = maze_->getObstacles();
        for (const auto& obstacle : obstacles) {
            if (!obstacle) continue;
            
            RealPoint position = obstacle->getPosition();
            bool isDynamic = obstacle->isDynamic();
            double radius = obstacle->getRadius();
            
            // 动态障碍物是橙色，静态障碍物是红色
            glm::vec3 color = isDynamic ? glm::vec3(0.8f, 0.5f, 0.2f) : glm::vec3(0.8f, 0.2f, 0.2f);
            
            tileBatch_.addObstacle(glm::vec2(position.x, position.y), radius, color, isDynamic);
        }
    }
    
    // 上传到GPU
    tileBatch_.upload();
}

} // namespace PathGlyph