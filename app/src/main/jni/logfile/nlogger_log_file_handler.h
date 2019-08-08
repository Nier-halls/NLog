//
// Created by fgd on 2019/5/18.
//

#ifndef NLOGGER_NLOGGER_LOG_FILE_HANDLER_H
#define NLOGGER_NLOGGER_LOG_FILE_HANDLER_H


#include <stdio.h>
#include <malloc.h>

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
    char *p_path; //日志文件路径 (包含日志名以及日志路径 p_path = p_dir + p_name)
    char *p_name; //日志文件名
    FILE *p_file; //日志文件
    int  state; //日志文件的状态
    long current_file_size; //日志文件存放内容的长度
    long long max_file_size; //日志文件的最大长度
};

int open_log_file(struct nlogger_log_struct *log);

int check_log_file_healthy(struct nlogger_log_struct *log, const char *log_file_name);

int is_log_file_name_valid(struct nlogger_log_struct *log);

//int set_log_file_save_dir(struct nlogger_log_struct *log, char *dir);

int init_log_file_config(struct nlogger_log_struct *log, const char *dir, const long long max_file_size);

char *current_log_file_name(struct nlogger_log_struct *log);

int flush_cache_to_log_file(struct nlogger_log_struct *log, char *cache, size_t cache_length);


#endif //NLOGGER_NLOGGER_LOG_FILE_HANDLER_H
