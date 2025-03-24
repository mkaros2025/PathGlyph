{
    depfiles_format = "gcc",
    depfiles = "tile.o: src/maze/tile.cpp src/maze/tile.h  thirdparty/glad/include/glad/glad.h  thirdparty/glad/include/KHR/khrplatform.h\
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
    files = {
        "src/maze/tile.cpp"
    }
}