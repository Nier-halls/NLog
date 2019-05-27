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


int init_cache(struct nlogger_cache_struct *cache, const char *cache_file_dir);

int current_cache_mode(struct nlogger_cache_struct *cache);

int create_cache_nlogger(struct nlogger_cache_struct *cache);

int parse_mmap_cache_head(char *cache_buffer, char **file_name);

int parse_mmap_cache_content_length(char *p_content_length);

int map_log_file_with_cache(struct nlogger_cache_struct *cache, const char *log_file_name);

char *cache_content_head(struct nlogger_cache_struct *cache);

size_t cache_content_length(struct nlogger_cache_struct *cache);

int check_cache_healthy(struct nlogger_cache_struct *cache);

size_t update_cache_section_length(char *section_length, unsigned int length);

size_t update_cache_content_length(char *content_length, unsigned int length);

size_t write_mmap_cache_header(char *cache_buffer, char *log_file_name);

int reset_nlogger_cache(struct nlogger_cache_struct *cache);

int init_cache_from_mmap_buffer(struct nlogger_cache_struct *cache, char **log_file_name);

#endif //NLOGGER_NLOGGER_MMAP_H