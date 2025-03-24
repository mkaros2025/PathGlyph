-- 声明支持调试和发布两种构建模式
add_rules("mode.debug", "mode.release")

target("PathGlyph")
    -- 声明生成可执行文件
    set_kind("binary")
    -- ** 是递归通配符
    add_files("src/**.cpp")
    add_files("thirdparty/glad/src/glad.c")

    -- 头文件路径 
    add_includedirs("src")
    add_includedirs("thirdparty/glad/include")  -- 添加核心头文件目录

    add_syslinks("pthread", "dl", "glfw", "X11", "GL")
    set_languages("c++20")

