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
    depfiles = "window.o: src/core/window.cpp src/core/window.h  thirdparty/glad/include/glad/glad.h  thirdparty/glad/include/KHR/khrplatform.h\
",
    files = {
        "src/core/window.cpp"
    }
}