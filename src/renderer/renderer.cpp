#include "renderer.h"
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <iostream>
#include <vector>

namespace PathGlyph {

Renderer::Renderer(Window* window)
    : window_(window) {
    // 获取初始窗口尺寸
    window_->getSize(viewportWidth_, viewportHeight_);
}

Renderer::~Renderer() {
    // 清理OpenGL资源
    if (pathVAO_ != 0) glDeleteVertexArrays(1, &pathVAO_);
    if (pathVBO_ != 0) glDeleteBuffers(1, &pathVBO_);
    if (obstacleVAO_ != 0) glDeleteVertexArrays(1, &obstacleVAO_);
    if (obstacleVBO_ != 0) glDeleteBuffers(1, &obstacleVBO_);
    if (agentVAO_ != 0) glDeleteVertexArrays(1, &agentVAO_);
    if (agentVBO_ != 0) glDeleteBuffers(1, &agentVBO_);
    
    // tileBatch_会自行清理其资源
}

GLuint Renderer::createVAO() {
  GLuint vao;
  glGenVertexArrays(1, &vao);
  return vao;
}

GLuint Renderer::createVBO(const void* data, size_t size, bool dynamic) {
  GLuint vbo;
  glGenBuffers(1, &vbo);
  glBindBuffer(GL_ARRAY_BUFFER, vbo);
  glBufferData(GL_ARRAY_BUFFER, size, data, dynamic ? GL_DYNAMIC_DRAW : GL_STATIC_DRAW);
  return vbo;
}

unsigned int Renderer::createEBO(const void* data, size_t size) {
  unsigned int ebo;
  glGenBuffers(1, &ebo);
  glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo);
  glBufferData(GL_ELEMENT_ARRAY_BUFFER, size, data, GL_STATIC_DRAW);
  return ebo;
}

// 初始化一下
bool Renderer::initialize() {
    // 加载着色器
    if (!loadShaders()) {
        std::cerr << "Failed to load shaders" << std::endl;
        return false;
    }
    
    // 创建渲染资源
    pathVAO_ = createVAO();
    pathVBO_ = createVBO(nullptr, 0, true); // 动态缓冲
    
    obstacleVAO_ = createVAO();
    obstacleVBO_ = createVBO(nullptr, 0, true); // 动态缓冲
    
    agentVAO_ = createVAO();
    agentVBO_ = createVBO(nullptr, 0, true); // 动态缓冲
    
    // 启用深度测试
    enableDepthTest(true);
    
    // 计算投影矩阵
    updateMatrices();
    
    return true;
}

bool Renderer::loadShaders() {
  try {
      // 加载迷宫图块着色器
      mazeShader_ = std::make_unique<Shader>("tile.vert", "tile.frag");
      
      // 加载路径着色器
      pathShader_ = std::make_unique<Shader>("path.vert", "path.frag");
      
      // 加载实体着色器 (用于障碍物和代理)
      entityShader_ = std::make_unique<Shader>("entity.vert", "entity.frag");
      
      return true;
  } catch (const std::exception& e) {
      std::cerr << "Shader loading error: " << e.what() << std::endl;
      return false;
  }
}

void Renderer::setMaze(const Maze* maze) {
    maze_ = maze;
    needsUpdateMaze_ = true;
    needsUpdatePath_ = true;
    needsUpdateObstacles_ = true;
}

void Renderer::render() {
    if (!maze_) return;
    
    // 清除屏幕和深度缓冲
    glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    
    // 更新变换矩阵
    updateMatrices();
    
    // 根据需要更新几何体数据
    if (needsUpdateMaze_) {
        updateMazeGeometry();
        needsUpdateMaze_ = false;
    }
    
    if (needsUpdatePath_ && config_.showPath) {
        updatePathGeometry();
        needsUpdatePath_ = false;
    }
    
    if (needsUpdateObstacles_ && config_.showObstacles) {
        updateObstacleGeometry();
        needsUpdateObstacles_ = false;
    }
    
    // 更新代理位置
    updateAgentGeometry();
    
    // 设置线框模式
    enableWireframe(config_.wireframeMode);
    
    // 按Z序渲染各个元素
    renderMaze();
    if (config_.showPath) renderPath();
    if (config_.showObstacles) renderObstacles();
    renderAgents();
    
    // 恢复线框模式
    enableWireframe(false);
}

// factor 是缩放倍率
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

void Renderer::updateVBO(unsigned int vbo, const void* data, size_t size, size_t offset) {
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferSubData(GL_ARRAY_BUFFER, offset, size, data);
}

void Renderer::setupVertexAttributes(int location, int size, int stride, int offset) {
    glVertexAttribPointer(location, size, GL_FLOAT, GL_FALSE, stride, (void*)(intptr_t)offset);
    glEnableVertexAttribArray(location);
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
    // 创建正交投影矩阵
    // 计算视口的宽高比（aspect ratio）
    float aspectRatio = static_cast<float>(viewportWidth_) / viewportHeight_;
    
    // 计算正交投影的基础尺寸，考虑缩放级别（zoomLevel）
    // 缩放级别越高，视图范围越小，场景看起来越大
    float orthoSize = 10.0f / config_.zoomLevel;

    // 使用 glm::ortho 创建正交投影矩阵
    // 参数分别为：左、右、下、上、近平面、远平面
    // 这里的近平面和远平面设置为 -100.0f 和 100.0f，确保场景中的所有物体都在可见范围内
    projectionMatrix_ = glm::ortho(
        -orthoSize * aspectRatio, orthoSize * aspectRatio, // 左、右边界
        -orthoSize, orthoSize,                             // 下、上边界
        -100.0f, 100.0f                                    // 近平面、远平面
    );

    // 创建视图矩阵
    // 初始化为单位矩阵
    viewMatrix_ = glm::mat4(1.0f);

    // 应用平移变换，根据相机的偏移量（cameraOffset）移动视图
    // 偏移量由用户的平移操作（pan）控制
    viewMatrix_ = glm::translate(viewMatrix_, glm::vec3(config_.cameraOffset.x, config_.cameraOffset.y, 0.0f));

    // 设置着色器的投影矩阵和视图矩阵，其实都是统一的
    // 如果迷宫着色器存在，设置其投影矩阵和视图矩阵
    if (mazeShader_) {
        mazeShader_->use(); // 激活迷宫着色器
        mazeShader_->setMat4("projection", projectionMatrix_); // 设置投影矩阵
        mazeShader_->setMat4("view", viewMatrix_);             // 设置视图矩阵
    }

    // 如果路径着色器存在，设置其投影矩阵和视图矩阵
    if (pathShader_) {
        pathShader_->use(); // 激活路径着色器
        pathShader_->setMat4("projection", projectionMatrix_); // 设置投影矩阵
        pathShader_->setMat4("view", viewMatrix_);             // 设置视图矩阵
    }

    // 如果实体着色器（用于障碍物和代理）存在，设置其投影矩阵和视图矩阵
    if (entityShader_) {
        entityShader_->use(); // 激活实体着色器
        entityShader_->setMat4("projection", projectionMatrix_); // 设置投影矩阵
        entityShader_->setMat4("view", viewMatrix_);             // 设置视图矩阵
    }
}

// 更新迷宫的几何渲染
void Renderer::updateMazeGeometry() {
    if (!maze_) return;
    
    // 清除之前的图块
    tileBatch_.clear();
    
    // 遍历迷宫格子
    for (int y = 0; y < maze_->getHeight(); ++y) {
        for (int x = 0; x < maze_->getWidth(); ++x) {
            // 确定图块类型
            Tile::Type tileType;
            
            // 一共就俩类型，墙和地面，与障碍物相交就是墙，没相交就是地面
            if (maze_->isObstacle(x, y)) {
                tileType = Tile::Type::Wall;
            } else {
                tileType = Tile::Type::Ground;
            }
            
            // 创建并添加图块
            Tile tile(x, y, tileType);
            tileBatch_.addTile(tile);
        }
    }
    
    // 上传数据到GPU
    tileBatch_.upload();
}

void Renderer::updatePathGeometry() {
    if (!maze_) return;
    
    struct PathVertex {
        glm::vec3 position;
        glm::vec3 color;
    };
    
    const std::vector<Point>& path = maze_->getPath();
    std::vector<PathVertex> vertices;
    vertices.reserve(path.size() * 4); // 每个路径点需要4个顶点
    
    if (!path.empty()) {
        // 路径颜色
        glm::vec3 pathColor(0.2f, 0.6f, 1.0f);
        
        for (const auto& point : path) {
            // 转换为等轴测坐标
            glm::vec2 iso = Tile::isoToScreen(point.x + 0.5f, point.y + 0.5f);
            
            // 创建路径点的四边形
            float halfWidth = Tile::TILE_WIDTH * 0.2f;  // 路径宽度为图块的20%
            float halfHeight = Tile::TILE_HEIGHT * 0.2f;
            float z = 0.1f; // 略高于地面
            
            vertices.push_back({{iso.x - halfWidth, iso.y - halfHeight, z}, pathColor});
            vertices.push_back({{iso.x + halfWidth, iso.y - halfHeight, z}, pathColor});
            vertices.push_back({{iso.x + halfWidth, iso.y + halfHeight, z}, pathColor});
            vertices.push_back({{iso.x - halfWidth, iso.y + halfHeight, z}, pathColor});
        }
    }
    
    // 更新路径VBO
    glBindVertexArray(pathVAO_);
    if (vertices.empty()) {
        pathVertexCount_ = 0;
        return;
    }
    
    updateVBO(pathVBO_, vertices.data(), vertices.size() * sizeof(PathVertex));
    
    // 设置顶点属性
    setupVertexAttributes(0, 3, sizeof(PathVertex), offsetof(PathVertex, position));
    setupVertexAttributes(1, 3, sizeof(PathVertex), offsetof(PathVertex, color));
    
    pathVertexCount_ = vertices.size();
}

void Renderer::updateObstacleGeometry() {
  if (!maze_) return;
  
  struct ObstacleVertex {
      glm::vec3 position;
      glm::vec3 color;
  };
  
  const auto& obstacles = maze_->getObstacles();
  std::vector<ObstacleVertex> vertices;
  vertices.reserve(obstacles.size() * 16); // 每个障碍物预估16个顶点
  
  // 障碍物颜色
  glm::vec3 staticColor(0.8f, 0.2f, 0.2f);    // 静态障碍物 - 红色
  glm::vec3 dynamicColor(0.8f, 0.5f, 0.2f);   // 动态障碍物 - 橙色
  
  for (const auto& obstacle : obstacles) {
      // 获取位置 - 使用公有接口而非直接访问私有成员
      RealPoint position = obstacle->getPosition();
      bool isDynamic = obstacle->isDynamic();
      double radius = obstacle->getRadius();
      
      // 计算等轴测坐标
      float x = position.x;
      float y = position.y;
      glm::vec2 iso = Tile::isoToScreen(x, y);
      
      // 障碍物大小
      float radiusScaled = radius * Tile::TILE_WIDTH * 0.5f;
      float z = 0.5f; // 障碍物高度
      
      // 选择颜色
      glm::vec3 color = isDynamic ? dynamicColor : staticColor;
      
        // 创建八边形近似圆形
        const int segments = 8;
        for (int i = 0; i < segments; ++i) {
            float angle1 = 2.0f * glm::pi<float>() * i / segments;
            float angle2 = 2.0f * glm::pi<float>() * ((i + 1) % segments) / segments;
            
            float x1 = iso.x + std::cos(angle1) * radius;
            float y1 = iso.y + std::sin(angle1) * radius;
            float x2 = iso.x + std::cos(angle2) * radius;
            float y2 = iso.y + std::sin(angle2) * radius;
            
            // 添加底部顶点
            vertices.push_back({{iso.x, iso.y, 0.0f}, color});
            vertices.push_back({{x1, y1, 0.0f}, color});
            vertices.push_back({{x2, y2, 0.0f}, color});
            
            // 添加顶部顶点
            vertices.push_back({{iso.x, iso.y, z}, color});
            vertices.push_back({{x1, y1, z}, color});
            vertices.push_back({{x2, y2, z}, color});
            
            // 添加侧面顶点
            vertices.push_back({{x1, y1, 0.0f}, color});
            vertices.push_back({{x1, y1, z}, color});
            vertices.push_back({{x2, y2, z}, color});
            vertices.push_back({{x2, y2, 0.0f}, color});
        }
    }
    
    // 更新障碍物VBO
    glBindVertexArray(obstacleVAO_);
    if (vertices.empty()) {
        obstacleVertexCount_ = 0;
        return;
    }
    
    updateVBO(obstacleVBO_, vertices.data(), vertices.size() * sizeof(ObstacleVertex));
    
    // 设置顶点属性
    setupVertexAttributes(0, 3, sizeof(ObstacleVertex), offsetof(ObstacleVertex, position));
    setupVertexAttributes(1, 3, sizeof(ObstacleVertex), offsetof(ObstacleVertex, color));
    
    obstacleVertexCount_ = vertices.size();
}

void Renderer::updateAgentGeometry() {
    if (!maze_) return;
    
    struct AgentVertex {
        glm::vec3 position;
        glm::vec3 color;
    };
    
    std::vector<AgentVertex> vertices;
    vertices.reserve(64); // 预留足够空间
    
    // 代理颜色
    glm::vec3 startColor(0.2f, 0.8f, 0.2f);   // 起点 - 绿色
    glm::vec3 goalColor(0.8f, 0.8f, 0.2f);    // 终点 - 黄色
    glm::vec3 currentColor(0.2f, 0.6f, 0.8f); // 当前位置 - 蓝色
    
    auto addAgent = [&](float x, float y, float z, const glm::vec3& color, float size) {
        glm::vec2 iso = Tile::isoToScreen(x, y);
        float halfSize = size * 0.5f;
        
        // 顶部六边形
        const int segments = 6;
        for (int i = 0; i < segments; ++i) {
            float angle1 = 2.0f * glm::pi<float>() * i / segments;
            float angle2 = 2.0f * glm::pi<float>() * ((i + 1) % segments) / segments;
            
            float x1 = iso.x + std::cos(angle1) * halfSize;
            float y1 = iso.y + std::sin(angle1) * halfSize;
            float x2 = iso.x + std::cos(angle2) * halfSize;
            float y2 = iso.y + std::sin(angle2) * halfSize;
            
            // 添加三角形
            vertices.push_back({{iso.x, iso.y, z}, color});
            vertices.push_back({{x1, y1, z}, color});
            vertices.push_back({{x2, y2, z}, color});
        }
    };
    
    // 添加起点
    const Point& start = maze_->getStart();
    addAgent(start.x + 0.5f, start.y + 0.5f, 0.3f, startColor, Tile::TILE_WIDTH * 0.6f);
    
    // 添加终点
    const Point& goal = maze_->getGoal();
    addAgent(goal.x + 0.5f, goal.y + 0.5f, 0.3f, goalColor, Tile::TILE_WIDTH * 0.6f);
    
    // 添加当前位置
    const Point& current = maze_->getCurrentPosition();
    addAgent(current.x + 0.5f, current.y + 0.5f, 0.4f, currentColor, Tile::TILE_WIDTH * 0.4f);
    
    // 更新代理VBO
    glBindVertexArray(agentVAO_);
    updateVBO(agentVBO_, vertices.data(), vertices.size() * sizeof(AgentVertex));
    
    // 设置顶点属性
    setupVertexAttributes(0, 3, sizeof(AgentVertex), offsetof(AgentVertex, position));
    setupVertexAttributes(1, 3, sizeof(AgentVertex), offsetof(AgentVertex, color));
}

void Renderer::renderMaze() {
    mazeShader_->use();
    
    // 使用TileBatch渲染图块
    glBindVertexArray(tileBatch_.getVAO());
    glDrawElements(GL_TRIANGLES, tileBatch_.getIndexCount(), GL_UNSIGNED_INT, 0);
}

void Renderer::renderPath() {
    if (pathVertexCount_ == 0) return;
    
    enableBlending(true);
    pathShader_->use();
    
    glBindVertexArray(pathVAO_);
    glDrawArrays(GL_QUADS, 0, pathVertexCount_);
    
    enableBlending(false);
}

void Renderer::renderObstacles() {
    if (obstacleVertexCount_ == 0) return;
    
    entityShader_->use();
    
    glBindVertexArray(obstacleVAO_);
    glDrawArrays(GL_TRIANGLES, 0, obstacleVertexCount_);
}

void Renderer::renderAgents() {
    entityShader_->use();
    
    enableBlending(true);
    glBindVertexArray(agentVAO_);
    glDrawArrays(GL_TRIANGLES, 0, 18 * 3); // 3个代理，每个6个三角形，每个三角形3个顶点
    enableBlending(false);
}

} // namespace PathGlyph