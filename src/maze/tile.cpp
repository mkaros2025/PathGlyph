#include "tile.h"
#include <glad/glad.h>
#include <cmath>

namespace PathGlyph {

// 构造函数
Tile::Tile(int x, int y, Type type) 
    : x_(x), y_(y), screenX_(0.0f), screenY_(0.0f) {
    // 初始化位域结构
    flags_.type = static_cast<unsigned int>(type);
    flags_.walkable = (type != Type::Wall);  // 墙不可通行
    flags_.highlighted = false;
    flags_.reserved = 0;
    
    // 计算等轴测坐标
    calculateIsoCoordinates();
}

// 设置图块类型
void Tile::setType(Type type) {
    flags_.type = static_cast<unsigned int>(type);
    flags_.walkable = (type != Type::Wall);  // 更新通行性
}

// 设置高亮状态
void Tile::setHighlighted(bool highlighted) {
    flags_.highlighted = highlighted;
}

// 计算等轴测坐标
void Tile::calculateIsoCoordinates() {
    // 等轴测投影：屏幕x = (逻辑x - 逻辑y) * TILE_WIDTH/2
    //            屏幕y = (逻辑x + 逻辑y) * TILE_HEIGHT/2
    glm::vec2 screenPos = isoToScreen(static_cast<float>(x_), static_cast<float>(y_));
    screenX_ = screenPos.x;
    screenY_ = screenPos.y;
}

// 等轴测转换：逻辑坐标→屏幕坐标
glm::vec2 Tile::isoToScreen(float x, float y) {
    return glm::vec2(
        (x - y) * (TILE_WIDTH / 2.0f),
        (x + y) * (TILE_HEIGHT / 2.0f)
    );
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

// 获取图块的顶点数据
void Tile::getVertices(std::array<Vertex, VERTICES_PER_TILE>& vertices) const {
    // 初始化顶点数组
    calculateVertexPositions(vertices);
    
    // 获取颜色
    glm::vec3 color = getTypeColor(getType());
    
    // 如果高亮，调整颜色亮度
    if (isHighlighted()) {
        color *= 1.3f; // 提高亮度
    }
    
    // 设置所有顶点的颜色
    for (auto& vertex : vertices) {
        vertex.color = color;
        
        // 侧面颜色稍微暗一些，创建深度感
        if (vertex.position.z < 0.01f) {
            vertex.color *= 0.7f;
        }
    }
}

// 计算顶点位置
void Tile::calculateVertexPositions(std::array<Vertex, VERTICES_PER_TILE>& vertices) const {
    float w = TILE_WIDTH / 2.0f;   // 半宽
    float h = TILE_HEIGHT / 2.0f;  // 半高
    float d = TILE_DEPTH;          // 深度
    
    // 判断是否为墙（需要高度）
    bool isWall = (getType() == Type::Wall);
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

// 获取图块的索引数据
void Tile::getIndices(std::array<unsigned int, INDICES_PER_TILE>& indices, unsigned int baseIndex) const {
    // 顶面两个三角形 (顺时针)
    indices[0] = baseIndex + 0;
    indices[1] = baseIndex + 1;
    indices[2] = baseIndex + 3;
    
    indices[3] = baseIndex + 1;
    indices[4] = baseIndex + 2;
    indices[5] = baseIndex + 3;
    
    // 侧面两个三角形 (仅用于墙)
    if (getType() == Type::Wall) {
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

// 获取图块类型对应的颜色
glm::vec3 Tile::getTypeColor(Type type) {
    switch (type) {
        case Type::Wall:
            return glm::vec3(0.5f, 0.5f, 0.5f);  // 灰色
        case Type::Ground:
            return glm::vec3(0.8f, 0.8f, 0.7f);  // 淡黄色
        case Type::Path:
            return glm::vec3(0.2f, 0.6f, 1.0f);  // 蓝色
        case Type::Start:
            return glm::vec3(0.0f, 0.8f, 0.0f);  // 绿色
        case Type::Goal:
            return glm::vec3(1.0f, 0.2f, 0.2f);  // 红色
        default:
            return glm::vec3(1.0f, 1.0f, 1.0f);  // 白色 (默认)
    }
}

// TileBatch 实现
// ---------------

// TileBatch 实现
// ---------------

TileBatch::TileBatch() 
    : vao_(0), vbo_(0), ebo_(0), vertexCount_(0), indexCount_(0) {
    // 创建顶点数组对象
    glGenVertexArrays(1, &vao_);
    glGenBuffers(1, &vbo_);
    glGenBuffers(1, &ebo_);
    
    // 设置顶点属性 - 这是关键的修复部分
    glBindVertexArray(vao_);
    
    // 绑定但不分配数据 - 数据将在 upload() 中分配
    glBindBuffer(GL_ARRAY_BUFFER, vbo_);
    
    // 设置顶点位置属性 (位置 0)
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Tile::Vertex), 
                         (void*)offsetof(Tile::Vertex, position));
    glEnableVertexAttribArray(0);
    
    // 设置顶点颜色属性 (位置 1)
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(Tile::Vertex), 
                         (void*)offsetof(Tile::Vertex, color));
    glEnableVertexAttribArray(1);
    
    // 绑定索引缓冲，数据将在 upload() 中分配
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo_);
    
    // 解绑 VAO，保持 EBO 绑定状态
    glBindVertexArray(0);
    
    // 预分配一些内存以减少重新分配
    vertices_.reserve(100 * Tile::VERTICES_PER_TILE);
    indices_.reserve(100 * Tile::INDICES_PER_TILE);
}

TileBatch::~TileBatch() {
    // 清理OpenGL资源
    if (vao_ != 0) glDeleteVertexArrays(1, &vao_);
    if (vbo_ != 0) glDeleteBuffers(1, &vbo_);
    if (ebo_ != 0) glDeleteBuffers(1, &ebo_);
}

void TileBatch::addTile(const Tile& tile) {
    // 获取顶点数据
    std::array<Tile::Vertex, Tile::VERTICES_PER_TILE> vertices;
    tile.getVertices(vertices);
    
    // 获取索引数据
    std::array<unsigned int, Tile::INDICES_PER_TILE> indices;
    tile.getIndices(indices, vertexCount_);
    
    // 添加到批次
    vertices_.insert(vertices_.end(), vertices.begin(), vertices.end());
    indices_.insert(indices_.end(), indices.begin(), indices.end());
    
    // 更新计数
    vertexCount_ += Tile::VERTICES_PER_TILE;
    indexCount_ += Tile::INDICES_PER_TILE;
}

void TileBatch::clear() {
    // 清空数据
    vertices_.clear();
    indices_.clear();
    vertexCount_ = 0;
    indexCount_ = 0;
}

// 新方法: 将顶点和索引数据上传到GPU
void TileBatch::upload() {
    if (vertices_.empty() || indices_.empty()) {
        return;  // 没有数据可上传
    }
    
    // 绑定VAO
    glBindVertexArray(vao_);
    
    // 上传顶点数据
    glBindBuffer(GL_ARRAY_BUFFER, vbo_);
    glBufferData(GL_ARRAY_BUFFER, vertices_.size() * sizeof(Tile::Vertex), 
                vertices_.data(), GL_STATIC_DRAW);
    
    // 上传索引数据
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo_);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices_.size() * sizeof(unsigned int), 
                indices_.data(), GL_STATIC_DRAW);
    
    // 解绑VAO
    glBindVertexArray(0);
}

} // namespace PathGlyph