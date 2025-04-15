#include "geometry/tileManager.h"
#include "maze/maze.h"
#include <algorithm>
#include <glm/gtc/matrix_transform.hpp>
#include <iostream>

namespace PathGlyph {

// 定义静态变换参数
const ModelTransformParams TileManager::groundParams = {
    1.0f,  // scaleFactor
    glm::vec3(0.0f, 0.0f, 0.0f),  // positionOffset
    glm::quat(1.0f, 0.0f, 0.0f, 0.0f)  // rotation
};

const ModelTransformParams TileManager::pathParams = {
    1.0f,  // scaleFactor
    glm::vec3(0.0f, 0.1f, 0.0f),  // positionOffset
    glm::quat(1.0f, 0.0f, 0.0f, 0.0f)  // rotation
};

const ModelTransformParams TileManager::obstacleParams = {
    0.5f,  // scaleFactor
    glm::vec3(0.0f, 0.9f, 0.5f),  // positionOffset
    glm::quat(1.0f, 0.0f, 0.0f, 0.0f)  // rotation
};

const ModelTransformParams TileManager::startParams = {
    1.0f,  // scaleFactor
    glm::vec3(0.0f, 0.5f, 0.0f),  // positionOffset
    glm::quat(1.0f, 0.0f, 0.0f, 0.0f)  // rotation
};

const ModelTransformParams TileManager::goalParams = {
    1.0f,  // scaleFactor
    glm::vec3(0.0f, 0.5f, 0.0f),  // positionOffset
    glm::quat(1.0f, 0.0f, 0.0f, 0.0f)  // rotation
};

const ModelTransformParams TileManager::agentParams = {
    0.4f,  // scaleFactor
    glm::vec3(0.0f, 1.0f, 0.0f),  // positionOffset
    glm::quat(1.0f, 0.0f, 0.0f, 0.0f)  // rotation
};

const ModelTransformParams TileManager::gridLineParams = {
    1.0f,  // scaleFactor
    glm::vec3(0.0f, 0.02f, 0.0f),  // positionOffset
    glm::quat(1.0f, 0.0f, 0.0f, 0.0f)  // rotation
};

// 构造函数 - 直接包含初始化逻辑
TileManager::TileManager(std::shared_ptr<Maze> maze, int width, int height) : maze_(maze), width_(width), height_(height) {
    // 清除现有数据
    tiles_.clear();
    
    // 调整大小并初始化所有图块
    tiles_.resize(height_);
    for (int y = 0; y < height_; ++y) {
        tiles_[y].resize(width_);
        for (int x = 0; x < width_; ++x) {
            createTile(x, y);
        }
    }
}

// 创建新图块
void TileManager::createTile(int x, int y) {
    Tile& tile = tiles_[y][x];
    tile.x = x;
    tile.y = y;
    tile.overlayType = TileOverlayType::None; // 默认为无覆盖
}

// 获取特定位置的图块
Tile* TileManager::getTileAt(int x, int y) {
    if (x < 0 || x >= width_ || y < 0 || y >= height_) {
        return nullptr;
    }
    return &tiles_[y][x];
}


// 获取图块的世界坐标并返回变换矩阵
glm::mat4 TileManager::getTileWorldPosition(int x, int y, const ModelTransformParams& params) const {
    glm::vec3 position(static_cast<float>(x), 0.0f, static_cast<float>(y));
    position += params.positionOffset;
    
    float scale = params.scaleFactor;

    glm::mat4 model = glm::mat4(1.0f);
    model = glm::translate(model, position);
    model = model * glm::mat4_cast(params.rotation); // 应用旋转
    model = glm::scale(model, glm::vec3(scale, scale, scale));
    
    return model;
}

// 获取地面变换矩阵
std::vector<glm::mat4> TileManager::getGroundTransforms() const {
    std::vector<glm::mat4> transforms;
    transforms.reserve(width_ * height_);

    for (int y = 0; y < height_; ++y) {
        for (int x = 0; x < width_; ++x) {
            const Tile& tile = tiles_[y][x];
            transforms.push_back(getTileWorldPosition(tile.x, tile.y, groundParams));
        }
    }
    
    return transforms;
}

// 获取路径变换矩阵
std::vector<glm::mat4> TileManager::getPathTransforms() const {
    std::vector<glm::mat4> transforms;
    
    const auto& path = maze_->getPath();
    transforms.reserve(path.size());
    
    for (const auto& point : path) {
        transforms.push_back(getTileWorldPosition(point.x, point.y, pathParams));
    }
    
    return transforms;
}

// 获取障碍物变换矩阵
std::vector<glm::mat4> TileManager::getObstacleTransforms() const {
    std::vector<glm::mat4> transforms;
    
    const auto& staticObstacles = maze_->getStaticObstacles();
    transforms.reserve(staticObstacles.size());
    
    for (const auto& obstacle : staticObstacles) {
        Point pos = obstacle->getLogicalPosition();
        transforms.push_back(getTileWorldPosition(pos.x, pos.y, obstacleParams));
    }
    
    return transforms;
}

// 获取动态障碍物变换矩阵
std::vector<glm::mat4> TileManager::getDynamicObstacleTransforms() const {
    std::vector<glm::mat4> transforms;
    
    if (maze_) {
        const auto& dynamicObstacles = maze_->getDynamicObstacles();
        transforms.reserve(dynamicObstacles.size());
        
        for (const auto& obstacle : dynamicObstacles) {
            Point pos = obstacle->getLogicalPosition();
            transforms.push_back(getTileWorldPosition(pos.x, pos.y, obstacleParams));
        }
    }
    
    return transforms;
}

// 获取起点变换矩阵
std::vector<glm::mat4> TileManager::getStartTransforms() const {
    std::vector<glm::mat4> transforms;
    
    const Point& start = maze_->getStart();
    if (start.x >= 0 && start.y >= 0) {
        transforms.push_back(getTileWorldPosition(start.x, start.y, startParams));
    }
    
    return transforms;
}

// 获取终点变换矩阵
std::vector<glm::mat4> TileManager::getGoalTransforms() const {
    std::vector<glm::mat4> transforms;
    
    const Point& goal = maze_->getGoal();
    if (goal.x >= 0 && goal.y >= 0) {
        transforms.push_back(getTileWorldPosition(goal.x, goal.y, goalParams));
    }
    
    return transforms;
}

// 获取代理变换矩阵
std::vector<glm::mat4> TileManager::getAgentTransforms() const {
    std::vector<glm::mat4> transforms;
    
    Point pos = maze_->getCurrentPosition();
    if (pos.x >= 0 && pos.y >= 0) {
        transforms.push_back(getTileWorldPosition(pos.x, pos.y, agentParams));
    }

    return transforms;
}

// 获取网格线的变换矩阵
std::vector<glm::mat4> TileManager::getGridLineTransforms() const {
    std::vector<glm::mat4> transforms;
    
    // 水平线 (Z轴方向)
    for (int y = 0; y <= height_; ++y) {
        for (int x = 0; x < width_; ++x) {
            ModelTransformParams params = gridLineParams;
            params.positionOffset = glm::vec3(0.5f, 0.0f, 0.0f); // 水平线的偏移
            transforms.push_back(getTileWorldPosition(x, y, params));
        }
    }
    
    // 垂直线 (X轴方向)
    for (int x = 0; x <= width_; ++x) {
        for (int y = 0; y < height_; ++y) {
            ModelTransformParams params = gridLineParams;
            params.positionOffset = glm::vec3(0.0f, 0.0f, 0.5f); // 垂直线的偏移
            transforms.push_back(getTileWorldPosition(x, y, params));
        }
    }
    
    return transforms;
}

// 屏幕坐标转换为图块坐标
bool TileManager::screenToTileCoordinate(const glm::vec2& screenPos, int& outX, int& outY, 
                                        const glm::mat4& viewProj) const {
    // 将屏幕坐标转换为归一化设备坐标（NDC）
    glm::vec4 rayClip = glm::vec4(
        (2.0f * screenPos.x) - 1.0f,  // X范围从-1到1
        1.0f - (2.0f * screenPos.y),  // Y范围从-1到1（翻转Y轴）
        -1.0f,                       // 近平面
        1.0f
    );
    
    // 计算视图投影矩阵的逆矩阵
    glm::mat4 invViewProj = glm::inverse(viewProj);
    
    // 将裁剪空间坐标转换回世界空间
    glm::vec4 rayWorld = invViewProj * rayClip;
    if (rayWorld.w == 0.0f) {
        return false;  // 无效射线
    }
    
    // 透视除法
    rayWorld /= rayWorld.w;
    
    // 计算近平面和远平面上的点
    glm::vec4 rayClipNear = glm::vec4((2.0f * screenPos.x) - 1.0f, 1.0f - (2.0f * screenPos.y), -1.0f, 1.0f);
    glm::vec4 rayClipFar = glm::vec4((2.0f * screenPos.x) - 1.0f, 1.0f - (2.0f * screenPos.y), 1.0f, 1.0f);
    
    glm::vec4 rayWorldNear = invViewProj * rayClipNear;
    glm::vec4 rayWorldFar = invViewProj * rayClipFar;
    
    if (rayWorldNear.w == 0.0f || rayWorldFar.w == 0.0f) {
        return false;  // 无效射线
    }
    
    // 透视除法
    rayWorldNear /= rayWorldNear.w;
    rayWorldFar /= rayWorldFar.w;
    
    // 获取射线起点和方向
    glm::vec3 rayOrigin = glm::vec3(rayWorldNear);
    glm::vec3 rayDirection = glm::normalize(glm::vec3(rayWorldFar) - glm::vec3(rayWorldNear));
    
    // 求射线与地面平面的交点
    // 地面平面可能并不是y=0，而是y=某个定值
    // 定义地面平面法向量(上方向)和平面上一点
    glm::vec3 planeNormal(0.0f, 1.0f, 0.0f); // 假设y轴是向上的
    glm::vec3 planePoint(0.0f, 0.0f, 0.0f);  // 地面上一点
    
    // 计算射线与平面的交点
    float denom = glm::dot(rayDirection, planeNormal);
    if (std::abs(denom) < 0.0001f) {
        std::cout << "射线几乎平行于地面，无法求交: " << rayDirection.x << "," << rayDirection.y << "," << rayDirection.z << std::endl;
        return false;  // 射线几乎平行于地面
    }
    
    // 计算射线到平面的距离
    float t = glm::dot(planePoint - rayOrigin, planeNormal) / denom;
    if (t < 0.0f) {
        std::cout << "交点在射线后方: t = " << t << ", rayOrigin.y = " << rayOrigin.y << ", rayDirection.y = " << rayDirection.y << std::endl;
        return false;  // 交点在射线后方
    }
    
    // 计算交点位置
    glm::vec3 intersection = rayOrigin + t * rayDirection;
    
    std::cout << "原始交点: " << intersection.x << "," << intersection.y << "," << intersection.z << std::endl;
    std::cout << "射线起点: " << rayOrigin.x << "," << rayOrigin.y << "," << rayOrigin.z << std::endl;
    std::cout << "射线方向: " << rayDirection.x << "," << rayDirection.y << "," << rayDirection.z << std::endl;
    
    // 完全简化的替代方法 - 使用直接映射
    // 假设鼠标点击为(screenPos.x, screenPos.y)，范围是(0,0)到(windowWidth,windowHeight)
    
    // 计算地图坐标 - 直接的映射方法
    float normalizedX = (screenPos.x - 300.0f) / 500.0f;  // 减去UI宽度，除以剩余区域宽度
    float normalizedY = screenPos.y / 600.0f;            // 归一化Y坐标
    
    // 归一化坐标限制在[0,1]范围内
    normalizedX = std::max(0.0f, std::min(normalizedX, 1.0f));
    normalizedY = std::max(0.0f, std::min(normalizedY, 1.0f));
    
    // 转换为地图坐标
    outX = static_cast<int>(normalizedX * (width_ - 1));
    outY = static_cast<int>((1.0f - normalizedY) * (height_ - 1));  // 反转Y轴
    
    std::cout << "屏幕位置: " << screenPos.x << "," << screenPos.y << std::endl;
    std::cout << "归一化坐标: " << normalizedX << "," << normalizedY << std::endl;
    std::cout << "地图坐标: " << outX << "," << outY << std::endl;
    std::cout << "有效范围: 0-" << (width_-1) << ", 0-" << (height_-1) << std::endl;
    
    // 检查坐标是否在图块范围内
    bool result = (outX >= 0 && outX < width_ && outY >= 0 && outY < height_);
    std::cout << "检查结果: " << (result ? "成功" : "失败") << std::endl;
    return result;
}


} // namespace PathGlyph
