cmake_minimum_required(VERSION 3.5)

set(MBEDTSL_INCLUDE ./mbedtls/include)
set(MBEDTSL_LIBRARY ./mbedtls/library)

include_directories(${MBEDTSL_INCLUDE})

add_subdirectory(${MBEDTSL_LIBRARY})

set(SOURCE_FILES
        clogan_core.c
        mmap_util.h
        mmap_util.c
        cJSON.c
        construct_data.h
        construct_data.c
        zlib_util.h
        logan_config.h
        zlib_util.c
        json_util.h
        json_util.c
        aes_util.h
        aes_util.c
        clogan_status.h
        directory_util.h
        directory_util.c
        base_util.h
        base_util.c
        console_util.h
        console_util.c)

add_library(nlogger-native ${SOURCE_FILES})

target_link_libraries(nlogger-native mbedcrypto)
