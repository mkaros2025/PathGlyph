{
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
    },
    depfiles = "shader.o: src/core/shader.cpp src/core/shader.h  thirdparty/glad/include/glad/glad.h  thirdparty/glad/include/KHR/khrplatform.h\
",
    files = {
        "src/core/shader.cpp"
    },
    depfiles_format = "gcc"
}