//
// Created by Nie R on 2019/5/1.
//

#ifndef NLOGGER_NLOGGER_MMAP_H
#define NLOGGER_NLOGGER_MMAP_H

#ifndef NLOGGER_MMAP_CACHE_MODE
#define NLOGGER_MMAP_CACHE_MODE 1
#endif

#ifndef NLOGGER_MEMORY_CACHE_MODE
#define NLOGGER_MEMORY_CACHE_MODE 2
#endif

#ifndef NLOGGER_INIT_CACHE_FAILED
#define NLOGGER_INIT_CACHE_FAILED 0
#endif

#ifndef NLOGGER_MMAP_CACHE_SIZE
#define NLOGGER_MMAP_CACHE_SIZE 150 * 1024 //150k
#endif


#ifndef NLOGGER_MEMORY_CACHE_SIZE
#define NLOGGER_MEMORY_CACHE_SIZE 150 * 1024 //150k
#endif


#include <stdio.h>
#include <unistd.h>
#include<sys/mman.h>
#include <fcntl.h>
#include <string.h>

/***
 *
 <pre>
 * ****************************************************************** *
 *                        LOG CACHE HEADER                            *
 *                                                                    *
 *  OFFSET    DATA TYPE                                               *
 *  @+0       head tag                      4 bytes                   *
 *  @+4       header content length         2 bytes                   *
 *  @+6       header content                @{header content length}  *
 *  @...-4    tail tag                      4 bytes                   *
 *                                                                    *
 * ****************************************************************** *

 * ****************************************************************** *
 *                       LOG CACHE CONTENT                            *
 *                                                                    *
 *  OFFSET    DATA TYPE                                               *
 *  @-3       total length                  3 bytes                   *
 *  @+0       head tag                      4 bytes                   *
 *  @+4       cache content length          4 bytes                   *
 *  @+8       cache content                 @{cache content length}   *
 *  @...-4    tail tag                      4 bytes                   *
 * ****************************************************************** *
 </pre>
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


int init_cache(struct nlogger_cache_struct *cache, const char *cache_file_dir);

int write_cache_content_header_tag_and_length_block(struct nlogger_cache_struct *cache);

void on_cache_written(struct nlogger_cache_struct *cache, size_t written_length);

int write_cache_content_tail_tag_block(struct nlogger_cache_struct *cache);

int is_cache_overflow(struct nlogger_cache_struct *cache);

int check_cache_healthy(struct nlogger_cache_struct *cache);

int map_log_file_with_cache(struct nlogger_cache_struct *cache, const char *log_file_name);

int build_log_json_data(struct nlogger_cache_struct *cache, int flag, char *log_content, long long local_time,
                        char *thread_name, long long thread_id, int is_main, char **result_json_data);

int init_cache_from_mmap_buffer(struct nlogger_cache_struct *cache, char **log_file_name);

char *cache_content_head(struct nlogger_cache_struct *cache);

size_t cache_content_length(struct nlogger_cache_struct *cache);

int reset_nlogger_cache(struct nlogger_cache_struct *cache);

char *obtain_cache_next_write(struct nlogger_cache_struct *cache);

#endif //NLOGGER_NLOGGER_MMAP_H