/*
 * Created by fgd on 2019/4/28.
 *
 * 日志库主文件，带下划线前缀的未私有方法，功能尽量独立，不引用全局的数据
 */


#include "compress/nlogger_data_handler.h"
#include "utils/nlogger_android_log.h"
#include "nlogger_error_code.h"
#include "nlogger_constants.h"
#include "utils/nlogger_file_utils.h"
#include "cache/nlogger_cache_handler.h"
#include "utils/nlogger_json_util.h"
#include "utils/nlogger_utils.h"
//#include "cache/nlogger_protocol.h"

#include "nlogger.h"

static nlogger *g_nlogger;

int _write_data(struct nlogger_cache_struct *cache, struct nlogger_data_handler_struct *data_handler, char *data, size_t data_length);

int _real_write(struct nlogger_cache_struct *cache, struct nlogger_data_handler_struct *data_handler, char *segment, size_t segment_length);

int _start_new_section(struct nlogger_data_handler_struct *data_handler, struct nlogger_cache_struct *cache);

int _end_current_section(struct nlogger_data_handler_struct *data_handler, struct nlogger_cache_struct *cache);

void on_compress_finish_callback(size_t handled_length);

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
int init_nlogger(const char *log_file_dir, const char *cache_file_dir,const long long max_log_file_size, const char *encrypt_key,
                 const char *encrypt_iv) {
    if (is_empty_string(log_file_dir) ||
        is_empty_string(cache_file_dir) ||
        is_empty_string(encrypt_key) ||
        is_empty_string(encrypt_iv)) {
        return ERROR_CODE_ILLEGAL_ARGUMENT;
    }
    //避免重复的去初始化
    if (g_nlogger != NULL && g_nlogger->state == NLOGGER_STATE_INIT) {
        return ERROR_CODE_OK;
    }

    if (g_nlogger != NULL) {
        //是不是需要加上对状态对判断
        free(g_nlogger);
    }
    g_nlogger = malloc(sizeof(nlogger));
    if (g_nlogger == NULL) {
        return ERROR_CODE_MALLOC_NLOGGER_STRUCT;
    }
    memset(g_nlogger, 0, sizeof(nlogger));
    //默认先把状态设置成error，稍后初始化成功以后再把状态设置到init
    g_nlogger->state = NLOGGER_STATE_ERROR;

    //创建日志文件的存放目录
    init_log_file_config(&g_nlogger->log, log_file_dir, max_log_file_size);

    //配置加密相关的参数
    init_encrypt(&g_nlogger->data_handler, encrypt_key, encrypt_iv);

    //初始换并且创建缓存（优先创建mmap）
    int init_cache_result = init_cache(&g_nlogger->cache, cache_file_dir);

    if (init_cache_result == NLOGGER_INIT_CACHE_FAILED) {
        g_nlogger->state = NLOGGER_STATE_ERROR;
        return ERROR_CODE_CREATE_CACHE_FAILED;
    } else {
        //标记当前初始化阶段成功，避免重复初始化
        g_nlogger->state = NLOGGER_STATE_INIT;
    }

    //尝试进行一次flush，将上次应用关闭前未flush的cache写入到日志文件中
    flush_nlogger();

    return ERROR_CODE_OK;
}


/**
 * 写入日志
 */
int write_nlogger(const char *log_file_name, int flag, char *log_content, long long local_time, char *thread_name,
                  long long thread_id, int is_main) {
    if (g_nlogger == NULL || g_nlogger->state == NLOGGER_STATE_ERROR) {
        return ERROR_CODE_NEED_INIT_NLOGGER_BEFORE_ACTION;
    }

    //step1 检查缓存状态
    int check_cache_result = check_cache_healthy(&g_nlogger->cache);

    if (check_cache_result <= 0) {
        //奇葩情况，一般不会走到这里，发现来再排查
        LOGE("write_nlogger", "###################  UNKNOW ERROR  ##################")
        LOGE("write_nlogger", "_check_cache_healthy on error >>> %d", check_cache_result)
        print_current_nlogger(g_nlogger);
        LOGE("write_nlogger", "###################  UNKNOW ERROR  ##################")
        return check_cache_result;
    }

    //step1.1 mmap缓存有问题，避免remain中缓存数据影响新插入的日志，先把它清空
    //todo 封装一个重置data_handler的方法
    if (check_cache_result == ERROR_CODE_CHANGE_CACHE_MODE_TO_MEMORY) {
        g_nlogger->data_handler.remain_data_length = 0;
    }

    //step2 检查日志文件
    //step2.1 检查日志文件是否打开，已经打开的日志文件是否和当前要写入的是同一个
    int check_log_result = check_log_file_healthy(&g_nlogger->log, log_file_name);

    if (check_log_result < 0) {
        LOGE("write_nlogger", "_check_log_file_healthy on error >>> %d", check_log_result)
        return check_log_result;
    }

    //第一次创建或者打开日志文件，需要重新映射当前mmap缓存指向的日志文件 方便下次启动将缓存flush到指定的日志文件中
    // todo [BUG] 2019年8月7日 这个时候万一remainData还有数据会发生数据错位
    // todo [BUG] 2019年8月7日 在logFile切换的情况下应该调用一次flush
    if (check_log_result == ERROR_CODE_NEW_LOG_FILE_OPENED) {
        int init_cache_result = map_log_file_with_cache(&g_nlogger->cache, log_file_name);
        if (init_cache_result != ERROR_CODE_OK) {
            return init_cache_result;
        }
    }


    //step2.2 检查日志文件的大小是否超过客户端设置的


    //step3 组装日志
    char *result_json_data;
    int  build_result;
    build_result = build_log_json_data(&g_nlogger->cache, flag, log_content, local_time,
                                       thread_name, thread_id, is_main, &result_json_data);
    if (build_result != ERROR_CODE_OK || result_json_data == NULL) {
        return build_result;
    }
    size_t log_json_data_length = strlen(result_json_data) + 1;
    if (log_json_data_length <= 1) {
        return ERROR_CODE_BUILD_LOG_BLOCK_FAILED;
    }

    LOGI("write", "raw log data size >>> %zd, log data >>> %s ", log_json_data_length, result_json_data)

    //step4 分段压缩加密写入
    int result = _write_data(&g_nlogger->cache, &g_nlogger->data_handler, result_json_data, log_json_data_length);

    return result;
}


/**
 * 写入数据到缓存中
 */
int _write_data(struct nlogger_cache_struct *cache, struct nlogger_data_handler_struct *data_handler, char *data,
                size_t data_length) {
//    size_t handled     = 0;
    char   *next_write = data;
    int    segment_num = (int) (data_length / NLOGGER_WRITE_SEGMENT_LENGTH);
    size_t remain      = data_length % NLOGGER_WRITE_SEGMENT_LENGTH;
    int    i           = 0;

    for (i = 0; i < segment_num; ++i) {
        int result = _real_write(cache, data_handler, next_write, NLOGGER_WRITE_SEGMENT_LENGTH);
        if (result != ERROR_CODE_OK) {
            return result;
        }
        next_write += NLOGGER_WRITE_SEGMENT_LENGTH;
    }

    if (remain != 0) {
        int result = _real_write(cache, data_handler, next_write, remain);
        if (result != ERROR_CODE_OK) {
            return result;
        }
    }

    return ERROR_CODE_OK;
}


/**
 * 写入数据片段到缓存中
 */
int _real_write(struct nlogger_cache_struct *cache, struct nlogger_data_handler_struct *data_handler, char *segment, size_t segment_length) {
    LOGD("real_write", "start _real_write cache->p_next_write >>> %ld", obtain_cache_next_write(cache));

    //判断是否有初始化，一般情况完成一个section的压缩就要重新初始化
    if (!is_data_handler_init(data_handler)) {
        _start_new_section(data_handler, cache);
//        _update_length(cache);
    }

    //加密压缩数据，尽量不去依赖缓存相关数据结构
    size_t compressed_length = compress_and_write_data(data_handler, obtain_cache_next_write(cache), segment, segment_length, &on_compress_finish_callback);
    if (compressed_length < 0) {
        return ERROR_CODE_COMPRESS_FAILED;
    }

    //判断是否需要flush缓存到日志文件中去
    if (is_cache_overflow(cache)) {
        LOGD("real_write", "flush cache log to log file.")
        return flush_nlogger();
    }

    //判断是否需要结束一段日志数据段
    if (compressed_length > NLOGGER_COMPRESS_SECTION_THRESHOLD) {
        int finish_result = _end_current_section(data_handler, cache);
        if (finish_result != ERROR_CODE_OK) {
            return finish_result;
        }
//        _update_length(cache);
    }

    LOGD("real_write", "finish _real_write cache->p_next_write >>> %ld", obtain_cache_next_write(cache));
    return ERROR_CODE_OK;
}


/**
 * 数据处理完成的回调，帮助更新cache结构体记录的指针
 *
 * @param handled_length
 */
void on_compress_finish_callback(size_t handled_length) {
    on_cache_written(&g_nlogger->cache, handled_length);
}

/**
 * 开始一段新的日志存储段（需要做一些准备工作）
 *
 * @param data_handler
 * @param cache
 * @return
 */
int _start_new_section(struct nlogger_data_handler_struct *data_handler, struct nlogger_cache_struct *cache) {
    LOGI("real_write", "start _real_write on new section");
    int init_result = init_zlib(data_handler);
    if (init_result != ERROR_CODE_OK) {
        return init_result;
    }

    write_cache_content_header_tag_and_length_block(cache);
    LOGI("real_write", "finish _real_write on new section");

    return ERROR_CODE_OK;
}


/**
 * 结束一段压缩过程，并且将剩余数据全部都写入到缓存中
 */
int _end_current_section(struct nlogger_data_handler_struct *data_handler, struct nlogger_cache_struct *cache) {
//    if (data_handler->state == NLOGGER_HANDLER_STATE_HANDLING) {
    //结束一次压缩流程，并且更新length，结束的时候会写入一些尾部标志位
    size_t finish_compressed_length = finish_compress_data(data_handler, obtain_cache_next_write(cache), &on_compress_finish_callback);
    LOGD("finish_section", "finish_compressed_length >>> %zd", finish_compressed_length)
    if (finish_compressed_length < 0) {
        return ERROR_CODE_COMPRESS_FAILED;
    }

    write_cache_content_tail_tag_block(cache);

    LOGI("finish_section", "finish_compressed_length end.")
    return ERROR_CODE_OK;
}


/**
 * 将缓存的数据写入到日志文件中
 */
int flush_nlogger() {
    if (g_nlogger == NULL || g_nlogger->state == NLOGGER_STATE_ERROR) {
        return ERROR_CODE_NEED_INIT_NLOGGER_BEFORE_ACTION;
    }

    print_current_nlogger(g_nlogger);

    //step1 检查数据处理状态，是否在压缩状态，如果在压缩处理数据阶段则先finish
    if (is_data_handler_processing(&g_nlogger->data_handler)) {
        int finish_result = _end_current_section(&g_nlogger->data_handler, &g_nlogger->cache);
        if (finish_result != ERROR_CODE_OK) {
            return finish_result;
        }
//        _update_length(&g_nlogger->cache);
    }

    //step2 检查日志文件, 在没有文件名的情况下去flush的时候需要从mmap中获取文件名然后再打开
    //（这一步是为了兼容init的时候进行初始化的操作）
    int  malloc_file_name_by_parse_mmap = 0; //标记是否有为 file 申请过内存
    char *file_name                     = NULL;
    int  file_name_configured           = is_log_file_name_valid(&g_nlogger->log) == ERROR_CODE_OK;
    //step2.1 获取缓存对应的日志文件名
    if (!file_name_configured) {
        int init_result = init_cache_from_mmap_buffer(&g_nlogger->cache, &file_name);
        if (init_result != ERROR_CODE_OK) {
            return init_result;
        }
        malloc_file_name_by_parse_mmap = 1;
    } else {
        file_name = current_log_file_name(&g_nlogger->log);
    }

    //step2.2 检查文件名对应到日志文件是否存在,不存在则创建并且打开
    int check_log_result = check_log_file_healthy(&g_nlogger->log, file_name);

    //先释放内存，避免内存泄露
    if (file_name != NULL && malloc_file_name_by_parse_mmap) {
        free(file_name);
    }

    //再检查日志文件检测的结果
    if (check_log_result <= 0) {
        LOGE("flush_nlogger", "_check_log_file_healthy on error >>> %d", check_log_result)
        return check_log_result;
    }

    //step3 将缓存到日志数据写入到日志文件中
    char   *cache_content = cache_content_head(&g_nlogger->cache);
    size_t content_length = cache_content_length(&g_nlogger->cache);
    flush_cache_to_log_file(&g_nlogger->log, cache_content, content_length);

    //step4 重制状态，为下次写入日志数据做准备
    reset_nlogger_cache(&g_nlogger->cache);

    return ERROR_CODE_OK;
}

//////////////////////////////  DEBUG  /////////////////////////////////

static char *g_null_string = "NULL";

void _get_string_data(char *data, char **result) {
    if (is_empty_string(data)) {
        *result = g_null_string;
        return;
    }
    *result = data;
}

void print_current_nlogger(struct nlogger_struct *g_nlogger) {
    if (g_nlogger == NULL) {
        return;
    }
    char *result;
    LOGW("debug", "\n");
    LOGW("debug", ">>>>>>>>>>>>>>>>  g_nlogger  >>>>>>>>>>>>>>>>");
    LOGW("debug", "cache.cache_mod = %d", g_nlogger->cache.cache_mode);
    LOGW("debug", "cache.length = %d", g_nlogger->cache.content_length);
    LOGW("debug", "cache.section_length = %d", g_nlogger->cache.section_length);
    _get_string_data(g_nlogger->cache.p_file_path, &result);
    LOGW("debug", "cache.p_file_path = %s", result);
    _get_string_data(g_nlogger->cache.p_buffer, &result);
    LOGW("debug", "cache.p_buffer = %s", result);
    LOGW("debug", "================================================");
    LOGW("debug", "log.state = %d", g_nlogger->log.state);
    LOGW("debug", "log.current_file_size = %ld", g_nlogger->log.current_file_size);
    _get_string_data(g_nlogger->log.p_path, &result);
    LOGW("debug", "log.p_path = %s", result);
    _get_string_data(g_nlogger->log.p_name, &result);
    LOGW("debug", "log.p_name = %s", result);
    _get_string_data(g_nlogger->log.p_dir, &result);
    LOGW("debug", "log.p_dir = %s", result);
    LOGW("debug", "<<<<<<<<<<<<<<<<  g_nlogger  <<<<<<<<<<<<<<<<");
    LOGW("debug", "\n");
}

//////////////////////////////  DEBUG  /////////////////////////////////

