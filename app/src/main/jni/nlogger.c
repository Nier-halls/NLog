//
// Created by fgd on 2019/4/28.
//

#include "nlogger.h"
#include "nlogger_android_log_util.h"
#include "nlogger_error_code.h"


/**
 * 初始化路径参数，并且创建文件
 *
 * @param log_file_path 日志文件的路径
 * @param cache_file_path 缓存文件的路径
 * @param encrypt_key 加密aes_key
 * @param encrypt_iv 加密偏移量
 *
 * @return error_code
 */
int nier_logger_init(char *log_file_path, char *cache_file_path, char *encrypt_key, char *encrypt_iv) {
    if (is_empty_string(log_file_path) ||
        is_empty_string(cache_file_path) ||
        is_empty_string(encrypt_key) ||
        is_empty_string(encrypt_iv)) {
        return ERROR_CODE_ILLEGAL_ARGUMENT;
    }

}

int is_empty_string(char *string) {
    return string != NULL && strlen(string) == 0;
}