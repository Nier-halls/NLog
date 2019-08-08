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
#include "compress/nlogger_data_handler.h"
#include "cache/nlogger_cache_handler.h"
#include "logfile/nlogger_log_file_handler.h"


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

int init_nlogger(const char *log_file_dir, const char *cache_file_dir, const long long max_file_size, const char *encrypt_key, const char *encrypt_iv);

int write_nlogger(const char *log_file_name, int flag, char *log_content, long long local_time, char *thread_name, long long thread_id, int is_main);

void print_current_nlogger(struct nlogger_struct *g_nlogger);

int flush_nlogger();

