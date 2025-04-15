#pragma once
#include "maze/obstacle.h"
#include "common/types.h"
#include <vector>
#include <queue>
#include <memory>
#include <string>
#include <glm/glm.hpp>
#include <unordered_map>

namespace PathGlyph {

// A*路径规划节点
struct AStarNode {
    int x, y;          // 保持使用整数网格坐标用于寻路
    double g;          // 起点到当前的代价
    double h;          // 启发式：当前到终点的估计代价
    double f;          // f = g + h
    std::shared_ptr<AStarNode> parent = nullptr;
    
    AStarNode(int x_, int y_, double g_ = 0, double h_ = 0, std::shared_ptr<AStarNode> parent_ = nullptr)
        : x(x_), y(y_), g(g_), h(h_), f(g_ + h_), parent(parent_) {}
        
    bool operator>(const AStarNode& other) const {
        return f > other.f;
    }
};

class Maze {
public:
    Maze(int width = 50, int height = 50);
    ~Maze();

    // 从JSON文件加载地图配置
    bool loadFromJson(const std::string& filename);
    
    int getWidth() const { return width_; }
    int getHeight() const { return height_; }
    const Point& getStart() const { return start_; }
    const Point& getGoal() const { return goal_; }
    const Point& getCurrentPosition() const { return current_; }
    
    // 更新动态障碍物和当前位置
    void update(float deltaTime);
    
    // 设置起点和终点
    void setStart(const Point& position);
    void setGoal(const Point& position);
    void clearStart() { start_ = Point(-1.0, -1.0); }
    void clearGoal() { goal_ = Point(-1.0, -1.0); }
    
    // 位置管理
    void setCurrentPosition(const Point& pos) { current_ = pos; }
    
    // 增加障碍物
    void addStaticObstacle(const Point& position);
    void addDynamicObstacle(const Point& position, float speed, glm::vec2 direction);
    void addDynamicObstacle(const Point& position, Point center, float radius, float angularSpeed);
    // 可以删除静态障碍物也可以删除动态障碍物
    void removeObstacle(const Point& position, double tolerance = 0.5);
    void clearStaticObstacles();
    void clearDynamicObstacles();

    // 重置障碍物和代理的位置
    void reset();
    
    // 路径管理
    void setPath(const std::vector<Point>& path);
    void clearPath() { path_.clear(); }
    
    // 路径状态查询
    bool isPathFound() const { return !path_.empty(); }
    bool hasValidPath() const { return isPathFound(); }
    bool hasReachedGoal() const;
    
    // 边界检测 - 考虑到Point是游戏中的连续坐标，通过四舍五入得到网格坐标
    bool isInBounds(const Point& position) const { 
        // 转换为整数网格坐标
        Point gridPos = position.toInt();
        return gridPos.x >= 0.0 && gridPos.x < static_cast<double>(width_) && 
               gridPos.y >= 0.0 && gridPos.y < static_cast<double>(height_); 
    }
    
    // 障碍物检测
    bool isStaticObstacle(const Point& position) const;
    bool isDynamicObstacle(const Point& position) const;
    
    // 起终点检测 - 使用距离比较而不是精确相等，因为坐标系统现在允许小数
    bool isStartPoint(const Point& position) const { 
        return position.distanceTo(start_) < 0.5;
    }
    bool isEndPoint(const Point& position) const { 
        return position.distanceTo(goal_) < 0.5;
    }
    
    // 碰撞检测
    bool checkCollision(const Point& pos, float radius = 0.5f) const;
    
    
    const std::vector<Point>& getPath() const { return path_; }
    const std::vector<std::shared_ptr<StaticObstacle>>& getStaticObstacles() const { return staticObstacles_; }
    const std::vector<std::shared_ptr<DynamicObstacle>>& getDynamicObstacles() const { return dynamicObstacles_; }
    

    // 世界坐标向逻辑坐标的转换
    Point worldToLogical(const glm::vec3& worldPos) const;

    // A*全局路径规划
    std::vector<Point> findPathAStar();
    
    // DWA局部路径规划
    glm::vec2 findBestLocalVelocity(const Point& currentPos, const glm::vec2& currentVel, 
                                 const Point& targetPos, float maxSpeed, float maxRotSpeed);
private:
    int width_;
    int height_;
    
    Point start_;
    Point goal_;
    Point current_;
    
    std::vector<Point> path_;  // 规划路径
    std::vector<std::shared_ptr<StaticObstacle>> staticObstacles_;  // 静态障碍物
    std::vector<std::shared_ptr<DynamicObstacle>> dynamicObstacles_;  // 动态障碍物
    
    // A*算法辅助方法
    // 检查是否在地图边界内
    bool isValid(int x, int y) const;
    // 检查是否没有静态障碍物
    bool isSafe(int x, int y) const;
    // 计算距离
    double heuristic(int x1, int y1, int x2, int y2) const;
    // 通过回溯来构建完整路径
    void reconstructPath(std::shared_ptr<AStarNode> node);
    
    // DWA算法辅助方法
    // 生成速度样本的函数。它根据当前速度、最大速度和最大旋转速度，在给定的样本数量内生成一系列可能的速度向量。
    void generateVelocitySamples(const glm::vec2& currentVel, float maxSpeed, float maxRotSpeed, 
                               int samples, std::vector<glm::vec2>& velocitySamples);
    // 计算轨迹的障碍物避开分数。它评估一条轨迹避开障碍物的能力，分数越高表示轨迹越安全
    float evaluateTrajectory(const glm::vec2& velocity, const Point& currentPos, 
                           const Point& targetPos, float predictTime);
    float calculateObstacleAvoidanceScore(const std::vector<Point>& trajectory);
    // 计算目标方向分数。它评估速度向量与朝向目标方向的一致程度，分数越高表示方向越接近目标
    float calculateGoalDirectionScore(const glm::vec2& velocity, const Point& currentPos, 
                                    const Point& targetPos);
    // 计算目标距离分数。它评估轨迹终点与目标位置的接近程度，分数越高表示越接近目标
    float calculateGoalDistanceScore(const Point& endPos, const Point& targetPos);
};

} // namespace PathGlyph