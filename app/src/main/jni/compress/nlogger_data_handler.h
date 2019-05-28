//
// Created by fgd on 2019/5/17.
//

#ifndef NLOGGER_NLOGGER_DATA_HANDLER_H
#define NLOGGER_NLOGGER_DATA_HANDLER_H

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


#include <stdio.h>
#include <zlib.h>
#include "stddef.h"
#include "string.h"

/**
 * 描述记录压缩加密的结构体
 */
typedef struct nlogger_data_handler_struct {
    z_stream *p_stream;
    char     *p_encrypt_key;
    char     *p_encrypt_iv_source;
    char     *p_encrypt_iv_pending;
    char     p_remain_data[16];
    size_t   remain_data_length;
    int      state;
};

int init_encrypt(struct nlogger_data_handler_struct *data_handler, const char *encrypt_key, const char *encrypt_iv);

int init_zlib(struct nlogger_data_handler_struct *data_handler);

size_t finish_compress_data(struct nlogger_data_handler_struct *data_handler, char *destination, void (*callback)(size_t));

int is_data_handler_init(struct nlogger_data_handler_struct *data_handler);

size_t compress_and_write_data(struct nlogger_data_handler_struct *data_handler, char *destination, char *source, size_t length, void (*callback)(size_t));

int is_data_handler_processing(struct nlogger_data_handler_struct *data_handler);

#endif //NLOGGER_NLOGGER_DATA_HANDLER_H
