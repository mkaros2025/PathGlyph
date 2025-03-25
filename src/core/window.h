#pragma once
#include <functional>
#include "core/gui/imgui_impl_glfw.h"
#include "core/gui/imgui_impl_opengl3.h"

// 前向声明GLFW类型避免头文件污染
struct GLFWwindow;

namespace PathGlyph {

class Window {
public:
    // 以窗口的宽高，标题来完成构造
    Window(int width = 800, int height = 600, const char* title = "PathGlyph");
    ~Window();

    // Window 类内部持有一个指向 GLFW 窗口的指针 (GLFWwindow* m_window)。
    // 如果允许拷贝，两个 Window 对象会共享同一个 GLFWwindow 资源，这可能导致资源管理问题（如重复释放资源）
    
    // 禁用拷贝构造函数
    Window(const Window&) = delete;
    // 禁用拷贝复制运算符
    Window& operator=(const Window&) = delete;

    // 检查窗口是否应该关闭
    // 返回值：
    // - true: 窗口应该关闭
    // - false: 窗口仍然打开
    bool shouldClose() const;

    // 交换前后缓冲区，用于显示渲染结果
    void swapBuffers() const;

    // 处理所有挂起的窗口事件（如键盘、鼠标输入等）
    void pollEvents() const;


    // 俩别名，设置了俩函数对象
    using KeyCallback = std::function<void(int key, int action)>;
    using MouseCallback = std::function<void(double x, double y)>;
    
    // 设置键盘输入回调函数
    // 参数：
    // - cb: 键盘回调函数，接收按键和动作作为参数
    void setKeyCallback(KeyCallback cb);

    // 设置鼠标输入回调函数
    // 参数：
    // - cb: 鼠标回调函数，接收鼠标位置（x, y）作为参数
    void setMouseCallback(MouseCallback cb);

    // 获取窗口的当前尺寸
    // 参数：
    // - width: 用于存储窗口宽度的引用
    // - height: 用于存储窗口高度的引用
    void getSize(int& width, int& height) const;

    // 获取鼠标的当前光标位置
    // 参数：
    // - x: 用于存储鼠标 x 坐标的引用
    // - y: 用于存储鼠标 y 坐标的引用
    void getCursorPos(double& x, double& y) const;

    // 关闭窗口
    void close();

    void DrawPanel(); // 调试面板主入口
    
private:
    // GLFW 窗口句柄，用于管理窗口资源
    GLFWwindow* m_window;    

    // 键盘输入回调函数
    KeyCallback m_keyCallback;

    // 鼠标输入回调函数
    MouseCallback m_mouseCallback;
    
    // 初始化窗口
    // 返回值：
    // - true: 初始化成功
    // - false: 初始化失败
    bool init();
    
    // 静态键盘回调函数，用于将 GLFW 的键盘事件转发到 Window 类的实例
    // 参数：
    // - window: GLFW 窗口指针
    // - key: 按键代码
    // - scancode: 系统扫描码
    // - action: 按键动作（如按下、释放）
    // - mods: 修饰键（如 Shift、Ctrl）
    static void keyCallback(GLFWwindow* window, int key, int scancode, int action, int mods);

    // 静态鼠标回调函数，用于将 GLFW 的鼠标事件转发到 Window 类的实例
    // 参数：
    // - window: GLFW 窗口指针
    // - xpos: 鼠标的 x 坐标
    // - ypos: 鼠标的 y 坐标
    static void cursorPosCallback(GLFWwindow* window, double xpos, double ypos);
    

    void DrawModeSwitch();
    void DrawEditControls();
    void DrawPathControls();
};

} // namespace PathGlyph