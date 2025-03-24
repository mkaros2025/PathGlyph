{
    depfiles = "glad.o: thirdparty/glad/src/glad.c thirdparty/glad/include/glad/glad.h  thirdparty/glad/include/KHR/khrplatform.h\
",
    files = {
        "thirdparty/glad/src/glad.c"
    },
    values = {
        "/usr/bin/gcc",
        {
            "-m64",
            "-fvisibility=hidden",
            "-O3",
            "-Isrc",
            "-Ithirdparty/glad/include",
            "-DNDEBUG"
        }
    },
    depfiles_format = "gcc"
}