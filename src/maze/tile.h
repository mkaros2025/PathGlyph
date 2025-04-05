#pragma once
#include <cstdint>  // 显式包含 uint8_t 的定义
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include "common/Types.h"

namespace PathGlyph {

// 图块类 - 纯数据结构，表示迷宫的一个逻辑图块
// 即定义迷宫中的基本图块单元
class Tile {
public:
  static constexpr float TILE_SIZE = 1.0f;    // 图块在3D空间中的尺寸
  static constexpr float TILE_HEIGHT = 0.5f;  // 图块高度

  // 3D坐标转换静态函数
  // 将世界坐标投影到屏幕坐标
  static glm::vec2 worldToScreen(const glm::vec3& worldPos, const glm::mat4& viewProj);
  // 将屏幕坐标（和深度）反投影到世界坐标
  static glm::vec3 screenToWorld(const glm::vec2& screenPos, float depth, 
                                const glm::mat4& invViewProj);

  // 构造函数
  Tile(int x, int y, TileOverlayType overlayType = TileOverlayType::None);
  
  // 基本属性访问方法
  int getX() const;
  int getY() const;
  bool isWalkable() const;  // 基于overlayType判断

  TileOverlayType getOverlayType() const;
  void setOverlayType(TileOverlayType type);
  
  glm::vec3 getWorldPosition() const;  // 获取图块在3D世界空间中的位置
  
  // 图块包围盒结构体
  struct BoundingBox {
    glm::vec3 min;
    glm::vec3 max;
  };
  BoundingBox getBoundingBox() const;  // 获取图块的包围盒

private:
  int x_, y_;  // 逻辑坐标
  TileOverlayType overlayType_ = TileOverlayType::None; // 叠加属性
};

}