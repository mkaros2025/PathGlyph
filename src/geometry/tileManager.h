#pragma once
#include <vector>
#include <memory>
#include <unordered_map>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/quaternion.hpp>
#include "common/types.h"

namespace PathGlyph {

// 前向声明
class Maze;

// 图块数据结构
struct Tile {
    int x = 0;                              // X坐标
    int y = 0;                              // Y坐标
    TileOverlayType overlayType = TileOverlayType::None;  // 叠加层类型
};

// 模型变换参数
struct ModelTransformParams {
    float scaleFactor = 1.0f;     // 缩放因子
    glm::vec3 positionOffset = glm::vec3(0.0f); // 位置偏移（包含高度）
    glm::quat rotation = glm::quat(1.0f, 0.0f, 0.0f, 0.0f); // 旋转（四元数）
};

// 图块管理器类 - 负责所有模型的空间变换和位置管理
class TileManager {
public:
  // 构造函数 - 合并初始化功能
  TileManager(std::shared_ptr<Maze> maze = nullptr, int width = 0, int height = 0);
  ~TileManager() = default;
  
  // 默认变换参数
  static const ModelTransformParams groundParams;
  static const ModelTransformParams pathParams;
  static const ModelTransformParams obstacleParams;
  static const ModelTransformParams startParams;
  static const ModelTransformParams goalParams;
  static const ModelTransformParams agentParams;
  static const ModelTransformParams gridLineParams;
  
  // 图块访问
  Tile* getTileAt(int x, int y);
  
  // 坐标转换 - 返回变换矩阵
  glm::mat4 getTileWorldPosition(int x, int y, const ModelTransformParams& params) const;
  
  bool screenToTileCoordinate(const glm::vec2& screenPos, int& outX, int& outY, 
                              const glm::mat4& viewProj) const;
  
  // 渲染数据收集 - 专用函数
  std::vector<glm::mat4> getGroundTransforms() const;
  std::vector<glm::mat4> getPathTransforms() const;
  std::vector<glm::mat4> getObstacleTransforms() const;
  std::vector<glm::mat4> getDynamicObstacleTransforms() const;
  std::vector<glm::mat4> getStartTransforms() const;
  std::vector<glm::mat4> getGoalTransforms() const;
  std::vector<glm::mat4> getAgentTransforms() const;
  
  // 获取网格线的变换矩阵（用于渲染坐标轴或网格）
  std::vector<glm::mat4> getGridLineTransforms() const;
  
  // 获取图块数量和尺寸
  int getWidth() const { return width_; }
  int getHeight() const { return height_; }
  
private:
  // 初始化地面图块
  void createTile(int x, int y);
  
  int width_;
  int height_;
  std::vector<std::vector<Tile>> tiles_; // 仅用于地面渲染
  std::shared_ptr<Maze> maze_; // 直接从迷宫获取信息
};

} // namespace PathGlyph