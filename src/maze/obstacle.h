#pragma once
#include "common/types.h"
#include <memory>
#include <glm/glm.hpp>
#include "geometry/tileManager.h"

namespace PathGlyph {

class Maze;

// 静态障碍物类
class StaticObstacle {
public:
    friend class Maze;

    StaticObstacle(Point pos, int width, int height);
    virtual ~StaticObstacle() = default;
    
    bool intersects(const Point& gridPosition) const;
    bool intersects(const Point& point, float agentRadius = 0.5f) const;
    
    // 获取世界坐标位置
    glm::vec3 getWorldPosition() const;
    // 获取逻辑坐标位置
    Point getLogicalPosition() const;
    // 获取当前位置的离散网格坐标
    Point getGridPosition() const;
    
protected:
    glm::vec3 position_;        // 固定位置
    int width_;                 // 迷宫宽度
    int height_;                // 迷宫高度
};

enum class MovementType {
    LINEAR,     
    CIRCULAR
};

// 动态障碍物类
class DynamicObstacle : public StaticObstacle {
public:
    // 线性运动障碍物构造函数
    DynamicObstacle(Point pos, float speed, glm::vec2 direction, int width, int height);
    // 圆周运动障碍物构造函数
    DynamicObstacle(Point pos, Point center, float radius, float angularSpeed, int width, int height);

    // 重置到初始位置
    void reset();
    // 更新位置 - 仅动态障碍物需要
    void update(float deltaTime);

    // 获取中心点位置（用于圆周运动）
    Point getCenterPoint() const;
    // 获取轨道半径
    double getOrbitRadius() const;

    // 获取运动类型
    MovementType getMovementType() const;
    
    // 获取碰撞预测位置
    glm::vec3 getPredictedPosition(float predictionTime) const;
    
private:
    void updateLinearMovement(float deltaTime);
    void updateCircularMovement(float deltaTime);

    glm::vec3 initialPosition_; // 初始位置（用于重置）
    
    // 障碍物属性
    MovementType movementType_ = MovementType::LINEAR;   // 运动类型
    float speed_ = 3.0f;                                // 线性移动速度
    glm::vec2 direction_ = glm::vec2(1.0f, 0.0f);         // 线性移动方向
    Point center_ = Point(0, 0);                        // 圆周运动中心
    float radius_ = 5.0f;                               // 圆周运动半径
    float angularSpeed_ = 1.0f;                         // 角速度(弧度/秒)
    
    // 运动状态变量
    glm::vec3 directionVec_;       // 线性运动方向的3D表示
    glm::vec3 centerVec_;          // 圆周运动中心的3D表示
    float angle_ = 0.0f;           // 当前角度
    double collisionRadius_ = 0.5; // 碰撞半径
};

} // namespace PathGlyph