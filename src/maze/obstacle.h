#pragma once
#include <cmath>
#include <memory>

namespace PathGlyph {

// 前向声明
class Maze;

// 连续坐标点
struct RealPoint {
    double x;
    double y;
    
    RealPoint() : x(0.0), y(0.0) {}
    RealPoint(double x_, double y_) : x(x_), y(y_) {}
    
    // 简化的距离计算
    double distanceTo(const RealPoint& other) const {
        double dx = x - other.x;
        double dy = y - other.y;
        return std::sqrt(dx*dx + dy*dy);
    }
};

// 统一的障碍物类（可以是静态或动态的）
class Obstacle {
public:
    // 将Maze类声明为友元，让其可以访问私有成员
    friend class Maze;
    
    // 静态障碍物构造
    Obstacle(double x, double y, double radius = 1.0) 
        : position_(x, y), radius_(radius), isDynamic_(false) {}
        
    // 动态障碍物构造（圆周运动）
    Obstacle(double centerX, double centerY, double orbitRadius, double speed, double startAngle = 0.0)
        : center_(centerX, centerY), position_(0, 0), 
          orbitRadius_(orbitRadius), radius_(1.0), 
          speed_(speed), angle_(startAngle), isDynamic_(true) {
        // 初始化位置
        position_.x = center_.x + orbitRadius_ * std::cos(angle_);
        position_.y = center_.y + orbitRadius_ * std::sin(angle_);
    }
    
    ~Obstacle() = default;
    
    // 检查碰撞
    bool intersects(int x, int y) const {
        RealPoint cellCenter(x + 0.5, y + 0.5);
        return position_.distanceTo(cellCenter) < radius_;
    }
    
    // 更新位置（对静态障碍物无效）
    void update(double dt) {
        if (isDynamic_) {
            angle_ += speed_ * dt;
            position_.x = center_.x + orbitRadius_ * std::cos(angle_);
            position_.y = center_.y + orbitRadius_ * std::sin(angle_);
        }
    }

    RealPoint getPosition() const { return position_; }
    bool isDynamic() const { return isDynamic_; }
    double getRadius() const { return radius_; }
    
private:
    RealPoint center_;     // 圆周运动的中心（仅用于动态障碍物）
    RealPoint position_;   // 当前位置
    double orbitRadius_ = 0.0; // 轨道半径（仅用于动态障碍物）
    double radius_;        // 障碍物半径
    double speed_ = 0.0;   // 角速度（仅用于动态障碍物）
    double angle_ = 0.0;   // 当前角度（仅用于动态障碍物）
    bool isDynamic_;       // 是否为动态障碍物
};

} // namespace PathGlyph