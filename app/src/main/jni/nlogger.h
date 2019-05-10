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
    char *p_file_path;//mmap缓存文件的路径
    char *p_buffer;//缓存的内存地址指针
    char *p_next_write;//缓存当前写入的地址指针

    char         *p_length;//指向缓存内容大小区域的指针
    unsigned int length;//缓存内容大小

    char         *p_content_length;//指向内容大小的指针（不包含内容的headTag、tailTag、cacheContentLength的大小）
    unsigned int content_length;//内容大小

    int cache_mode;//当前缓存的模式1为mmap，2为memory
};
//cache_file;


#ifndef NLOGGER_LOG_STATE_CLOSE
#define NLOGGER_LOG_STATE_CLOSE 0
#endif

#ifndef NLOGGER_LOG_STATE_OPEN
#define NLOGGER_LOG_STATE_OPEN 1
#endif

/**
 * 描述记录log文件的结构体
 */
typedef struct nlogger_log_struct {
    char *p_dir; //日志文件存放目录
    char *p_path; //日志文件路径
    char *p_name; //日志文件名
    FILE *p_file; //日志文件
    int  state; //日志文件的状态
    long file_length; //日志文件存放内容的长度
};
//log_file;

/**
 * 初始化前，finish一次数据处理流程以后的状态，此状态需要进行初始化
 */
#ifndef NLOGGER_HANDLER_STATE_IDLE
#define NLOGGER_HANDLER_STATE_IDLE 0
#endif

/**
 * 初始化完成状态
 */
#ifndef NLOGGER_HANDLER_STATE_INIT
#define NLOGGER_HANDLER_STATE_INIT 1
#endif

/**
 * 正在处理数据，这个时候是不允许去初始化的，因为可能有缓存数据被放在p_remain_data中
 * 并且z_stream中的资源也没有被释放
 */
#ifndef NLOGGER_HANDLER_STATE_HANDLING
#define NLOGGER_HANDLER_STATE_HANDLING 2
#endif


/**
 * 描述记录压缩加密的结构体
 */
typedef struct nlogger_data_handler_struct {
    z_stream *p_stream;
    char     *p_encrypt_key;
    char     *p_encrypt_iv;
    char     *p_encrypt_iv_pending;
    char     p_remain_data[16];
    size_t   remain_data_length;
    int      state;
};
//data_handler;

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

int write_nlogger(const char *log_file_name, int flag, char *log_content, long long local_time, char *thread_name,
                  long long thread_id, int is_main);