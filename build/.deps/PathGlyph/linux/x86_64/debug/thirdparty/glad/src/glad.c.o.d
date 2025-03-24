{
    values = {
        "/usr/bin/gcc",
        {
            "-m64",
            "-g",
            "-O0",
            "-Isrc",
            "-Ithirdparty/glad/include"
        }
    },
    depfiles = "glad.o: thirdparty/glad/src/glad.c thirdparty/glad/include/glad/glad.h  thirdparty/glad/include/KHR/khrplatform.h\
",
    files = {
        "thirdparty/glad/src/glad.c"
    },
    depfiles_format = "gcc"
}