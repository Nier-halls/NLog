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
#include <malloc.h>
#include <sys/stat.h>
#include "utils/cJSON.h"
#include <sys/mman.h>
#include "nlogger_data_handler.h"
#include "nlogger_log_file_handler.h"

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
    char *p_file_path;//mmap缓存文件的路径
    char *p_buffer;//缓存的内存地址指针
    char *p_next_write;//缓存当前写入的地址指针

    char         *p_content_length;//指向缓存内容大小区域的指针
    unsigned int content_length;//缓存内容大小

    char         *p_section_length;//指向内容大小的指针（不包含内容的headTag、tailTag、cacheContentLength的大小）
    unsigned int section_length;//内容大小

    int cache_mode;//当前缓存的模式1为mmap，2为memory
};

#ifndef NLOGGER_STATE_ERROR
#define NLOGGER_STATE_ERROR (-1)
#endif

#ifndef NLOGGER_STATE_INIT
#define NLOGGER_STATE_INIT 1
#endif

/**
 * 总的管理数据结构
 */
typedef struct nlogger_struct {
    int                                state;
    struct nlogger_cache_struct        cache;
    struct nlogger_log_struct          log;
    struct nlogger_data_handler_struct data_handler;
} nlogger;

int init_nlogger(const char *log_file_dir, const char *cache_file_dir, const char *encrypt_key, const char *encrypt_iv);

int write_nlogger(const char *log_file_name, int flag, char *log_content, long long local_time, char *thread_name,long long thread_id, int is_main);

void print_current_nlogger(struct nlogger_struct *g_nlogger);

int flush_nlogger();

