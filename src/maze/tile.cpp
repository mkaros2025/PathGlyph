#include "tile.h"
#include <glad/glad.h>
#include <cmath>

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

      // 设置动画时间（假设为 0.0f，后续可以动态更新）
      vertex.animationTime = 0.0f;

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
    // 成一个顶点数组对象（VAO），用于存储顶点属性的配置。
    glGenVertexArrays(1, &vao_);
    // 顶点缓冲对象，用于存储顶点数据（如位置、颜色等）。
    glGenBuffers(1, &vbo_);
    // 索引缓冲对象，用于存储索引数据（定义顶点的连接关系）。
    glGenBuffers(1, &ebo_);
    
    // 绑定到 opengl 上下文可以理解为将一个指针指向了这个对象
    // 绑定 VAO 到当前 opengl 上下文状态中，表示接下来的顶点属性配置将存储在这个 VAO 中。
    glBindVertexArray(vao_);
    // 绑定 VBO 到当前 OpenGL 上下文的状态中，表示接下来的顶点数据操作将作用于这个缓冲对象。
    glBindBuffer(GL_ARRAY_BUFFER, vbo_);

    // 该配置基于 当前绑定的 VBO
    // 定义顶点属性数据格式
    // 用于告诉 GPU 如何从顶点缓冲对象（VBO）中解析顶点数据
    // 配置顶点属性：颜色 (location = 0)
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

    // 配置顶点属性：动画时间 (location = 3)
    glVertexAttribPointer(3, 1, GL_FLOAT, GL_FALSE, sizeof(Tile::Vertex), 
            (void*)offsetof(Tile::Vertex, animationTime));
    glEnableVertexAttribArray(3);

    // 配置顶点属性：纹理ID (location = 4)
    glVertexAttribIPointer(4, 1, GL_INT, sizeof(Tile::Vertex), 
            (void*)offsetof(Tile::Vertex, textureID));
    glEnableVertexAttribArray(4);
    
    // 绑定 EBO，表示接下来的索引数据操作将作用于这个缓冲对象
    // 索引数据将在 upload() 方法中上传到 GPU
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

// 来咯
void TileBatch::addTile(const Tile& tile) {
    // 创建顶点数据，存储单个图块的顶点数据
    std::array<Tile::Vertex, Tile::VERTICES_PER_TILE> vertices;
    tile.getVertices(vertices);
    
    // 创建索引数据，存储单个图块的索引数据
    std::array<unsigned int, Tile::INDICES_PER_TILE> indices;
    tile.getIndices(indices, vertexCount_);
    
    // 添加到批次的末尾
    vertices_.insert(vertices_.end(), vertices.begin(), vertices.end());
    indices_.insert(indices_.end(), indices.begin(), indices.end());
    
    // 更新计数
    vertexCount_ += Tile::VERTICES_PER_TILE;
    indexCount_ += Tile::INDICES_PER_TILE;
}

void TileBatch::clear() {
  vertices_.clear();  // 清空存储顶点数据的容器
  indices_.clear();   // 清空存储索引数据的容器 （EBO）
  vertexCount_ = 0;   // 重置顶点计数器
  indexCount_ = 0;    // 重置索引计数器
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