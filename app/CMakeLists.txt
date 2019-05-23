cmake_minimum_required(VERSION 3.4.1)
SET(CMAKE_C_FLAGS_DEBUG "-g")
SET(CMAKE_C_FLAGS_RELEASE "-O2 -DNDEBUG")

#当前库的外部依赖库
#SET(NLOGGER_NATIVE_DEPENDENCE ../nlogger-native)
SET(ENCRYPT_LIBRARY_INCLUDE ./src/main/jni/encrypt/mbedtls/include)

SET(ENCRYPT_LIBRARY_SOURCE ./src/main/jni/encrypt/mbedtls/library)

#当前库需要呗一起编译进去的的源码文件
SET(SOURCE_FILE src/main/jni/nlogger_native_interface.h
        src/main/jni/nlogger_native_interface.c
        src/main/jni/nlogger_android_log.h
        src/main/jni/nlogger.h
        src/main/jni/nlogger.c
        src/main/jni/nlogger_constants.h
        src/main/jni/nlogger_file_utils.c
        src/main/jni/nlogger_file_utils.h
        src/main/jni/nlogger_cache.h
        src/main/jni/nlogger_cache.c
        src/main/jni/nlogger_json_util.h
        src/main/jni/nlogger_json_util.c
        src/main/jni/nlogger_utils.h
        src/main/jni/nlogger_utils.c
        src/main/jni/cJSON.h
        src/main/jni/cJSON.c
        src/main/jni/nlogger_protocol.h
        src/main/jni/nlogger_protocol.c
        src/main/jni/nlogger_data_handler.h
        src/main/jni/nlogger_data_handler.c
        src/main/jni/nlogger_log_file_handler.h
        src/main/jni/nlogger_log_file_handler.c
        )

#find_library( # Sets the name of the path variable.
#        log-lib
#
#        # Specifies the name of the NDK library that
#        # you want CMake to locate.
#        log)

#include_directories(${NLOGGER_NATIVE_DEPENDENCE} ${ENCRYPT_LIBRARY_INCLUDE})
#
#add_subdirectory(${NLOGGER_NATIVE_DEPENDENCE} .out)

include_directories(${ENCRYPT_LIBRARY_INCLUDE})

add_subdirectory(${ENCRYPT_LIBRARY_SOURCE})

add_library(nlogger SHARED ${SOURCE_FILE})

target_link_libraries(nlogger log z mbedcrypto)


if (${CMAKE_HOST_SYSTEM_NAME} MATCHES "Windows")
    ADD_DEFINITIONS(-DWindows)
else (${CMAKE_HOST_SYSTEM_NAME} MATCHES "Linux")
    ADD_DEFINITIONS(-DLinux)
endif ()