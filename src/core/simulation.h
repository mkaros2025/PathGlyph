#pragma once

#include <memory>
#include <vector>
#include "common/types.h"
#include "maze/maze.h"

namespace PathGlyph {

class Simulation {
public:
    Simulation(std::shared_ptr<Maze> maze, std::shared_ptr<EditState> editState);
    ~Simulation() = default;
    
    // 仿真控制
    void start();
    void reset();
    void update(float deltaTime);
    
    // 状态查询
    bool isRunning() const { return m_state == SimulationState::RUNNING; }
    bool isFinished() const { return m_state == SimulationState::FINISHED; }
    bool isIdle() const { return m_state == SimulationState::IDLE; }
    float getSimulationTime() const { return m_simulationTime; }
    const std::vector<Point>& getTraversedPath() const { return m_traversedPath; }
    
    // 访问Agent属性 - 通过maze访问
    const Point& getAgentPosition() const { return m_maze->getCurrentPosition(); }
    void setAgentPosition(const Point& position) { m_maze->setCurrentPosition(position); }
    
    // 访问DWA参数
    float getMaxSpeed() const { return m_maxSpeed; }
    void setMaxSpeed(float speed) { m_maxSpeed = speed; }
    
    float getMaxRotationSpeed() const { return m_maxRotationSpeed; }
    void setMaxRotationSpeed(float rotSpeed) { m_maxRotationSpeed = rotSpeed; }
    
    float getSensorRange() const { return m_sensorRange; }
    void setSensorRange(float range) { m_sensorRange = range; }
    
private:
    // 引用核心组件
    std::shared_ptr<Maze> m_maze;
    std::shared_ptr<EditState> m_editState;
    
    // 仿真状态
    SimulationState m_state = SimulationState::IDLE;
    glm::vec2 m_agentVelocity{0.0f, 0.0f};
    std::vector<Point> m_traversedPath;
    float m_simulationTime = 0.0f;
    
    // DWA参数
    float m_maxSpeed = 5.0f;
    float m_maxRotationSpeed = 2.0f;
    float m_sensorRange = 5.0f;
    
    // 内部方法
    void updateAgentPosition(float deltaTime);
};

} // namespace PathGlyph