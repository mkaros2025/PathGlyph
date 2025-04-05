#include "renderer.h"
#include "model.h"
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <iostream>
#include <algorithm>
#include <filesystem>

namespace PathGlyph {

// 构造函数
Renderer::Renderer(GLFWwindow* window, std::shared_ptr<Maze> maze, std::shared_ptr<EditState> editState)
    : window_(window), maze_(maze), editState_(editState), tileManager_(nullptr) {
    
    // 获取窗口大小
    glfwGetFramebufferSize(window_, &viewportWidth_, &viewportHeight_);
    
    // 加载着色器
    if (!loadShaders()) {
        std::cerr << "Failed to load shaders!" << std::endl;
    }
    
    // 初始化TileManager
    if (maze_) {
        tileManager_ = std::make_shared<TileManager>(maze_);
        tileManager_->initialize(maze_->getWidth(), maze_->getHeight());
        tileManager_->syncWithMaze();
    }
    
    // 初始化模型和渲染参数数组
    initModelArray();
    initRenderParamsArray();
    
    // 设置初始视图和投影矩阵
    updateMatrices();
    enableDepthTest(true);
}

// 析构函数
Renderer::~Renderer() {
    // 模型和着色器使用智能指针自动清理
}

// 主渲染循环
void Renderer::render() {
    // 获取当前帧时间，用于动画（保留这部分以便将来使用）
    float currentTime = glfwGetTime();
    float deltaTime = currentTime - lastFrameTime_;
    lastFrameTime_ = currentTime;
    
    // 清空缓冲区
    glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    
    // 确保深度测试已启用
    glEnable(GL_DEPTH_TEST);
    
    // 更新变换矩阵
    updateMatrices();
    
    // 更新几何体（如果需要）
    if (needsUpdateGeometry_) {
        updateModelTransforms();
        needsUpdateGeometry_ = false;
    }
    
    // 使用模型着色器
    modelShader_->use();
    
    // 设置矩阵
    modelShader_->setMat4("view", viewMatrix_);
    modelShader_->setMat4("projection", projectionMatrix_);
    
    // 设置光照参数
    modelShader_->setVec3("viewPos", glm::vec3(0.0f, 5.0f, 5.0f));
    modelShader_->setFloat("ambientStrength", 1.0f); // 增加环境光强度
    modelShader_->setVec3("lightPos", glm::vec3(0.0f, 10.0f, 0.0f)); // 调整光源位置
    modelShader_->setVec3("lightColor", glm::vec3(1.0f, 1.0f, 1.0f));
    modelShader_->setFloat("specularStrength", 0.5f);
    
    // 1. 渲染地面
    renderGround();
    
    // // 2. 渲染路径
    // renderPath();
    
    // // 3. 渲染障碍物
    renderObstacles();
    
    // // 4. 渲染起点和终点
    // renderStart();
    // renderGoal();
    
    // // 5. 渲染代理
    // renderAgents();
}

// 视图控制函数
void Renderer::zoom(float factor) {
    editState_->zoomLevel *= factor;
    // 限制缩放范围
    editState_->zoomLevel = glm::clamp(editState_->zoomLevel, 0.1f, 10.0f);
    updateMatrices();
}
void Renderer::pan(float dx, float dy) {
    // 根据当前缩放级别调整平移速度
    float panSpeed = 0.01f / editState_->zoomLevel;
    editState_->cameraOffset.x += dx * panSpeed;
    editState_->cameraOffset.y -= dy * panSpeed; // 反转Y方向，因为OpenGL坐标系Y轴向上
    updateMatrices();
}
void Renderer::handleResize(int width, int height) {
    // 更新视口大小
    viewportWidth_ = width;
    viewportHeight_ = height;
    glViewport(0, 0, width, height);
    
    // 更新投影矩阵
    updateMatrices();
}

// 渲染设置函数
void Renderer::setWireframeMode(bool enabled) {
    editState_->showWireframe = enabled;
}
void Renderer::setShowPath(bool show) {
    editState_->showPath = show;
}
void Renderer::setShowObstacles(bool show) {
    editState_->showObstacles = show;
}

// 获取对应覆盖类型的渲染参数
RenderParams Renderer::getRenderParamsForOverlay(TileOverlayType type) const {
    switch (type) {
        case TileOverlayType::None:
            return renderParams_[static_cast<int>(ModelType::GROUND)];
        case TileOverlayType::Start:
            return renderParams_[static_cast<int>(ModelType::START)];
        case TileOverlayType::Goal:
            return renderParams_[static_cast<int>(ModelType::GOAL)];
        case TileOverlayType::Path:
            return renderParams_[static_cast<int>(ModelType::PATH)];
        case TileOverlayType::Obstacle:
            return renderParams_[static_cast<int>(ModelType::OBSTACLE)];
        default:
            return renderParams_[static_cast<int>(ModelType::GROUND)];
    }
}

// 屏幕坐标到图块坐标的转换
bool Renderer::screenToTileCoordinate(const glm::vec2& screenPos, int& tileX, int& tileY) const {
    if (!maze_) return false;
    
    // 将屏幕坐标转换为归一化设备坐标 (NDC)
    float ndcX = (2.0f * screenPos.x) / viewportWidth_ - 1.0f;
    float ndcY = 1.0f - (2.0f * screenPos.y) / viewportHeight_;
    
    // 计算逆变换
    glm::mat4 invView = glm::inverse(viewMatrix_);
    glm::mat4 invProj = glm::inverse(projectionMatrix_);
    
    // 从NDC转换为视图空间
    glm::vec4 rayClip = glm::vec4(ndcX, ndcY, -1.0f, 1.0f);
    glm::vec4 rayView = invProj * rayClip;
    rayView = glm::vec4(rayView.x, rayView.y, -1.0f, 0.0f);
    
    // 从视图空间转换为世界空间
    glm::vec4 rayWorld = invView * rayView;
    glm::vec3 rayDirection = glm::normalize(glm::vec3(rayWorld));
    
    // 从相机位置发射射线
    glm::vec3 cameraPos = glm::vec3(invView[3]);
    
    // 与Y=0平面相交
    float t = -cameraPos.y / rayDirection.y;
    glm::vec3 intersectPoint = cameraPos + t * rayDirection;
    
    // 将世界坐标转换为图块坐标
    if (tileManager_) {
        // 使用迷宫的坐标转换
        Point tilePoint = maze_->worldToLogical(intersectPoint);
        tileX = tilePoint.x;
        tileY = tilePoint.y;
        return maze_->isInBounds(tileX, tileY);
    }
    
    // 如果没有图块管理器，使用简单的坐标变换
    tileX = static_cast<int>(intersectPoint.x + 0.5f);
    tileY = static_cast<int>(intersectPoint.z + 0.5f);
    
    // 检查坐标是否在迷宫范围内
    return maze_->isInBounds(tileX, tileY);
}

// 渲染状态控制
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

// 资源管理
bool Renderer::loadShaders() {
    try {
        // 直接使用固定路径
        std::string vertPath = "../../../../assets/shaders/model.vert";
        std::string fragPath = "../../../../assets/shaders/model.frag";

        modelShader_ = std::make_unique<Shader>(vertPath, fragPath);
        
        // 检查是否加载成功
        if (modelShader_->getID() == 0) {
            return false;
        }
        return true;
    } catch (const std::exception& e) {
        return false;
    }
}

void Renderer::setupRenderData() {
    // 首先更新TileManager状态
    if (tileManager_) {
        // 如果tileManager_不为null，更新状态
        tileManager_->update(0.0); // 传入0.0作为deltaTime
        tileManager_->syncWithMaze(); // 同步TileManager与Maze状态
    } else if (maze_) {
        // 如果tileManager_为null但maze_不为null，创建并初始化TileManager
        tileManager_ = std::make_shared<TileManager>(maze_);
        tileManager_->initialize(maze_->getWidth(), maze_->getHeight());
        tileManager_->syncWithMaze();
    }
    
    // 只有在首次运行时才初始化模型数组
    if (models_.empty()) {
        initModelArray();
    }
    
    // 更新渲染参数数组
    initRenderParamsArray();
    
    // 更新模型的世界变换矩阵
    updateModelTransforms();
}

// 初始化模型数组
void Renderer::initModelArray() {
    // 清除现有模型
    models_.clear();
    
    // 创建地面模型
    std::unique_ptr<Model> groundModel = std::make_unique<Model>();
    if (!groundModel->loadModel(ModelType::GROUND)) {
        std::cerr << "Failed to load ground model" << std::endl;
    }
    models_.push_back(std::move(groundModel));
    
    // 创建路径模型
    std::unique_ptr<Model> pathModel = std::make_unique<Model>();
    if (!pathModel->loadModel(ModelType::PATH)) {
        std::cerr << "Failed to load path model" << std::endl;
    }
    models_.push_back(std::move(pathModel));
    
    // 创建障碍物模型
    std::unique_ptr<Model> obstacleModel = std::make_unique<Model>();
    if (!obstacleModel->loadModel(ModelType::OBSTACLE)) {
        std::cerr << "Failed to load obstacle model" << std::endl;
    }
    models_.push_back(std::move(obstacleModel));
    
    // 创建代理模型
    std::unique_ptr<Model> agentModel = std::make_unique<Model>();
    if (!agentModel->loadModel(ModelType::AGENT)) {
        std::cerr << "Failed to load agent model" << std::endl;
    }
    models_.push_back(std::move(agentModel));
    
    // 创建起点模型
    std::unique_ptr<Model> startModel = std::make_unique<Model>();
    if (!startModel->loadModel(ModelType::START)) {
        std::cerr << "Failed to load start model" << std::endl;
    }
    models_.push_back(std::move(startModel));
    
    // 创建终点模型
    std::unique_ptr<Model> goalModel = std::make_unique<Model>();
    if (!goalModel->loadModel(ModelType::GOAL)) {
        std::cerr << "Failed to load goal model" << std::endl;
    }
    models_.push_back(std::move(goalModel));
}

// 初始化渲染参数数组
void Renderer::initRenderParamsArray() {
    // 清除现有渲染参数
    renderParams_.clear();
    
    // 地面渲染参数
    RenderParams groundParams;
    groundParams.baseColor = glm::vec4(1.0f, 1.0f, 1.0f, 1.0f);
    groundParams.heightOffset = 0.0f;
    groundParams.emissiveStrength = 0.0f;
    groundParams.transparency = 1.0f;
    renderParams_.push_back(groundParams);
    
    // 路径渲染参数
    RenderParams pathParams;
    pathParams.baseColor = glm::vec4(0.0f, 0.8f, 0.0f, 1.0f);
    pathParams.heightOffset = 0.1f;
    pathParams.emissiveStrength = 0.2f;
    pathParams.transparency = 1.0f;
    renderParams_.push_back(pathParams);
    
    // 障碍物渲染参数
    RenderParams obstacleParams;
    obstacleParams.baseColor = glm::vec4(0.8f, 0.2f, 0.2f, 1.0f);
    obstacleParams.heightOffset = 0.0f;
    obstacleParams.emissiveStrength = 0.0f;
    obstacleParams.transparency = 1.0f;
    renderParams_.push_back(obstacleParams);
    
    // 代理渲染参数
    RenderParams agentParams;
    agentParams.baseColor = glm::vec4(0.0f, 0.0f, 0.8f, 1.0f);
    agentParams.heightOffset = 0.5f;
    agentParams.emissiveStrength = 0.4f;
    agentParams.transparency = 1.0f;
    renderParams_.push_back(agentParams);
    
    // 起点渲染参数
    RenderParams startParams;
    startParams.baseColor = glm::vec4(0.0f, 0.8f, 0.8f, 1.0f);
    startParams.heightOffset = 0.2f;
    startParams.emissiveStrength = 0.5f;
    startParams.transparency = 1.0f;
    renderParams_.push_back(startParams);
    
    // 终点渲染参数
    RenderParams goalParams;
    goalParams.baseColor = glm::vec4(0.8f, 0.8f, 0.0f, 1.0f);
    goalParams.heightOffset = 0.2f;
    goalParams.emissiveStrength = 0.5f;
    goalParams.transparency = 1.0f;
    renderParams_.push_back(goalParams);
}

// 应用渲染参数到着色器
void Renderer::applyRenderParams(const RenderParams& params) {
    modelShader_->use();
    
    // 强制使用白色作为基础颜色，保持原始颜色不变
    modelShader_->setVec4("material.diffuse", glm::vec4(1.0f, 1.0f, 1.0f, params.transparency));
    modelShader_->setFloat("material.shininess", 32.0f);
    
    // 只有在需要使用纹理时才设置hasTexture标志
    if (params.useTexture) {
        modelShader_->setBool("material.hasTexture", true);
    } else {
        modelShader_->setBool("material.hasTexture", false);
    }
    
    // 启用透明度混合（如果需要）
    if (params.transparency < 1.0f) {
        enableBlending(true);
    }
}

// 渲染单个模型或实例化渲染
void Renderer::renderModels(ModelType modelType, const std::vector<glm::mat4>& transforms) {
    if (transforms.empty()) return;
    
    int modelIndex = static_cast<int>(modelType);
    if (modelIndex < 0 || modelIndex >= models_.size()) return;
    
    // 应用该模型类型的渲染参数
    applyRenderParams(renderParams_[modelIndex]);
    
    // 启用线框模式（如果需要）
    if (editState_->showWireframe) {
        enableWireframe(true);
    }
    
    // 获取当前模型
    const Model* model = models_[modelIndex].get();
    if (!model) return;
    
    // 获取模型的所有网格
    const auto& meshes = model->getMeshes();
    
    // 设置是否使用实例化渲染
    bool useInstancing = transforms.size() > 1;
    modelShader_->setBool("isInstanced", useInstancing);
    
    // 为不同类型模型定义不同的缩放因子
    float scaleFactor = 1.0f;
    // 根据模型类型设置不同的缩放因子
    switch (modelType) {
        case ModelType::GROUND:
            scaleFactor = 1.0f;
            break;
        case ModelType::OBSTACLE:
            // 使障碍物更小，避免交叉
            scaleFactor = 0.5f;
            break;
        case ModelType::PATH:
            scaleFactor = 0.9f;
            break;
        case ModelType::START:
            scaleFactor = 0.8f;
            break;
        case ModelType::GOAL:
            scaleFactor = 0.8f;
            break;
        case ModelType::AGENT:
            scaleFactor = 0.8f;
            break;
        default:
            scaleFactor = 1.0f;
            break;
    }
    
    if (useInstancing) {
        // 实例化渲染 - 为每个实例创建和传递一个模型矩阵
        
        // 创建新的实例矩阵数组，添加缩放
        std::vector<glm::mat4> scaledTransforms;
        scaledTransforms.reserve(transforms.size());
        
        for (const auto& transform : transforms) {
            // 提取平移部分
            glm::vec3 position(transform[3]);
            
            // 创建新的矩阵，添加统一缩放
            glm::mat4 scaledTransform = glm::translate(glm::mat4(1.0f), position);
            scaledTransform = glm::scale(scaledTransform, glm::vec3(scaleFactor));
            scaledTransforms.push_back(scaledTransform);
        }
        
        // 创建实例化矩阵缓冲区
        GLuint instanceVBO;
        glGenBuffers(1, &instanceVBO);
        glBindBuffer(GL_ARRAY_BUFFER, instanceVBO);
        glBufferData(GL_ARRAY_BUFFER, scaledTransforms.size() * sizeof(glm::mat4), 
                    scaledTransforms.data(), GL_STATIC_DRAW);
        
        // 为每个网格设置实例属性
        for (const auto& mesh : meshes) {
            GLuint VAO = mesh->getVAO();
            glBindVertexArray(VAO);
            
            // 设置实例化矩阵属性 (mat4 = 4个vec4)
            GLsizei vec4Size = sizeof(glm::vec4);
            for (int i = 0; i < 4; i++) {
                glEnableVertexAttribArray(4 + i); // 位置4、5、6、7用于实例矩阵
                glVertexAttribPointer(4 + i, 4, GL_FLOAT, GL_FALSE, sizeof(glm::mat4), 
                                     reinterpret_cast<void*>(static_cast<intptr_t>(i * vec4Size)));
                glVertexAttribDivisor(4 + i, 1); // 每个实例更新一次
            }
            
            // 渲染实例化网格
            glBindVertexArray(VAO);
            glDrawElementsInstanced(GL_TRIANGLES, mesh->getIndexCount(), GL_UNSIGNED_INT, 0, 
                                   scaledTransforms.size());
        }
        
        // 清理实例VBO
        glDeleteBuffers(1, &instanceVBO);
    } else {
        // 普通渲染 - 单个模型
        // 提取平移部分
        glm::vec3 position(transforms[0][3]);
        
        // 创建新的矩阵，添加统一缩放
        glm::mat4 scaledTransform = glm::translate(glm::mat4(1.0f), position);
        scaledTransform = glm::scale(scaledTransform, glm::vec3(scaleFactor));
        
        modelShader_->setMat4("model", scaledTransform);
        
        // 渲染每个网格
        for (const auto& mesh : meshes) {
            // 设置材质
            const Material& material = mesh->getMaterial();
            modelShader_->setVec4("material.diffuse", material.diffuse);
            modelShader_->setFloat("material.shininess", material.shininess);
            modelShader_->setBool("material.hasTexture", material.hasTexture());
            
            // 确保设置顶点颜色渲染模式
            if (modelType == ModelType::GROUND) {
                modelShader_->setBool("useVertexColor", true);
            }
            
            // 如果有纹理，绑定纹理
            if (material.hasTexture()) {
                glActiveTexture(GL_TEXTURE0);
                glBindTexture(GL_TEXTURE_2D, material.diffuseMap);
                modelShader_->setInt("diffuseMap", 0);
            }
            
            // 绑定VAO并绘制
            glBindVertexArray(mesh->getVAO());
            glDrawElements(GL_TRIANGLES, mesh->getIndexCount(), GL_UNSIGNED_INT, 0);
        }
    }
    
    // 重置状态
    glBindVertexArray(0);
    
    // 如果启用了混合，禁用它
    if (renderParams_[modelIndex].transparency < 1.0f) {
        enableBlending(false);
    }
    
    // 重置线框模式
    if (editState_->showWireframe) {
        enableWireframe(false);
    }
}

// 更新矩阵
void Renderer::updateMatrices() {
    // 更新投影矩阵
    float aspectRatio = static_cast<float>(viewportWidth_) / static_cast<float>(viewportHeight_);
    projectionMatrix_ = glm::perspective(glm::radians(45.0f), aspectRatio, 0.1f, 100.0f);
    
    // 检查是否在编辑模式
    if (editState_->mode == EditMode::EDIT) {
        // 编辑模式 - 垂直俯瞰视图
        
        // 计算地图尺寸
        float mapWidth = maze_ ? static_cast<float>(maze_->getWidth()) : 50.0f;
        float mapHeight = maze_ ? static_cast<float>(maze_->getHeight()) : 50.0f;
        float mapSize = std::max(mapWidth, mapHeight);
        
        // 计算摄像机应该放置的高度
        float height = mapSize * 0.75f / tanf(glm::radians(22.5f)); // 45/2=22.5度
        
        // 摄像机的目标点为地图中央
        glm::vec3 target(mapWidth * 0.5f, 0.0f, mapHeight * 0.5f);
        
        // 摄像机位置正好在目标点上方
        glm::vec3 eye = target;
        eye.y = height; // 只改变高度，保持XZ坐标与目标点相同
        
        // 应用偏移
        glm::vec2 offset = editState_->cameraOffset;
        eye.x += offset.x;
        eye.z += offset.y;
        target.x += offset.x;
        target.z += offset.y;
        
        // 对于垂直向下的视图，向上向量应该是指向北方(Z轴负方向)
        // 因为OpenGL中Y轴是向上的，我们向下看，所以"北"方向是-Z
        viewMatrix_ = glm::lookAt(eye, target, glm::vec3(0.0f, 0.0f, -1.0f));
    } else {
        // 普通查看模式 - 轨道相机模型
        
        // 计算相机距离
        float distance = 10.0f / editState_->zoomLevel;
        
        // 相机初始位置在Z轴正向
        glm::vec3 cameraPos(0.0f, 0.0f, distance);
        
        // 先绕X轴旋转，再绕Y轴旋转
        glm::mat4 rotationMatrix = glm::mat4(1.0f);
        rotationMatrix = glm::rotate(rotationMatrix, editState_->cameraRotationY, glm::vec3(0.0f, 1.0f, 0.0f));
        rotationMatrix = glm::rotate(rotationMatrix, editState_->cameraRotationX, glm::vec3(1.0f, 0.0f, 0.0f));
        
        // 应用旋转得到最终相机位置
        cameraPos = glm::vec3(rotationMatrix * glm::vec4(cameraPos, 1.0f));
        
        // 增加一定的高度，避免与地面平行
        cameraPos.y += 2.0f;
        
        // 加上相机偏移
        glm::vec3 cameraTarget(editState_->cameraOffset.x, 0.0f, editState_->cameraOffset.y);
        cameraPos += glm::vec3(editState_->cameraOffset.x, 0.0f, editState_->cameraOffset.y);
        
        // 创建lookAt矩阵
        viewMatrix_ = glm::lookAt(cameraPos, cameraTarget, glm::vec3(0.0f, 1.0f, 0.0f));
    }
}

// 渲染地面
void Renderer::renderGround() {
    if (!maze_ || !tileManager_) return;
    
    // 强制使用顶点颜色渲染
    modelShader_->use();
    
    // 设置更强的环境光照
    modelShader_->setFloat("ambientStrength", 1.0f);
    
    // 批量渲染地面
    renderModels(ModelType::GROUND, groundTransforms_);
}

// 渲染路径
void Renderer::renderPath() {
    if (!editState_->showPath) return;
    renderModels(ModelType::PATH, pathTransforms_);
}

// 渲染障碍物
void Renderer::renderObstacles() {
    if (!editState_->showObstacles) return;
    renderModels(ModelType::OBSTACLE, obstacleTransforms_);
}

// 渲染代理
void Renderer::renderAgents() {
    if (!maze_) return;
    renderModels(ModelType::AGENT, agentTransforms_);
}

// 渲染起点
void Renderer::renderStart() {
    renderModels(ModelType::START, startTransforms_);
}

// 渲染终点
void Renderer::renderGoal() {
    renderModels(ModelType::GOAL, goalTransforms_);
}

// 更新模型的世界变换矩阵
void Renderer::updateModelTransforms() {
    if (!tileManager_ || !maze_) return;
    
    // 首先与迷宫同步，确保TileManager反映最新的迷宫状态
    tileManager_->syncWithMaze();
    
    // 更新地面模型的变换
    groundTransforms_.clear();
    int width = maze_->getWidth();
    int height = maze_->getHeight();
    for (int y = 0; y < height; y++) {
        for (int x = 0; x < width; x++) {
            if (!maze_->isInBounds(x, y)) continue;
            
            glm::vec3 worldPos = tileManager_->getTileWorldPosition(x, y);
            glm::mat4 model = glm::translate(glm::mat4(1.0f), worldPos);
            groundTransforms_.push_back(model);
        }
    }
    
    // 更新路径模型的变换
    pathTransforms_ = tileManager_->getTransformsByType(TileOverlayType::Path);
    
    // 更新障碍物模型的变换
    obstacleTransforms_ = tileManager_->getTransformsByType(TileOverlayType::Obstacle);
    
    // 更新起点模型的变换
    startTransforms_ = tileManager_->getTransformsByType(TileOverlayType::Start);
    
    // 更新终点模型的变换
    goalTransforms_ = tileManager_->getTransformsByType(TileOverlayType::Goal);
    
    // 更新代理模型的变换
    agentTransforms_ = tileManager_->getTransformsByType(TileOverlayType::Current);
}

void Renderer::rotate(float dx, float dy) {
    // 降低旋转速度，特别是对于Y轴旋转
    float xRotationSpeed = 0.005f; 
    float yRotationSpeed = 0.005f;
    
    // Y轴旋转（左右）
    editState_->cameraRotationY -= dx * yRotationSpeed;
    
    // X轴旋转（上下）
    float newRotationX = editState_->cameraRotationX - dy * xRotationSpeed;
    
    // 严格限制X轴旋转角度在±85度以内，防止接近极点
    const float MAX_X_ROTATION = glm::radians(85.0f);
    editState_->cameraRotationX = glm::clamp(newRotationX, -MAX_X_ROTATION, MAX_X_ROTATION);
    
    // 更新视图矩阵
    updateMatrices();
}

}  // namespace PathGlyph
