#include "ui/ImGuiWindow.h"
#include "gui/imgui_impl_glfw.h"
#include "gui/imgui_impl_opengl3.h"
#include <iostream>

namespace PathGlyph {

ImGuiWindow::ImGuiWindow(GLFWwindow* window, std::shared_ptr<EditState> state, std::shared_ptr<Simulation> simulation)
    : window_(window), currentState_(state), simulation_(simulation) {
    std::cout << "ImGuiWindow created." << std::endl;
}

ImGuiWindow::~ImGuiWindow() {
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();
}

bool ImGuiWindow::initialize() {
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
    
    ImGui::StyleColorsDark();

    if (!ImGui_ImplGlfw_InitForOpenGL(window_, true)) {
        std::cerr << "Failed to initialize ImGui GLFW backend" << std::endl;
        return false;
    }
    if (!ImGui_ImplOpenGL3_Init("#version 420")) {
        std::cerr << "Failed to initialize ImGui OpenGL3 backend" << std::endl;
        return false;
    }

    return true;
}

void ImGuiWindow::beginFrame() {
    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();
}

void ImGuiWindow::endFrame() {
    ImGui::Render();
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
}

void ImGuiWindow::handleInput() {
    ImGuiIO& io = ImGui::GetIO();
    // 处理鼠标按钮
    io.MouseDown[0] = glfwGetMouseButton(window_, GLFW_MOUSE_BUTTON_LEFT) == GLFW_PRESS;
    
    // 获取鼠标位置
    double mouseX, mouseY;
    glfwGetCursorPos(window_, &mouseX, &mouseY);
    io.MousePos = ImVec2(static_cast<float>(mouseX), static_cast<float>(mouseY));
    
    // 设置 ImGui 是否捕获鼠标
    // 如果鼠标位于 ImGui 窗口内，则由 ImGui 处理输入
    // if (mouseX < sidePanelWidth_) {
    //     io.WantCaptureMouse = true;
    // }
}

void ImGuiWindow::drawControlPanel() {
    // 创建ImGui窗口
    ImGui::SetNextWindowPos(ImVec2(0, 0));
    ImGui::SetNextWindowSize(ImVec2(300, 600));
    ImGui::Begin("Controls", nullptr, ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove);
    
    // 编辑模式选择
    if (ImGui::RadioButton("View", currentState_->mode == EditMode::VIEW || currentState_->mode == EditMode::SIMULATION)) {
        currentState_->mode = EditMode::VIEW;
    }
    ImGui::SameLine();
    if (ImGui::RadioButton("Edit", currentState_->mode == EditMode::EDIT)) {
        currentState_->mode = EditMode::EDIT;
    }
    
    ImGui::Separator();
    
    if (currentState_->mode == EditMode::VIEW || currentState_->mode == EditMode::SIMULATION) {
        if (ImGui::CollapsingHeader("Render Settings", ImGuiTreeNodeFlags_DefaultOpen)) {
            ImGui::Checkbox("Show Wireframe", &currentState_->showWireframe);
            ImGui::Checkbox("Show Path", &currentState_->showPath);
            ImGui::Checkbox("Show Obstacles", &currentState_->showObstacles);
        }
        
        ImGui::Separator();
        
        ImGui::Text("Path Controls:");

        if (ImGui::Button("Start Simulation", ImVec2(ImGui::GetContentRegionAvail().x, 0))) {
            currentState_->shouldStartSimulation = true;
        }
        if (ImGui::Button("Reset State", ImVec2(ImGui::GetContentRegionAvail().x, 0))) {
            currentState_->shouldResetState = true;
        }
        
        // 显示当前仿真状态
        const char* stateText = "Idle";
        if (simulation_->isRunning()) {
            stateText = "Running";
        } else if (simulation_->isFinished()) {
            stateText = "Finished";
        }
        ImGui::Text("Simulation State: %s", stateText);
        
        // 如果仿真正在运行或已完成，显示仿真时间
        ImGui::Text("Simulation Time: %.2f s", simulation_->getSimulationTime());
    } else {
        // 编辑模式下的控制选项
        ImGui::Text("Edit Type:");
        if (ImGui::RadioButton("Start Point", currentState_->editType == 0)) {
            currentState_->editType = 0;
        }
        if (ImGui::RadioButton("End Point", currentState_->editType == 1)) {
            currentState_->editType = 1;
        }
        if (ImGui::RadioButton("Obstacles", currentState_->editType == 2)) {
            currentState_->editType = 2;
        }
        
        if (currentState_->editType == 2) {  // 障碍物编辑
            ImGui::Separator();
            ImGui::Text("Obstacle Action:");
            if (ImGui::RadioButton("Add", currentState_->obstacleAction == 0)) {
                currentState_->obstacleAction = 0;
            }
            if (ImGui::RadioButton("Delete", currentState_->obstacleAction == 1)) {
                currentState_->obstacleAction = 1;
            }
            
            if (currentState_->obstacleAction == 0) {  // 添加障碍物
                ImGui::Separator();
                ImGui::Text("Obstacle Type:");
                if (ImGui::RadioButton("Static", currentState_->obstacleType == 0)) {
                    currentState_->obstacleType = 0;
                }
                if (ImGui::RadioButton("Dynamic", currentState_->obstacleType == 1)) {
                    currentState_->obstacleType = 1;
                }
                
                if (currentState_->obstacleType == 1) {  // 动态障碍物
                    ImGui::Separator();
                    ImGui::Text("Motion Type:");
                    if (ImGui::RadioButton("Linear", currentState_->motionType == 0)) {
                        currentState_->motionType = 0;
                    }
                    if (ImGui::RadioButton("Circular", currentState_->motionType == 1)) {
                        currentState_->motionType = 1;
                    }
                }
            }
        }
    }
    
    ImGui::End();
}
} // namespace PathGlyph 