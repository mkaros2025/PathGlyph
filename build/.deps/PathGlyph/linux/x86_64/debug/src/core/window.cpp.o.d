{
    files = {
        "src/core/window.cpp"
    },
    depfiles_format = "gcc",
    depfiles = "window.o: src/core/window.cpp src/core/window.h  thirdparty/glad/include/glad/glad.h  thirdparty/glad/include/KHR/khrplatform.h\
",
    values = {
        "/usr/bin/gcc",
        {
            "-m64",
            "-g",
            "-O0",
            "-std=c++20",
            "-Isrc",
            "-Ithirdparty/glad/include"
        }
    }
}