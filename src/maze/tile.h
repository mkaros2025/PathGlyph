#pragma once
#include <array>
#include <cstdint>  // 显式包含 uint8_t 的定义
#include <glm/glm.hpp>
#include <vector>  // 添加缺失的头文件

namespace PathGlyph {

// 图块类型枚举
// 每个基础图块类型支持 多个变体
enum class TileType : uint8_t {
  Wall = 0,    // 墙
  Ground = 1,  // 地面
};

// 等轴测图块渲染类
class Tile {
 public:
  // 图块顶点数据结构
  struct Vertex {
    // 顶点位置是每个顶点在三维空间中的坐标，用于定义图形的几何形状。
    glm::vec3 position;
    // 纹理坐标 (texCoord)
    // 是每个顶点在纹理空间中的坐标，用于将纹理映射到图形表面
    glm::vec2 texCoord;
    glm::vec3 normal;  // 法线
    // float animationTime; // 动画时间
    int textureID;  // 纹理ID
  };

  // 渲染状态
  struct RenderState {
    bool isOverlayActive = false;  // 是否启用叠加层
    glm::vec3 overlayColor{1.0f};  // 叠加层颜色
    float glowIntensity = 0.0f;    // 发光强度
    // float animationTime = 0.0f;       // 动画时间
    float overlayHeight = 0.0f;  // 叠加层高度偏移
  };

  void setOverlay(const OverlayConfig &config) {
    renderState_.hasOverlay = true;
    renderState_.overlayColor = config.color;
    renderState_.overlayHeight = config.height;
    renderState_.glowIntensity = config.glowIntensity;
  }

  // 图块尺寸常量 (像素)
  // 图块在屏幕上的宽度
  static constexpr float TILE_WIDTH = 64.0f;   // 图块宽度
  static constexpr float TILE_HEIGHT = 32.0f;  // 图块高度
  static constexpr float TILE_DEPTH = 16.0f;   // 图块深度（用于3D绘制）

  // 单个图块的顶点数据 (顶部面4个顶点 + 侧面4个顶点)
  static constexpr size_t VERTICES_PER_TILE = 8;
  // 单个图块的索引数据 (顶部面2个三角形 + 侧面2个三角形)
  static constexpr size_t INDICES_PER_TILE = 12;

  // 构造函数
  Tile(int x, int y, TileType type = TileType::Ground);

  int getTextureIDForType(TileType type) const;
  glm::vec2 getTexCoordForVertex(size_t vertexIndex) const;

  // 获取图块数据
  int getX() const { return x_; }
  int getY() const { return y_; }
  bool isWalkable() const { return flags_.walkable; }
  TileType getType() const { return static_cast<TileType>(flags_.type); }

  // 设置图块属性
  void setType(TileType type);
  void setRenderState(const RenderState &state) { renderState_ = state; }
  const RenderState &getRenderState() const { return renderState_; }

  // 等轴测坐标转换
  static glm::vec2 isoToScreen(float x, float y);
  static glm::vec2 screenToIso(float x, float y);

  // 获取顶点数据（用于批量渲染）
  void getVertices(std::array<Vertex, VERTICES_PER_TILE> &vertices) const;
  void getIndices(std::array<unsigned int, INDICES_PER_TILE> &indices,
                  unsigned int baseIndex) const;

 private:
  // 逻辑坐标
  int x_, y_;

  // 使用位域压缩数据
  struct {
    unsigned int type : 3;      // 图块类型 (3位，支持8种类型)
    unsigned int walkable : 1;  // 是否可通行 (1位)
    unsigned int style : 3;     // 样式变体 (3位)
  } flags_;

  // 预计算图块的屏幕坐标（等轴测投影）
  float screenX_, screenY_;
  // 渲染状态
  RenderState renderState_;

  // 计算等轴测坐标
  void calculateIsoCoordinates();
  // 预计算顶点位置（用于优化渲染）
  void calculateVertexPositions(
      std::array<Vertex, VERTICES_PER_TILE> &vertices) const;
};

// 批量渲染图块的辅助类
class TileBatch {
 public:
  // 初始化批次渲染器
  TileBatch();
  ~TileBatch();

  // 添加图块到批次
  void addTile(const Tile &tile);

  // 清空批次
  void clear();
  void upload();

  // 获取批次数据
  unsigned int getVAO() const { return vao_; }
  unsigned int getIndexCount() const { return indexCount_; }

 private:
  unsigned int vao_ = 0;          // 顶点数组对象，初始化为0
  unsigned int vbo_ = 0;          // 顶点缓冲对象，初始化为0
  unsigned int ebo_ = 0;          // 索引缓冲对象，初始化为0
  unsigned int vertexCount_ = 0;  // 批次中的顶点数量，初始化为0
  unsigned int indexCount_ = 0;   // 批次中的索引数量，初始化为0

  // 存储整个批次的数据的全局容器
  // 在 TileBatch::upload 函数中，vertices_ 中的数据会被上传到 GPU
  // 的顶点缓冲对象（VBO）中，用于渲染
  std::vector<Tile::Vertex> vertices_;  // 顶点数据
  std::vector<unsigned int> indices_;   // 索引数据
};

}  // namespace PathGlyph