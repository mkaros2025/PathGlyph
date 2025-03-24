#pragma once

#include "../core/shader.h"
#include "../core/window.h"
#include "../maze/maze.h"
#include "../maze/tile.h"
#include <memory>
#include <glm/glm.hpp>
#include <glad/glad.h>

namespace PathGlyph {

// 渲染配置结构体 - 保留基本设置
struct RendererConfig {
    bool wireframeMode = false;     // 是否启用线框模式
    bool showPath = true;           // 是否显示路径
    bool showObstacles = true;      // 是否显示障碍物
    float zoomLevel = 1.0f;         // 缩放级别
    glm::vec2 cameraOffset{0.0f};   // 相机偏移
};

class Renderer {
public:
    // 构造函数 - 接收窗口指针
    Renderer(Window* window);
    ~Renderer();

    // Renderer 类管理了大量的资源，包括指针、OpenGL 资源（如 VAO、VBO）、着色器等。
    // 这些资源通常不能被简单地拷贝
    // 否则会出资源冲突之类乱七八糟的问题
    
    // 禁用拷贝
    Renderer(const Renderer&) = delete;
    Renderer& operator=(const Renderer&) = delete;
    
    // 初始化渲染器
    bool initialize();
    
    // 设置迷宫实例
    void setMaze(const Maze* maze);
    
    // 主渲染函数
    void render();
    
    // 配置访问器
    RendererConfig& getConfig() { return config_; }
    
    // 视图控制
    void zoom(float factor);
    void pan(float dx, float dy);
    void resetView();
    
    // 窗口事件处理
    void handleResize(int width, int height);
    
private:
    // GPU资源管理
    // 创建一个 顶点数组对象（VAO, Vertex Array Object），并返回其 ID。
    GLuint createVAO();
    // 创建一个 顶点缓冲对象（VBO, Vertex Buffer Object），并将顶点数据上传到 GPU。
    GLuint createVBO(const void* data, size_t size, bool dynamic = false);
    // 创建一个 索引缓冲对象（EBO, Element Buffer Object），并将索引数据上传到 GPU。
    GLuint createEBO(const void* data, size_t size);
    // 更新一个已存在的 顶点缓冲对象（VBO） 的数据。
    void updateVBO(unsigned int vbo, const void* data, size_t size, size_t offset = 0);
    // 配置顶点属性，告诉 OpenGL 如何解析顶点缓冲区中的数据。
    void setupVertexAttributes(int location, int size, int stride, int offset);
    
    // 渲染状态控制 - 简化版

    // 控制是否启用 线框模式
    void enableWireframe(bool enable);
    // 控制是否启用 深度测试
    // 深度测试用于确定像素的前后关系，确保场景中靠近相机的物体遮挡远处的物体。
    void enableDepthTest(bool enable);
    // 控制是否启用 混合模式
    // 混合模式用于处理透明度效果，例如玻璃、半透明物体等。
    void enableBlending(bool enable);
    
    // 逻辑分组的渲染函数
    void renderMaze();
    void renderPath();
    void renderObstacles();
    void renderAgents();  // 代表起点、终点、当前位置
    
    // 初始化和更新资源

    // 加载着色器
    // 功能：加载并编译用于渲染迷宫、路径、障碍物和代理的着色器程序。
    bool loadShaders();
    // 更新迷宫几何数据
    // 功能：根据当前迷宫的状态，生成或更新迷宫的顶点数据并上传到 GPU。
    // 主要用于渲染迷宫的图块。
    void updateMazeGeometry();
    // 更新路径几何数据
    // 功能：根据路径规划的结果，生成或更新路径的顶点数据并上传到 GPU。
    // 主要用于渲染迷宫中的路径。
    void updatePathGeometry();
    // 更新障碍物几何数据
    // 功能：根据迷宫中的障碍物信息，生成或更新障碍物的顶点数据并上传到 GPU。
    // 主要用于渲染静态和动态障碍物。
    void updateObstacleGeometry();
    // 更新代理几何数据
    // 功能：根据迷宫中的代理（如起点、终点、当前位置）信息，生成或更新代理的顶点数据并上传到 GPU。
    // 主要用于渲染迷宫中的特殊点（如起点、终点）。
    void updateAgentGeometry();
    
    // 矩阵计算
    void updateMatrices();
    
    // 成员变量

    Window* window_;                      // 窗口指针
    // 这个 const 的意思是无法修改这个指针指向的 Maze 对象
    const Maze* maze_ = nullptr;          // 迷宫指针
    RendererConfig config_;               // 渲染配置
    
    // 着色器资源
    std::unique_ptr<Shader> mazeShader_;   // 用于迷宫图块
    std::unique_ptr<Shader> pathShader_;   // 用于路径
    std::unique_ptr<Shader> entityShader_; // 用于障碍物和代理
    
    // OpenGL资源 - 迷宫图块
    TileBatch tileBatch_;                  // 复用已有的TileBatch类
    
    // OpenGL资源 - 路径
    // OpenGL 分配的顶点数组对象（VAO, Vertex Array Object）ID
    // 这个 ID 是由 OpenGL 在运行时生成的，用来唯一标识一个 VAO
    GLuint pathVAO_ = 0;
    GLuint pathVBO_ = 0;
    // 用于记录路径（Path）的顶点数量。
    // 它的主要作用是告诉 OpenGL 在渲染路径时需要绘制多少个顶点。
    size_t pathVertexCount_ = 0;
    
    // OpenGL资源 - 障碍物
    GLuint obstacleVAO_ = 0;
    GLuint obstacleVBO_ = 0;
    size_t obstacleVertexCount_ = 0;
    
    // OpenGL资源 - 代理（起点/终点/当前位置）
    GLuint agentVAO_ = 0;
    GLuint agentVBO_ = 0;
    
    // 矩阵
    // 投影矩阵，将 3D 场景投影到 2D 屏幕
    glm::mat4 projectionMatrix_ = glm::mat4(1.0f);
    // 视图矩阵，用于定义相机的位置和方向
    glm::mat4 viewMatrix_ = glm::mat4(1.0f);
    
    // 状态标志
    // 标记迷宫的几何数据是否需要更新。
    bool needsUpdateMaze_ = true;
    bool needsUpdatePath_ = true;
    bool needsUpdateObstacles_ = true;
    
    // 窗口尺寸
    int viewportWidth_ = 800;
    int viewportHeight_ = 600;
};

} // namespace PathGlyph