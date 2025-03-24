#pragma once
#include <array>
#include <vector>  // 添加缺失的头文件
#include <cstdint> // 显式包含 uint8_t 的定义
#include <glm/glm.hpp>

namespace PathGlyph {

// 等轴测图块渲染类
class Tile {
public:
    // 图块类型枚举
    enum class Type : uint8_t {
        Wall = 0,   // 墙
        Ground = 1, // 地面
        Path = 2,   // 路径
        Start = 3,  // 起点
        Goal = 4    // 终点
    };

    // 图块尺寸常量 (像素)
    static constexpr float TILE_WIDTH = 64.0f;   // 图块宽度
    static constexpr float TILE_HEIGHT = 32.0f;  // 图块高度
    static constexpr float TILE_DEPTH = 16.0f;   // 图块深度（用于3D绘制）
    
    // 图块顶点数据结构
    struct Vertex {
        glm::vec3 position;  // 顶点位置
        glm::vec3 color;     // 顶点颜色
    };
    
    // 单个图块的顶点数据 (顶部面4个顶点 + 侧面4个顶点)
    static constexpr size_t VERTICES_PER_TILE = 8;
    // 单个图块的索引数据 (顶部面2个三角形 + 侧面2个三角形)
    static constexpr size_t INDICES_PER_TILE = 12;
    
    // 构造函数
    Tile(int x, int y, Type type = Type::Ground);
    
    // 获取图块数据
    int getX() const { return x_; }
    int getY() const { return y_; }
    bool isWalkable() const { return flags_.walkable; }
    bool isHighlighted() const { return flags_.highlighted; }
    Type getType() const { return static_cast<Type>(flags_.type); }
    
    // 设置图块属性
    void setType(Type type);
    void setHighlighted(bool highlighted);
    
    // 等轴测坐标转换
    static glm::vec2 isoToScreen(float x, float y);
    static glm::vec2 screenToIso(float x, float y);
    
    // 获取顶点数据（用于批量渲染）
    void getVertices(std::array<Vertex, VERTICES_PER_TILE>& vertices) const;
    void getIndices(std::array<unsigned int, INDICES_PER_TILE>& indices, unsigned int baseIndex) const;
    
    // 获取图块颜色
    static glm::vec3 getTypeColor(Type type);
    
private:
    int x_;  // 逻辑X坐标
    int y_;  // 逻辑Y坐标
    
    // 使用位域压缩数据
    struct {
        unsigned int type : 3;          // 图块类型 (3位，支持8种类型)
        unsigned int walkable : 1;      // 是否可通行 (1位)
        unsigned int highlighted : 1;   // 是否高亮 (1位)
        unsigned int reserved : 3;      // 保留位，未来扩展使用
    } flags_;
    
    // 预计算图块的屏幕坐标（等轴测投影）
    float screenX_;
    float screenY_;
    
    // 计算等轴测坐标
    void calculateIsoCoordinates();
    
    // 预计算顶点位置（用于优化渲染）
    void calculateVertexPositions(std::array<Vertex, VERTICES_PER_TILE>& vertices) const;
};

// 批量渲染图块的辅助类
class TileBatch {
public:
    // 初始化批次渲染器
    TileBatch();
    ~TileBatch();
    
    // 添加图块到批次
    void addTile(const Tile& tile);
    
    // 清空批次
    void clear();
    void upload();
    
    // 获取批次数据
    unsigned int getVAO() const { return vao_; }
    unsigned int getIndexCount() const { return indexCount_; }  // 修改方法名以匹配返回值
    
private:
    unsigned int vao_ = 0;         // 顶点数组对象，初始化为0
    unsigned int vbo_ = 0;         // 顶点缓冲对象，初始化为0
    unsigned int ebo_ = 0;         // 索引缓冲对象，初始化为0
    unsigned int vertexCount_ = 0; // 批次中的顶点数量，初始化为0
    unsigned int indexCount_ = 0;  // 批次中的索引数量，初始化为0
    
    std::vector<Tile::Vertex> vertices_;   // 顶点数据
    std::vector<unsigned int> indices_;    // 索引数据
};

} // namespace PathGlyph