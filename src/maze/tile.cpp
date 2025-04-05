#include "tile.h"

namespace PathGlyph {

// 静态方法实现
glm::vec2 Tile::worldToScreen(const glm::vec3& worldPos, const glm::mat4& viewProj) {
    // 将世界坐标转换为裁剪空间坐标
    glm::vec4 clipPos = viewProj * glm::vec4(worldPos, 1.0f);
    
    // 执行透视除法，得到归一化设备坐标（NDC）
    glm::vec3 ndcPos = glm::vec3(clipPos) / clipPos.w;
    
    // 将NDC坐标转换为屏幕坐标 (范围从[-1,1]转换到[0,1])
    return glm::vec2(
        (ndcPos.x + 1.0f) * 0.5f,
        (ndcPos.y + 1.0f) * 0.5f
    );
}

glm::vec3 Tile::screenToWorld(const glm::vec2& screenPos, float depth, const glm::mat4& invViewProj) {
    // 从屏幕坐标转换回NDC坐标 (从[0,1]转换回[-1,1])
    glm::vec4 ndcPos = glm::vec4(
        screenPos.x * 2.0f - 1.0f,
        screenPos.y * 2.0f - 1.0f,
        depth * 2.0f - 1.0f,  // 深度也需要转换到NDC空间
        1.0f
    );
    
    // 使用逆视图投影矩阵转换回世界坐标
    glm::vec4 worldPos = invViewProj * ndcPos;
    
    // 执行透视除法，得到3D世界坐标
    return glm::vec3(worldPos) / worldPos.w;
}

// 构造函数
Tile::Tile(int x, int y, TileOverlayType overlayType)
    : x_(x), y_(y), overlayType_(overlayType) {
}

// 基本属性访问方法实现
int Tile::getX() const {
    return x_;
}

int Tile::getY() const {
    return y_;
}

bool Tile::isWalkable() const {
    return overlayType_ != TileOverlayType::Obstacle;
}

TileOverlayType Tile::getOverlayType() const {
    return overlayType_;
}

void Tile::setOverlayType(TileOverlayType type) {
    overlayType_ = type;
}

glm::vec3 Tile::getWorldPosition() const {
    // 将逻辑坐标转换为世界坐标
    // 注意Y轴在3D空间中通常是向上的，而迷宫坐标Y通常是向下的
    // 所以Y坐标可能需要翻转，这取决于坐标系的设定
    return glm::vec3(
        x_ * TILE_SIZE,        // X位置
        0.0f,                  // Y位置（高度），基础图块高度为0
        y_ * TILE_SIZE         // Z位置
    );
}

Tile::BoundingBox Tile::getBoundingBox() const {
    glm::vec3 pos = getWorldPosition();
    
    // 计算图块的包围盒
    BoundingBox box;
    box.min = glm::vec3(
        pos.x - TILE_SIZE * 0.5f,          // 左边界
        pos.y,                             // 底部
        pos.z - TILE_SIZE * 0.5f           // 前边界
    );
    
    box.max = glm::vec3(
        pos.x + TILE_SIZE * 0.5f,          // 右边界
        pos.y + TILE_HEIGHT,               // 顶部
        pos.z + TILE_SIZE * 0.5f           // 后边界
    );
    
    return box;
}

} // namespace PathGlyph 