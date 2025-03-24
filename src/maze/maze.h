#pragma once
#include "obstacle.h"
#include <vector>
#include <queue>
#include <memory>
#include <string>

namespace PathGlyph {

// 简单坐标结构
struct Point {
    int x;
    int y;
    
    Point() : x(0), y(0) {}
    Point(int x_, int y_) : x(x_), y(y_) {}
    
    double distanceTo(const Point& other) const {
        double dx = x - other.x;
        double dy = y - other.y;
        return std::sqrt(dx*dx + dy*dy);
    }
};

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
    Maze(); // 默认构造函数
    // Maze(int width, int height);
    ~Maze();
    
    // 从JSON文件加载地图配置
    bool loadFromJson(const std::string& filename);
    
    // 基本设置
    void setStart(int x, int y);
    void setGoal(int x, int y);
    
    // 障碍物管理 - 简化为统一接口
    template<typename... Args>
    void addObstacle(Args&&... args) {
        obstacles_.push_back(std::make_shared<Obstacle>(std::forward<Args>(args)...));
    }

    void clearObstacles();
    
    // A*路径规划
    bool findPathAStar();
    
    // 动态窗口法（DWA）
    bool planDWA(double& resultVx, double& resultVy, double maxSpeed = 2.0);
    
    // 更新动态障碍物和当前位置
    void update(double dt);
    
    // 查询方法
    int getWidth() const { return width_; }
    int getHeight() const { return height_; }
    bool isPathFound() const { return !path_.empty(); }
    bool hasReachedGoal() const;
    bool isObstacle(int x, int y) const;
    
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
    
    // DWA辅助函数
    double evaluateDWA(double vx, double vy, double dt);
    
    int width_;
    int height_;
    
    Point start_;
    Point goal_;
    Point current_;  // 当前位置（用于DWA）
    
    std::vector<Point> path_;  // 规划路径
    std::vector<std::shared_ptr<Obstacle>> obstacles_;  // 所有障碍物
};

} // namespace PathGlyph