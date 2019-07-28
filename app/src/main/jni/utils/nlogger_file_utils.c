//
// Created by Nie R on 2019/4/30.
//


#include <stddef.h>
#include <memory.h>
#include <zconf.h>
#include <sys/stat.h>
#include "../nlogger_error_code.h"

#define LOGAN_MAX_PATH 1024

//判断文件和目录是否存在
int is_file_exist_nlogger(const char *path) {
    int isExist = 0;
    if (NULL != path && strnlen(path, 1) > 0) {
        if (access(path, F_OK) == 0) {
            isExist = 1;
        }
    }
    return isExist;
}

//根据路径创建目录
int makedir_nlogger(const char *path) {
    size_t beginCmpPath = 0;
    size_t endCmpPath = 0;
    size_t pathLen = strlen(path);
    char currentPath[LOGAN_MAX_PATH] = {0};

    //相对路径
    if ('/' != path[0]) {
        //获取当前路径
        getcwd(currentPath, LOGAN_MAX_PATH);
        strcat(currentPath, "/");
        beginCmpPath = strlen(currentPath);
        strcat(currentPath, path);
        if (path[pathLen - 1] != '/') {
            strcat(currentPath, "/");
        }
        endCmpPath = strlen(currentPath);
    } else {
        //绝对路径
        strcpy(currentPath, path);
        if (path[pathLen - 1] != '/') {
            strcat(currentPath, "/");
        }
        beginCmpPath = 1;
        endCmpPath = strlen(currentPath);
    }

    //创建各级目录
    for (size_t i = beginCmpPath; i < endCmpPath; i++) {
        if ('/' == currentPath[i]) {
            currentPath[i] = '\0';
            if (access(currentPath, F_OK) != 0) {
                if (mkdir(currentPath, 0777) == -1) {
                    return -1;
                }
            }
            currentPath[i] = '/';
        }
    }
    return ERROR_CODE_OK;
}