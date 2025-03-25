#include <glad/glad.h>

#include <chrono>
#include <iostream>
#include <GLFW/glfw3.h>
#include <memory>
#include <thread>
#include <vector>

#include "core/input_system.h"
#include "core/shader.h"
#include "core/window.h"
#include "maze/maze.h"
#include "maze/obstacle.h"
#include "renderer/renderer.h"

using namespace PathGlyph;

// 全局渲染器和迷宫指针，供回调函数使用
// Renderer *g_renderer = nullptr;
// Maze *g_maze = nullptr;

int main() {
  try {
    Window window(800, 600, "PathGlyph Renderer Test");

    auto maze = std::make_unique<Maze>();

    const std::string jsonFile = "../../../../assets/mazes/test_maze.json";
    if (!maze->loadFromJson(jsonFile)) {
      std::cerr << "Failed to load maze from JSON file: " << jsonFile
                << std::endl;
      return -1;
    }

    Renderer renderer(&window);
    if (!renderer.initialize()) {
      std::cerr << "Failed to initialize renderer" << std::endl;
      return -1;
    }
    std::cout << "Renderer initialized successfully" << std::endl;

    // 创建输入系统
    InputSystem inputSystem(&renderer, maze.get());
    // 显示控制说明
    // std::cout << inputSystem.getControlsDescription() << std::endl;
    // 设置按键回调
    window.setKeyCallback([&inputSystem](int key, int action) {
      inputSystem.handleInput(key, action);
    });

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

    auto lastFrame = std::chrono::high_resolution_clock::now();

    while (!window.shouldClose()) {
      auto currentFrame = std::chrono::high_resolution_clock::now();
      double deltaTime = std::chrono::duration<double>(currentFrame - lastFrame).count();
      lastFrame = currentFrame;
  
      window.pollEvents();
  
      // 处理游戏逻辑输入
      processCameraInput(deltaTime);
  
      // 4. 准备ImGui帧
      ImGui_ImplOpenGL3_NewFrame();
      ImGui_ImplGlfw_NewFrame(); 
      ImGui::NewFrame();
  
      maze->update(deltaTime);
  
      renderer.render();
  
      // 绘制 ui
      DrawPanel();

      // 提交ImGui绘制
      ImGui::Render();
      ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
  
      // 9. 交换缓冲区
      window.swapBuffers();
  }
  
    return 0;

  } catch (const std::exception &e) {
    std::cerr << "Error: " << e.what() << std::endl;
    return -1;
  }
}