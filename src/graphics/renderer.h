#pragma once
#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <memory>
#include <vector>
#include <unordered_map>
#include <glm/glm.hpp>

#include "maze/maze.h"
#include "common/types.h"
#include "geometry/tileManager.h"
#include "graphics/shader.h"

namespace PathGlyph {

// 模型类前置声明
class Model;

// 主渲染器类
class Renderer {
public:
    // 构造函数和析构函数
    Renderer(GLFWwindow* window, std::shared_ptr<Maze> maze = nullptr, 
             std::shared_ptr<EditState> editState = nullptr);
    ~Renderer();

    // 核心渲染功能
    void render();

    // 视图控制 - 保留在Renderer中
    void zoom(float factor);
    void pan(float dx, float dy);
    void rotate(float dx, float dy);
    void handleResize(int width, int height);

    // 渲染设置
    void setWireframeMode(bool enabled);
    void setShowPath(bool show);
    void setShowObstacles(bool show);

    // 标记几何数据需要更新
    void markGeometryForUpdate() { needsUpdateGeometry_ = true; }

    // 获取对应覆盖类型的渲染参数
    RenderParams getRenderParamsForOverlay(TileOverlayType type) const;
    
    std::shared_ptr<TileManager> getTileManager() const { return tileManager_; }
    std::shared_ptr<EditState> getEditState() { return editState_; }
    
    // 获取当前视图投影矩阵
    glm::mat4 getViewProjectionMatrix() const { return projectionMatrix_ * viewMatrix_; }

private:
    // 渲染状态控制
    void enableWireframe(bool enable);
    void enableDepthTest(bool enable);
    void enableBlending(bool enable);

    // 资源管理
    bool loadShaders();
    void setupRenderData();
    
    // 初始化模型和渲染参数数组
    void initModelArray();
    void initRenderParamsArray();

    // 应用渲染参数到着色器
    void applyRenderParams(const RenderParams& params);
    
    // 通用渲染函数 - 支持实例化渲染
    void renderModels(ModelType modelType, const std::vector<glm::mat4>& transforms);
    
    // 更新视图和投影矩阵
    void updateMatrices();

    // 简化的渲染函数 - 使用TileManager提供的变换矩阵
    void renderGround();     // 渲染地面
    void renderPath();       // 渲染路径
    void renderObstacles();  // 渲染障碍物
    void renderAgents();     // 渲染代理
    void renderStart();      // 渲染起点
    void renderGoal();       // 渲染终点
    void renderGridLines();  // 渲染网格线

    GLFWwindow* window_;
    int viewportWidth_ = 800;
    int viewportHeight_ = 600;

    std::shared_ptr<Maze> maze_; // 迷宫
    std::shared_ptr<EditState> editState_; // 编辑状态
    std::shared_ptr<TileManager> tileManager_; // 图块管理器
    std::unique_ptr<Shader> modelShader_;  // 着色器
    std::vector<std::unique_ptr<Model>> models_; // 模型资源数组
    std::vector<RenderParams> renderParams_; // 预定义的渲染参数数组

    // 变换矩阵 - 仅保留视图和投影矩阵
    glm::mat4 projectionMatrix_ = glm::mat4(1.0f);
    glm::mat4 viewMatrix_ = glm::mat4(1.0f);

    bool needsUpdateGeometry_ = true;
    float lastFrameTime_ = 0.0f;
};

}