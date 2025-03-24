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
    depfiles_format = "gcc",
    depfiles = "maze.o: src/maze/maze.cpp src/maze/maze.h src/maze/obstacle.h\
",
    files = {
        "src/maze/maze.cpp"
    }
}