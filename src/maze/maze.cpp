#include "maze.h"
#include <fstream>
#include <algorithm>
#include <cmath>
#include <limits>
#include <nlohmann/json.hpp>

using json = nlohmann::json;

namespace PathGlyph {

Maze::Maze() // 初始化默认值
  : width_(10), height_(10), start_{0, 0}, goal_{9, 9}, current_{0, 0} {
}

// Maze::Maze(int width, int height)
//     : width_(width), height_(height), 
//       start_(0, 0), goal_(width-1, height-1), current_(0, 0) {
// }

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
        
        // 清除现有障碍物
        obstacles_.clear();
        
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
                double centerX = obs["center_x"];
                double centerY = obs["center_y"];
                double orbitRadius = obs["orbit_radius"];
                double speed = obs["speed"];
                double startAngle = obs.contains("start_angle") ? static_cast<double>(obs["start_angle"]) : 0.0;
                
                addObstacle(centerX, centerY, orbitRadius, speed, startAngle);
            }
        }
        
        // 尝试找到路径
        path_.clear();
        findPathAStar();
        
        return true;
    } catch (const std::exception& e) {
        // 处理JSON解析错误
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

void Maze::clearObstacles() {
    obstacles_.clear();
    path_.clear(); // 清除现有路径
}

bool Maze::findPathAStar() {
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
    
    // 清理内存
    for (auto node : allNodes) {
        delete node;
    }
    
    return pathFound;
}

bool Maze::planDWA(double& resultVx, double& resultVy, double maxSpeed) {
    if (path_.empty()) {
        return false;  // 没有路径可以跟随
    }
    
    // 设置DWA参数
    const double dt = 0.1;  // 预测时间步长
    const int numSamples = 10;  // 速度采样数
    
    double bestScore = -std::numeric_limits<double>::max();
    resultVx = 0.0;
    resultVy = 0.0;
    
    // 均匀采样速度空间
    std::vector<double> vxSamples(numSamples);
    std::vector<double> vySamples(numSamples);
    
    for (int i = 0; i < numSamples; i++) {
        double angle = 2.0 * M_PI * i / numSamples;
        vxSamples[i] = maxSpeed * std::cos(angle);
        vySamples[i] = maxSpeed * std::sin(angle);
    }
    
    // 评估每个速度样本
    for (int i = 0; i < numSamples; i++) {
        double vx = vxSamples[i];
        double vy = vySamples[i];
        
        // 预测位置
        double predX = current_.x + vx * dt;
        double predY = current_.y + vy * dt;
        
        // 修复: 使用适当的类型转换并保证边界检查
        int predXInt = static_cast<int>(predX);
        int predYInt = static_cast<int>(predY);
        
        // 如果预测位置无效或不安全，跳过该样本
        if (!isValid(predXInt, predYInt) || !isSafe(predXInt, predYInt)) {
            continue;
        }
        
        // 计算得分
        double score = evaluateDWA(vx, vy, dt);
        
        if (score > bestScore) {
            bestScore = score;
            resultVx = vx;
            resultVy = vy;
        }
    }
    
    return bestScore > -std::numeric_limits<double>::max();
}

void Maze::update(double dt) {
    // 更新所有动态障碍物
    for (auto& obstacle : obstacles_) {
        if (obstacle) { // 修复: 添加空指针检查
            obstacle->update(dt);
        }
    }
    
    // 如果有有效路径和速度，更新当前位置
    if (!path_.empty()) {
        double vx, vy;
        if (planDWA(vx, vy)) {
            // 更新位置（连续空间）
            current_.x += vx * dt;
            current_.y += vy * dt;
        }
    }
}

bool Maze::hasReachedGoal() const {
    // 当距离目标足够近时，认为已到达
    const double GOAL_THRESHOLD = 0.5;  // 半个单元格距离
    return current_.distanceTo(goal_) < GOAL_THRESHOLD;
}

// 检查每个障碍物是否与给定位置相交
bool Maze::isObstacle(int x, int y) const {
    for (const auto& obstacle : obstacles_) {
        if (obstacle && obstacle->intersects(x, y)) { // 修复: 添加空指针检查
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
    return std::sqrt(dx*dx + dy*dy);
}

void Maze::reconstructPath(AStarNode* node) {
    // 清除现有路径
    path_.clear();
    
    // 从目标向起点回溯
    while (node) {
        path_.push_back(Point(node->x, node->y));
        node = node->parent;
    }
    
    // 翻转路径，使其从起点到目标
    std::reverse(path_.begin(), path_.end());
}

double Maze::evaluateDWA(double vx, double vy, double dt) {
    // 预测下一位置
    double nextX = current_.x + vx * dt;
    double nextY = current_.y + vy * dt;
    
    // 避障得分 - 距离障碍物越远越好
    double obstacleScore = std::numeric_limits<double>::max();
    for (const auto& obstacle : obstacles_) {
        if (!obstacle) continue; // 修复: 添加空指针检查
        
        // 修复: 利用友元关系直接访问障碍物属性
        RealPoint nextPos(nextX, nextY);
        double distance = nextPos.distanceTo(obstacle->position_) - obstacle->radius_;
        obstacleScore = std::min(obstacleScore, distance);
    }
    
    // 修复: 确保避障得分不为零
    obstacleScore = std::max(obstacleScore, 0.001);
    
    // 路径跟随得分 - 先找出路径中下一个目标点
    double pathScore = std::numeric_limits<double>::max();
    size_t nextPointIndex = 0;
    
    // 找出当前位置在路径中的位置
    for (size_t i = 0; i < path_.size(); i++) {
        double distToCurrent = std::sqrt(std::pow(current_.x - path_[i].x, 2) + 
                                        std::pow(current_.y - path_[i].y, 2));
        if (distToCurrent < 1.0) { // 在某点附近
            nextPointIndex = std::min(i + 1, path_.size() - 1);
            break;
        }
    }
    
    // 计算到下一个路径点的距离
    if (nextPointIndex < path_.size()) {
        pathScore = std::sqrt(std::pow(nextX - path_[nextPointIndex].x, 2) + 
                             std::pow(nextY - path_[nextPointIndex].y, 2));
    }
    
    // 修复: 确保路径得分不为零
    pathScore = std::max(pathScore, 0.001);
    double pathScoreNormalized = 1.0 / pathScore;
    
    // 目标导向得分 - 考虑到目标的距离和方向
    double goalDistance = std::sqrt(std::pow(nextX - goal_.x, 2) + std::pow(nextY - goal_.y, 2));
    double goalScore = 1.0 / (goalDistance + 0.001);  // 距离目标越近，得分越高
    
    // 权重组合
    return (10.0 / obstacleScore) + (2.0 * pathScoreNormalized) + (1.0 * goalScore);
}

} // namespace PathGlyph