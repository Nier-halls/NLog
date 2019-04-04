cmake_minimum_required(VERSION 3.4.1)

SET(CMAKE_C_FLAGS_DEBUG "-g")
SET(CMAKE_C_FLAGS_RELEASE "-O2 -DNDEBUG")

SET(NLOGGER_NATIVE_DEPENDENCE ../nlogger-native)

find_library( # Sets the name of the path variable.
        log-lib

        # Specifies the name of the NDK library that
        # you want CMake to locate.
        log)

include_directories(${NLOGGER_NATIVE_DEPENDENCE})

add_subdirectory(${NLOGGER_NATIVE_DEPENDENCE} .out)

add_library(nlogger SHARED src/main/jni/nlogger_native_interface.c)

target_link_libraries(nlogger ${log-lib} z nlogger-native)