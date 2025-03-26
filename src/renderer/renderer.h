#pragma once

#include <glad/glad.h>
#include <GLFW/glfw3.h>

#include <glm/glm.hpp>
#include <memory>
#include <array>

#include "../core/shader.h"
#include "../maze/maze.h"
#include "../maze/tile.h"
#include "../common/Point.h"

namespace PathGlyph {

// 渲染配置结构体
struct RendererConfig {
  bool wireframeMode = false;    // 是否启用线框模式
  bool showPath = true;          // 是否显示路径
  bool showObstacles = true;     // 是否显示障碍物
  float zoomLevel = 1.0f;        // 缩放级别
  glm::vec2 cameraOffset{0.0f};  // 相机偏移
};

class Renderer {
 public:
  Renderer(GLFWwindow* window);

  // 禁用拷贝
  Renderer(const Renderer&) = delete;
  Renderer& operator=(const Renderer&) = delete;

  // 核心方法
  bool initialize();
  void setMaze(const Maze* maze);
  void render();

  // 配置和控制
  RendererConfig& getConfig() { return config_; }
  void zoom(float factor);
  void pan(float dx, float dy);
  void resetView();
  void handleResize(int width, int height);

 private:
  // 渲染状态控制
  void enableWireframe(bool enable);
  void enableDepthTest(bool enable);
  void enableBlending(bool enable);

  // 更新几何数据
  bool loadShaders();
  void updateMazeGeometry();
  void updateMatrices();

  // 窗口和状态
  GLFWwindow* window_;
  const Maze* maze_ = nullptr;
  RendererConfig config_;
  int viewportWidth_ = 800;
  int viewportHeight_ = 600;

  // 着色器
  std::unique_ptr<Shader> universalShader_;

  // 几何资源
  TileBatch tileBatch_;

  // 更新标志
  bool needsUpdateMaze_ = true;

  // 变换矩阵
  glm::mat4 projectionMatrix_ = glm::mat4(1.0f);
  glm::mat4 viewMatrix_ = glm::mat4(1.0f);
};

}  // namespace PathGlyph