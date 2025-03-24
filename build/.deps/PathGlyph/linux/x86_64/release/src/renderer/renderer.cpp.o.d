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
    depfiles = "renderer.o: src/renderer/renderer.cpp\
",
    files = {
        "src/renderer/renderer.cpp"
    }
}