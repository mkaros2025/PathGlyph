{
    depfiles_format = "gcc",
    values = {
        "/usr/bin/gcc",
        {
            "-m64",
            "-fvisibility=hidden",
            "-fvisibility-inlines-hidden",
            "-O3",
            "-std=c++20",
            "-Isrc",
            "-Ithirdparty/glad/include",
            "-DNDEBUG"
        }
    },
    depfiles = "maze.o: src/maze/maze.cpp src/maze/maze.h src/maze/obstacle.h\
",
    files = {
        "src/maze/maze.cpp"
    }
}