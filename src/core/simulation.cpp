#include "core/simulation.h"
#include <iostream>

namespace PathGlyph {

Simulation::Simulation(std::shared_ptr<Maze> maze, std::shared_ptr<EditState> editState)
    : m_maze(maze), m_editState(editState) {
    // 初始化，但不再尝试从EditState同步simState
}

void Simulation::start() {
    const Point& start = m_maze->getStart();
    const Point& goal = m_maze->getGoal();
    
    if (!m_maze->isInBounds(start) || !m_maze->isInBounds(goal)) {
        std::cout << "Invalid start or goal point, cannot start simulation" << std::endl;
        return;
    }
    
    // 重置仿真状态
    reset();
    
    // 设置状态为运行
    m_state = SimulationState::RUNNING;
    // 设置为SIMULATION模式，这对于仿真功能是必要的cmft
    m_editState->mode = EditMode::SIMULATION;
    
    std::cout << "Simulation started: from (" << start.x << "," << start.y 
              << ") to (" << goal.x << "," << goal.y << ")" << std::endl;
}

void Simulation::reset() {
    // 停止仿真
    if (m_state == SimulationState::RUNNING) {
        m_state = SimulationState::FINISHED;
        std::cout << "Simulation stopped" << std::endl;
    }
    
    m_state = SimulationState::IDLE;
    m_simulationTime = 0.0f;
    m_maze->clearPath();
    m_traversedPath.clear();
    m_maze->reset();
    m_agentVelocity = glm::vec2(0.0f, 0.0f);
    
    // 重置仿真状态
    m_state = SimulationState::IDLE;
    m_editState->mode = EditMode::VIEW;
    
    std::cout << "Simulation reset" << std::endl;
}

void Simulation::update(float deltaTime) {
    if (!isRunning()) {
        return;
    }
    
    // 更新仿真时间
    m_simulationTime += deltaTime;
    
    // 更新动态障碍物
    m_maze->update(deltaTime);
    // 更新Agent位置
    updateAgentPosition(deltaTime);
    
    // 如果到达终点，结束仿真
    if (m_maze->hasReachedGoal()) {
        m_state = SimulationState::FINISHED;
        if (m_editState) {
            m_editState->showPath = true;
            m_editState->mode = EditMode::VIEW;
        }
        m_maze->setPath(m_traversedPath);
        
        std::cout << "Agent reached goal, simulation complete" << std::endl;
        std::cout << "Total time: " << m_simulationTime << " seconds" << std::endl;
    }
}

void Simulation::updateAgentPosition(float deltaTime) {
    // 获取当前位置和目标位置
    const Point& currentPos = m_maze->getCurrentPosition();
    const Point& goal = m_maze->getGoal();
    
    // 只在开始时使用A*算法规划一次路径
    if (m_maze->getPath().empty()) {
        // 规划A*路径
        std::vector<Point> path = m_maze->findPathAStar();
        
        // 检查是否找到了有效路径
        if (path.empty()) {
            std::cout << "A*算法无法找到有效路径！" << std::endl;
            return;
        }
        
        // 设置规划好的路径
        m_maze->setPath(path);
        std::cout << "使用A*算法规划了一条新路径，共" << path.size() << "个点" << std::endl;
    }
    
    // 获取当前规划的路径
    const std::vector<Point>& path = m_maze->getPath();
    
    // 如果路径为空，直接返回
    if (path.empty()) {
        return;
    }
    
    // 寻找当前位置对应的路径点索引
    size_t currentIndex = 0;
    float minDist = std::numeric_limits<float>::max();
    
    for (size_t i = 0; i < path.size(); ++i) {
        float dist = currentPos.distanceTo(path[i]);
        if (dist < minDist) {
            minDist = dist;
            currentIndex = i;
        }
    }
    
    // 如果已经到达终点附近，直接移动到终点
    if (currentIndex >= path.size() - 1 && minDist < 0.1f) {
        m_maze->setCurrentPosition(goal);
        m_agentVelocity = glm::vec2(0.0f, 0.0f);
        return;
    }
    
    // 确定下一个目标点
    size_t nextIndex = currentIndex + 1;
    if (nextIndex >= path.size()) {
        nextIndex = path.size() - 1;
    }
    
    // 向下一个点移动
    const Point& nextPoint = path[nextIndex];
    
    // 计算方向向量
    glm::vec2 direction(nextPoint.x - currentPos.x, nextPoint.y - currentPos.y);
    float distance = glm::length(direction);
    
    // 如果距离很近，直接到达下一个点
    if (distance < 0.1f) {
        m_maze->setCurrentPosition(nextPoint);
        
        // 记录轨迹
        if (m_traversedPath.empty() || m_traversedPath.back().distanceTo(nextPoint) > 0.01) {
            m_traversedPath.push_back(nextPoint);
        }
        
        return;
    }
    
    // 归一化方向向量
    direction = glm::normalize(direction);
    
    // 设置恒定速度
    float speed = 2.0f; 
    m_agentVelocity = direction * speed;
    
    // 计算新位置
    double newX = currentPos.x + m_agentVelocity.x * deltaTime;
    double newY = currentPos.y + m_agentVelocity.y * deltaTime;
    Point newPos(newX, newY);
    
    // 直接更新位置，不检查碰撞
    m_maze->setCurrentPosition(newPos);
    
    // 记录轨迹
    if (m_traversedPath.empty() || m_traversedPath.back().distanceTo(newPos) > 0.01) {
        m_traversedPath.push_back(newPos);
    }
}

} // namespace PathGlyph