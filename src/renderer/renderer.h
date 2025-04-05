#pragma once
#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <memory>
#include <vector>
#include <unordered_map>
#include <glm/glm.hpp>

#include "core/shader.h"
#include "maze/maze.h"
#include "common/Types.h"
#include "maze/tileManager.h"

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

    // 视图控制
    void zoom(float factor);
    void pan(float dx, float dy);
    void rotate(float dx, float dy);
    void handleResize(int width, int height);

    // 渲染设置
    void setWireframeMode(bool enabled);
    void setShowPath(bool show);
    void setShowObstacles(bool show);
    std::shared_ptr<EditState> getEditState() { return editState_; }

    // 标记几何数据需要更新
    void markGeometryForUpdate() { needsUpdateGeometry_ = true; }

    // 获取对应覆盖类型的渲染参数
    RenderParams getRenderParamsForOverlay(TileOverlayType type) const;
    
    // 新增：将屏幕坐标转换为迷宫逻辑坐标（使用Tile的静态方法）
    bool screenToTileCoordinate(const glm::vec2& screenPos, int& tileX, int& tileY) const;
    
    // 设置图块管理器
    void setTileManager(std::shared_ptr<TileManager> tileManager) { tileManager_ = tileManager; }
    std::shared_ptr<TileManager> getTileManager() const { return tileManager_; }

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
    
    // 通用渲染函数 - 支持单个模型或批量渲染
    // 当transforms只有一个元素时，使用普通渲染；当有多个元素时，使用实例化渲染
    void renderModels(ModelType modelType, const std::vector<glm::mat4>& transforms);
    
    // 图块类型特定的渲染函数
    void updateMatrices();
    void updateModelTransforms();
    void renderGround();     // 渲染地面
    void renderPath();       // 渲染路径
    void renderObstacles();  // 渲染障碍物
    void renderAgents();     // 渲染代理
    void renderStart();      // 渲染起点
    void renderGoal();        // 渲染终点

    GLFWwindow* window_;
    int viewportWidth_ = 800;
    int viewportHeight_ = 600;

    std::shared_ptr<Maze> maze_; // 迷宫
    std::shared_ptr<EditState> editState_; // 编辑状态
    std::shared_ptr<TileManager> tileManager_; // 图块管理器
    std::unique_ptr<Shader> modelShader_;  // 着色器
    std::vector<std::unique_ptr<Model>> models_; // 模型资源数组
    std::vector<RenderParams> renderParams_; // 预定义的渲染参数数组，与ModelType枚举对应

    // 变换矩阵
    glm::mat4 projectionMatrix_ = glm::mat4(1.0f);
    glm::mat4 viewMatrix_ = glm::mat4(1.0f);

    bool needsUpdateGeometry_ = true;
    
    // 用于时间计算
    float lastFrameTime_ = 0.0f;
    
    // 存储各类型的变换矩阵
    std::vector<glm::mat4> groundTransforms_;
    std::vector<glm::mat4> pathTransforms_;
    std::vector<glm::mat4> obstacleTransforms_;
    std::vector<glm::mat4> startTransforms_;
    std::vector<glm::mat4> goalTransforms_;
    std::vector<glm::mat4> agentTransforms_;
};

}