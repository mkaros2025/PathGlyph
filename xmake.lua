-- 设置项目名称和版本
set_project("PathGlyph")
set_version("0.0.1")

-- 设置最小xmake版本要求
set_xmakever("2.7.0")

-- 设置构建模式
add_rules("mode.debug", "mode.release")

-- 声明包依赖
add_requires("glfw", "glad")

-- 全局配置
if is_mode("debug") then
    add_defines("DEBUG")
    set_symbols("debug")
    set_optimize("none")
end

-- 定义目标
target("PathGlyph")
    -- 设置为二进制目标
    set_kind("binary")
    -- 设置语言标准
    set_languages("c++20")
    
    -- 添加源文件
    add_files("src/**.cpp")
    
    -- 添加ImGui源文件
    add_files("thirdparty/imgui/*.cpp")
    add_files("src/gui/*.cpp")
    
    -- 添加GLAD源文件
    add_files("thirdparty/glad/src/glad.c")
    
    -- 添加头文件搜索路径
    add_includedirs("src")
    add_includedirs("thirdparty/imgui")
    add_includedirs("thirdparty/glad/include")
    add_includedirs("thirdparty/tinygltf")
    
    -- 添加包依赖
    add_packages("glfw", "glad")
    
    -- 链接系统库
    if is_plat("linux") then
        add_links("pthread", "dl")
        add_ldflags("-Wl,--no-as-needed")
        add_links("GL")
    elseif is_plat("windows") then
        add_links("opengl32")
    end
    add_defines("GLFW_INCLUDE_NONE")
