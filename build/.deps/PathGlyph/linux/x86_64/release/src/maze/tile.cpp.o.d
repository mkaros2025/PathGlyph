{
    depfiles_format = "gcc",
    depfiles = "tile.o: src/maze/tile.cpp src/maze/tile.h  thirdparty/glad/include/glad/glad.h  thirdparty/glad/include/KHR/khrplatform.h\
",
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
    files = {
        "src/maze/tile.cpp"
    }
}