#pragma once
#include <cmath>

namespace PathGlyph {

// 简单坐标结构
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

} // namespace PathGlyph 