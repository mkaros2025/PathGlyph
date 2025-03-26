#pragma once
#include "obstacle.h"
#include "../common/Point.h"
#include <vector>
#include <queue>
#include <memory>
#include <string>

namespace PathGlyph {

// A*路径规划节点
struct AStarNode {
    int x, y;
    double g;          // 起点到当前的代价
    double h;          // 启发式：当前到终点的估计代价
    double f;          // f = g + h
    AStarNode* parent;
    
    AStarNode(int x_, int y_, double g_ = 0, double h_ = 0, AStarNode* parent_ = nullptr)
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
    
    // 基本设置
    void setStart(int x, int y);
    void setGoal(int x, int y);
    
    // 清除起点或终点
    void clearStart() { start_ = Point(-1, -1); }
    void clearGoal() { goal_ = Point(-1, -1); }
    
    // 障碍物管理
    void addObstacle(int x, int y);
    void addObstacle(double x, double y, double radius = 1.0); // 静态障碍物
    void addObstacle(double centerX, double centerY, double orbitRadius, double speed, double startAngle = 0.0); // 动态障碍物
    void removeObstacle(int x, int y);
    void clearObstacles();
    
    // A*路径规划
    std::vector<Point> findPathAStar();
    
    // 清除路径
    void clearPath() { path_.clear(); }
    
    // 更新动态障碍物和当前位置
    void update(double dt);
    
    // 查询方法
    int getWidth() const { return width_; }
    int getHeight() const { return height_; }
    bool isPathFound() const { return !path_.empty(); }
    bool hasValidPath() const { return isPathFound(); }
    bool hasReachedGoal() const;
    bool isObstacle(int x, int y) const;
    bool isStartPoint(int x, int y) const { return start_.x == x && start_.y == y; }
    bool isEndPoint(int x, int y) const { return goal_.x == x && goal_.y == y; }
    
    // 获取状态信息（用于渲染）
    const Point& getStart() const { return start_; }
    const Point& getGoal() const { return goal_; }
    const Point& getCurrentPosition() const { return current_; }
    const std::vector<Point>& getPath() const { return path_; }
    const std::vector<std::shared_ptr<Obstacle>>& getObstacles() const { return obstacles_; }
    
private:
    // 判断位置是否有效和安全
    bool isValid(int x, int y) const;
    bool isSafe(int x, int y) const;
    
    // A*辅助函数
    double heuristic(int x1, int y1, int x2, int y2) const;
    void reconstructPath(AStarNode* node);
    
    int width_;
    int height_;
    
    Point start_;
    Point goal_;
    Point current_;  // 当前位置（用于DWA）
    
    std::vector<Point> path_;  // 规划路径
    std::vector<std::shared_ptr<Obstacle>> obstacles_;  // 所有障碍物
};

} // namespace PathGlyph