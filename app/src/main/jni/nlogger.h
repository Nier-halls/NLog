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
    char *p_file_path;
    char *p_buffer;
    char *p_legth;
    int  length;
    char *p_content_length;
    int  content_length;
    char *p_next_write;
} cache_file;

/**
 * 描述记录log文件的结构体
 */
typedef struct nlogger_file_struct {
    char *p_dir; //日志文件存放目录
    char *p_path; //日志文件路径
    char *p_file_name; //日志文件名
    FILE *p_file; //日志文件
    int  state; //日志文件的状态
    int  file_length; //日志文件存放内容的长度
} log_file;

/**
 * 描述记录压缩加密的结构体
 */
typedef struct nlogger_data_handler_struct {
    z_stream *p_stream;
    char     *p_encrypt_key;
    char     *p_encrypt_iv;
    char     *p_remain_data[16];
    int      remain_data_length;
    int      state;
} data_handler;

int is_empty_string(char *string);