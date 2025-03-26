#include "tile.h"
#include <glad/glad.h>
#include <cmath>
#include <iostream>
#include <glm/gtc/constants.hpp>

namespace PathGlyph {

// 构造函数
Tile::Tile(int x, int y, TileType type) 
    : x_(x), y_(y), screenX_(0.0f), screenY_(0.0f) {
    // 初始化位域结构
    flags_.type = static_cast<unsigned int>(type);
    flags_.walkable = (type != TileType::Wall);  // 墙不可通行
    flags_.style = 0;
    
    // 计算等轴测坐标
    calculateIsoCoordinates();
}

// 等轴测转换：逻辑坐标→屏幕坐标
glm::vec2 Tile::isoToScreen(float x, float y) {
  return glm::vec2(
      (x - y) * (TILE_WIDTH / 2.0f),
      (x + y) * (TILE_HEIGHT / 2.0f)
  );
}

// 计算等轴测坐标
void Tile::calculateIsoCoordinates() {
  // 等轴测投影：屏幕x = (逻辑x - 逻辑y) * TILE_WIDTH/2
  //            屏幕y = (逻辑x + 逻辑y) * TILE_HEIGHT/2
  glm::vec2 screenPos = isoToScreen(static_cast<float>(x_), static_cast<float>(y_));
  screenX_ = screenPos.x;
  screenY_ = screenPos.y;
}

// 设置图块类型
void Tile::setType(TileType type) {
    flags_.type = static_cast<unsigned int>(type);
    flags_.walkable = (type != TileType::Wall);  // 更新通行性
}

// 等轴测转换：屏幕坐标→逻辑坐标
glm::vec2 Tile::screenToIso(float x, float y) {
    // 逆变换矩阵求解
    float tileWidthHalf = TILE_WIDTH / 2.0f;
    float tileHeightHalf = TILE_HEIGHT / 2.0f;
    
    // 求解线性方程组：
    // x_screen = (x_logic - y_logic) * tileWidthHalf
    // y_screen = (x_logic + y_logic) * tileHeightHalf
    float logicX = (y / tileHeightHalf + x / tileWidthHalf) / 2.0f;
    float logicY = (y / tileHeightHalf - x / tileWidthHalf) / 2.0f;
    
    return glm::vec2(logicX, logicY);
}

int Tile::getTextureIDForType(TileType type) const {
  switch (type) {
      case TileType::Wall:   return 1; // 墙的纹理ID
      case TileType::Ground: return 2; // 地面的纹理ID
      default:           return 0; // 默认纹理ID
  }
}

glm::vec2 Tile::getTexCoordForVertex(size_t vertexIndex) const {
  // 假设顶点的纹理坐标是基于顶点的顺序设置的
  switch (vertexIndex) {
      case 0: return glm::vec2(0.0f, 1.0f); // 左上
      case 1: return glm::vec2(0.5f, 0.0f); // 顶部
      case 2: return glm::vec2(1.0f, 1.0f); // 右上
      case 3: return glm::vec2(0.5f, 1.0f); // 底部
      case 4: return glm::vec2(0.0f, 0.0f); // 左下（侧面）
      case 5: return glm::vec2(0.5f, 0.0f); // 顶部（侧面）
      case 6: return glm::vec2(1.0f, 0.0f); // 右下（侧面）
      case 7: return glm::vec2(0.5f, 1.0f); // 底部（侧面）
      default: return glm::vec2(0.0f, 0.0f); // 默认值
  }
}

// 获取图块的顶点数据
void Tile::getVertices(std::array<Vertex, VERTICES_PER_TILE>& vertices) const {
  // 初始化顶点数组，计算顶点位置
  calculateVertexPositions(vertices);

  // 获取纹理ID（假设每种类型对应一个固定的纹理ID）
  int textureID = getTextureIDForType(getType());

  // 为每个顶点设置属性
  for (size_t i = 0; i < VERTICES_PER_TILE; ++i) {
      Vertex& vertex = vertices[i];

      // 设置纹理坐标
      vertex.texCoord = getTexCoordForVertex(i);

      // 设置法线（假设法线向上，对于水平面来说已经够了）
      vertex.normal = glm::vec3(0.0f, 0.0f, 1.0f);

      // 设置纹理ID
      vertex.textureID = textureID;
  }
}

// 计算顶点位置
void Tile::calculateVertexPositions(std::array<Vertex, VERTICES_PER_TILE>& vertices) const {
    float w = TILE_WIDTH / 2.0f;   // 半宽
    float h = TILE_HEIGHT / 2.0f;  // 半高
    float d = TILE_DEPTH;          // 深度
    
    // 判断是否为墙（需要高度）
    bool isWall = (getType() == TileType::Wall);
    float height = isWall ? d : 0.0f;
    
    // 基础位置 (等轴测投影的中心点)
    float baseX = screenX_;
    float baseY = screenY_;
    
    // 顶面四个顶点
    vertices[0].position = glm::vec3(baseX - w, baseY - h/2.0f, height);  // 左上
    vertices[1].position = glm::vec3(baseX,     baseY,          height);  // 顶部
    vertices[2].position = glm::vec3(baseX + w, baseY - h/2.0f, height);  // 右上
    vertices[3].position = glm::vec3(baseX,     baseY - h,       height);  // 底部
    
    // 侧面四个顶点 (仅用于墙)
    if (isWall) {
        vertices[4].position = glm::vec3(baseX - w, baseY - h/2.0f, 0.0f);  // 左上-底
        vertices[5].position = glm::vec3(baseX,     baseY,          0.0f);  // 顶部-底
        vertices[6].position = glm::vec3(baseX + w, baseY - h/2.0f, 0.0f);  // 右上-底
        vertices[7].position = glm::vec3(baseX,     baseY - h,       0.0f);  // 底部-底
    } else {
        // 非墙体，侧面顶点与顶面相同
        for (int i = 0; i < 4; ++i) {
            vertices[i + 4].position = vertices[i].position;
        }
    }
}

// 生成一个图块的索引数据，用于定义顶点之间的连接关系，从而告诉 GPU 如何绘制图块的几何形状
void Tile::getIndices(std::array<unsigned int, INDICES_PER_TILE>& indices, unsigned int baseIndex) const {
    // 顶面两个三角形 (顺时针)
    indices[0] = baseIndex + 0;
    indices[1] = baseIndex + 1;
    indices[2] = baseIndex + 3;
    
    indices[3] = baseIndex + 1;
    indices[4] = baseIndex + 2;
    indices[5] = baseIndex + 3;
    
    // 侧面两个三角形 (仅用于墙)
    if (getType() == TileType::Wall) {
        // 前侧面
        indices[6] = baseIndex + 0;
        indices[7] = baseIndex + 4;
        indices[8] = baseIndex + 3;
        
        indices[9] = baseIndex + 3;
        indices[10] = baseIndex + 4;
        indices[11] = baseIndex + 7;
    } else {
        // 非墙体，填充剩余索引，但不实际渲染
        for (int i = 6; i < 12; ++i) {
            indices[i] = baseIndex;
        }
    }
}

// TileBatch 实现
// ---------------

TileBatch::TileBatch() // 初始 id 和数量都设置为 0
    : vao_(0), vbo_(0), ebo_(0), vertexCount_(0), indexCount_(0) {
    try {
        // 成一个顶点数组对象（VAO），用于存储顶点属性的配置。
        glGenVertexArrays(1, &vao_);
        if (vao_ == 0) {
            std::cerr << "ERROR: Failed to generate VAO for TileBatch" << std::endl;
            throw std::runtime_error("Failed to generate VAO");
        }
        
        // 顶点缓冲对象，用于存储顶点数据（如位置、颜色等）。
        glGenBuffers(1, &vbo_);
        if (vbo_ == 0) {
            std::cerr << "ERROR: Failed to generate VBO for TileBatch" << std::endl;
            throw std::runtime_error("Failed to generate VBO");
        }
        
        // 索引缓冲对象，用于存储索引数据（定义顶点的连接关系）。
        glGenBuffers(1, &ebo_);
        if (ebo_ == 0) {
            std::cerr << "ERROR: Failed to generate EBO for TileBatch" << std::endl;
            throw std::runtime_error("Failed to generate EBO");
        }
        
        // 绑定到 opengl 上下文可以理解为将一个指针指向了这个对象
        // 绑定 VAO 到当前 opengl 上下文状态中，表示接下来的顶点属性配置将存储在这个 VAO 中。
        glBindVertexArray(vao_);
        // 绑定 VBO 到当前 OpenGL 上下文的状态中，表示接下来的顶点数据操作将作用于这个缓冲对象。
        glBindBuffer(GL_ARRAY_BUFFER, vbo_);

        // 该配置基于 当前绑定的 VBO
        // 定义顶点属性数据格式
        // 用于告诉 GPU 如何从顶点缓冲对象（VBO）中解析顶点数据
        // 配置顶点属性：位置 (location = 0)
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Tile::Vertex), 
                (void*)offsetof(Tile::Vertex, position));
        // OpenGL 默认禁用所有顶点属性。若未启用，顶点着色器中的 location=0 将无法接收数据，导致渲染错误（如黑屏或顶点位置错误）。
        // 启用顶点属性位置 0，允许顶点着色器读取该属性数据
        // 该启用状态会被当前绑定的 VAO 记录，后续渲染时只需绑定 VAO 即可恢复状态
        glEnableVertexAttribArray(0);

        // 配置顶点属性：纹理坐标 (location = 1)
        glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, sizeof(Tile::Vertex), 
                (void*)offsetof(Tile::Vertex, texCoord));
        glEnableVertexAttribArray(1);
        
        // 配置顶点属性：法线 (location = 2)
        glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, sizeof(Tile::Vertex), 
                (void*)offsetof(Tile::Vertex, normal));
        glEnableVertexAttribArray(2);
        
        // 配置顶点属性：纹理ID (location = 3)
        glVertexAttribIPointer(3, 1, GL_INT, sizeof(Tile::Vertex), 
                (void*)offsetof(Tile::Vertex, textureID));
        glEnableVertexAttribArray(3);
        
        // 绑定索引缓冲对象
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo_);
        
        // 解绑顶点数组对象，以避免之后的操作意外修改它
        glBindVertexArray(0);
        
        // 检查OpenGL错误
        GLenum err = glGetError();
        if (err != GL_NO_ERROR) {
            std::cerr << "OpenGL error in TileBatch constructor: " << err << std::endl;
        } else {
            std::cout << "TileBatch initialized successfully: VAO=" << vao_ 
                    << ", VBO=" << vbo_ << ", EBO=" << ebo_ << std::endl;
        }
    } catch (const std::exception& e) {
        std::cerr << "Exception in TileBatch constructor: " << e.what() << std::endl;
        
        // 清理已创建的资源
        if (vao_ != 0) {
            glDeleteVertexArrays(1, &vao_);
            vao_ = 0;
        }
        if (vbo_ != 0) {
            glDeleteBuffers(1, &vbo_);
            vbo_ = 0;
        }
        if (ebo_ != 0) {
            glDeleteBuffers(1, &ebo_);
            ebo_ = 0;
        }
        
        // 不重新抛出异常，让程序有机会继续执行
    }
}

TileBatch::~TileBatch() {
    // 清理OpenGL资源
    glDeleteVertexArrays(1, &vao_);
    glDeleteBuffers(1, &vbo_);
    glDeleteBuffers(1, &ebo_);
}

void TileBatch::addTile(const Tile& tile) {
    // 获取顶点数据
    std::array<Tile::Vertex, Tile::VERTICES_PER_TILE> vertices;
    tile.getVertices(vertices);
    
    // 获取索引数据
    std::array<unsigned int, Tile::INDICES_PER_TILE> indices;
    tile.getIndices(indices, vertexCount_);
    
    // 添加到批次中
    vertices_.insert(vertices_.end(), vertices.begin(), vertices.end());
    indices_.insert(indices_.end(), indices.begin(), indices.end());
    
    // 更新计数
    vertexCount_ += Tile::VERTICES_PER_TILE;
    indexCount_ += Tile::INDICES_PER_TILE;
}

void TileBatch::upload() {
    // 数据上传至OpenGL
    if (vao_ == 0 || vbo_ == 0 || ebo_ == 0) {
        std::cerr << "ERROR: Cannot upload data - OpenGL objects not initialized" << std::endl;
        return;
    }
    
    if (vertices_.empty()) {
        std::cerr << "WARNING: No vertex data to upload to GPU" << std::endl;
        return;
    }
    
    try {
        // 记录图块顶点数 - 图块总是先添加的
        tileVertexCount_ = vertices_.size() - (pathVertexCount_ + obstacleVertexCount_ + agentVertexCount_);
        
        glBindVertexArray(vao_);
        
        // 上传顶点数据
        glBindBuffer(GL_ARRAY_BUFFER, vbo_);
        glBufferData(GL_ARRAY_BUFFER, vertices_.size() * sizeof(Tile::Vertex), 
                    vertices_.data(), GL_STATIC_DRAW);
        
        // 上传索引数据（如果有）
        if (!indices_.empty()) {
            glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo_);
            glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices_.size() * sizeof(unsigned int), 
                        indices_.data(), GL_STATIC_DRAW);
            indexCount_ = indices_.size();
        }
        
        // 设置顶点属性
        // 位置 (location = 0)
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Tile::Vertex), 
                (void*)offsetof(Tile::Vertex, position));
        glEnableVertexAttribArray(0);
        
        // 纹理坐标 (location = 1)
        glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, sizeof(Tile::Vertex), 
                (void*)offsetof(Tile::Vertex, texCoord));
        glEnableVertexAttribArray(1);
        
        // 法线 (location = 2)
        glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, sizeof(Tile::Vertex), 
                (void*)offsetof(Tile::Vertex, normal));
        glEnableVertexAttribArray(2);
        
        // 颜色 (location = 3) - 新增
        glVertexAttribPointer(3, 3, GL_FLOAT, GL_FALSE, sizeof(Tile::Vertex), 
                (void*)offsetof(Tile::Vertex, color));
        glEnableVertexAttribArray(3);
        
        // 纹理ID (location = 4)
        glVertexAttribIPointer(4, 1, GL_INT, sizeof(Tile::Vertex), 
                (void*)offsetof(Tile::Vertex, textureID));
        glEnableVertexAttribArray(4);
        
        // 元素类型 (location = 5) - 新增
        glVertexAttribIPointer(5, 1, GL_UNSIGNED_BYTE, sizeof(Tile::Vertex), 
                (void*)offsetof(Tile::Vertex, elementType));
        glEnableVertexAttribArray(5);
        
        // 检查OpenGL错误
        GLenum err = glGetError();
        if (err != GL_NO_ERROR) {
            std::cerr << "OpenGL error in TileBatch::upload: " << err << std::endl;
        } else {
            std::cout << "Uploaded vertices to GPU: " << vertices_.size() 
                    << " (Tiles: " << tileVertexCount_ 
                    << ", Path: " << pathVertexCount_ 
                    << ", Obstacles: " << obstacleVertexCount_ 
                    << ", Agents: " << agentVertexCount_ << ")" << std::endl;
        }
        
        glBindVertexArray(0);
    } catch (const std::exception& e) {
        std::cerr << "Exception in TileBatch::upload: " << e.what() << std::endl;
    }
}

// 将GenericVertex转换为Tile::Vertex
Tile::Vertex TileBatch::convertToTileVertex(const GenericVertex& genericVertex) {
    Tile::Vertex vertex;
    vertex.position = genericVertex.position;
    vertex.color = genericVertex.color;
    vertex.elementType = genericVertex.elementType;
    
    // 默认值
    vertex.texCoord = glm::vec2(0.0f, 0.0f);
    vertex.normal = glm::vec3(0.0f, 1.0f, 0.0f);  // 默认向上的法线
    vertex.textureID = 0;  // 无纹理
    
    return vertex;
}

// 添加路径段
void TileBatch::addPathSegment(const glm::vec3& start, const glm::vec3& end, const glm::vec3& color, float width, float height) {
    // 路径的方向
    glm::vec3 dir = end - start;
    glm::vec2 dir2d(dir.x, dir.y);
    glm::vec2 perp(-dir2d.y, dir2d.x);
    
    if (glm::length(dir2d) > 0.0001f) {
        perp = glm::normalize(perp) * width;
    } else {
        perp = glm::vec2(width, 0.0f);
    }
    
    // 创建四边形（两个三角形）
    GenericVertex v1, v2, v3, v4;
    v1.position = glm::vec3(start.x + perp.x, start.y + perp.y, height);
    v2.position = glm::vec3(start.x - perp.x, start.y - perp.y, height);
    v3.position = glm::vec3(end.x - perp.x, end.y - perp.y, height);
    v4.position = glm::vec3(end.x + perp.x, end.y + perp.y, height);
    
    v1.color = v2.color = v3.color = v4.color = color;
    v1.elementType = v2.elementType = v3.elementType = v4.elementType = 
        static_cast<uint8_t>(RenderElementType::Path);
    
    // 转换为Tile::Vertex并添加到顶点数组
    vertices_.push_back(convertToTileVertex(v1));
    vertices_.push_back(convertToTileVertex(v2));
    vertices_.push_back(convertToTileVertex(v3));
    
    vertices_.push_back(convertToTileVertex(v1));
    vertices_.push_back(convertToTileVertex(v3));
    vertices_.push_back(convertToTileVertex(v4));
    
    // 更新路径顶点计数
    pathVertexCount_ += 6;
    vertexCount_ += 6;
}

// 添加障碍物
void TileBatch::addObstacle(const glm::vec2& position, float radius, const glm::vec3& color, bool isDynamic) {
    glm::vec2 iso = Tile::isoToScreen(position.x, position.y);
    float radiusScaled = radius * Tile::TILE_WIDTH * 0.5f;
    float z = 0.5f;  // 障碍物高度
    
    // 创建八边形近似圆形
    const int segments = 8;
    for (int i = 0; i < segments; ++i) {
        float angle1 = 2.0f * glm::pi<float>() * i / segments;
        float angle2 = 2.0f * glm::pi<float>() * ((i + 1) % segments) / segments;
        
        float x1 = iso.x + std::cos(angle1) * radiusScaled;
        float y1 = iso.y + std::sin(angle1) * radiusScaled;
        float x2 = iso.x + std::cos(angle2) * radiusScaled;
        float y2 = iso.y + std::sin(angle2) * radiusScaled;
        
        // 顶面三角形
        GenericVertex v1, v2, v3;
        v1.position = glm::vec3(iso.x, iso.y, z);
        v2.position = glm::vec3(x1, y1, z);
        v3.position = glm::vec3(x2, y2, z);
        
        v1.color = v2.color = v3.color = color;
        v1.elementType = v2.elementType = v3.elementType = 
            static_cast<uint8_t>(RenderElementType::Obstacle);
        
        vertices_.push_back(convertToTileVertex(v1));
        vertices_.push_back(convertToTileVertex(v2));
        vertices_.push_back(convertToTileVertex(v3));
        
        // 侧面三角形 (如果需要)
        GenericVertex v4, v5, v6, v7, v8, v9;
        v4.position = glm::vec3(x1, y1, z);
        v5.position = glm::vec3(x1, y1, 0.0f);
        v6.position = glm::vec3(x2, y2, 0.0f);
        
        v7.position = glm::vec3(x1, y1, z);
        v8.position = glm::vec3(x2, y2, 0.0f);
        v9.position = glm::vec3(x2, y2, z);
        
        v4.color = v5.color = v6.color = v7.color = v8.color = v9.color = color;
        v4.elementType = v5.elementType = v6.elementType = 
            v7.elementType = v8.elementType = v9.elementType = 
            static_cast<uint8_t>(RenderElementType::Obstacle);
        
        vertices_.push_back(convertToTileVertex(v4));
        vertices_.push_back(convertToTileVertex(v5));
        vertices_.push_back(convertToTileVertex(v6));
        
        vertices_.push_back(convertToTileVertex(v7));
        vertices_.push_back(convertToTileVertex(v8));
        vertices_.push_back(convertToTileVertex(v9));
    }
    
    // 更新障碍物顶点计数
    obstacleVertexCount_ += segments * 9;  // 9个顶点/段 (3个顶点/三角形 * 3个三角形/段)
    vertexCount_ += segments * 9;
}

// 添加代理（起点/终点/当前位置）
void TileBatch::addAgent(const glm::vec2& position, const glm::vec3& color, float size, float height) {
    glm::vec2 iso = Tile::isoToScreen(position.x, position.y);
    float halfSize = size * 0.5f;
    
    // 创建六边形
    const int segments = 6;
    for (int i = 0; i < segments; ++i) {
        float angle1 = 2.0f * glm::pi<float>() * i / segments;
        float angle2 = 2.0f * glm::pi<float>() * ((i + 1) % segments) / segments;
        
        float x1 = iso.x + std::cos(angle1) * halfSize;
        float y1 = iso.y + std::sin(angle1) * halfSize;
        float x2 = iso.x + std::cos(angle2) * halfSize;
        float y2 = iso.y + std::sin(angle2) * halfSize;
        
        // 添加三角形
        GenericVertex v1, v2, v3;
        v1.position = glm::vec3(iso.x, iso.y, height);
        v2.position = glm::vec3(x1, y1, height);
        v3.position = glm::vec3(x2, y2, height);
        
        v1.color = v2.color = v3.color = color;
        v1.elementType = v2.elementType = v3.elementType = 
            static_cast<uint8_t>(RenderElementType::Agent);
        
        vertices_.push_back(convertToTileVertex(v1));
        vertices_.push_back(convertToTileVertex(v2));
        vertices_.push_back(convertToTileVertex(v3));
    }
    
    // 更新代理顶点计数
    agentVertexCount_ += segments * 3;  // 3个顶点/三角形 * 6个三角形
    vertexCount_ += segments * 3;
}

// 覆盖清空方法，重置所有计数器
void TileBatch::clear() {
    vertices_.clear();
    indices_.clear();
    vertexCount_ = 0;
    indexCount_ = 0;
    tileVertexCount_ = 0;
    pathVertexCount_ = 0;
    obstacleVertexCount_ = 0;
    agentVertexCount_ = 0;
}

// 新的渲染方法
void TileBatch::render(unsigned int shaderProgram) {
    if (vertexCount_ == 0) return;
    
    glUseProgram(shaderProgram);
    glBindVertexArray(vao_);
    
    // 渲染图块（使用索引）
    if (indexCount_ > 0) {
        glDrawElements(GL_TRIANGLES, indexCount_, GL_UNSIGNED_INT, 0);
    }
    
    // 渲染其他元素（不使用索引）
    unsigned int offset = tileVertexCount_;
    
    // 渲染路径
    if (pathVertexCount_ > 0) {
        glDrawArrays(GL_TRIANGLES, offset, pathVertexCount_);
        offset += pathVertexCount_;
    }
    
    // 渲染障碍物
    if (obstacleVertexCount_ > 0) {
        glDrawArrays(GL_TRIANGLES, offset, obstacleVertexCount_);
        offset += obstacleVertexCount_;
    }
    
    // 渲染代理
    if (agentVertexCount_ > 0) {
        glDrawArrays(GL_TRIANGLES, offset, agentVertexCount_);
    }
    
    glBindVertexArray(0);
}

} // namespace PathGlyph