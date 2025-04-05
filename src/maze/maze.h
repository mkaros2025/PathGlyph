#pragma once
#include "obstacle.h"
#include "tile.h"
#include "common/Types.h"
#include <vector>
#include <queue>
#include <memory>
#include <string>
#include <glm/glm.hpp>
#include <unordered_map>

namespace PathGlyph {

// A*路径规划节点 - 扩展到3D空间
struct AStarNode {
    int x, y;  // 维持2D网格坐标作为基础（高度通过Tile处理）
    double g;          // 起点到当前的代价
    double h;          // 启发式：当前到终点的估计代价
    double f;          // f = g + h
    AStarNode* parent;
    
    AStarNode(int x_, int y_, double g_ = 0, double h_ = 0, AStarNode* parent_ = nullptr)
        : x(x_), y(y_), g(g_), h(h_), f(g_ + h_), parent(parent_) {}
        
    bool operator>(const AStarNode& other) const {
        return f > other.f;
    }
    
    // 获取该节点对应的世界坐标位置
    glm::vec3 getWorldPosition() const {
        Tile tile(x, y);
        return tile.getWorldPosition();
    }
};

class Maze {
public:
    Maze(int width = 50, int height = 50);
    ~Maze();
    
    // 从JSON文件加载地图配置
    bool loadFromJson(const std::string& filename);
    
    // 基本设置
    void setStart(int x, int y);
    void setGoal(int x, int y);
    
    // 清除起点或终点
    void clearStart() { start_ = Point(-1, -1); }
    void clearGoal() { goal_ = Point(-1, -1); }
    
    // 障碍物管理
    void addObstacle(int x, int y);
    void addObstacle(double x, double y, double radius = 1.0); // 静态障碍物
    
    // 动态障碍物管理
    void addDynamicObstacle(int x, int y, MovementType type, const ObstacleProperties& props);
    std::vector<Point> getDynamicObstaclePositions() const;
    std::vector<ObstacleProperties> getDynamicObstacleProperties() const;
    
    void removeObstacle(int x, int y);
    void clearObstacles();
    void clearDynamicObstacles();
    
    // 设置路径 - 用于直接设置完成的路径
    void setPath(const std::vector<Point>& path);
    
    // A*路径规划
    std::vector<Point> findPathAStar();
    
    // DWA局部规划 - 返回最佳速度向量
    Vector2D findBestLocalVelocity(const Point& currentPos, const Vector2D& currentVel, 
                                  const Point& targetPos, float maxSpeed, float maxRotSpeed);
    
    // 获取邻近障碍物
    std::vector<std::shared_ptr<Obstacle>> getNearbyObstacles(const Point& center, float radius) const;
    
    // 清除路径
    void clearPath() { path_.clear(); worldPath_.clear(); }
    
    // 更新动态障碍物和当前位置
    void update(float deltaTime);
    
    // 查询方法
    int getWidth() const { return width_; }
    int getHeight() const { return height_; }
    bool isPathFound() const { return !path_.empty(); }
    bool hasValidPath() const { return isPathFound(); }
    bool hasReachedGoal() const;
    bool isObstacle(int x, int y) const;
    bool isObstacleAt(const Point& pos) const { return isObstacle(pos.x, pos.y); }
    bool isDynamicObstacle(int x, int y) const;
    bool isStartPoint(int x, int y) const { return start_.x == x && start_.y == y; }
    bool isEndPoint(int x, int y) const { return goal_.x == x && goal_.y == y; }
    
    // 碰撞检测 - 检查点是否会与任何障碍物碰撞
    bool checkCollision(const Point& pos, float radius = 0.5f) const;
    
    // 判断位置是否在迷宫范围内
    bool isInBounds(int x, int y) const { return x >= 0 && x < width_ && y >= 0 && y < height_; }
    bool isInBounds(const Point& pos) const { return isInBounds(pos.x, pos.y); }
    
    // 获取状态信息（用于渲染）
    const Point& getStart() const { return start_; }
    const Point& getGoal() const { return goal_; }
    const Point& getCurrentPosition() const { return current_; }
    void setCurrentPosition(const Point& pos) { current_ = pos; }
    
    // 逻辑坐标路径
    const std::vector<Point>& getPath() const { return path_; }
    
    // 世界坐标路径 - 用于3D渲染
    const std::vector<glm::vec3>& getWorldPath() const { return worldPath_; }
    
    // 将逻辑坐标转换为世界坐标
    glm::vec3 logicalToWorld(const Point& point) const {
        Tile tile(point.x, point.y);
        return tile.getWorldPosition();
    }
    
    // 将世界坐标转换为逻辑坐标
    Point worldToLogical(const glm::vec3& worldPos) const {
        // 简单的反向计算 - 假设世界坐标系与逻辑坐标系对齐
        int x = static_cast<int>(worldPos.x / Tile::TILE_SIZE + 0.5f);
        int y = static_cast<int>(worldPos.z / Tile::TILE_SIZE + 0.5f);
        return Point(x, y);
    }
    
    const std::vector<std::shared_ptr<Obstacle>>& getObstacles() const { return obstacles_; }
    const std::vector<std::shared_ptr<Obstacle>>& getDynamicObstacles() const { return dynamicObstacles_; }
    
    // 重置障碍物到初始位置
    void resetObstacles();
    
private:
    // 判断位置是否有效和安全
    bool isValid(int x, int y) const;
    bool isSafe(int x, int y) const;
    
    // A*辅助函数
    double heuristic(int x1, int y1, int x2, int y2) const;
    void reconstructPath(AStarNode* node);
    
    // DWA辅助函数
    void generateVelocitySamples(const Vector2D& currentVel, float maxSpeed, float maxRotSpeed, 
                                int samples, std::vector<Vector2D>& velocitySamples);
    float evaluateTrajectory(const Vector2D& velocity, const Point& currentPos, 
                            const Point& targetPos, float predictTime);
    float calculateObstacleAvoidanceScore(const std::vector<Point>& trajectory);
    float calculateGoalDirectionScore(const Vector2D& velocity, const Point& currentPos, 
                                     const Point& targetPos);
    float calculateGoalDistanceScore(const Point& endPos, const Point& targetPos);
    
    // 更新世界坐标路径
    void updateWorldPath();
    
    int width_;
    int height_;
    
    Point start_;
    Point goal_;
    Point current_;  // 当前位置（用于DWA）
    
    std::vector<Point> path_;  // 规划路径（逻辑坐标）
    std::vector<glm::vec3> worldPath_;  // 规划路径（世界坐标）
    std::vector<std::shared_ptr<Obstacle>> obstacles_;  // 静态障碍物
    std::vector<std::shared_ptr<Obstacle>> dynamicObstacles_;  // 动态障碍物
    
    // 存储障碍物属性的映射，用于重置
    std::unordered_map<std::shared_ptr<Obstacle>, ObstacleProperties> obstacleProperties_;
};

} // namespace PathGlyph