//
// Created by Nie R on 2019/5/1.
//

#ifndef TAG
#define TAG "cache" //150k
#endif

#ifndef INIT_OK
#define INIT_OK 1
#endif

#ifndef INIT_FAILED
#define INIT_FAILED 0
#endif


#include <stddef.h>
#include <memory.h>
#include <fcntl.h>
#include <stdio.h>
#include <sys/mman.h>
#include <zconf.h>
#include <malloc.h>
#include "nlogger_cache.h"
#include "nlogger_android_log.h"

int _open_mmap(char *mmap_cache_file_path, char **mmap_cache_buffer) {
    int result = INIT_FAILED;
    //step1 打开文件获取一个文件描述符fd
    int fd     = open(mmap_cache_file_path,
                      O_RDWR | O_CREAT, //以读写的模式或者创建的模式打开，文件不存在则创建，存在则读写
                      S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP); //赋予用户及用户组读写的权限

    if (fd != -1) {
        //step2 创建一个文件流，检查文件的大小，保证把150kb的数据填充满
        int  is_file_ok = 0;
        FILE *file      = fopen(mmap_cache_file_path, "rb+");
        if (file != NULL) {
            fseek(file, 0, SEEK_END);//将文件指针移动到文件的末尾
            long file_length = ftell(file);//获取当前文件指针的位置
            //如果文件的内容少于指定的缓存大小，则覆盖'/0'直至放满为止
            if (file_length < NLOGGER_MMAP_CACHE_SIZE) {
                char null_data[NLOGGER_MMAP_CACHE_SIZE];
                memset(null_data, 0, NLOGGER_MMAP_CACHE_SIZE);
                size_t written = fwrite(null_data, sizeof(char), NLOGGER_MMAP_CACHE_SIZE, file);
                fflush(file);

                if (written == NLOGGER_MMAP_CACHE_SIZE) {
                    is_file_ok = 1;
                } else {
                    LOGW(TAG, "pending data failed.expect %d, written %zd", NLOGGER_MMAP_CACHE_SIZE,
                         written);
                }
            } else {
                is_file_ok = 1;
            }
            //检查完文件的大小关闭文件流
            fclose(file);
        } else {
            LOGW(TAG, "mmap cache file fopen failed");
        }

        char *temp_mmap_buffer;
        //step3 建立mmap文件内存映射（在映射前要保证文件的内容被填满吗？）
        if (is_file_ok) {
            temp_mmap_buffer = mmap(NULL,//mmap内存映射到的地址，如果传NULL由内核自己决定映射到哪里
                                    NLOGGER_MMAP_CACHE_SIZE,//需要映射的文件长度
                                    PROT_READ | PROT_WRITE,//mapping映射的权限
                                    MAP_SHARED,//映射后更新会同时生效到文件显示在别的进程中的映射
                                    fd, 0);//
            if (temp_mmap_buffer != MAP_FAILED && temp_mmap_buffer != NULL) {
                result = INIT_OK;
                LOGD(TAG, "mmap success.");
            } else {
                LOGW(TAG, "mmap failed.");
            }
            //完成mmap映射后关闭文件描述符，mmap能在fd关闭的情况下仍然进行映射
            close(fd);

            //将mmap执行的内存地址让指针持有
            if (result == INIT_OK) {
                if (access(mmap_cache_file_path, F_OK) != -1) {
                    *mmap_cache_buffer = temp_mmap_buffer;
                    LOGD(TAG, "nlogger access mmap file success.");
                } else {
                    //文件有问题即使切换至内存缓存模式
                    result = INIT_FAILED;
                    LOGW(TAG,
                         "nlogger build mmap cache failed, access >> %s failed",
                         mmap_cache_file_path);
                }
            }
            //mmap失败时及时回收资源
            if (result == INIT_FAILED && temp_mmap_buffer != NULL) {
                munmap(temp_mmap_buffer, NLOGGER_MMAP_CACHE_SIZE);
            }
        }
    } else {
        LOGW(TAG, "open file failed.");
    }
    return result;
}

int _init_memory_cache(char **mmap_cache_buffer) {
    int  result       = INIT_FAILED;
    char *temp_buffer = malloc(NLOGGER_MEMORY_CACHE_SIZE);
    if (temp_buffer != NULL) {
        *mmap_cache_buffer = temp_buffer;
        result = INIT_OK;
    } else {
        LOGW(TAG, "on init memory, malloc failed.");
    }
    return result;
}

int init_cache_nlogger(char *mmap_cache_file_path, char **mmap_cache_buffer) {
    int result;
    //首先尝试用mmap的形式作为缓存
    if (mmap_cache_file_path == NULL || strnlen(mmap_cache_file_path, 16) == 0) {
        LOGW(TAG, "argument mmap cache file is invalid.")
        result = NLOGGER_MEMORY_CACHE_MODE;
    } else if (_open_mmap(mmap_cache_file_path, mmap_cache_buffer) == INIT_OK) {
        result = NLOGGER_MMAP_CACHE_MODE;
    } else {
        result = NLOGGER_MEMORY_CACHE_MODE;
    }
    //mmap失败则使用内存来作为缓存方式
    if (result == NLOGGER_MEMORY_CACHE_MODE && _init_memory_cache(mmap_cache_buffer) != INIT_OK) {
        result = NLOGGER_INIT_CACHE_FAILED;
    }
    return result;
}

