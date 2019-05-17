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

int create_cache_nlogger(char *mmap_cache_file_path, char **mmap_cache_buffer);

int parse_mmap_cache_head(char *cache_buffer, char **file_name);

int parse_mmap_cache_content_length(char *p_content_length);

size_t update_cache_section_length(char *section_length, unsigned int length);

size_t update_cache_content_length(char *content_length, unsigned int length);

size_t write_mmap_cache_header(char *cache_buffer, char *log_file_name);

#endif //NLOGGER_NLOGGER_MMAP_H