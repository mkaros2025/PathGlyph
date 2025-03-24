{
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
    depfiles_format = "gcc",
    files = {
        "src/core/shader.cpp"
    },
    depfiles = "shader.o: src/core/shader.cpp src/core/shader.h  thirdparty/glad/include/glad/glad.h  thirdparty/glad/include/KHR/khrplatform.h\
"
}