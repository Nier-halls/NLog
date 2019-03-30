cmake_minimum_required(VERSION 3.4.1)

SET(CMAKE_C_FLAGS_DEBUG "-g")
SET(CMAKE_C_FLAGS_RELEASE "-O2 -DNDEBUG")


find_library( # Sets the name of the path variable.
        log-lib

        # Specifies the name of the NDK library that
        # you want CMake to locate.
        log)


add_library(nier-native SHARED src/main/jni/native-lib.c)
target_link_libraries(nier-native ${log-lib})