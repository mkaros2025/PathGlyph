#pragma once
#include "../common/Types.h"
#include <memory>
#include <glm/glm.hpp>
#include "tile.h" // 引入Tile以使用其定义的常量和方法

namespace PathGlyph {

// 前向声明
class Maze;

// 统一的障碍物类（可以是静态或动态的）
class Obstacle {
public:
    // 将Maze类声明为友元，让其可以访问私有成员
    friend class Maze;
    
    // 静态障碍物构造
    Obstacle(double x, double y, double radius = 1.0) 
        : position_(x, 0.0, y), radius_(radius), isDynamic_(false),
          movementType_(MovementType::LINEAR) {}
        
    // 动态障碍物构造 - 通用构造函数
    Obstacle(double x, double y, MovementType type)
        : position_(x, 0.0, y), initialPosition_(x, 0.0, y),  
          radius_(1.0), isDynamic_(true), movementType_(type) {}
    
    // 设置线性运动参数
    void setLinearMovement(float speed, const Vector2D& direction) {
        if (movementType_ != MovementType::LINEAR) {
            return;  // 如果不是线性运动类型，忽略
        }
        
        speed_ = speed;
        direction_ = glm::vec3(direction.x, 0.0f, direction.y);
        direction_ = glm::normalize(direction_);  // 确保方向是单位向量
    }
    
    // 设置圆周运动参数
    void setCircularMovement(const Point& center, float radius, float angularSpeed) {
        if (movementType_ != MovementType::CIRCULAR) {
            return;  // 如果不是圆周运动类型，忽略
        }
        
        center_ = glm::vec3(center.x, 0.0f, center.y);
        orbitRadius_ = radius;
        angularSpeed_ = angularSpeed;
        
        // 计算初始角度
        glm::vec3 toObstacle = position_ - center_;
        angle_ = atan2(toObstacle.z, toObstacle.x);
    }
    
    ~Obstacle() = default;
    
    // 检查碰撞 - 3D版本
    bool intersects(int x, int y) const {
        // 创建一个tile并获取其包围盒
        Tile tile(x, y);
        Tile::BoundingBox tileBox = tile.getBoundingBox();
        
        // 球体与AABB包围盒的碰撞检测
        return sphereAABBIntersection(position_, radius_, tileBox);
    }
    
    // 检查与点的碰撞（用于Agent碰撞检测）
    bool intersects(const Point& point, float agentRadius = 0.5f) const {
        glm::vec3 pointPos(point.x, 0.0f, point.y);
        float distance = glm::distance(position_, pointPos);
        return distance < (radius_ + agentRadius);
    }
    
    // 球体与AABB包围盒的碰撞检测
    static bool sphereAABBIntersection(const glm::vec3& sphereCenter, float sphereRadius, 
                                      const Tile::BoundingBox& box) {
        // 计算AABB到球体中心的最近点
        glm::vec3 closest(
            glm::clamp(sphereCenter.x, box.min.x, box.max.x),
            glm::clamp(sphereCenter.y, box.min.y, box.max.y),
            glm::clamp(sphereCenter.z, box.min.z, box.max.z)
        );
        
        // 计算最近点到球体中心的距离
        float distance = glm::length(closest - sphereCenter);
        
        // 如果距离小于球体半径，则有碰撞
        return distance < sphereRadius;
    }
    
    // 更新位置
    void update(float deltaTime) {
        if (!isDynamic_) {
            return;  // 静态障碍物不更新
        }
        
        if (movementType_ == MovementType::LINEAR) {
            updateLinearMovement(deltaTime);
        } else if (movementType_ == MovementType::CIRCULAR) {
            updateCircularMovement(deltaTime);
        }
    }
    
    // 重置到初始位置
    void reset() {
        position_ = initialPosition_;
        
        if (movementType_ == MovementType::CIRCULAR) {
            // 圆周运动需要重置角度
            angle_ = 0.0f;
            updatePosition();
        }
    }

    // 获取世界坐标位置
    glm::vec3 getWorldPosition() const { return position_; }
    
    // 更新逻辑坐标到RealPoint（用于兼容之前的代码）
    RealPoint getLogicalPosition() const { 
        return RealPoint(position_.x, position_.z); 
    }
    
    // 获取中心点位置（用于圆周运动）
    Point getCenterPoint() const { 
        return Point(static_cast<int>(center_.x), static_cast<int>(center_.z)); 
    }
    
    // 获取当前位置的离散网格坐标
    Point getGridPosition() const {
        return Point(static_cast<int>(position_.x), static_cast<int>(position_.z));
    }
    
    bool isDynamic() const { return isDynamic_; }
    double getRadius() const { return radius_; }
    MovementType getMovementType() const { return movementType_; }
    
    // 获取碰撞预测位置 - 在指定时间后障碍物的预期位置
    glm::vec3 getPredictedPosition(float predictionTime) const {
        if (!isDynamic_) {
            return position_;  // 静态障碍物位置不变
        }
        
        glm::vec3 predictedPos = position_;
        
        if (movementType_ == MovementType::LINEAR) {
            predictedPos += direction_ * speed_ * predictionTime;
        } else if (movementType_ == MovementType::CIRCULAR) {
            float futureAngle = angle_ + angularSpeed_ * predictionTime;
            predictedPos.x = center_.x + orbitRadius_ * std::cos(futureAngle);
            predictedPos.z = center_.z + orbitRadius_ * std::sin(futureAngle);
        }
        
        return predictedPos;
    }
    
private:
    // 线性运动更新
    void updateLinearMovement(float deltaTime) {
        // 计算新位置
        glm::vec3 newPosition = position_ + direction_ * speed_ * deltaTime;
        
        // 检查是否碰到边界
        bool boundaryHit = false;
        
        // 简单边界检查: 0-width/height
        if (newPosition.x < 0.0f || newPosition.x >= 50.0f) {  // 假设地图宽50
            direction_.x = -direction_.x;
            boundaryHit = true;
        }
        
        if (newPosition.z < 0.0f || newPosition.z >= 50.0f) {  // 假设地图高50
            direction_.z = -direction_.z;
            boundaryHit = true;
        }
        
        // 如果碰到边界，调整位置
        if (boundaryHit) {
            position_ += direction_ * speed_ * deltaTime;
        } else {
            position_ = newPosition;
        }
    }
    
    // 圆周运动更新
    void updateCircularMovement(float deltaTime) {
        angle_ += angularSpeed_ * deltaTime;
        updatePosition();
    }
    
    // 更新位置助手方法 (圆周运动)
    void updatePosition() {
        position_.x = center_.x + orbitRadius_ * std::cos(angle_);
        position_.z = center_.z + orbitRadius_ * std::sin(angle_);
        // Y坐标保持不变（高度不变）
    }

    glm::vec3 initialPosition_; // 初始位置（用于重置）
    glm::vec3 center_;     // 圆周运动的中心（仅用于动态障碍物）
    glm::vec3 position_;   // 当前3D位置
    glm::vec3 direction_;  // 线性运动方向
    
    double orbitRadius_ = 0.0; // 轨道半径（仅用于圆周动态障碍物）
    double radius_;        // 障碍物半径
    float speed_ = 3.0f;   // 移动速度 (线性) 或 角速度（圆周）
    float angularSpeed_ = 1.0f; // 角速度（仅用于圆周动态障碍物）
    float angle_ = 0.0f;   // 当前角度（仅用于圆周动态障碍物）
    bool isDynamic_;       // 是否为动态障碍物
    MovementType movementType_; // 移动类型（线性或圆周）
};

} // namespace PathGlyph