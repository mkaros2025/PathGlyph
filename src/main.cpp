#include "core/shader.h"
#include "core/window.h"
#include "maze/maze.h"
#include "maze/obstacle.h"
#include "renderer/renderer.h"
#include <chrono>
#include <glad/glad.h>
#include <iostream>
#include <memory>
#include <thread>
#include <vector>

using namespace PathGlyph;

// 全局渲染器和迷宫指针，供回调函数使用
Renderer *g_renderer = nullptr;
Maze *g_maze = nullptr;

// 键盘回调
void handleKeyInput(int key, int action) {
    if (!g_renderer || !g_maze) return;

    if (action == 1) { // 按下按键
        switch (key) {
        case 256: // ESC 退出
            exit(0);
            break;
        case 87: // W 向上平移
            g_renderer->pan(0.0f, -0.5f);
            break;
        case 83: // S 向下平移
            g_renderer->pan(0.0f, 0.5f);
            break;
        case 65: // A 向左平移
            g_renderer->pan(0.5f, 0.0f);
            break;
        case 68: // D 向右平移
            g_renderer->pan(-0.5f, 0.0f);
            break;
        case 61:
        case 334: // + 或 NUM+ 放大
            g_renderer->zoom(1.1f);
            break;
        case 45:
        case 333: // - 或 NUM- 缩小
            g_renderer->zoom(0.9f);
            break;
        case 82: // R 重置视图
            g_renderer->resetView();
            break;
        case 80: // P 切换路径显示
            g_renderer->getConfig().showPath = !g_renderer->getConfig().showPath;
            break;
        case 79: // O 切换障碍物显示
            g_renderer->getConfig().showObstacles = !g_renderer->getConfig().showObstacles;
            break;
        case 70: // F 切换线框模式
            g_renderer->getConfig().wireframeMode = !g_renderer->getConfig().wireframeMode;
            break;
        case 32: // 空格 重新计算路径
            g_maze->findPathAStar();
            break;
        }
    }
}

int main() {
    try {
        // 创建窗口
        Window window(800, 600, "PathGlyph Renderer Test");

        // // 启用 V-Sync
        // glfwSwapInterval(1); // 每帧等待一次垂直同步信号

        // 创建迷宫对象
        auto maze = std::make_unique<Maze>();

        // 从 JSON 文件加载迷宫
        const std::string jsonFile = "../../../../assets/mazes/test_maze.json"; // JSON 文件路径
        if (!maze->loadFromJson(jsonFile)) {
            std::cerr << "Failed to load maze from JSON file: " << jsonFile << std::endl;
            return -1;
        }

        // 创建渲染器
        Renderer renderer(&window);

        // 回调函数 handleKeyInput 是全局的，无法直接访问 main 函数中的局部变量。
        // 因此，需要通过全局指针 g_maze 和 g_renderer 将这些对象暴露给回调函数。

        // 设置全局指针（供回调使用）
        g_renderer = &renderer;
        g_maze = maze.get();
        

        // 设置键盘回调
        window.setKeyCallback(handleKeyInput);

        // 初始化渲染器
        if (!renderer.initialize()) {
            std::cerr << "Failed to initialize renderer" << std::endl;
            return -1;
        }

        // 添加日志信息
        std::cout << "Renderer initialized successfully" << std::endl;

        // 设置迷宫
        renderer.setMaze(maze.get());
        std::cout << "Maze set to renderer" << std::endl;

        // 设置初始视图 - 添加这一行以确保能看到整个迷宫
        renderer.resetView();
        std::cout << "View reset" << std::endl;

        // 确保迷宫可见
        // 尝试将视图缩小一点，确保整个迷宫在视口内
        renderer.zoom(0.5f);
        std::cout << "Zoomed out for better view" << std::endl;

        // 显示控制指令
        std::cout << "渲染器测试程序\n";
        std::cout << "----------------------\n";
        std::cout << "WASD: 平移视图\n";
        std::cout << "+/-: 缩放视图\n";
        std::cout << "R: 重置视图\n";
        std::cout << "P: 切换路径显示\n";
        std::cout << "O: 切换障碍物显示\n";
        std::cout << "F: 切换线框模式\n";
        std::cout << "空格: 重新计算路径\n";
        std::cout << "ESC: 退出\n";

        // 主循环
        auto lastFrame = std::chrono::high_resolution_clock::now();

        // 主循环
        while (!window.shouldClose()) {
          // 处理用户输入等事件
          window.pollEvents();

          // 更新动态障碍物和当前位置信息等
          maze->update(0.0f); 

          // 渲染场景
          renderer.render();

          // 交换缓冲区（V-Sync 会在这里等待垂直同步信号）
          window.swapBuffers();
        }

      return 0;
    } catch (const std::exception &e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return -1;
    }
}