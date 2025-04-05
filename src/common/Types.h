#pragma once

#include <array>
#include <vector>
#include <string>
#include <functional>
#include <cmath>
#include <cstdint>
#include <glm/glm.hpp>

namespace PathGlyph {

// 基本类型定义
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
    
    bool operator==(const Point& other) const {
        return x == other.x && y == other.y;
    }
    
    // 添加点的加法操作符
    Point operator+(const Point& other) const {
        return Point(x + other.x, y + other.y);
    }
    
    // 添加点的减法操作符
    Point operator-(const Point& other) const {
        return Point(x - other.x, y - other.y);
    }
};

// 2D向量类型，用于表示方向和速度
struct Vector2D {
    float x;
    float y;
    
    Vector2D() : x(0.0f), y(0.0f) {}
    Vector2D(float x_, float y_) : x(x_), y(y_) {}
    
    // 向量长度
    float length() const {
        return std::sqrt(x*x + y*y);
    }
    
    // 归一化向量
    Vector2D normalize() const {
        float len = length();
        if (len < 0.0001f) return Vector2D(0.0f, 0.0f);
        return Vector2D(x / len, y / len);
    }
    
    // 向量加法
    Vector2D operator+(const Vector2D& other) const {
        return Vector2D(x + other.x, y + other.y);
    }
    
    // 向量减法
    Vector2D operator-(const Vector2D& other) const {
        return Vector2D(x - other.x, y - other.y);
    }
    
    // 向量乘以标量
    Vector2D operator*(float scalar) const {
        return Vector2D(x * scalar, y * scalar);
    }
    
    // 向量取反
    Vector2D operator-() const {
        return Vector2D(-x, -y);
    }
    
    // 点乘
    float dot(const Vector2D& other) const {
        return x * other.x + y * other.y;
    }
};

// 障碍物移动类型枚举
enum class MovementType {
    LINEAR,     // 线性运动
    CIRCULAR    // 圆周运动
};

// 障碍物属性结构体
struct ObstacleProperties {
    MovementType movementType = MovementType::LINEAR;   // 运动类型
    float speed = 3.0f;                                // 线性移动速度
    Vector2D direction = Vector2D(1.0f, 0.0f);         // 线性移动方向
    Point center = Point(0, 0);                        // 圆周运动中心
    float radius = 5.0f;                               // 圆周运动半径
    float angularSpeed = 1.0f;                         // 角速度(弧度/秒)
};

// 实数坐标结构，用于高精度计算
struct RealPoint {
    double x;
    double y;
    
    RealPoint() : x(0.0), y(0.0) {}
    RealPoint(double x_, double y_) : x(x_), y(y_) {}
    
    double distanceTo(const RealPoint& other) const {
        double dx = x - other.x;
        double dy = y - other.y;
        return std::sqrt(dx*dx + dy*dy);
    }
    
    bool operator==(const RealPoint& other) const {
        return x == other.x && y == other.y;
    }
    
    // 转换为整数Point
    Point toIntPoint() const {
        return Point(static_cast<int>(x), static_cast<int>(y));
    }
};

enum class ModelType {
    GROUND,
    PATH,
    OBSTACLE,
    START,
    GOAL,
    AGENT,
    COUNT  // 用于标记模型类型的数量
};

// 图块叠加状态枚举
enum class TileOverlayType : uint8_t {
  None = 0,     // 无叠加
  Path = 1,     // 路径
  Start = 2,    // 起点
  Goal = 3,     // 终点
  Current = 4,  // 当前位置
  Obstacle = 5, // 障碍物
};

// 编辑模式枚举
enum class EditMode {
    VIEW,      // 查看模式
    EDIT,      // 编辑模式
    SIMULATION // 仿真模式
};

// 编辑对象类型
enum class EditObjectType {
    START_POINT,   // 起点
    END_POINT,     // 终点
    OBSTACLE       // 障碍物
};

// 仿真状态枚举
enum class SimulationState {
    IDLE,     // 空闲状态
    RUNNING,  // 运行状态
    FINISHED  // 完成状态
};

// 编辑器状态结构
struct EditState {
    EditMode mode = EditMode::VIEW;
    // 显示选项
    bool showWireframe = false;  // 线框模式
    bool showPath = true;        // 显示路径
    bool showObstacles = true;   // 显示障碍物
    bool shouldStartSimulation = false; // 是否应该开始模拟
    bool shouldResetState = false;
    
    // 摄像机控制
    float zoomLevel = 1.0f;                 // 缩放级别
    glm::vec2 cameraOffset = {0.0f, 0.0f};  // 摄像机平移偏移
    float cameraRotationX = 0.0f;           // 摄像机X轴旋转（围绕X轴）
    float cameraRotationY = 0.0f;           // 摄像机Y轴旋转（围绕Y轴）
    
    // 编辑模式下的状态
    int editType = 0;            // 0: 起点, 1: 终点, 2: 障碍物
    int obstacleAction = 0;      // 0: 添加, 1: 删除
    int obstacleType = 0;        // 0: 静态, 1: 动态
    int motionType = 0;          // 0: 直线, 1: 圆周
    Point currentAgentPosition{-1, -1};  // 当前代理位置
    
    // 仿真相关参数
    SimulationState simState = SimulationState::IDLE;  // 仿真状态
    float agentSpeed = 5.0f;                          // Agent移动速度
    float simulationTime = 0.0f;                      // 仿真运行时间
    
    // DWA参数
    float maxSpeed = 5.0f;        // 最大速度
    float maxRotationSpeed = 2.0f; // 最大旋转速度
    float sensorRange = 5.0f;     // 传感器范围
};

// 渲染参数结构体 - 用于配置单一着色器的不同效果
struct RenderParams {
  glm::vec4 baseColor{1.0f};     // 基础颜色
  float heightOffset = 0.0f;     // 高度偏移
  float emissiveStrength = 0.0f; // 发光强度
  float transparency = 1.0f;     // 透明度
  bool useTexture = true;        // 是否使用纹理
};

} // namespace PathGlyph 