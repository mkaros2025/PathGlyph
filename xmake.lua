add_rules("mode.debug", "mode.release")

-- 包依赖声明（全局作用域）
add_requires("glfw 3.3.6", {system = false})
add_requires("glm 0.9.9.8")
add_requires("imgui v1.89", {
    configs = {
        glfw_opengl3 = true,
        wchar32 = true
    }
})

target("PathGlyph")
    set_kind("binary")
    set_languages("c++20")

    -- 文件包含（避免重复）
    add_files("src/**.cpp")             -- 递归包含src下所有cpp
    add_files("thirdparty/glad/src/glad.c")  -- 单独指定C文件

    -- 依赖包链接
    add_packages("glfw", "glm", "imgui")

    -- 头文件路径
    add_includedirs(
        "src",
        "thirdparty/glad/include",
        "thirdparty/imgui"
    )

    -- 仅需显式指定必要系统库
    add_syslinks("dl", "pthread")  -- Linux平台必要基础库
    add_links("GL")  -- OpenGL库
