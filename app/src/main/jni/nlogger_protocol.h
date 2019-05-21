//
// Created by fgd on 2019/5/17.
//
#include <stdio.h>
#include <zlib.h>
#include "stddef.h"
#include "string.h"
#include <cJSON.h>
#include "nlogger_json_util.h"


#ifndef NLOGGER_NLOGGER_PROTOCOL_H
#define NLOGGER_NLOGGER_PROTOCOL_H

#define NLOGGER_PROTOCOL_KEY_CACHE_TYPE "ct"
#define NLOGGER_PROTOCOL_KEY_FLAG "f"
#define NLOGGER_PROTOCOL_KEY_CONTENT "c"
#define NLOGGER_PROTOCOL_KEY_LOCAL_TIME "t"
#define NLOGGER_PROTOCOL_KEY_THREAD_NAME "tn"
#define NLOGGER_PROTOCOL_KEY_THREAD_ID "ti"
#define NLOGGER_PROTOCOL_KEY_MAIN_THREAD "mt"

#define NLOGGER_MMAP_CACHE_HEADER_KEY_VERSION "version"
#define NLOGGER_MMAP_CACHE_HEADER_KEY_FILE "file"
#define NLOGGER_MMAP_CACHE_HEADER_KEY_DATE "date"

#define NLOGGER_VERSION 1


int malloc_and_build_log_json_data(int cache_type,
                                   int flag,
                                   char *log_content,
                                   long long local_time,
                                   char *thread_name,
                                   long long thread_id,
                                   int is_main,
                                   char **result_json_data);


int malloc_and_build_cache_header_json_data(char *log_file_name, char **result_json_data);


int parse_header_json_data(char *header_json_data, char **file_name, int **version, long **date);


int add_mmap_head_tag(char *cache);

int add_mmap_tail_tag(char *cache);

int check_mmap_head_tag(char *tag_point);

int check_mmap_tail_tag(char *cache);

int add_section_head_tag(char *cache);

int add_section_tail_tag(char *cache);

#endif //NLOGGER_NLOGGER_PROTOCOL_H
