#include "obstacle.h"
#include "maze.h"
#include <cmath>

namespace PathGlyph {

StaticObstacle::StaticObstacle(Point pos, int width, int height)
    : width_(width), height_(height)
{
    Point intPos = pos.toInt();
    position_ = glm::vec3(intPos.x, 0.0, intPos.y);
}

bool StaticObstacle::intersects(const Point& pos) const {
    Point gridPos = getGridPosition();
    return gridPos == pos;
}

bool StaticObstacle::intersects(const Point& point, float agentRadius) const {
    // 计算点到障碍物中心的距离
    Point logicalPos = getLogicalPosition();
    
    // 判断点是否在障碍物半径+代理半径范围内
    float collisionRadius = 0.5f; // 静态障碍物默认半径为0.5（半个网格单元）
    float totalRadius = collisionRadius + agentRadius;
    
    return logicalPos.distanceTo(point) < totalRadius;
}

glm::vec3 StaticObstacle::getWorldPosition() const {
    return position_;
}

Point StaticObstacle::getLogicalPosition() const {
    return Point(position_.x, position_.z);
}

Point StaticObstacle::getGridPosition() const {
    Point logicalPos = getLogicalPosition();
    return logicalPos.toInt();
}

// --- DynamicObstacle 实现 ---

// 线性运动障碍物构造函数实现
DynamicObstacle::DynamicObstacle(Point pos, float speed, glm::vec2 direction, int width, int height)
    : StaticObstacle(pos, width, height), // 将宽高传递给基类构造函数
      movementType_(MovementType::LINEAR), speed_(speed), direction_(direction) {
    
    initialPosition_ = position_;
    directionVec_ = glm::vec3(direction_.x, 0.0f, direction_.y);
}

// 圆周运动障碍物构造函数实现
DynamicObstacle::DynamicObstacle(Point pos, Point center, float radius, float angularSpeed, int width, int height)
    : StaticObstacle(pos, width, height), // 将宽高传递给基类构造函数
      movementType_(MovementType::CIRCULAR), 
      center_(center), radius_(radius), angularSpeed_(angularSpeed) {
    
    initialPosition_ = position_;
    centerVec_ = glm::vec3(center_.x, 0.0f, center_.y);
    
    float dx = position_.x - centerVec_.x;
    float dz = position_.z - centerVec_.z;
    angle_ = std::atan2(dz, dx);
}

void DynamicObstacle::reset() {
    position_ = initialPosition_;
    angle_ = 0.0f;
}

void DynamicObstacle::update(float deltaTime) {
    if (movementType_ == MovementType::LINEAR) {
        updateLinearMovement(deltaTime);
    } else if (movementType_ == MovementType::CIRCULAR) {
        updateCircularMovement(deltaTime);
    }
}

void DynamicObstacle::updateLinearMovement(float deltaTime) {
    // 先预测下一步位置
    glm::vec3 nextPos = getPredictedPosition(deltaTime);
    Point nextGridPos = Point(nextPos.x, nextPos.z).toInt();
    
    // 检查预测位置是否超出边界
    const int WIDTH = width_;  // 使用存储的宽度
    const int HEIGHT = height_; // 使用存储的高度
    bool needsReverse = false;
    
    if (nextGridPos.x < 0 || nextGridPos.x >= WIDTH) {
        directionVec_.x = -directionVec_.x;
        needsReverse = true;
    }
    
    if (nextGridPos.y < 0 || nextGridPos.y >= HEIGHT) {
        directionVec_.z = -directionVec_.z;
        needsReverse = true;
    }
    
    if (needsReverse) {
        direction_ = glm::vec2(directionVec_.x, directionVec_.z);
        float distance = speed_ * deltaTime;
        position_ += directionVec_ * distance;

        return;
    }
    
    position_ = nextPos;
}

// 更新圆周运动
void DynamicObstacle::updateCircularMovement(float deltaTime) {
    // 预测下一个位置
    glm::vec3 predictedPos = getPredictedPosition(deltaTime);
    Point predictedGridPos = Point(predictedPos.x, predictedPos.z).toInt();
    
    const int WIDTH = width_;  // 使用存储的宽度
    const int HEIGHT = height_; // 使用存储的高度
    
    if (predictedGridPos.x < 0 || predictedGridPos.x >= WIDTH || 
        predictedGridPos.y < 0 || predictedGridPos.y >= HEIGHT) {
        // 提前反转角速度
        angularSpeed_ = -angularSpeed_;
            
        // 实际更新位置
        angle_ += angularSpeed_ * deltaTime;
        while (angle_ >= 2 * M_PI) angle_ -= 2 * M_PI;
        while (angle_ < 0) angle_ += 2 * M_PI;
        
        // 计算新位置
        float x = centerVec_.x + radius_ * std::cos(angle_);
        float z = centerVec_.z + radius_ * std::sin(angle_);
        position_ = glm::vec3(x, 0.0f, z);

        return;
    }

    position_ = predictedPos;
}

// 获取预测位置
glm::vec3 DynamicObstacle::getPredictedPosition(float predictionTime) const {
    if (movementType_ == MovementType::LINEAR) {
        return position_ + directionVec_ * speed_ * predictionTime;
    } else { // CIRCULAR
        float predictedAngle = angle_ + angularSpeed_ * predictionTime;

        float x = centerVec_.x + radius_ * std::cos(predictedAngle);
        float z = centerVec_.z + radius_ * std::sin(predictedAngle);
        
        return glm::vec3(x, 0.0f, z);
    }
}

Point DynamicObstacle::getCenterPoint() const {
    return center_;
}

double DynamicObstacle::getOrbitRadius() const {
    return radius_;
}

MovementType DynamicObstacle::getMovementType() const {
    return movementType_;
}

} // namespace PathGlyph
