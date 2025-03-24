{
    depfiles = "main.o: src/main.cpp src/core/shader.h src/core/window.h src/maze/maze.h  src/maze/obstacle.h src/renderer/renderer.h src/renderer/../maze/tile.h  thirdparty/glad/include/glad/glad.h  thirdparty/glad/include/KHR/khrplatform.h\
",
    depfiles_format = "gcc",
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
    files = {
        "src/main.cpp"
    }
}