#include "tileManager.h"
#include "maze.h"
#include <algorithm>
#include <glm/gtc/matrix_transform.hpp>

namespace PathGlyph {

// 构造函数
TileManager::TileManager(std::shared_ptr<Maze> maze)
    : width_(0), height_(0), maze_(maze) {
}

// 析构函数
TileManager::~TileManager() {
    // 智能指针会自动清理内存
}

// 初始化图块管理器
void TileManager::initialize(int width, int height) {
    width_ = width;
    height_ = height;
    
    // 调整二维数组大小
    tiles_.resize(width);
    for (int x = 0; x < width; ++x) {
        tiles_[x].resize(height);
        for (int y = 0; y < height; ++y) {
            createTile(x, y);
        }
    }
    
    syncWithMaze();
}

// 更新所有图块状态
void TileManager::update(double dt) {
    // 在此实现图块的动画或状态更新逻辑
    // 例如：使路径图块闪烁、光效等
    
    // 如果有迷宫，同步迷宫状态变化
    syncWithMaze();
}

// 与迷宫数据同步
void TileManager::syncWithMaze() {
    // 清除所有现有的覆盖类型
    for (int x = 0; x < width_; ++x) {
        for (int y = 0; y < height_; ++y) {
            clearTileOverlay(x, y);
        }
    }
    
    if (!maze_) return; // 确保maze_有效
    
    // 设置路径图块
    for (const auto& point : maze_->getPath()) {
        if (point.x >= 0 && point.x < width_ && point.y >= 0 && point.y < height_) {
            setTileOverlay(point.x, point.y, TileOverlayType::Path);
        }
    }
    
    // 设置静态障碍物图块
    for (const auto& obstacle : maze_->getObstacles()) {
        // 确保障碍物指针有效
        if (!obstacle) continue;
        
        Point pos = maze_->worldToLogical(obstacle->getWorldPosition());
        if (pos.x >= 0 && pos.x < width_ && pos.y >= 0 && pos.y < height_) {
            setTileOverlay(pos.x, pos.y, TileOverlayType::Obstacle);
        }
    }
    
    // 设置动态障碍物图块
    // 动态障碍物直接记录其世界坐标（连续位置），不再转换为网格坐标
    dynamicObstaclePositions_.clear();
    for (const auto& obstacle : maze_->getDynamicObstacles()) {
        // 确保障碍物指针有效
        if (!obstacle) continue;
        
        // 保存精确的世界位置，不再转换为网格坐标
        dynamicObstaclePositions_.push_back(obstacle->getWorldPosition());
    }
    
    // 设置起点图块
    const Point& start = maze_->getStart();
    if (start.x >= 0 && start.x < width_ && start.y >= 0 && start.y < height_) {
        setTileOverlay(start.x, start.y, TileOverlayType::Start);
    }
    
    // 设置终点图块
    const Point& goal = maze_->getGoal();
    if (goal.x >= 0 && goal.x < width_ && goal.y >= 0 && goal.y < height_) {
        setTileOverlay(goal.x, goal.y, TileOverlayType::Goal);
    }
    
    // 设置当前位置图块
    const Point& current = maze_->getCurrentPosition();
    if (current.x >= 0 && current.x < width_ && current.y >= 0 && current.y < height_) {
        setTileOverlay(current.x, current.y, TileOverlayType::Current);
    }
}

// 获取特定位置的图块
Tile* TileManager::getTileAt(int x, int y) {
    if (x >= 0 && x < width_ && y >= 0 && y < height_) {
        return tiles_[x][y].get();
    }
    return nullptr;
}

// 设置图块覆盖类型
void TileManager::setTileOverlay(int x, int y, TileOverlayType type) {
    Tile* tile = getTileAt(x, y);
    if (!tile) return;
    
    // 从之前的类型列表中移除
    TileOverlayType oldType = tile->getOverlayType();
    auto& oldTypeList = tilesByType_[oldType];
    oldTypeList.erase(
        std::remove(oldTypeList.begin(), oldTypeList.end(), tile),
        oldTypeList.end()
    );
    
    // 设置新类型并添加到对应类型列表
    tile->setOverlayType(type);
    tilesByType_[type].push_back(tile);
}

// 清除图块覆盖
void TileManager::clearTileOverlay(int x, int y) {
    setTileOverlay(x, y, TileOverlayType::None);
}

// 获取特定类型的所有图块
std::vector<Tile*> TileManager::getTilesByType(TileOverlayType type) {
    return tilesByType_[type];
}

// 获取图块世界位置
glm::vec3 TileManager::getTileWorldPosition(int x, int y) const {
    return glm::vec3(
        x * Tile::TILE_SIZE,
        0.0f,
        y * Tile::TILE_SIZE
    );
}

// 屏幕坐标转换为图块坐标
bool TileManager::screenToTileCoordinate(const glm::vec2& screenPos, int& outX, int& outY, 
                                         const glm::mat4& viewProj) const {
    // 需要实现基于视图投影矩阵的坐标转换
    // 这里使用简化的射线与平面相交检测
    // 假设y=0平面是图块所在的平面
    
    // 创建逆视图投影矩阵
    glm::mat4 invViewProj = glm::inverse(viewProj);
    
    // 从屏幕坐标获取两个深度位置的世界坐标，用于射线构建
    glm::vec3 nearPoint = Tile::screenToWorld(screenPos, 0.0f, invViewProj);
    glm::vec3 farPoint = Tile::screenToWorld(screenPos, 1.0f, invViewProj);
    
    // 计算射线方向
    glm::vec3 rayDir = glm::normalize(farPoint - nearPoint);
    
    // 检查射线是否平行于y=0平面
    if (std::abs(rayDir.y) < 0.0001f) return false;
    
    // 计算射线与y=0平面的交点
    float t = -nearPoint.y / rayDir.y;
    if (t < 0) return false; // 交点在射线反方向
    
    // 计算交点坐标
    glm::vec3 intersectionPoint = nearPoint + rayDir * t;
    
    // 将世界坐标转换为图块坐标
    outX = static_cast<int>(intersectionPoint.x / Tile::TILE_SIZE);
    outY = static_cast<int>(intersectionPoint.z / Tile::TILE_SIZE);
    
    // 检查是否在有效范围内
    return (outX >= 0 && outX < width_ && outY >= 0 && outY < height_);
}

// 获取特定类型图块的变换矩阵数组
std::vector<glm::mat4> TileManager::getTransformsByType(TileOverlayType type) const {
    std::vector<glm::mat4> transforms;
    
    // 查找对应类型的图块
    auto it = tilesByType_.find(type);
    if (it != tilesByType_.end()) {
        const auto& tiles = it->second;
        transforms.reserve(tiles.size());
        
        // 为每个图块创建变换矩阵
        for (const Tile* tile : tiles) {
            glm::vec3 position = tile->getWorldPosition();
            
            // 根据类型添加高度偏移
            position.y += getHeightOffsetForType(type);
            
            // 创建变换矩阵
            glm::mat4 transform = glm::translate(glm::mat4(1.0f), position);
            transforms.push_back(transform);
        }
    }
    
    return transforms;
}

// 获取所有图块的变换矩阵
std::vector<glm::mat4> TileManager::getAllTileTransforms() const {
    std::vector<glm::mat4> transforms;
    transforms.reserve(width_ * height_);
    
    for (int x = 0; x < width_; ++x) {
        for (int y = 0; y < height_; ++y) {
            const Tile* tile = tiles_[x][y].get();
            glm::vec3 position = tile->getWorldPosition();
            
            // 添加基础高度偏移
            position.y += getHeightOffsetForType(tile->getOverlayType());
            
            // 创建变换矩阵
            glm::mat4 transform = glm::translate(glm::mat4(1.0f), position);
            transforms.push_back(transform);
        }
    }
    
    return transforms;
}

// 获取图块包围盒
Tile::BoundingBox TileManager::getTileBoundingBox() const {
    // 返回第一个图块的包围盒作为参考
    // 假设所有图块的几何尺寸相同
    static Tile::BoundingBox defaultBox = {
        glm::vec3(-Tile::TILE_SIZE/2.0f, 0.0f, -Tile::TILE_SIZE/2.0f),
        glm::vec3(Tile::TILE_SIZE/2.0f, Tile::TILE_HEIGHT, Tile::TILE_SIZE/2.0f)
    };
    
    if (width_ > 0 && height_ > 0 && tiles_[0][0]) {
        return tiles_[0][0]->getBoundingBox();
    }
    
    return defaultBox;
}

// 创建新图块
void TileManager::createTile(int x, int y, TileOverlayType type) {
    if (x >= 0 && x < width_ && y >= 0 && y < height_) {
        tiles_[x][y] = std::make_unique<Tile>(x, y, type);
        tilesByType_[type].push_back(tiles_[x][y].get());
    }
}

// 获取类型的高度偏移
float TileManager::getHeightOffsetForType(TileOverlayType type) const {
    // 为不同类型的图块提供不同的高度偏移，用于视觉区分
    switch (type) {
        case TileOverlayType::None:
            return 0.0f;
        case TileOverlayType::Path:
            return 0.05f;
        case TileOverlayType::Start:
            return 0.1f;
        case TileOverlayType::Goal:
            return 0.1f;
        case TileOverlayType::Current:
            return 0.15f;
        case TileOverlayType::Obstacle:
            return 0.2f;
        default:
            return 0.0f;
    }
}

} // namespace PathGlyph 