#include "maze.h"
#include <fstream>
#include <algorithm>
#include <cmath>
#include <limits>
#include <iostream>
#include <nlohmann/json.hpp>
#include <glm/glm.hpp>
#include <random>

using json = nlohmann::json;

namespace PathGlyph {

Maze::Maze(int width, int height)
    : width_(width), height_(height), 
      start_(0, 0), goal_(width-1, height-1), current_(0, 0) {
}

Maze::~Maze() {
    // 智能指针会自动处理内存清理
}

bool Maze::loadFromJson(const std::string& filename) {
    try {
        // 打开并解析JSON文件
        std::ifstream file(filename);
        if (!file.is_open()) {
            return false;
        }
        
        json data = json::parse(file);
        
        // 读取地图尺寸
        if (data.contains("width") && data.contains("height")) {
            width_ = data["width"];
            height_ = data["height"];
        }
        
        // 读取起点和终点
        if (data.contains("start")) {
            start_.x = data["start"][0];
            start_.y = data["start"][1];
            current_ = start_; // 初始化当前位置为起点
        }
        
        if (data.contains("goal")) {
            goal_.x = data["goal"][0];
            goal_.y = data["goal"][1];
        }
        
        // 读取静态障碍物
        if (data.contains("static_obstacles")) {
            for (const auto& obs : data["static_obstacles"]) {
                double x = obs["x"];
                double y = obs["y"];
                Point position(x, y);
                addStaticObstacle(position);
            }
        }
        
        // 读取动态障碍物
        if (data.contains("dynamic_obstacles")) {
            for (const auto& obs : data["dynamic_obstacles"]) {
                double x = obs["x"];
                double y = obs["y"];
                Point position(x, y);
                MovementType moveType = obs["movement_type"].get<std::string>() == "linear" ? 
                                      MovementType::LINEAR : MovementType::CIRCULAR;
                
                if (moveType == MovementType::LINEAR) {
                    float speed = obs["speed"];
                    glm::vec2 direction(obs["direction"][0], obs["direction"][1]);
                    addDynamicObstacle(position, speed, direction);
                } else {
                    Point center(obs["center"][0], obs["center"][1]);
                    float radius = obs["radius"];
                    float angularSpeed = obs["angular_speed"];
                    addDynamicObstacle(position, center, radius, angularSpeed);
                }
            }
        }
        return true;
    } catch (const std::exception& e) {
        std::cerr << "JSON parsing error: " << e.what() << std::endl;
        return false;
    }
}

void Maze::setStart(const Point& position) {
    if (isInBounds(position) && !isStaticObstacle(position) && !isDynamicObstacle(position)) {
        start_ = position;
        current_ = start_; // 重置当前位置
        path_.clear(); // 清除现有路径
    }
}

void Maze::setGoal(const Point& position) {
    if (isInBounds(position) && !isStaticObstacle(position) && !isDynamicObstacle(position)) {
        goal_ = position;
        path_.clear(); // 清除现有路径
    }
}

void Maze::setPath(const std::vector<Point>& path) {
    path_ = path;
}

void Maze::clearStaticObstacles() {
    staticObstacles_.clear();
}

void Maze::clearDynamicObstacles() {
    dynamicObstacles_.clear();
}

void Maze::reset() {
    for (auto& obstacle : dynamicObstacles_) {
        obstacle->reset();
    }
    current_ = start_;
}

// 路径搜索 - 使用网格坐标系统进行规划
std::vector<Point> Maze::findPathAStar() {
    // 清除现有路径
    path_.clear();
    
    // 定义方向数组（8个方向：上、右、下、左及四个对角线）
    const int dx[] = {-1, -1, 0, 1, 1, 1, 0, -1};
    const int dy[] = {0, 1, 1, 1, 0, -1, -1, -1};
    
    // 重新定义比较函数，用于std::shared_ptr的比较
    auto compare = [](const std::shared_ptr<AStarNode>& a, const std::shared_ptr<AStarNode>& b) {
        return a->f > b->f;
    };
    
    // 创建优先队列，全部使用智能指针
    std::priority_queue<std::shared_ptr<AStarNode>, 
                         std::vector<std::shared_ptr<AStarNode>>, 
                         decltype(compare)> openSet(compare);
    
    // 创建访问标记数组
    std::vector<std::vector<bool>> visited(width_, std::vector<bool>(height_, false));
    
    // 创建起始节点
    int startX = static_cast<int>(current_.x);
    int startY = static_cast<int>(current_.y);
    int goalX = static_cast<int>(goal_.x);
    int goalY = static_cast<int>(goal_.y);
    
    auto startNode = std::make_shared<AStarNode>(
        startX, startY, 
        0, 
        heuristic(startX, startY, goalX, goalY),
        nullptr // 起始节点没有父节点
    );
    openSet.push(startNode);
    
    bool pathFound = false;
    
    // A*主循环
    while (!openSet.empty()) {
        // 获取代价最小的节点
        auto current = openSet.top();
        openSet.pop();
        
        // 检查是否到达目标
        if (current->x == goalX && current->y == goalY) {
            reconstructPath(current);  // 使用智能指针
            pathFound = true;
            break;
        }
        
        // 标记为已访问
        visited[current->x][current->y] = true;
        
        // 遍历所有可能的移动方向
        for (int i = 0; i < 8; ++i) {
            int newX = current->x + dx[i];
            int newY = current->y + dy[i];
            
            // 检查新位置是否有效且安全
            if (isValid(newX, newY) && isSafe(newX, newY) && !visited[newX][newY]) {
                // 计算移动代价（对角线移动代价为1.414，垂直/水平移动代价为1.0）
                double moveCost = (i % 2 == 0) ? 1.0 : 1.414;
                double newG = current->g + moveCost;
                double newH = heuristic(newX, newY, goalX, goalY);
                
                // 创建新节点，直接使用智能指针
                auto neighbor = std::make_shared<AStarNode>(newX, newY, newG, newH, current);
                
                // 添加到开集
                openSet.push(neighbor);
                visited[newX][newY] = true;  // 提前标记以避免重复添加
            }
        }
    }
    
    return path_; // 返回生成的路径
}

// 更新动态障碍物
void Maze::update(float deltaTime) {
    for (auto& obstacle : dynamicObstacles_) {
        obstacle->update(deltaTime);
    }
}

// 判断位置是否有静态障碍物
bool Maze::isStaticObstacle(const Point& position) const {
    // 检查所有静态障碍物
    for (const auto& obstacle : staticObstacles_) {
        if (position.distanceTo(obstacle->getGridPosition()) < 0.5) {
            return true;
        }
    }
    
    return false;
}

// 判断位置是否有动态障碍物
bool Maze::isDynamicObstacle(const Point& position) const {
    // 检查所有动态障碍物
    for (const auto& obstacle : dynamicObstacles_) {
        if (position.distanceTo(obstacle->getGridPosition()) < 0.5) {
            return true;
        }
    }
    
    return false;
}

// 检查是否到达目标
bool Maze::hasReachedGoal() const {
    return current_.distanceTo(goal_) < 0.5;
}

// 碰撞检测 - 检查点是否与任何障碍物碰撞
bool Maze::checkCollision(const Point& pos, float radius) const {
    // 检查是否与静态障碍物碰撞
    for (const auto& obstacle : staticObstacles_) {
        if (obstacle->intersects(pos, radius)) {
            return true;
        }
    }
    
    // 检查是否与动态障碍物碰撞
    for (const auto& obstacle : dynamicObstacles_) {
        if (obstacle->intersects(pos, radius)) {
            return true;
        }
    }
    
    return false;
}

// 判断位置是否在地图范围内
bool Maze::isValid(int x, int y) const {
    // 整数坐标表示网格中心
    return x >= 0 && x < width_ && y >= 0 && y < height_;
}

// 判断位置是否安全（无障碍物）
bool Maze::isSafe(int x, int y) const {
    // 整数坐标表示网格中心，直接作为Point使用
    return !isStaticObstacle(Point(x, y));
}

// 启发式函数 
double Maze::heuristic(int x1, int y1, int x2, int y2) const {
     return std::sqrt((x1 - x2) * (x1 - x2) + (y1 - y2) * (y1 - y2));
}

// 从A*节点重建路径
void Maze::reconstructPath(std::shared_ptr<AStarNode> node) {
    // 清除现有路径
    path_.clear();
    
    // 从目标节点回溯到起始节点
    while (node != nullptr) {
        // 转换网格坐标为逻辑坐标（网格中心）
        Point logicalPos = Point(node->x, node->y) ;
        path_.insert(path_.begin(), logicalPos);
        node = node->parent;
    }
}

// 添加静态障碍物
void Maze::addStaticObstacle(const Point& position) {
    if (!isInBounds(position) || isStaticObstacle(position) || isDynamicObstacle(position)) {
        return;
    }
    
    // 创建新的静态障碍物
    auto obstacle = std::make_shared<StaticObstacle>(position, width_, height_);
    staticObstacles_.push_back(obstacle);
    
    // 清除现有路径（因为可能被新障碍物阻断）
    path_.clear();
}

// 添加线性运动的动态障碍物
void Maze::addDynamicObstacle(const Point& position, float speed, glm::vec2 direction) {
    // 检查是否已存在障碍物
    if (isStaticObstacle(position) || isDynamicObstacle(position)) {
        return; // 位置已占用
    }
    
    // 检查位置是否有效
    if (!isInBounds(position)) {
        return; // 位置超出边界
    }
    
    // 创建新的动态障碍物(线性运动)
    auto obstacle = std::make_shared<DynamicObstacle>(position, speed, direction, width_, height_);
    dynamicObstacles_.push_back(obstacle);
    
    // 清除现有路径（因为可能被新障碍物阻断）
    path_.clear();
}

// 添加圆周运动的动态障碍物
void Maze::addDynamicObstacle(const Point& position, Point center, float radius, float angularSpeed) {
    // 检查是否已存在障碍物
    if (isStaticObstacle(position) || isDynamicObstacle(position)) {
        return; // 位置已占用
    }
    
    // 检查位置是否有效
    if (!isInBounds(position)) {
        return; // 位置超出边界
    }
    
    // 创建新的动态障碍物(圆周运动)
    auto obstacle = std::make_shared<DynamicObstacle>(position, center, radius, angularSpeed, width_, height_);
    dynamicObstacles_.push_back(obstacle);
    
    // 清除现有路径（因为可能被新障碍物阻断）
    path_.clear();
}

// 移除障碍物
void Maze::removeObstacle(const Point& position, double tolerance) {
    // 移除静态障碍物
    for (auto it = staticObstacles_.begin(); it != staticObstacles_.end();) {
        Point obstaclePos = (*it)->getLogicalPosition();
        float dx = position.x - obstaclePos.x;
        float dy = position.y - obstaclePos.y;
        float distSq = dx * dx + dy * dy;
        
        if (distSq <= tolerance * tolerance) {
            it = staticObstacles_.erase(it);
        } else {
            ++it;
        }
    }
    
    // 移除动态障碍物
    for (auto it = dynamicObstacles_.begin(); it != dynamicObstacles_.end();) {
        Point obstaclePos = (*it)->getLogicalPosition();
        float dx = position.x - obstaclePos.x;
        float dy = position.y - obstaclePos.y;
        float distSq = dx * dx + dy * dy;
        
        if (distSq <= tolerance * tolerance) {
            it = dynamicObstacles_.erase(it);
        } else {
            ++it;
        }
    }
    
    // 清除现有路径（因为可能需要重新计算）
    path_.clear();
}

// 世界坐标转逻辑坐标
Point Maze::worldToLogical(const glm::vec3& worldPos) const {
    return Point(worldPos.x, worldPos.z);
}

// DWA局部规划
glm::vec2 Maze::findBestLocalVelocity(const Point& currentPos, const glm::vec2& currentVel, 
                                    const Point& targetPos, float maxSpeed, float maxRotSpeed) {
    // 生成速度空间采样
    const int VELOCITY_SAMPLES = 20;  // 速度采样数量
    const float PREDICTION_TIME = 2.0f; // 轨迹预测时间(秒)
    
    std::vector<glm::vec2> velocitySamples;
    generateVelocitySamples(currentVel, maxSpeed, maxRotSpeed, VELOCITY_SAMPLES, velocitySamples);
    
    // 找到最佳速度
    float bestScore = -std::numeric_limits<float>::max();
    glm::vec2 bestVelocity = currentVel;
    
    for (const auto& velocity : velocitySamples) {
        float score = evaluateTrajectory(velocity, currentPos, targetPos, PREDICTION_TIME);
        
        if (score > bestScore) {
            bestScore = score;
            bestVelocity = velocity;
        }
    }
    
    return bestVelocity;
}

// 生成速度采样
void Maze::generateVelocitySamples(const glm::vec2& currentVel, float maxSpeed, float maxRotSpeed, 
                                 int samples, std::vector<glm::vec2>& velocitySamples) {
    // 使用随机数生成器来采样速度空间
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_real_distribution<> speedDist(0, maxSpeed);
    std::uniform_real_distribution<> angleDist(-maxRotSpeed, maxRotSpeed);
    
    // 清空速度样本容器
    velocitySamples.clear();
    
    // 添加当前速度
    velocitySamples.push_back(currentVel);
    
    // 计算当前速度的角度
    float currentAngle = atan2(currentVel.y, currentVel.x);
    float currentSpeed = glm::length(currentVel);
    
    // 生成速度样本
    for (int i = 0; i < samples; ++i) {
        // 随机调整当前速度和方向
        float speedAdjustment = speedDist(gen);
        float angleAdjustment = angleDist(gen);
        
        // 计算新速度和角度
        float speed = std::min(currentSpeed + speedAdjustment, maxSpeed);
        speed = std::max(0.1f, speed); // 确保速度不会太小
        float newAngle = currentAngle + angleAdjustment;
        
        // 创建新的速度向量
        glm::vec2 newVel(speed * cos(newAngle), speed * sin(newAngle));
        velocitySamples.push_back(newVel);
    }
}

// 评估轨迹得分
float Maze::evaluateTrajectory(const glm::vec2& velocity, const Point& currentPos, 
                             const Point& targetPos, float predictTime) {
    // 距离目标的接近程度、避障能力、与目标方向的一致性
    
    // 1. 模拟轨迹，计算终点位置
    float x = currentPos.x;
    float y = currentPos.y;
    
    // 简单欧拉积分计算终点
    x += velocity.x * predictTime;
    y += velocity.y * predictTime;
    
    Point endPos(x, y);
    
    // 边界检查 - 如果终点超出边界，给予极低的评分
    if (!isInBounds(endPos)) {
        return -2000; // 比障碍物碰撞更低的分数
    }
    
    // 收集轨迹上的点（用于检查碰撞）
    std::vector<Point> trajectory;
    const int steps = 10; // 轨迹分段数
    
    for (int i = 0; i <= steps; ++i) {
        float t = static_cast<float>(i) / steps * predictTime;
        float px = currentPos.x + velocity.x * t;
        float py = currentPos.y + velocity.y * t;
        Point predictedPos(px, py);
        
        // 对轨迹上的每个点也进行边界检查
        if (!isInBounds(predictedPos)) {
            return -2000;
        }
        
        trajectory.push_back(predictedPos);
    }
    
    // 2. 计算各种得分
    float obstacleScore = calculateObstacleAvoidanceScore(trajectory);
    float directionScore = calculateGoalDirectionScore(velocity, currentPos, targetPos);
    float distanceScore = calculateGoalDistanceScore(endPos, targetPos);
    
    // 如果轨迹会导致碰撞，给予极低的评分
    if (obstacleScore < 0) {
        return -1000;
    }
    
    // 3. 综合评分（权重可调整）
    float finalScore = obstacleScore * 0.4f + directionScore * 0.3f + distanceScore * 0.3f;
    
    return finalScore;
}

// 计算轨迹的避障得分
float Maze::calculateObstacleAvoidanceScore(const std::vector<Point>& trajectory) {
    // 判断轨迹上的点是否与当前障碍物碰撞
    for (const auto& point : trajectory) {
        if (checkCollision(point)) {
            return -1000; // 如果发生碰撞，给予极低的得分
        }
    }
    
    // 计算最小距离
    float minDistance = std::numeric_limits<float>::max();
    const Point& endPoint = trajectory.back();
    
    // 检查静态障碍物
    for (const auto& obstacle : staticObstacles_) {
        Point obstaclePos = obstacle->getLogicalPosition();
        float dx = endPoint.x - obstaclePos.x;
        float dy = endPoint.y - obstaclePos.y;
        float distSq = dx * dx + dy * dy;
        minDistance = std::min(minDistance, static_cast<float>(sqrt(distSq)));
    }
    
    // 检查动态障碍物的未来位置
    const float PREDICTION_TIME = 2.0f; // 同evaluateTrajectory中的预测时间保持一致
    
    for (const auto& obstacle : dynamicObstacles_) {
        // 获取障碍物在未来时间的预测位置
        glm::vec3 futurePos = obstacle->getPredictedPosition(PREDICTION_TIME);
        Point futurePosPoint(futurePos.x, futurePos.z);
        
        float dx = endPoint.x - futurePosPoint.x;
        float dy = endPoint.y - futurePosPoint.y;
        float distSq = dx * dx + dy * dy;
        minDistance = std::min(minDistance, static_cast<float>(sqrt(distSq)));
    }
    
    return minDistance;
}

// 计算轨迹的方向得分
float Maze::calculateGoalDirectionScore(const glm::vec2& velocity, const Point& currentPos, 
                                      const Point& targetPos) {
    // 计算到目标的方向向量
    glm::vec2 toGoal(targetPos.x - currentPos.x, targetPos.y - currentPos.y);
    float goalDist = glm::length(toGoal);
    
    if (goalDist < 0.001f) {
        return 1.0f; // 已经非常接近目标
    }
    
    // 归一化向量
    toGoal = glm::normalize(toGoal);
    glm::vec2 normVelocity = glm::normalize(velocity);
    
    // 计算方向相似度（点积）
    float dotProduct = glm::dot(toGoal, normVelocity);
    
    // 转换为[0,1]范围的得分
    return (dotProduct + 1.0f) / 2.0f;
}

// 计算轨迹的距离得分
float Maze::calculateGoalDistanceScore(const Point& endPos, const Point& targetPos) {
    // 计算轨迹终点到目标的距离
    float dx = endPos.x - targetPos.x;
    float dy = endPos.y - targetPos.y;
    float distance = sqrt(dx * dx + dy * dy);
    
    // 使用指数衰减函数计算得分，距离越近得分越高
    return exp(-distance / 10.0f); // 调整常数以控制衰减速率
}

} // namespace PathGlyph