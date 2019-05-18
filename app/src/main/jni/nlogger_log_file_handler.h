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
    char *p_path; //日志文件路径 (包含日志名以及日志路径)
    char *p_name; //日志文件名
    FILE *p_file; //日志文件
    int  state; //日志文件的状态
    long file_length; //日志文件存放内容的长度
};

int open_log_file(struct nlogger_log_struct *log);

//int set_log_file_name(struct nlogger_log_struct *log, const char *log_file_name);

int check_log_file_healthy(struct nlogger_log_struct *log, const char *log_file_name);

int is_log_file_name_valid(struct nlogger_log_struct *log);

int set_log_file_save_dir(struct nlogger_log_struct *log, char *dir);

char *current_log_file_name(struct nlogger_log_struct *log);

#endif //NLOGGER_NLOGGER_LOG_FILE_HANDLER_H
