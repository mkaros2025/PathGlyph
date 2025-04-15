#include "graphics/renderer.h"
#include "geometry/model.h"
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <iostream>
#include <algorithm>
#include <string>

namespace PathGlyph {

// 构造函数和析构函数
Renderer::Renderer(GLFWwindow* window, std::shared_ptr<Maze> maze, std::shared_ptr<EditState> editState)
    : window_(window), maze_(maze), editState_(editState) {

    glfwGetFramebufferSize(window_, &viewportWidth_, &viewportHeight_);
    tileManager_ = std::make_shared<TileManager>(maze_, maze->getWidth(), maze->getHeight());
    
    // 计算地图中心点
    float mapCenterX = maze->getWidth() / 2.0f;
    float mapCenterZ = maze->getHeight() / 2.0f;
    
    // 设置相机位置正对地图中心上方
    // 高度调高一些，确保能看到整个地图
    float cameraHeight = 25.0f; // 调高相机位置
    float cameraDistance = 10.0f; // 相机与地图中心的水平距离
    
    // 相机位置 - 正对地图
    glm::vec3 cameraPos = glm::vec3(mapCenterX, cameraHeight, mapCenterZ + cameraDistance);
    glm::vec3 cameraTarget = glm::vec3(mapCenterX, 0.0f, mapCenterZ); // 看向地图中心
    glm::vec3 cameraUp = glm::vec3(0.0f, 1.0f, 0.0f);     // y轴向上
    
    viewMatrix_ = glm::lookAt(cameraPos, cameraTarget, cameraUp);
    projectionMatrix_ = glm::perspective(glm::radians(45.0f), 
                                         static_cast<float>(viewportWidth_) / viewportHeight_, 
                                         0.1f, 100.0f);
    
    // 启用 OpenGL 功能
    enableDepthTest(true);
    glEnable(GL_MULTISAMPLE); 
    // 可以考虑在这里也启用混合，如果透明度常用的话
    // enableBlending(true);
    
    // 加载着色器和设置渲染数据
    if (!loadShaders()) {
        std::cerr << "Failed to load shaders" << std::endl;
    }
    
    // 初始化状态
    if (editState_) {
        // 设置初始状态
        editState_->zoomLevel = 1.0f;
        editState_->cameraOffset = glm::vec2(0.0f, 0.0f);
        editState_->cameraRotationX = 45.0f; // 俯视角度
        editState_->cameraRotationY = 0.0f;  // 正对地图，不倾斜
    }
    
    initModelArray();
    initRenderParamsArray();
}

Renderer::~Renderer() {
    // 着色器和模型对象会通过智能指针自动释放
}

// 核心渲染功能
void Renderer::render() {
    // 计算帧时间
    float currentTime = static_cast<float>(glfwGetTime());
    float deltaTime = currentTime - lastFrameTime_;
    lastFrameTime_ = currentTime;
    
    // 清除缓冲
    glClearColor(0.1f, 0.1f, 0.2f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    
    // 如果几何数据需要更新，更新标志
    if (needsUpdateGeometry_) {
        needsUpdateGeometry_ = false;
    }
    
    // 更新矩阵
    updateMatrices();
    
    // 渲染地面
    renderGround();
    
    // 渲染路径
    renderPath();
    
    // 渲染障碍物
    renderObstacles();
    
    // 渲染起点和终点
    // renderStart();
    renderGoal();
    
    // 渲染代理
    renderAgents();
}

// 视图控制函数
void Renderer::zoom(float factor) {
    if (editState_) {
        // 缩放操作
        editState_->zoomLevel *= factor;
        // 限制缩放范围
        editState_->zoomLevel = std::max(0.1f, std::min(editState_->zoomLevel, 10.0f));
        std::cout << "缩放设置为: " << editState_->zoomLevel << std::endl;
    }
}

// 绝对不允许修改这个函数
void Renderer::pan(float dx, float dy) {
    if (editState_) {
        // 根据当前相机旋转角度计算平移方向
        float rotY = glm::radians(editState_->cameraRotationY);
        
        // 计算平移在世界坐标系中的方向
        float worldDx = dx * cos(rotY) + dy * sin(rotY);
        float worldDy = dx * sin(rotY) - dy * cos(rotY);
        
        // 应用平移（乘以系数控制速度）
        editState_->cameraOffset.x += worldDx * 0.05f;
        editState_->cameraOffset.y += worldDy * 0.05f;
    }
}

// 绝对不允许修改这个函数
void Renderer::rotate(float dx, float dy) {
    if (editState_) {
        // 旋转相机
        editState_->cameraRotationY -= dx * 0.5f;
        editState_->cameraRotationX += dy * 0.5f;
        
        // 限制X轴旋转角度，避免翻转
        editState_->cameraRotationX = std::max(-89.0f, std::min(editState_->cameraRotationX, 89.0f));
    }
}

void Renderer::handleResize(int width, int height) {
    viewportWidth_ = width;
    viewportHeight_ = height;
    glViewport(0, 0, width, height);
    projectionMatrix_ = glm::perspective(glm::radians(45.0f), 
                                         static_cast<float>(width) / height, 
                                         0.1f, 100.0f);
}

// 渲染设置函数
void Renderer::setWireframeMode(bool enabled) {
    if (editState_) {
        editState_->showWireframe = enabled;
        enableWireframe(enabled);
    }
}

void Renderer::setShowPath(bool show) {
    if (editState_) {
        editState_->showPath = show;
    }
}

void Renderer::setShowObstacles(bool show) {
    if (editState_) {
        editState_->showObstacles = show;
    }
}

// 获取对应覆盖类型的渲染参数
RenderParams Renderer::getRenderParamsForOverlay(TileOverlayType type) const {
    int index = static_cast<int>(type);
    if (index >= 0 && index < renderParams_.size()) {
        return renderParams_[index];
    }
    // 默认参数
    return RenderParams{};
}

// 私有方法实现
void Renderer::enableWireframe(bool enable) {
    if (enable) {
        glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
    } else {
        glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
    }
}

void Renderer::enableDepthTest(bool enable) {
    if (enable) {
        glEnable(GL_DEPTH_TEST);
    } else {
        glDisable(GL_DEPTH_TEST);
    }
}

void Renderer::enableBlending(bool enable) {
    if (enable) {
        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    } else {
        glDisable(GL_BLEND);
    }
}

bool Renderer::loadShaders() {
    try {
        modelShader_ = std::make_unique<Shader>();
        
        // 假设着色器代码已编译到对象中
        return true;
    } catch (const std::exception& e) {
        std::cerr << "Shader loading error: " << e.what() << std::endl;
        return false;
    }
}

void Renderer::initModelArray() {
    // 创建模型数组，每种模型类型一个
    models_.resize(static_cast<size_t>(ModelType::COUNT));
    
    // 为每种类型加载模型
    for (int i = 0; i < static_cast<int>(ModelType::COUNT); i++) {
        ModelType type = static_cast<ModelType>(i);
        models_[i] = std::make_unique<Model>();
        
        // 加载模型
        if (!models_[i]->loadModel(type)) {
            std::cerr << "Failed to load model for type: " << i << std::endl;
        }
    }
}

void Renderer::initRenderParamsArray() {
    // 初始化渲染参数数组
    renderParams_.resize(6); // TileOverlayType 枚举的数量
    
    // 设置默认参数
    // None
    renderParams_[0] = {
        glm::vec4(0.5f, 0.5f, 0.5f, 1.0f), // 灰色
        0.0f,  // 发光强度
        1.0f,  // 不透明
        true,  // 使用纹理
        true   // 使用模型自带颜色
    };
    
    // Path
    renderParams_[1] = {
        glm::vec4(0.0f, 0.8f, 1.0f, 1.0f), // 亮蓝色
        0.5f,  // 轻微发光
        0.8f,  // 略微透明
        true,  // 使用纹理
        true   // 使用模型自带颜色
    };
    
    // Start
    renderParams_[2] = {
        glm::vec4(0.0f, 1.0f, 0.0f, 1.0f), // 绿色
        0.8f,  // 较强发光
        1.0f,  // 不透明
        true,  // 使用纹理
        true   // 使用模型自带颜色
    };
    
    // Goal
    renderParams_[3] = {
        glm::vec4(1.0f, 0.0f, 0.0f, 1.0f), // 红色
        0.8f,  // 较强发光
        1.0f,  // 不透明
        true,  // 使用纹理
        true   // 使用模型自带颜色
    };
    
    // Agent
    renderParams_[4] = {
        glm::vec4(1.0f, 1.0f, 0.0f, 1.0f), // 黄色
        0.5f,  // 轻微发光
        1.0f,  // 不透明
        true,  // 使用纹理
        true   // 使用模型自带颜色
    };
    
    // Obstacle
    renderParams_[5] = {
        glm::vec4(0.6f, 0.3f, 0.1f, 1.0f), // 棕色
        0.0f,  // 不发光
        1.0f,  // 不透明
        true,  // 使用纹理
        true   // 使用模型自带颜色
    };
}

void Renderer::applyRenderParams(const RenderParams& params) {
    if (modelShader_) {
        modelShader_->use();
        modelShader_->setVec4("baseColor", params.baseColor);
        modelShader_->setFloat("emissiveStrength", params.emissiveStrength);
        modelShader_->setBool("useTexture", params.useTexture);
        modelShader_->setFloat("alpha", params.transparency);
        modelShader_->setBool("useModelColor", params.useModelColor);
    }
}

void Renderer::renderModels(ModelType modelType, const std::vector<glm::mat4>& transforms) {
    // 获取模型索引
    size_t modelIndex = static_cast<size_t>(modelType);

    // 确保模型索引有效且有有效的模型
    if (modelIndex >= models_.size() || !models_[modelIndex]) {
        std::cerr << "无效的模型索引或模型未加载: " << modelIndex << std::endl;
        return;
    }

    // 如果没有提供任何变换矩阵，则不渲染
    if (transforms.empty()) {
        // std::cout << "没有提供变换矩阵，跳过渲染模型类型: " << static_cast<int>(modelType) << std::endl;
        return; // Or handle as needed, maybe render at origin? For now, skip.
    }

    // 获取模型
    const auto& model = models_[modelIndex];
    if (!model) {
        std::cerr << "获取模型失败，索引: " << modelIndex << std::endl;
        return;
    }
    // 获取节点网格信息
    const auto& nodeMeshes = model->getNodeMeshes();

    // 设置着色器参数
    if (!modelShader_) {
         std::cerr << "模型着色器未初始化！" << std::endl;
         return;
    }
    modelShader_->use();

    // 设置是否使用实例化渲染
    bool useInstanced = transforms.size() > 1;
    modelShader_->setBool("isInstanced", useInstanced); // 告知着色器是否为实例化

    // 添加统一的模型缩放 (保持不变，这是一个全局缩放因子)
    float modelScale = 0.5f; // 调整此值以适应您的模型大小
    modelShader_->setFloat("modelScale", modelScale);

    // 设置世界变换 Uniform
    size_t instanceCount = 0; // 用于渲染调用
    if (useInstanced) {
        // 多实例: 设置 instanceTransforms 数组
        instanceCount = std::min(transforms.size(), size_t(500)); // 假设着色器最多支持 500 个实例
        modelShader_->setMat4Array("instanceTransforms", transforms.data(), instanceCount);
        // (可选)可以设置 model uniform 为单位阵，以防着色器逻辑混淆，但这取决于着色器实现
        // modelShader_->setMat4("model", glm::mat4(1.0f));
    } else {
        // 单实例: 设置 model uniform 为传入的第一个变换
        instanceCount = 1; // 用于渲染调用计数（虽然不是真的实例化）
        modelShader_->setMat4("model", transforms[0]); // 使用传入的世界变换！
    }

    // 渲染每个节点的网格
    for (const auto& nodeMesh : nodeMeshes) {
        // 设置节点的局部变换矩阵 (nodeTransform uniform)
        // 这是网格相对于模型根节点的变换
        if (modelShader_) {
            modelShader_->setMat4("nodeTransform", nodeMesh.transform);
        }

        // 获取网格
        auto mesh = nodeMesh.mesh;
        if (!mesh) {
            // std::cerr << "警告: 节点包含空的网格指针。" << std::endl;
            continue;
        }

        // 渲染：根据是否实例化调用不同函数
        if (useInstanced) {
             mesh->renderInstanced(modelShader_.get(), instanceCount);
        } else {
             // 单实例渲染
             mesh->render(modelShader_.get());
        }
    }
}

void Renderer::updateMatrices() {
    if (editState_) {
        // 从编辑状态中获取相机参数
        float zoom = editState_->zoomLevel;
        glm::vec2 offset = editState_->cameraOffset;
        float rotX = editState_->cameraRotationX;
        float rotY = editState_->cameraRotationY;
        
        // 获取地图中心
        float mapCenterX = maze_->getWidth() / 2.0f;
        float mapCenterZ = maze_->getHeight() / 2.0f;
        
        // 计算相机到地图中心的距离 (远近)
        float distanceToCenter = 15.0f / zoom;
        
        // 计算相机在水平面内的位置（基于旋转角度）
        float angleY = glm::radians(rotY);
        float horizontalDist = distanceToCenter * cos(glm::radians(rotX));
        
        // 基于旋转角度计算相机位置偏移量
        float offsetX = horizontalDist * sin(angleY);
        float offsetZ = horizontalDist * cos(angleY);
        
        // 计算相机位置 - 围绕地图中心旋转
        glm::vec3 cameraPos = glm::vec3(
            mapCenterX + offsetX + offset.x,
            distanceToCenter * sin(glm::radians(rotX)), // 高度由rotX决定
            mapCenterZ + offsetZ + offset.y
        );
        
        // 目标点始终是地图中心，考虑平移偏移
        glm::vec3 targetPos = glm::vec3(
            mapCenterX + offset.x,
            0.0f,
            mapCenterZ + offset.y
        );
        
        // 上向量保持垂直
        glm::vec3 upVector = glm::vec3(0.0f, 1.0f, 0.0f);
        
        // 创建视图矩阵 - 单步完成视角计算
        viewMatrix_ = glm::lookAt(cameraPos, targetPos, upVector);
        
        // 更新投影矩阵
        projectionMatrix_ = glm::perspective(glm::radians(45.0f), 
                                           static_cast<float>(viewportWidth_) / viewportHeight_, 
                                           0.1f, 100.0f);
        
        // 设置着色器矩阵和相关 uniform
        if (modelShader_) {
            modelShader_->use();
            modelShader_->setMat4("view", viewMatrix_);
            modelShader_->setMat4("projection", projectionMatrix_);
            modelShader_->setVec3("viewPos", cameraPos);
            
            // --- 设置点光源位置 --- 
            // 将光源放在地图中心上方较高处，并略微偏移，形成斜照效果
            float lightHeight = 30.0f; // 光源高度
            float lightOffsetX = -10.0f; // X方向偏移
            float lightOffsetZ = -10.0f; // Z方向偏移
            glm::vec3 lightPos = glm::vec3(mapCenterX + lightOffsetX, lightHeight, mapCenterZ + lightOffsetZ);
            modelShader_->setVec3("lightPos", lightPos);
        }
    }
}

void Renderer::renderGround() {
    // 获取地面的变换矩阵
    auto transforms = tileManager_->getGroundTransforms();
    
    applyRenderParams(getRenderParamsForOverlay(TileOverlayType::None));
    renderModels(ModelType::GROUND, transforms);
    
    // 添加网格线框描边以便识别坐标
    // 保存当前多边形模式
    GLint polygonMode;
    glGetIntegerv(GL_POLYGON_MODE, &polygonMode);
    
    glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
    glDepthMask(GL_FALSE);
    glLineWidth(10.0f);
    
    // 在着色器中设置线框颜色为黑色
    if (modelShader_) {
        modelShader_->use();
        modelShader_->setVec4("material.diffuse", glm::vec4(0.0f, 0.0f, 0.0f, 1.0f)); // 黑色线框
        modelShader_->setBool("material.hasTexture", false);
    }
    
    // 渲染线框
    renderModels(ModelType::GROUND, transforms);
    
    // 在地图边缘特别标记坐标轴
    if (modelShader_) {
        // 获取地图尺寸
        int width = tileManager_->getWidth();
        int height = tileManager_->getHeight();
        
        // 设置渲染参数 - 使用红色
        modelShader_->use();
        modelShader_->setVec4("material.diffuse", glm::vec4(1.0f, 0.0f, 0.0f, 1.0f)); // 红色
        
        // X轴方向加粗线
        glLineWidth(2.0f);
        for (int x = 0; x < width; ++x) {
            // 在底边绘制X轴刻度线
            glm::mat4 transform = tileManager_->getTileWorldPosition(x, 0, TileManager::groundParams);
            std::vector<glm::mat4> xAxisTransform = {transform};
            renderModels(ModelType::GROUND, xAxisTransform);
        }
        
        // Y轴方向加粗线
        modelShader_->setVec4("material.diffuse", glm::vec4(0.0f, 0.0f, 1.0f, 1.0f)); // 蓝色
        for (int y = 0; y < height; ++y) {
            // 在左边绘制Y轴刻度线
            glm::mat4 transform = tileManager_->getTileWorldPosition(0, y, TileManager::groundParams);
            std::vector<glm::mat4> yAxisTransform = {transform};
            renderModels(ModelType::GROUND, yAxisTransform);
        }
    }
    
    // 恢复深度写入
    glDepthMask(GL_TRUE);
    // 恢复原来的多边形模式
    glPolygonMode(GL_FRONT_AND_BACK, polygonMode);
    // 恢复线宽
    glLineWidth(1.0f);
}

void Renderer::renderPath() {
    // 获取路径的变换矩阵
    auto transforms = tileManager_->getPathTransforms();
    
    // 如果有路径才渲染
    if (!transforms.empty()) {
        // 启用混合模式
        enableBlending(true);
        
        // 设置渲染参数
        applyRenderParams(getRenderParamsForOverlay(TileOverlayType::Path));
        
        // 渲染路径
        renderModels(ModelType::PATH, transforms);
        
        // 禁用混合模式
        enableBlending(false);
    }
}

void Renderer::renderObstacles() {
    // 获取静态障碍物的变换矩阵
    auto staticTransforms = tileManager_->getObstacleTransforms();
    auto dynamicTransforms = tileManager_->getDynamicObstacleTransforms();
    
    // 合并所有障碍物变换
    std::vector<glm::mat4> allObstacles;
    allObstacles.reserve(staticTransforms.size() + dynamicTransforms.size());
    allObstacles.insert(allObstacles.end(), staticTransforms.begin(), staticTransforms.end());
    allObstacles.insert(allObstacles.end(), dynamicTransforms.begin(), dynamicTransforms.end());
    
    // 如果有障碍物才渲染
    if (!allObstacles.empty()) {
        applyRenderParams(getRenderParamsForOverlay(TileOverlayType::Obstacle));
        renderModels(ModelType::OBSTACLE, allObstacles);
    }
}

void Renderer::renderAgents() {
    // 获取代理的变换矩阵
    auto transforms = tileManager_->getAgentTransforms();
    
    // 如果有代理才渲染
    if (!transforms.empty()) {
        // 设置渲染参数
        applyRenderParams(getRenderParamsForOverlay(TileOverlayType::Agent));
        
        // 渲染代理
        renderModels(ModelType::AGENT, transforms);
    }
}

void Renderer::renderStart() {
    // 获取起点的变换矩阵
    auto transforms = tileManager_->getStartTransforms();
    
    // 如果有起点才渲染
    if (!transforms.empty()) {
        // 设置渲染参数
        applyRenderParams(getRenderParamsForOverlay(TileOverlayType::Start));
        
        // 渲染起点
        renderModels(ModelType::START, transforms);
    }
}

void Renderer::renderGoal() {
    // 获取终点的变换矩阵
    auto transforms = tileManager_->getGoalTransforms();
    
    // 如果有终点才渲染
    if (!transforms.empty()) {
        // 设置渲染参数
        applyRenderParams(getRenderParamsForOverlay(TileOverlayType::Goal));
        
        // 渲染终点
        renderModels(ModelType::GOAL, transforms);
    }
}

} // namespace PathGlyph
