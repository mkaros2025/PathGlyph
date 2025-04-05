#pragma once
#include <vector>
#include <memory>
#include <unordered_map>
#include <glm/glm.hpp>
#include "tile.h"
#include "../common/Types.h"

namespace PathGlyph {

// 前向声明
class Renderer;
class Maze;

/**
 * 图块管理器类
 * 负责管理所有图块，并提供给渲染器所需的接口
 * 作为Tile和Renderer之间的桥梁
 */
class TileManager {
public:
  // 构造函数
  TileManager(std::shared_ptr<Maze> maze = nullptr);
  ~TileManager();

  void initialize(int width, int height);
  void update(double dt); // 更新所有图块状态
  void syncWithMaze(); // 与迷宫数据同步

  // 图块访问和修改
  Tile* getTileAt(int x, int y);
  void setTileOverlay(int x, int y, TileOverlayType type);
  void clearTileOverlay(int x, int y);
  
  // 批量获取特定类型的图块（用于渲染）
  std::vector<Tile*> getTilesByType(TileOverlayType type);
  
  // 坐标转换
  glm::vec3 getTileWorldPosition(int x, int y) const;
  bool screenToTileCoordinate(const glm::vec2& screenPos, int& outX, int& outY, 
                              const glm::mat4& viewProj) const;
  
  // 渲染数据收集
  std::vector<glm::mat4> getTransformsByType(TileOverlayType type) const;
  
  // 获取所有图块的变换矩阵（用于批量渲染）
  std::vector<glm::mat4> getAllTileTransforms() const;
  
  // 几何数据相关
  Tile::BoundingBox getTileBoundingBox() const;
  
  // 获取图块数量和尺寸
  int getWidth() const { return width_; }
  int getHeight() const { return height_; }
  
private:
  // 创建新图块
  void createTile(int x, int y, TileOverlayType type = TileOverlayType::None);
  
  // 计算基于类型的图块高度偏移
  float getHeightOffsetForType(TileOverlayType type) const;
  
  int width_;
  int height_;
  std::vector<std::vector<std::unique_ptr<Tile>>> tiles_; // 二维图块数组
  std::shared_ptr<Maze> maze_; // 关联的迷宫
  
  // 类型到图块的映射（用于快速访问特定类型的图块）
  std::unordered_map<TileOverlayType, std::vector<Tile*>> tilesByType_;
};

} // namespace PathGlyph