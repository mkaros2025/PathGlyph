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
                double radius = obs.contains("radius") ? static_cast<double>(obs["radius"]) : 1.0;
                
                addObstacle(x, y, radius);
            }
        }
        
        // 读取动态障碍物
        if (data.contains("dynamic_obstacles")) {
            for (const auto& obs : data["dynamic_obstacles"]) {
                double x = obs["x"];
                double y = obs["y"];
                MovementType moveType = obs["movement_type"].get<std::string>() == "linear" ? 
                                      MovementType::LINEAR : MovementType::CIRCULAR;
                
                ObstacleProperties props;
                props.movementType = moveType;
                
                if (moveType == MovementType::LINEAR) {
                    props.speed = obs["speed"];
                    props.direction = Vector2D(obs["direction"][0], obs["direction"][1]);
                } else {
                    props.center = Point(obs["center"][0], obs["center"][1]);
                    props.radius = obs["radius"];
                    props.angularSpeed = obs["angular_speed"];
                }
                
                addDynamicObstacle(x, y, moveType, props);
            }
        }
        
        // 尝试找到路径
        path_.clear();
        findPathAStar();
        
        return true;
    } catch (const std::exception& e) {
        // 处理JSON解析错误
        std::cerr << "JSON parsing error: " << e.what() << std::endl;
        return false;
    }
}

void Maze::setStart(int x, int y) {
    if (isValid(x, y) && !isObstacle(x, y)) {
        start_ = Point(x, y);
        current_ = start_; // 重置当前位置
        path_.clear(); // 清除现有路径
    }
}

void Maze::setGoal(int x, int y) {
    if (isValid(x, y) && !isObstacle(x, y)) {
        goal_ = Point(x, y);
        path_.clear(); // 清除现有路径
    }
}

void Maze::setPath(const std::vector<Point>& path) {
    path_ = path;
    updateWorldPath();
}

void Maze::clearObstacles() {
    obstacles_.clear();
    dynamicObstacles_.clear();
    obstacleProperties_.clear();
    path_.clear(); // 清除现有路径
}

void Maze::clearDynamicObstacles() {
    dynamicObstacles_.clear();
    // 清理障碍物属性中关联到动态障碍物的条目
    for (auto it = obstacleProperties_.begin(); it != obstacleProperties_.end();) {
        if (it->first->isDynamic()) {
            it = obstacleProperties_.erase(it);
        } else {
            ++it;
        }
    }
}

void Maze::resetObstacles() {
    // 重置所有动态障碍物到初始位置
    for (auto& obstacle : dynamicObstacles_) {
        obstacle->reset();
    }
}

// 路径搜索
std::vector<Point> Maze::findPathAStar() {
    // 清除现有路径
    path_.clear();
    
    // 定义方向数组（8个方向：上、右、下、左及四个对角线）
    const int dx[] = {-1, -1, 0, 1, 1, 1, 0, -1};
    const int dy[] = {0, 1, 1, 1, 0, -1, -1, -1};
    
    // 创建优先队列
    std::priority_queue<AStarNode*, std::vector<AStarNode*>, std::greater<AStarNode*>> openSet;
    
    // 创建访问标记数组
    std::vector<std::vector<bool>> visited(width_, std::vector<bool>(height_, false));
    
    // 创建起始节点
    AStarNode* startNode = new AStarNode(start_.x, start_.y, 0, heuristic(start_.x, start_.y, goal_.x, goal_.y));
    openSet.push(startNode);
    
    // 跟踪所有创建的节点以便清理内存
    std::vector<AStarNode*> allNodes;
    allNodes.push_back(startNode);
    
    bool pathFound = false;
    
    // A*主循环
    while (!openSet.empty()) {
        // 获取代价最小的节点
        AStarNode* current = openSet.top();
        openSet.pop();
        
        // 检查是否到达目标
        if (current->x == goal_.x && current->y == goal_.y) {
            reconstructPath(current);
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
                double newH = heuristic(newX, newY, goal_.x, goal_.y);
                
                // 创建新节点
                AStarNode* neighbor = new AStarNode(newX, newY, newG, newH, current);
                allNodes.push_back(neighbor);
                
                // 添加到开集
                openSet.push(neighbor);
                visited[newX][newY] = true;  // 提前标记以避免重复添加
            }
        }
    }
    
    // 更新世界坐标路径
    if (pathFound) {
        updateWorldPath();
    }
    
    // 清理内存
    for (auto node : allNodes) {
        delete node;
    }
    
    return path_; // 返回生成的路径
}

// DWA局部规划
Vector2D Maze::findBestLocalVelocity(const Point& currentPos, const Vector2D& currentVel, 
                                    const Point& targetPos, float maxSpeed, float maxRotSpeed) {
    // 生成速度空间采样
    const int VELOCITY_SAMPLES = 20;  // 速度采样数量
    const float PREDICTION_TIME = 2.0f; // 轨迹预测时间(秒)
    
    std::vector<Vector2D> velocitySamples;
    generateVelocitySamples(currentVel, maxSpeed, maxRotSpeed, VELOCITY_SAMPLES, velocitySamples);
    
    // 找到最佳速度
    float bestScore = -std::numeric_limits<float>::max();
    Vector2D bestVelocity = currentVel;
    
    for (const auto& velocity : velocitySamples) {
        float score = evaluateTrajectory(velocity, currentPos, targetPos, PREDICTION_TIME);
        
        if (score > bestScore) {
            bestScore = score;
            bestVelocity = velocity;
        }
    }
    
    return bestVelocity;
}

void Maze::generateVelocitySamples(const Vector2D& currentVel, float maxSpeed, float maxRotSpeed, 
                                 int samples, std::vector<Vector2D>& velocitySamples) {
    // 使用随机数生成器创建均匀分布的速度样本
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_real_distribution<float> speedDist(0.0f, maxSpeed);
    std::uniform_real_distribution<float> angleDist(-maxRotSpeed, maxRotSpeed);
    
    // 清空样本列表
    velocitySamples.clear();
    
    // 始终包含当前速度
    velocitySamples.push_back(currentVel);
    
    // 生成样本
    for (int i = 0; i < samples; ++i) {
        // 生成随机速度大小
        float speed = speedDist(gen);
        
        // 计算当前速度方向
        float currentAngle = 0.0f;
        if (currentVel.length() > 0.001f) {
            currentAngle = atan2(currentVel.y, currentVel.x);
        }
        
        // 生成随机角度变化
        float angleChange = angleDist(gen);
        float newAngle = currentAngle + angleChange;
        
        // 创建新速度向量
        Vector2D newVel(speed * cos(newAngle), speed * sin(newAngle));
        velocitySamples.push_back(newVel);
    }
}

float Maze::evaluateTrajectory(const Vector2D& velocity, const Point& currentPos, 
                             const Point& targetPos, float predictTime) {
    // 预测轨迹
    std::vector<Point> trajectory;
    const int STEPS = 10;  // 预测步数
    const float dt = predictTime / STEPS;
    
    float x = currentPos.x;
    float y = currentPos.y;
    
    for (int i = 0; i < STEPS; ++i) {
        x += velocity.x * dt;
        y += velocity.y * dt;
        
        Point predictedPos(static_cast<int>(x), static_cast<int>(y));
        trajectory.push_back(predictedPos);
        
        // 如果轨迹中有碰撞，则提前结束
        if (!isInBounds(predictedPos) || checkCollision(predictedPos)) {
            break;
        }
    }
    
    // 计算轨迹得分
    float obstacleScore = calculateObstacleAvoidanceScore(trajectory);
    float goalDirectionScore = calculateGoalDirectionScore(velocity, currentPos, targetPos);
    float goalDistanceScore = calculateGoalDistanceScore(trajectory.empty() ? currentPos : trajectory.back(), targetPos);
    
    // 组合得分（权重可以根据需要调整）
    float totalScore = 0.5f * obstacleScore + 0.3f * goalDirectionScore + 0.2f * goalDistanceScore;
    
    return totalScore;
}

float Maze::calculateObstacleAvoidanceScore(const std::vector<Point>& trajectory) {
    if (trajectory.empty()) {
        return 0.0f;  // 空轨迹，无法评估
    }
    
    // 初始得分为最大值
    float score = 1.0f;
    
    // 检查轨迹中是否有碰撞
    for (const auto& point : trajectory) {
        if (!isInBounds(point)) {
            // 轨迹超出边界，大幅度降低分数
            return 0.1f;
        }
        
        if (checkCollision(point)) {
            // 轨迹与障碍物碰撞，大幅度降低分数
            return 0.1f;
        }
        
        // 检查距离障碍物的最近距离
        float minDistance = std::numeric_limits<float>::max();
        
        // 检查与静态障碍物的距离
        for (const auto& obstacle : obstacles_) {
            Point obsPos = obstacle->getGridPosition();
            float distance = point.distanceTo(obsPos);
            minDistance = std::min(minDistance, static_cast<float>(distance));
        }
        
        // 检查与动态障碍物的距离
        for (const auto& obstacle : dynamicObstacles_) {
            Point obsPos = obstacle->getGridPosition();
            float distance = point.distanceTo(obsPos);
            minDistance = std::min(minDistance, static_cast<float>(distance));
        }
        
        // 如果非常接近障碍物，降低得分
        if (minDistance < 2.0f) {
            score = std::min(score, minDistance / 2.0f);
        }
    }
    
    return score;
}

float Maze::calculateGoalDirectionScore(const Vector2D& velocity, const Point& currentPos, 
                                       const Point& targetPos) {
    if (velocity.length() < 0.001f) {
        return 0.5f;  // 速度太小，中等分数
    }
    
    // 计算朝向目标的方向向量
    Vector2D toGoal(targetPos.x - currentPos.x, targetPos.y - currentPos.y);
    if (toGoal.length() < 0.001f) {
        return 1.0f;  // 已经在目标位置，最高分数
    }
    
    // 归一化方向向量
    Vector2D normalizedVelocity = velocity.normalize();
    Vector2D normalizedToGoal = toGoal.normalize();
    
    // 计算两个方向的点积（cos值）
    float cos = normalizedVelocity.dot(normalizedToGoal);
    
    // 将cos映射到0-1的得分范围（cos的范围为-1到1）
    return (cos + 1.0f) / 2.0f;
}

float Maze::calculateGoalDistanceScore(const Point& endPos, const Point& targetPos) {
    // 计算到目标的距离
    float distance = static_cast<float>(endPos.distanceTo(targetPos));
    
    // 距离越近，得分越高
    // 使用指数衰减函数将距离映射到0-1的分数范围
    return exp(-distance / 10.0f);  // 参数10控制衰减速率
}

void Maze::update(float deltaTime) {
    // 更新所有动态障碍物
    for (auto& obstacle : dynamicObstacles_) {
        obstacle->update(deltaTime);
    }
}

std::vector<std::shared_ptr<Obstacle>> Maze::getNearbyObstacles(const Point& center, float radius) const {
    std::vector<std::shared_ptr<Obstacle>> nearby;
    
    // 检查静态障碍物
    for (const auto& obstacle : obstacles_) {
        Point obsPos = obstacle->getGridPosition();
        if (center.distanceTo(obsPos) <= radius) {
            nearby.push_back(obstacle);
        }
    }
    
    // 检查动态障碍物
    for (const auto& obstacle : dynamicObstacles_) {
        Point obsPos = obstacle->getGridPosition();
        if (center.distanceTo(obsPos) <= radius) {
            nearby.push_back(obstacle);
        }
    }
    
    return nearby;
}

bool Maze::hasReachedGoal() const {
    return current_.x == goal_.x && current_.y == goal_.y;
}

bool Maze::isObstacle(int x, int y) const {
    // 检查是否有静态障碍物
    for (const auto& obstacle : obstacles_) {
        if (obstacle->intersects(x, y)) {
            return true;
        }
    }
    
    // 检查是否有动态障碍物
    for (const auto& obstacle : dynamicObstacles_) {
        if (obstacle->intersects(x, y)) {
            return true;
        }
    }
    
    return false;
}

bool Maze::isDynamicObstacle(int x, int y) const {
    // 检查是否有动态障碍物
    for (const auto& obstacle : dynamicObstacles_) {
        if (obstacle->intersects(x, y)) {
            return true;
        }
    }
    
    return false;
}

bool Maze::checkCollision(const Point& pos, float radius) const {
    // 检查与静态障碍物的碰撞
    for (const auto& obstacle : obstacles_) {
        if (obstacle->intersects(pos, radius)) {
            return true;
        }
    }
    
    // 检查与动态障碍物的碰撞
    for (const auto& obstacle : dynamicObstacles_) {
        if (obstacle->intersects(pos, radius)) {
            return true;
        }
    }
    
    return false;
}

bool Maze::isValid(int x, int y) const {
    return x >= 0 && x < width_ && y >= 0 && y < height_;
}

bool Maze::isSafe(int x, int y) const {
    return !isObstacle(x, y);
}

double Maze::heuristic(int x1, int y1, int x2, int y2) const {
    // 使用欧几里得距离作为启发式
    double dx = x1 - x2;
    double dy = y1 - y2;
    return std::sqrt(dx * dx + dy * dy);
}

void Maze::reconstructPath(AStarNode* node) {
    // 清除现有路径
    path_.clear();
    
    // 从目标节点回溯到起始节点，构建路径
    while (node) {
        path_.insert(path_.begin(), Point(node->x, node->y));
        node = node->parent;
    }
    
    // 更新世界坐标路径
    updateWorldPath();
}

void Maze::updateWorldPath() {
    worldPath_.clear();
    
    for (const auto& point : path_) {
        worldPath_.push_back(logicalToWorld(point));
    }
}

void Maze::addObstacle(int x, int y) {
    if (isValid(x, y) && !isObstacle(x, y)) {
        // 转换为世界坐标，网格坐标位于网格中心
        double worldX = x;
        double worldY = y;
        
        auto obstacle = std::make_shared<Obstacle>(worldX, worldY);
        obstacles_.push_back(obstacle);
    }
}

void Maze::addObstacle(double x, double y, double radius) {
    auto obstacle = std::make_shared<Obstacle>(x, y, radius);
    obstacles_.push_back(obstacle);
}

void Maze::addDynamicObstacle(int x, int y, MovementType type, const ObstacleProperties& props) {
    // 创建动态障碍物
    auto obstacle = std::make_shared<Obstacle>(x, y, type);
    
    // 根据类型设置运动参数
    if (type == MovementType::LINEAR) {
        obstacle->setLinearMovement(props.speed, props.direction);
    } else if (type == MovementType::CIRCULAR) {
        obstacle->setCircularMovement(props.center, props.radius, props.angularSpeed);
    }
    
    // 存储障碍物属性用于重置
    obstacleProperties_[obstacle] = props;
    
    // 添加到动态障碍物列表
    dynamicObstacles_.push_back(obstacle);
}

std::vector<Point> Maze::getDynamicObstaclePositions() const {
    std::vector<Point> positions;
    for (const auto& obstacle : dynamicObstacles_) {
        positions.push_back(obstacle->getGridPosition());
    }
    return positions;
}

std::vector<ObstacleProperties> Maze::getDynamicObstacleProperties() const {
    std::vector<ObstacleProperties> properties;
    for (const auto& obstacle : dynamicObstacles_) {
        auto it = obstacleProperties_.find(obstacle);
        if (it != obstacleProperties_.end()) {
            properties.push_back(it->second);
        }
    }
    return properties;
}

void Maze::removeObstacle(int x, int y) {
    // 移除指定位置的障碍物
    obstacles_.erase(
        std::remove_if(obstacles_.begin(), obstacles_.end(),
            [x, y](const std::shared_ptr<Obstacle>& obstacle) {
                return obstacle->intersects(x, y);
            }),
        obstacles_.end()
    );
    
    // 移除指定位置的动态障碍物
    auto it = std::remove_if(dynamicObstacles_.begin(), dynamicObstacles_.end(),
        [x, y, this](const std::shared_ptr<Obstacle>& obstacle) {
            // 如果障碍物与指定位置相交
            if (obstacle->intersects(x, y)) {
                // 从属性映射中也移除
                obstacleProperties_.erase(obstacle);
                return true;
            }
            return false;
        });
    
    // 执行移除
    dynamicObstacles_.erase(it, dynamicObstacles_.end());
}

} // namespace PathGlyph