{
    depfiles = "renderer.o: src/renderer/renderer.cpp src/renderer/renderer.h  src/renderer/../core/shader.h src/renderer/../core/window.h  src/renderer/../maze/maze.h src/renderer/../maze/obstacle.h  src/renderer/../maze/tile.h thirdparty/glad/include/glad/glad.h  thirdparty/glad/include/KHR/khrplatform.h\
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
    },
    depfiles_format = "gcc",
    files = {
        "src/renderer/renderer.cpp"
    }
}