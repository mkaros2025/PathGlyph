#pragma once

#include <array>
#include <vector>
#include <string>
#include <functional>
#include <cmath>
#include <cstdint>
#include <glm/glm.hpp>
#include <unordered_map>

namespace PathGlyph {

// 基本类型定义
struct Point {
    double x;
    double y;
    
    Point() : x(0.0), y(0.0) {}
    Point(double x_, double y_) : x(x_), y(y_) {}
    
    double distanceTo(const Point& other) const {
        double dx = x - other.x;
        double dy = y - other.y;
        return std::sqrt(dx * dx + dy * dy);
    }
    
    bool operator==(const Point& other) const {
        const double EPSILON = 0.001;
        return std::abs(x - other.x) < EPSILON && std::abs(y - other.y) < EPSILON;
    }
    
    // 添加点的加法操作符
    Point operator+(const Point& other) const {
        return Point(x + other.x, y + other.y);
    }
    
    // 添加点的减法操作符
    Point operator-(const Point& other) const {
        return Point(x - other.x, y - other.y);
    }
    
    // 获取整数坐标（四舍五入）
    Point toInt() const {
        return Point(std::round(x), std::round(y));
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

// 添加模型路径映射
inline const std::unordered_map<ModelType, std::string> MODEL_PATHS = {
    {ModelType::GROUND, "../../../../assets/models/ground.glb"},
    {ModelType::AGENT, "../../../../assets/models/agent.glb"},
    {ModelType::OBSTACLE, "../../../../assets/models/obstacle.glb"},
    {ModelType::GOAL, "../../../../assets/models/goal.glb"},
    {ModelType::START, "../../../../assets/models/start.glb"},
    {ModelType::PATH, "../../../../assets/models/path.glb"}
};

// 图块叠加状态枚举
enum class TileOverlayType : uint8_t {
  None = 0,     // 无叠加
  Path = 1,     // 路径
  Start = 2,    // 起点
  Goal = 3,     // 终点
  Agent = 4,  // 当前位置
  Obstacle = 5, // 障碍物
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

// 编辑模式枚举
enum class EditMode {
    VIEW,      // 查看模式
    EDIT,      // 编辑模式
    SIMULATION // 仿真模式
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
    float zoomLevel = 0.1f;                 // 缩放级别
    glm::vec2 cameraOffset = {0.0f, 0.0f};  // 摄像机平移偏移
    float cameraRotationX = 0.0f;           // 摄像机X轴旋转（围绕X轴）
    float cameraRotationY = 0.0f;           // 摄像机Y轴旋转（围绕Y轴）
    
    // 编辑模式下的状态
    int editType = 0;            // 0: 起点, 1: 终点, 2: 障碍物
    int obstacleAction = 0;      // 0: 添加, 1: 删除
    int obstacleType = 0;        // 0: 静态, 1: 动态
    int motionType = 0;          // 0: 直线, 1: 圆周
};

// 渲染参数结构体 - 用于配置单一着色器的不同效果
struct RenderParams {
  glm::vec4 baseColor{1.0f};     // 基础颜色
  float emissiveStrength = 0.0f; // 发光强度
  float transparency = 1.0f;     // 透明度
  bool useTexture = true;        // 是否使用纹理
  bool useModelColor = false;    // 是否使用模型自带颜色
};

} // namespace PathGlyph