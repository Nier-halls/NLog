/*
 * Created by fgd on 2019/4/28.
 *
 * 日志库主文件，带下划线前缀的未私有方法，功能尽量独立，不引用全局的数据
 */


#include "nlogger_data_handler.h"
#include "utils/nlogger_android_log.h"
#include "nlogger_error_code.h"
#include "nlogger_constants.h"
#include "utils/nlogger_file_utils.h"
#include "nlogger_cache.h"
#include "utils/nlogger_json_util.h"
#include "utils/nlogger_utils.h"
#include "nlogger_protocol.h"

#include "nlogger.h"

static nlogger *g_nlogger;

/**
 * 创建mmap缓存文件用的路径（包含缓存文件名）
 */
int _create_cache_file_path(const char *cache_file_dir, char **result_path) {
    if (is_empty_string(cache_file_dir)) {
        return ERROR_CODE_ILLEGAL_ARGUMENT;
    }
    int appendSeparate = 0;
    if (cache_file_dir[strlen(cache_file_dir)] != '/') {
        appendSeparate = 1;
    }
    size_t path_string_length = strlen(cache_file_dir) +
                                (appendSeparate ? strlen("/") : 0) +
                                strlen(NLOGGER_CACHE_DIR) +
                                strlen("/") +
                                strlen(NLOGGER_CACHE_FILE_NAME) +
                                strlen("\0");
    *result_path = malloc(path_string_length);
    if (*result_path == NULL) {
        return ERROR_CODE_MALLOC_CACHE_DIR_STRING;
    }
    memset(*result_path, 0, path_string_length);
    memcpy(*result_path, cache_file_dir, strlen(cache_file_dir));
    if (appendSeparate) {
        strcat(*result_path, "/");
    }
    strcat(*result_path, NLOGGER_CACHE_DIR);
    strcat(*result_path, "/");
    if (makedir_nlogger(*result_path) < 0) {
        return ERROR_CODE_CREATE_LOG_CACHE_DIR_FAILED;
    }
    LOGD("init", "create mmap cache dir_string >>> %s", *result_path);
    strcat(*result_path, NLOGGER_CACHE_FILE_NAME);
    return ERROR_CODE_OK;
}

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
int init_nlogger(const char *log_file_dir, const char *cache_file_dir, const char *encrypt_key,
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

    //todo 检查是否有原来的 结构体存在 考虑释放原来的结构体
    //创建mmap缓存文件的目录
    char *final_mmap_cache_path;
    _create_cache_file_path(cache_file_dir, &final_mmap_cache_path);
    LOGD("init", "finish create log cache path>>>> %s ", final_mmap_cache_path);
    g_nlogger->cache.p_file_path = final_mmap_cache_path;

    //创建日志文件的存放目录
    set_log_file_save_dir(&g_nlogger->log, log_file_dir);

    //配置加密相关的参数
    init_encrypt(&g_nlogger->data_handler, encrypt_key, encrypt_iv);

    //初始化缓存文件，首先采用mmap缓存，失败则用内存缓存
    int create_cache_result = create_cache_nlogger(g_nlogger->cache.p_file_path, &g_nlogger->cache.p_buffer);

    if (create_cache_result == NLOGGER_INIT_CACHE_FAILED) {
        g_nlogger->state = NLOGGER_STATE_ERROR;
        return ERROR_CODE_CREATE_CACHE_FAILED;
    } else {
        //标记当前初始化阶段成功，避免重复初始化
        g_nlogger->state            = NLOGGER_STATE_INIT;
        g_nlogger->cache.cache_mode = create_cache_result;
    }
    print_current_nlogger(g_nlogger);

    LOGD("init", "init cache result >>> %d.", create_cache_result);

    //todo try flush cache log
    flush_nlogger();

    return ERROR_CODE_OK;
}

/**
 * 初始化配置缓存管理结构体
 *
 * @param log_file_name
 * @param cache
 * @return
 */
int _init_cache_on_new_log_file_opened(struct nlogger_cache_struct *cache, const char *log_file_name) {

    if (cache->cache_mode == NLOGGER_MMAP_CACHE_MODE) {
        //直接忽略旧的日志头，覆盖写入新的日志头，指定当前mmap文件映射的log file name
        size_t written_header_length = write_mmap_cache_header(cache->p_buffer, log_file_name);

        if (written_header_length > 0) {
            cache->p_content_length = cache->p_buffer + written_header_length;
        } else {
            //写入失败则切换成Memory缓存模式
            //（不能建立关系就失去了意外中断导致日志丢失的优势，这和内存缓存没有区别）
            if (cache->p_buffer != NULL) {
                munmap(cache->p_buffer, NLOGGER_MMAP_CACHE_SIZE);
            }
            //路径传空，强制Memory模式缓存
            if (NLOGGER_INIT_CACHE_FAILED == create_cache_nlogger(NULL, &cache->p_buffer)) {
                return ERROR_CODE_INIT_CACHE_FAILED;
            }
        }
    }

    //mmap模式会在完成映射关系（写入file name）后指定代表content长度指针的位置
    //memory模式需要手动指定代表长度的指针，也就是内存缓存的开头地址
    if (cache->cache_mode == NLOGGER_MEMORY_CACHE_MODE) {
        cache->p_content_length = cache->p_buffer;
    }
    cache->p_next_write   = cache->p_content_length + NLOGGER_CONTENT_LENGTH_BYTE_SIZE;
    cache->content_length = 0;
    cache->section_length = 0;

    return ERROR_CODE_OK;
}

/**
 * 检查缓存是否健康，如果mmap缓存有问题及时切换成memory缓存
 *
 * @param cache
 * @return
 */
int _check_cache_healthy(struct nlogger_cache_struct *cache) {
    int result = ERROR_CODE_OK;
    if (cache->cache_mode == NLOGGER_MMAP_CACHE_MODE &&
        !is_file_exist_nlogger(cache->p_file_path)) {
        //mmap缓存文件不见了，这个时候主动切换成Memory缓存
        if (cache->p_buffer != NULL) {
            munmap(cache->p_buffer, NLOGGER_MMAP_CACHE_SIZE);
        }
        //路径传空，强制Memory缓存
        if (NLOGGER_INIT_CACHE_FAILED == create_cache_nlogger(NULL, &cache->p_buffer)) {
            return ERROR_CODE_INIT_CACHE_FAILED;
        }
        cache->p_content_length = cache->p_buffer;
        cache->p_next_write     = cache->p_content_length + NLOGGER_CONTENT_LENGTH_BYTE_SIZE;
        cache->content_length   = 0;
        cache->section_length   = 0;
        result = ERROR_CODE_CHANGE_CACHE_MODE_TO_MEMORY;
    }

    //最后检查一下缓存指针是否存在，如果不存在则直接报错返回
    if (cache->p_buffer == NULL) {
        result = ERROR_CODE_INIT_CACHE_FAILED;
    }

    return result;
}

/**
 * 同时更新content的长度和section的长度到缓存中
 *
 * @param cache
 */
void _update_length(struct nlogger_cache_struct *cache) {
    update_cache_section_length(cache->p_section_length, cache->section_length);
    update_cache_content_length(cache->p_content_length, cache->content_length);
}

/**
 * 检查是否符合从缓存写入到日志文件到要求
 */
int _check_can_flush_after_compress(struct nlogger_cache_struct *cache) {
    int result = 0;
    if (cache->cache_mode == NLOGGER_MMAP_CACHE_MODE &&
        cache->content_length >= NLOGGER_FLUSH_MMAP_CACHE_SIZE_THRESHOLD) {
        result = 1;
    } else if (cache->cache_mode == NLOGGER_MEMORY_CACHE_MODE &&
               cache->content_length >= NLOGGER_FLUSH_MEMORY_CACHE_SIZE_THRESHOLD) {
        result = 1;
    }
    return result;
}

int _start_new_section(struct nlogger_data_handler_struct *data_handler, struct nlogger_cache_struct *cache) {
    LOGI("real_write", "start _real_write on new section");
    int init_result = init_zlib(data_handler);
    if (init_result != ERROR_CODE_OK) {
        return init_result;
    }
    //压缩日志段开始，写入开始标志符号，todo 改成用魔数到形式
//    *cache->p_next_write = NLOGGER_MMAP_CACHE_CONTENT_HEAD_TAG;
//    cache->p_next_write += 1;
//    cache->content_length += 1;

    int written_length = add_section_head_tag(cache->p_next_write);
    cache->p_next_write += written_length;
    cache->content_length += written_length;

    //保存 p_section_length 地址，初始化section长度字段
    cache->section_length   = 0;
    cache->p_section_length = cache->p_next_write;

    //移动写入指针，section长度字段（目前占位4byte）
    cache->p_next_write += NLOGGER_CONTENT_SUB_SECTION_LENGTH_BYTE_SIZE;
    cache->content_length += NLOGGER_CONTENT_SUB_SECTION_LENGTH_BYTE_SIZE;
    LOGI("real_write", "finish _real_write on new section");

    return ERROR_CODE_OK;
}


/**
 * 结束一段压缩过程，并且将剩余数据全部都写入到缓存中
 */
int _end_current_section(struct nlogger_data_handler_struct *data_handler, struct nlogger_cache_struct *cache) {
//    if (data_handler->state == NLOGGER_HANDLER_STATE_HANDLING) {
    //结束一次压缩流程，并且更新length，结束的时候会写入一些尾部标志位
    size_t finish_compressed_length = finish_compress_data(data_handler, cache->p_next_write);
    LOGD("finish_section", "finish_compressed_length >>> %zd", finish_compressed_length)
    if (finish_compressed_length < 0) {
        return ERROR_CODE_COMPRESS_FAILED;
    }

    cache->p_next_write += finish_compressed_length;
    cache->section_length += finish_compressed_length;
    cache->content_length += finish_compressed_length;

    //压缩日志段结束，写入结束标志符号，todo 改成用魔数到形式
//    *cache->p_next_write = NLOGGER_MMAP_CACHE_CONTENT_TAIL_TAG;
//    cache->p_next_write += 1;
//    cache->content_length += 1;
//    }

    int written_length = add_section_tail_tag(cache->p_next_write);
    cache->p_next_write += written_length;
    cache->content_length += written_length;

    LOGI("finish_section", "finish_compressed_length end.")
    return ERROR_CODE_OK;
}

/**
 * 写入数据片段到缓存中
 */
int _real_write(struct nlogger_cache_struct *cache, struct nlogger_data_handler_struct *data_handler, char *segment, size_t segment_length) {
    LOGD("real_write", "start _real_write cache->p_next_write >>> %ld", cache->p_next_write);

    //判断是否有初始化，一般情况完成一个section的压缩就要重新初始化
    if (!is_data_handler_init(data_handler)) {
        _start_new_section(data_handler, cache);
        _update_length(cache);
    }


    //加密压缩数据，尽量不去依赖缓存相关数据结构
    size_t compressed_length = compress_and_write_data(data_handler, cache->p_next_write, segment, segment_length);
    if (compressed_length < 0) {
        return ERROR_CODE_COMPRESS_FAILED;
    }

    cache->p_next_write += compressed_length;
    cache->section_length += compressed_length;
    cache->content_length += compressed_length;
    _update_length(cache);

    //判断是否需要flush缓存到日志文件中去
    if (_check_can_flush_after_compress(cache)) {
        LOGD("real_write", "flush cache log to log file.")
        return flush_nlogger();
    }

    //判断是否需要结束一段日志数据段
    if (compressed_length > NLOGGER_COMPRESS_SECTION_THRESHOLD || 1) {
        int finish_result = _end_current_section(data_handler, cache);
        if (finish_result != ERROR_CODE_OK) {
            return finish_result;
        }
        _update_length(cache);
    }

    LOGD("real_write", "finish _real_write cache->p_next_write >>> %ld", cache->p_next_write);
    return ERROR_CODE_OK;
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
 * 写入日志
 */
int write_nlogger(const char *log_file_name, int flag, char *log_content, long long local_time, char *thread_name,
                  long long thread_id, int is_main) {
    if (g_nlogger == NULL || g_nlogger->state == NLOGGER_STATE_ERROR) {
        return ERROR_CODE_NEED_INIT_NLOGGER_BEFORE_ACTION;
    }

    //step1 检查缓存状态
    int check_cache_result = _check_cache_healthy(&g_nlogger->cache);
    if (check_cache_result <= 0) {
        LOGE("write_nlogger", "_check_cache_healthy on error >>> %d", check_cache_result)
        return check_cache_result;
    }
    //step1.1 mmap缓存有问题，避免remain中缓存数据影响新插入的日志，先把它清空
    //todo 封装一个重置data_handler的方法
    if (check_cache_result == ERROR_CODE_CHANGE_CACHE_MODE_TO_MEMORY) {
        g_nlogger->data_handler.remain_data_length = 0;
    }

    //step2 检查日志文件
    int check_log_result = check_log_file_healthy(&g_nlogger->log, log_file_name);

    if (check_log_result <= 0) {
        LOGE("write_nlogger", "_check_log_file_healthy on error >>> %d", check_log_result)
        return check_log_result;
    }

    //step2.1 第一次创建或者打开日志文件，需要重新指定当前mmap缓存指向的日志文件
    //方便下次启动将缓存flush到指定的日志文件中
    if (check_log_result == ERROR_CODE_NEW_LOG_FILE_OPENED) {
        int init_cache_result = _init_cache_on_new_log_file_opened(&g_nlogger->cache, log_file_name);
        if (init_cache_result != ERROR_CODE_OK) {
            return init_cache_result;
        }
    }

    //step3 组装日志
    char *result_json_data;
    int  build_result;
    build_result = malloc_and_build_log_json_data(g_nlogger->cache.cache_mode, flag, log_content, local_time,
                                                  thread_name, thread_id, is_main, &result_json_data);
    if (build_result != ERROR_CODE_OK || result_json_data == NULL) {
        return build_result;
    }
    size_t log_json_data_length = strlen(result_json_data) + 1;
    if (log_json_data_length <= 1) {
        return ERROR_CODE_BUILD_LOG_BLOCK_FAILED;
    }

    LOGI("write", "raw log data size >>> %zd, log data >>> %s ", log_json_data_length, result_json_data)

    //todo 移到其它cache中
    if (g_nlogger->cache.p_next_write == NULL) {
        LOGE("write_nlogger", "###################  UNKNOW ERROR  ##################")
        print_current_nlogger(g_nlogger);
        LOGE("write_nlogger", "###################  UNKNOW ERROR  ##################")
    }

    //step4 分段压缩写入
    int result = _write_data(&g_nlogger->cache, &g_nlogger->data_handler, result_json_data, log_json_data_length);

    return result;
}

/**
 * 重制cache缓存状态
 */
int _reset_nlogger_cache(struct nlogger_cache_struct *cache) {
    cache->content_length = 0;
    cache->p_next_write   = cache->p_content_length + NLOGGER_CONTENT_LENGTH_BYTE_SIZE;
    update_cache_content_length(cache->p_content_length, cache->content_length);

    return ERROR_CODE_OK;
}

/**
 * 解析mmap缓存的header，根据header来
 * 初始化缓存的几个重要指针，以及返回 日志名 指针
 * 
 * @param cache 
 * @param file_name 
 * @return 
 */
int _init_cache_from_mmap_buffer(struct nlogger_cache_struct *cache, char **log_file_name) {
    int error_code = ERROR_CODE_CAN_NOT_PARSE_ON_MEMORY_CACHE_MODE;
    if (cache->cache_mode == NLOGGER_MMAP_CACHE_MODE) {
        //解析日志文件名以及cache header的长度，如果结果大于0代表成功，结果数值就是header的长度
        int parse_result = parse_mmap_cache_head(cache->p_buffer, log_file_name);
        if (parse_result > 0) {
            cache->p_content_length = cache->p_buffer + parse_result;
            //解析日志cache content的长度，如果结果大于0代表成功，结果数值就是content的长度
            parse_result = parse_mmap_cache_content_length(cache->p_content_length);
            if (parse_result > 0) {
                cache->content_length = (unsigned int) parse_result;
                error_code = ERROR_CODE_OK;
            } else {
                LOGE("flush_nlogger", "parse_mmap_cache_content_length on error, error code  >>> %d", parse_result);
                error_code = parse_result;
            }
        } else {
            LOGE("flush_nlogger", "parse_mmap_cache_header on error, error code >>> %d", parse_result);
            error_code = parse_result;
        }
    }

    return error_code;
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
        _update_length(&g_nlogger->cache);
    }

    //step2 检查日志文件, 在没有文件名的情况下去flush的时候需要从mmap中获取文件名然后再打开
    //（这一步是为了兼容init的时候进行初始化的操作）
    int  malloc_file_name_by_parse_mmap = 0; //标记是否有为 file 申请过内存
    char *file_name                     = NULL;
    int  file_name_configured           = is_log_file_name_valid(&g_nlogger->log) == ERROR_CODE_OK;
    //step2.1 获取缓存对应到日志文件名
    if (!file_name_configured && g_nlogger->cache.cache_mode == NLOGGER_MMAP_CACHE_MODE) {
        int init_result = _init_cache_from_mmap_buffer(&g_nlogger->cache, &file_name);
        if (init_result != ERROR_CODE_OK) {
            return init_result;
        }
        malloc_file_name_by_parse_mmap = 1;
    } else if (file_name_configured) {
        file_name = current_log_file_name(&g_nlogger->log);
    } else {
        //到目前为止没有能获取到文件名的方法
        //没有文件名的同时，也不是mmap缓存，通常是init过后采用内存缓存策略的同时去flush了
        //因此没有本地缓存，flush方法也没有意义
        return ERROR_CODE_UNEXPECT_STATE_WHEN_ON_FLUSH;
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
    //todo 返回地址  返回长度
    char *cache_content = g_nlogger->cache.p_content_length + NLOGGER_CONTENT_LENGTH_BYTE_SIZE;

    LOGD("flush", "mmap buffer addr=%ld , content addr=%ld .", g_nlogger->cache.p_buffer, cache_content);

    flush_cache_to_log_file(&g_nlogger->log, cache_content, g_nlogger->cache.content_length);

    //step4 重制状态，为下次写入日志数据做准备
    _reset_nlogger_cache(&g_nlogger->cache);

    print_current_nlogger(g_nlogger);

    return ERROR_CODE_OK;
}

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
    LOGW("debug", "log.file_length = %ld", g_nlogger->log.file_length);
    _get_string_data(g_nlogger->log.p_path, &result);
    LOGW("debug", "log.p_path = %s", result);
    _get_string_data(g_nlogger->log.p_name, &result);
    LOGW("debug", "log.p_name = %s", result);
    _get_string_data(g_nlogger->log.p_dir, &result);
    LOGW("debug", "log.p_dir = %s", result);
    LOGW("debug", "<<<<<<<<<<<<<<<<  g_nlogger  <<<<<<<<<<<<<<<<");
    LOGW("debug", "\n");
}



