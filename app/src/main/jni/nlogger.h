//
// Created by fgd on 2019/4/28.
//

#ifndef NLOGGER_NLOGGER_H
#define NLOGGER_NLOGGER_H

#endif //NLOGGER_NLOGGER_H

#include <stdio.h>
#include <zlib.h>
#include "stddef.h"
#include "string.h"

/***
 *
 * ****************************************************************** *
 *                        LOG CACHE HEADER                            *
 *                                                                    *
 *  OFFSET    DATA TYPE                                               *
 *  @+0       head tag                      1 bytes                   *
 *  @+1       header content length         2 bytes                   *
 *  @+3       header content                @{header content length}  *
 *  @...-1    tail tag                      1 bytes                   *
 *                                                                    *
 * ****************************************************************** *

 * ****************************************************************** *
 *                       LOG CACHE CONTENT                            *
 *                                                                    *
 *  OFFSET    DATA TYPE                                               *
 *  @-3       total length                  3 bytes                   *
 *  @+0       head tag                      1 bytes                   *
 *  @+1       cache content length          4 bytes                   *
 *  @+5       cache content                 @{cache content length}   *
 *  @...-1    tail tag                      1 bytes                   *
 * ****************************************************************** *
 *
 ***/
typedef struct nlogger_cache_struct {
    char *cache_file_path;
    char *cache_point;
    char *cache_legth_point;
    int cache_length;

    char *content_length_point;
    int content_length;

    char *next_write;

} cache_file;

/**
 * 描述记录log文件的结构体
 */
typedef struct nlogger_file_struct {
    char *log_file_dir; //日志文件存放目录
    char *log_file_path; //日志文件路径
    char *log_file_name; //日志文件名
    FILE *log_file; //日志文件
    int log_file_state; //日志文件的状态
    int log_file_length; //日志文件存放内容的长度
} log_file;

/**
 * 描述记录压缩加密的结构体
 */
typedef struct nlogger_data_handler_struct {
    z_stream *stream;
    char *remain_data[16];
    char *encrypt_key;
    char *encrypt_iv;
    int remain_data_length;
    int state;
} data_handler;

int is_empty_string(char *string);