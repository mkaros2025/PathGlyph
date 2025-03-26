#include <glad/glad.h>

#include <chrono>
#include <iostream>
#include <memory>
#include <thread>
#include <vector>

#include "core/Application.h"

using namespace PathGlyph;

// 全局渲染器和迷宫指针，供回调函数使用
// Renderer *g_renderer = nullptr;
// Maze *g_maze = nullptr;

int main(int argc, char* argv[]) {
    try {        
        PathGlyph::Application app(1000, 600, "PathGlyph");
        app.run();
        return 0;
    } catch (const std::exception& e) {
        std::cerr << "Fatal error: " << e.what() << std::endl;
        return 1;
    }
}