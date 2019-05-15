/*
 * Created by fgd on 2019/4/28.
 *
 * 日志库主文件，带下划线前缀的未私有方法，功能尽量独立，不引用全局的数据
 */

#include <malloc.h>
#include <sys/stat.h>
#include <cJSON.h>
#include <sys/mman.h>
#include "nlogger.h"
#include "nlogger_android_log.h"
#include "nlogger_error_code.h"
#include "nlogger_constants.h"
#include "nlogger_file_utils.h"
#include "nlogger_cache.h"
#include "nlogger_json_util.h"
#include "nlogger_utils.h"


static nlogger *g_nlogger;
static char    *g_null_string = "NULL";

void _get_string_data(char *data, char **result) {
    if (is_empty_string(data)) {
        *result = g_null_string;
        return;
    }
    *result = data;
}

void _print_current_nlogger() {
    if (g_nlogger == NULL) {
        return;
    }
    char *result;
    LOGD("debug", "\n");
    LOGD("debug", ">>>>>>>>>>>>>>>>  g_nlogger  >>>>>>>>>>>>>>>>");
    LOGD("debug", "cache.cache_mod = %d", g_nlogger->cache.cache_mode);
    LOGD("debug", "cache.length = %d", g_nlogger->cache.length);
    LOGD("debug", "cache.content_length = %d", g_nlogger->cache.content_length);
    _get_string_data(g_nlogger->cache.p_file_path, &result);
    LOGD("debug", "cache.p_file_path = %s", result);
    _get_string_data(g_nlogger->cache.p_buffer, &result);
    LOGD("debug", "cache.p_buffer = %s", result);
    LOGD("debug", "================================================");
    LOGD("debug", "log.state = %d", g_nlogger->log.state);
    LOGD("debug", "log.file_length = %ld", g_nlogger->log.file_length);
    _get_string_data(g_nlogger->log.p_path, &result);
    LOGD("debug", "log.p_path = %s", result);
    _get_string_data(g_nlogger->log.p_name, &result);
    LOGD("debug", "log.p_name = %s", result);
    _get_string_data(g_nlogger->log.p_dir, &result);
    LOGD("debug", "log.p_dir = %s", result);
    LOGD("debug", "<<<<<<<<<<<<<<<<  g_nlogger  <<<<<<<<<<<<<<<<");
    LOGD("debug", "\n");
}

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
 * 创建日志文件存放的目录（不包含日志文件名）
 */
int _create_log_file_dir(const char *log_file_dir, char **result_dir) {
    if (is_empty_string(log_file_dir)) {
        return ERROR_CODE_ILLEGAL_ARGUMENT;
    }
    int appendSeparate = 0;
    if (log_file_dir[strlen(log_file_dir)] != '/') {
        appendSeparate = 1;
    }
    size_t path_string_length = strlen(log_file_dir) +
                                (appendSeparate ? strlen("/") : 0) +
                                strlen(NLOGGER_CACHE_DIR) +
                                strlen("\0");
    *result_dir = malloc(path_string_length);
    if (*result_dir == NULL) {
        return ERROR_CODE_MALLOC_LOG_FILE_DIR_STRING;
    }
    memset(*result_dir, 0, path_string_length);
    memcpy(*result_dir, log_file_dir, strlen(log_file_dir));
    if (appendSeparate) {
        strcat(*result_dir, "/");
    }
    LOGD("init", "create log file dir >>> %s ", *result_dir);
    return makedir_nlogger(*result_dir);
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
    char *final_log_file_dir;
    _create_log_file_dir(log_file_dir, &final_log_file_dir);
    LOGD("init", "finish create log file dir >>> %s ", final_log_file_dir)
    g_nlogger->log.p_dir = final_log_file_dir;

    //配置加密相关的参数
    char *temp_encrypt_key = malloc(sizeof(char) * 16);
    memcpy(temp_encrypt_key, encrypt_key, sizeof(char) * 16);
    g_nlogger->data_handler.p_encrypt_key = temp_encrypt_key;
    char *temp_encrypt_iv = malloc(sizeof(char) * 16);
    memcpy(temp_encrypt_iv, encrypt_iv, sizeof(char) * 16);
    g_nlogger->data_handler.p_encrypt_iv         = temp_encrypt_iv;
    g_nlogger->data_handler.p_encrypt_iv_pending = malloc(sizeof(char) * 16);
    //初始化缓存文件，首先采用mmap缓存，失败则用内存缓存
    int init_cache_result = init_cache_nlogger(g_nlogger->cache.p_file_path, &g_nlogger->cache.p_buffer);

    if (init_cache_result == NLOGGER_INIT_CACHE_FAILED) {
        g_nlogger->state = NLOGGER_STATE_ERROR;
    } else {
        //标记当前初始化阶段成功，避免重复初始化
        g_nlogger->state            = NLOGGER_STATE_INIT;
        g_nlogger->cache.cache_mode = init_cache_result;
    }

    LOGD("init", "init cache result >>> %d.", init_cache_result);

    //todo try flush cache log
    return ERROR_CODE_OK;
}

/**
 * 尝试在日志文件不存的时候创建日志文件，如果日志文件存在的话，则关闭之前的日志文件并且flush，
 *
 */
int _open_log_file(struct nlogger_log_struct *log) {
    if (!is_file_exist_nlogger(log->p_dir)) {
        makedir_nlogger(log->p_dir);
    }
    FILE *log_file = fopen(log->p_path, "ab+");
    if (log_file != NULL) {
        log->p_file = log_file;
        fseek(log_file, 0, SEEK_END);
        log->file_length = ftell(log_file);
        LOGD("check", "open log file %s  success.", log->p_path);
        log->state = NLOGGER_LOG_STATE_OPEN;
    } else {
        LOGW("check", "open log file %s failed.", log->p_path);
        log->state = NLOGGER_LOG_STATE_CLOSE;
        return ERROR_CODE_OPEN_LOG_FILE_FAILED;
    }
    return ERROR_CODE_OK;
}

int _set_log_file_name(struct nlogger_log_struct *log, const char *log_file_name) {
    size_t file_name_size = strlen(log_file_name);
    size_t file_dir_size  = strlen(log->p_dir);
    size_t end_tag_size   = 1;
    LOGD("check", "log_file_name >>> %s .", log->p_dir);

    LOGD("check", "log_file_name >>> %s .", log_file_name);

    LOGD("check", "file_name_size >>> %zd , file_dir_size >>> %zd , final_file_name >>> %zd.", file_name_size,
         file_dir_size, file_name_size + end_tag_size);

    log->p_name = malloc(file_name_size + end_tag_size);
    if (log->p_name != NULL) {
        memset(log->p_name, 0, file_name_size + end_tag_size);
        memcpy(log->p_name, log_file_name, file_name_size);
        LOGD("check", "### size of log->p_name >>> %zd .", strlen(log->p_name));

        LOGD("check", "### log->p_name >>> %s .", log->p_name);
    } else {
        return ERROR_CODE_MALLOC_LOG_FILE_NAME_STRING;
    }

    log->p_path = malloc(file_dir_size + file_name_size + end_tag_size);
    if (log->p_path != NULL) {
        memset(log->p_path, 0, file_dir_size + file_name_size + end_tag_size);
        memcpy(log->p_path, log->p_dir, file_dir_size);
        memcpy(log->p_path + file_dir_size, log->p_name, file_name_size);
    } else {
        return ERROR_CODE_MALLOC_LOG_FILE_PATH_STRING;
    }
    LOGD("check", "_config_log_file_name finish. log file name >>> %s, log file path >>> %s.", log->p_name,
         log->p_path);
    return ERROR_CODE_OK;
}

/**
 * 映射文件名到mmap文件中
 * 如果将文件名写入到mmap cache文件中失败，则采取内存映射（mmap的优势已经没有了，如果重新打开mmap中的内容不知道恢复到哪里去）
 */
int _write_mmap_header(struct nlogger_cache_struct *cache, const char *log_file_name) {
    int result = ERROR_CODE_MAP_LOG_NAME_TO_CACHE_FAILED;
    if (cache->cache_mode == NLOGGER_MMAP_CACHE_MODE) {
        cJSON            *root        = cJSON_CreateObject();
        json_map_nlogger *map         = create_json_map_nlogger();
        char             *result_json = NULL;
        if (root != NULL) {
            if (map != NULL) {
                add_item_number_nlogger(map, NLOGGER_MMAP_CACHE_HEADER_KEY_VERSION, NLOGGER_VERSION);
                add_item_number_nlogger(map, NLOGGER_MMAP_CACHE_HEADER_KEY_DATE, system_current_time_millis_nlogger());
                add_item_string_nlogger(map, NLOGGER_MMAP_CACHE_HEADER_KEY_FILE, log_file_name);
                inflate_json_by_map_nlogger(root, map);
                result_json = cJSON_PrintUnformatted(root);
            }

            if (result_json != NULL) {
                LOGD("mmap_header", "build json finished. header >>> %s ", result_json);
                size_t content_length = strlen(result_json) + 1;
                char   *mmap_buffer   = cache->p_buffer;
                *mmap_buffer = NLOGGER_MMAP_CACHE_HEADER_HEAD_TAG;
                mmap_buffer++;
                *mmap_buffer = content_length;
                mmap_buffer++;
                *mmap_buffer = content_length >> 8;
                mmap_buffer++;
                memcpy(mmap_buffer, result_json, content_length);
                mmap_buffer = mmap_buffer + content_length;
                *mmap_buffer = NLOGGER_MMAP_CACHE_HEADER_TAIL_TAG;
                mmap_buffer++;
                cache->p_length = mmap_buffer;
                //这里是否有必要释放内存
                free(result_json);
                result = ERROR_CODE_OK;
            }
            cJSON_Delete(root);
        }
        if (map != NULL) {
            delete_json_map_nlogger(map);
        }
    }
    return result;
}

/**
 * 检查日志文件的状态，如果当次写入和上次的不一致则需要关闭原来的文件，创建新的文件流
 */
int _check_log_file_healthy(struct nlogger_log_struct *log, const char *log_file_name) {
    LOGD("check", "#### start check log file !!!");
//    这一步放在外面来判断，也就是write的地方
//    if (g_nlogger == NULL || g_nlogger->state == NLOGGER_STATE_ERROR) {
//        return ERROR_CODE_NEED_INIT_NLOGGER_BEFORE_ACTION;
//    }
    if (is_empty_string(log_file_name)) {
        return ERROR_CODE_ILLEGAL_ARGUMENT;
    }

    //检查之前保存的日志文件名
    if (is_empty_string(log->p_name)) {
        /*第一次创建，之前没有使用过，这个时候应该在init的时候flush过一次，因此不再需要flush*/
        int result_code = _set_log_file_name(log, log_file_name);
        //检查创建是否成功，如果path 或者 name创建失败则直接返回；
        if (result_code != ERROR_CODE_OK) {
            LOGW("check", "first malloc file string on error >>> %d .", result_code);
            return result_code;
        }
        LOGD("check", "first config log file success log file path >>> %s.", log->p_path);
    } else if (strcmp(log->p_name, log_file_name) != 0) {
        /*非第一次创建，之前已经有一个已经打开的日志文件，并且日志文件是打开状态*/
        if (log->state == NLOGGER_LOG_STATE_OPEN &&
            log->p_file != NULL) {
            //如果当前是打开状态，则尝试关闭，这里是否需要flush
            //todo try flush cache log
            //释放资源
            fclose(log->p_file);
            free(log->p_name);
            free(log->p_path);
            log->state = NLOGGER_LOG_STATE_CLOSE;
        }
        //如果日志文件的目录都有问题说明需要重新初始化
        if (log->p_dir == NULL) {
            LOGW("check", "log file dir is empty on check.");
            return ERROR_CODE_NEED_INIT_NLOGGER_BEFORE_ACTION;
        }
        int result_code = _set_log_file_name(log, log_file_name);
        //检查创建是否成功，如果path 或者 name创建失败则直接返回；
        if (result_code != ERROR_CODE_OK) {
            LOGW("check", "malloc file string on error >>> %d .", result_code);
            return result_code;
        }
        LOGD("check", "close last once and new create log file, config success %s .", log->p_path);
    } else {
        /*表示当前写入的日志文件和上次写入的日志文件是一致的*/
        if (!is_empty_string(log->p_path) &&
            is_file_exist_nlogger(log->p_path) &&
            log->p_file != NULL &&
            log->state == NLOGGER_LOG_STATE_OPEN) {
            //说明当前已经打开的日志文件和之前的是一致的
            //并且日志文件已经是打开状态，这个时候什么事都不用做
            LOGD("check", "same of last once log file config, success %s .", log->p_path);
            return ERROR_CODE_OK;
        } else {
            int temp_error_code = 0;
            if (log->p_name != NULL) {
                temp_error_code |= 0x0001;
                free(log->p_name);
            }
            if (log->p_path != NULL) {
                temp_error_code |= 0x0010;
                free(log->p_path);
            }
            if (log->p_file != NULL) {
                temp_error_code |= 0x0100;
                fclose(log->p_file);
            }
            if (log->state == NLOGGER_LOG_STATE_CLOSE) {
                temp_error_code |= 0x1000;
            }
            log->state = NLOGGER_LOG_STATE_CLOSE;
            LOGE("check", "on error log same as last once, but state invalid. state >>> %d", temp_error_code);
            //初始化状态重新调用一次，表示当前日志文件状态有问题重新配置日志文件
            _check_log_file_healthy(log, log_file_name);
        }
    }

    //如果是关闭状态，则检查是否需要创建目录，如果是打开状态则肯定已经做了日志文件长度这些状态的记录
    if (log->state == NLOGGER_LOG_STATE_CLOSE) {
        int result = _open_log_file(log);
        if (result == ERROR_CODE_OK) {
            //表示第一次打开日志文件
            return ERROR_CODE_NEW_LOG_FILE_OPENED;
        }
        //打开文件失败的情况
        return result;
    }

    return ERROR_CODE_OK;
}

int _init_cache_on_new_log_file_opened(const char *log_file_name, struct nlogger_cache_struct *cache) {
    if (cache->cache_mode == NLOGGER_MMAP_CACHE_MODE) {
        //写入日志头，指定当前mmap文件映射的log file name
        int map_result = _write_mmap_header(cache, log_file_name);
        //写入失败则切换成Memory缓存模式
        //（不能建立关系就失去了意外中断导致日志丢失的优势，这和内存缓存没有区别）
        if (map_result != ERROR_CODE_OK) {
            if (cache->p_buffer != NULL) {
                munmap(cache->p_buffer, NLOGGER_MMAP_CACHE_SIZE);
            }
            //路径传空，强制Memory模式缓存
            if (NLOGGER_INIT_CACHE_FAILED == init_cache_nlogger(NULL, &cache->p_buffer)) {
                return ERROR_CODE_INIT_CACHE_FAILED;
            }
        }
    }
    //mmap模式会在完成映射关系（写入file name）后指定代表content长度指针的位置
    //memory模式需要手动指定代表长度的指针，也就是内存缓存的开头地址
    if (cache->cache_mode == NLOGGER_MEMORY_CACHE_MODE) {
        cache->p_length = cache->p_buffer;
    }

    cache->p_next_write   = cache->p_length + NLOGGER_CONTENT_LENGTH_BYTE_SIZE;
    cache->length         = 0;
    cache->content_length = 0;

    return ERROR_CODE_OK;
}

int _check_cache_healthy(struct nlogger_cache_struct *cache) {
    int result = ERROR_CODE_OK;
    if (cache->cache_mode == NLOGGER_MMAP_CACHE_MODE &&
        !is_file_exist_nlogger(cache->p_file_path)) {
        //mmap缓存文件不见了，这个时候主动切换成Memory缓存
        if (cache->p_buffer != NULL) {
            munmap(cache->p_buffer, NLOGGER_MMAP_CACHE_SIZE);
        }
        //路径传空，强制Memory缓存
        if (NLOGGER_INIT_CACHE_FAILED == init_cache_nlogger(NULL, &cache->p_buffer)) {
            return ERROR_CODE_INIT_CACHE_FAILED;
        }
        cache->p_length       = cache->p_buffer;
        cache->p_next_write   = cache->p_length + NLOGGER_CONTENT_LENGTH_BYTE_SIZE;
        cache->length         = 0;
        cache->content_length = 0;
        result = ERROR_CODE_CHANGE_CACHE_MODE_TO_MEMORY;
    }

    //最后检查一下缓存指针是否存在，如果不存在则直接报错返回
    if (cache->p_buffer == NULL) {
        result = ERROR_CODE_INIT_CACHE_FAILED;
    }

    return result;
}

/**
 * 申请内存并且组装一个json数据
 *
 * @param cache_type 缓存的类型 1:MMAP 2:MEMORY
 * @param flag 日志类型
 * @param log_content 日志内容
 * @param local_time 日志生成的时间
 * @param thread_name 线程的名称
 * @param thread_id 线程id
 * @param result_json_data 用于返回数据的指针
 *
 * @return 数据的长度
 */
size_t _malloc_and_build_json_data(int cache_type, int flag, char *log_content, long long local_time,
                                   char *thread_name, long long thread_id, int is_main, char **result_json_data) {
    size_t result_size = 0;

    cJSON            *root        = cJSON_CreateObject();
    json_map_nlogger *map         = create_json_map_nlogger();
    char             *result_json = NULL;
    if (root != NULL) {
        if (map != NULL) {
            add_item_number_nlogger(map, NLOGGER_PROTOCOL_KEY_CACHE_TYPE, cache_type);
            add_item_number_nlogger(map, NLOGGER_PROTOCOL_KEY_FLAG, flag);
            add_item_string_nlogger(map, NLOGGER_PROTOCOL_KEY_CONTENT, log_content);
            add_item_number_nlogger(map, NLOGGER_PROTOCOL_KEY_LOCAL_TIME, local_time);
            add_item_string_nlogger(map, NLOGGER_PROTOCOL_KEY_THREAD_NAME, thread_name);
            add_item_number_nlogger(map, NLOGGER_PROTOCOL_KEY_THREAD_ID, thread_id);
            add_item_number_nlogger(map, NLOGGER_PROTOCOL_KEY_MAIN_THREAD, is_main);
            inflate_json_by_map_nlogger(root, map);
            result_json = cJSON_PrintUnformatted(root);
        }

        if (result_json != NULL) {
            size_t content_length = strlen(result_json);
            size_t final_length   = content_length + 1; //加上末尾的换行符\n
            char   *log_data      = malloc(final_length);
            memset(log_data, 0, final_length);
            memcpy(log_data, result_json, content_length);
            memcpy(log_data + content_length, "\n", 1);
            *result_json_data = log_data;
            //这里是否有必要释放内存
            free(result_json);
            result_size = content_length;
        }
        cJSON_Delete(root);
    }
    if (map != NULL) {
        delete_json_map_nlogger(map);
    }

    return result_size;
}

int _prepare_compress_data(struct nlogger_data_handler_struct *data_handler) {

    if (data_handler->p_stream == NULL) {
        data_handler->p_stream = malloc(sizeof(z_stream));
    }

    if (data_handler->p_stream == NULL) {
        return ERROR_CODE_INIT_HANDLER_MALLOC_STREAM_FAILED;
    }

    memset(data_handler->p_stream, 0, sizeof(z_stream));
    data_handler->p_stream->zalloc = Z_NULL;
    data_handler->p_stream->zfree  = Z_NULL;
    data_handler->p_stream->opaque = Z_NULL;
    int zlib_init_result = deflateInit2(
            data_handler->p_stream, //z_stream
            Z_BEST_COMPRESSION, // gives best compression压缩模式
            Z_DEFLATED, // It must be Z_DEFLATED in this version of the library 写死的不用管
            (15 + 16), //不知道干啥用的 默认15，16会加上与i个header
            8,//默认值表明会使用较大的内存来提高速度
            Z_DEFAULT_STRATEGY//压缩算法，影响加密比率
    );
    if (zlib_init_result != Z_OK) {
        return ERROR_CODE_INIT_HANDLER_INIT_STREAM_FAILED;
    }

    data_handler->remain_data_length = 0;
    //初始化填充iv
    memcpy(data_handler->p_encrypt_iv_pending, data_handler->p_encrypt_iv, 16);
    return ERROR_CODE_OK;
}

size_t _aes_encrypt(struct nlogger_data_handler_struct *data_handler, char *destination, unsigned char *data,
                    size_t data_length) {
    size_t        handled               = 0;
    size_t        unencrypt_data_length = data_length + data_handler->remain_data_length;
    size_t        handle_data_length    = (unencrypt_data_length / NLOGGER_AES_ENCRYPT_UNIT) * (size_t) NLOGGER_AES_ENCRYPT_UNIT;
    size_t        remain_data_length    = unencrypt_data_length % (size_t) NLOGGER_AES_ENCRYPT_UNIT;
//    char          *curr                 = destination;
    unsigned char handle_data[handle_data_length];
    unsigned char *next_copy_point      = handle_data;

    if (handle_data_length) {
        size_t copy_data_length = handle_data_length - data_handler->remain_data_length;
        if (data_handler->remain_data_length) {
            memcpy(next_copy_point, data_handler->p_remain_data, data_handler->remain_data_length);
            next_copy_point += data_handler->remain_data_length;
        }
        memcpy(next_copy_point, data, copy_data_length);
        LOGE("encrypt", "write data >>> %zd", handle_data_length)
        //todo encrypt
        memcpy(destination, handle_data, handle_data_length);
        handled += handle_data_length;
//        curr += handle_data_length;
    }


    if (remain_data_length) {
        if (handle_data_length) {
            size_t copied_data_length = handle_data_length - data_handler->remain_data_length;
            next_copy_point = data;
            next_copy_point += copied_data_length;
            memcpy(data_handler->p_remain_data, next_copy_point, remain_data_length);
        } else {
            next_copy_point = (unsigned char *) data_handler->p_remain_data;
            next_copy_point += data_handler->remain_data_length;
            memcpy(next_copy_point, data, remain_data_length);
        }
        data_handler->remain_data_length = remain_data_length;
//
//        memcpy(curr, data_handler->p_remain_data, remain_data_length);
//        handled += remain_data_length;
    }

    return handled;
}

/**
 * gzip压缩 + aes加密
 * todo 把加密逻辑抽离出来
 *
 * @param data_handler
 * @param destination
 * @param source
 * @param source_length
 * @param type
 * @return
 */
size_t _zlib_compress_and_aes_encrypt(struct nlogger_data_handler_struct *data_handler, char *destination, char *source,
                                      size_t source_length, int type) {
    size_t   handled = 0;
    z_stream *stream = data_handler->p_stream;

    unsigned char out[NLOGGER_ZLIB_COMPRESS_CHUNK_SIZE];
    unsigned int  have;
//    if (type == Z_FINISH) {
//        stream->avail_in = NULL;
//        stream->next_in  = 0;
//    }else{
    stream->avail_in = (uInt) source_length;
    stream->next_in  = (Bytef *) source;
//    }
    do {
        stream->avail_out = NLOGGER_ZLIB_COMPRESS_CHUNK_SIZE;
        stream->next_out  = out;
        int def_res = deflate(stream, type);
        if (Z_STREAM_ERROR == def_res) {
            //这里如果出现错误会导致日志错位，日志截断，怎么主动发现这个错误的日志
            deflateEnd(stream);
            data_handler->state = NLOGGER_HANDLER_STATE_IDLE;
            return (size_t) 0;
        }
        have = NLOGGER_ZLIB_COMPRESS_CHUNK_SIZE - stream->avail_out;
        handled += _aes_encrypt(data_handler, destination, out, have);
//        memcpy(destination, out, have);
        destination += have;
//        handled += have;
    } while (0 == stream->avail_out);

    return handled;
}

/**
 * 压缩数据加密
 *
 * @param data_handler
 * @param destination 写入的目的地指针
 * @param source 数据源指针
 * @param length 源数据长度
 *
 * @return 写入数据的长度
 */
size_t _compress_and_write_data(struct nlogger_data_handler_struct *data_handler, char *destination, char *source, size_t length) {

    size_t handled = 0;
    //todo 压缩数据 加密数据 不一定所有数据都会写入，会有一小部分数据缓存等待下一次写入
    handled = _zlib_compress_and_aes_encrypt(data_handler, destination, source, length, Z_SYNC_FLUSH);
    LOGW("compress_data", "after compress_and_write_data, handled >>> %zd", handled);
    data_handler->state = NLOGGER_HANDLER_STATE_HANDLING;
    return handled;
}

/**
 * 结束压缩数据
 *
 * @param data_handler
 * @param destination 写入的目的地指针
 *
 * @return 写入数据的长度
 */
size_t _finish_compress_data(struct nlogger_data_handler_struct *data_handler, char *destination) {
    size_t handled = 0;
    handled = _zlib_compress_and_aes_encrypt(data_handler, destination, NULL, 0, Z_FINISH);
    deflateEnd(data_handler->p_stream);
    destination += handled;
    if (data_handler->remain_data_length > 0) {
        LOGD("finish_compress", "push remain data to cache remain_length >>> %zd", data_handler->remain_data_length)
        char remain[NLOGGER_AES_ENCRYPT_UNIT];
        memset(remain, '6', NLOGGER_AES_ENCRYPT_UNIT);
        memcpy(remain, data_handler->p_remain_data, data_handler->remain_data_length);
        //todo encrypt data
        memcpy(destination, remain, NLOGGER_AES_ENCRYPT_UNIT);
        data_handler->remain_data_length = 0;
        handled += NLOGGER_AES_ENCRYPT_UNIT;
    }
    //todo 关于remain_data写入以及delete_end等收尾工作
    return handled;
}


/**
 * 是否有必要大头
 *
 * @param p_content_length
 * @param length
 * @return
 */
size_t _update_content_length(char *p_content_length, unsigned int length) {
    size_t handled = 0;
    // 为了兼容java,采用高字节序
    *p_content_length = length >> 24;
    p_content_length++;
    handled++;
    *p_content_length = length >> 16;
    p_content_length++;
    handled++;
    *p_content_length = length >> 8;
    p_content_length++;
    handled++;
    *p_content_length = length;
    handled++;
    LOGD("up_cont_len", "length >>> %d ", length);
    return handled;
}
//
///**
// * 初始化日志，标志开始一次压缩数据
// *
// * 插入 head tag
// * 记录 p_content_length 指针
// *
// * @return
// */
//int _write_section_head(struct nlogger_cache_struct *cache) {
//
//    *(cache->p_next_write) = NLOGGER_MMAP_CACHE_CONTENT_HEAD_TAG;
//    cache->p_next_write += 1;
//
//    cache->content_length   = 0;
//    cache->p_content_length = cache->p_next_write;
//
//    //第一次写入长度字段（目前占位4byte）
//    cache->p_next_write += NLOGGER_CONTENT_SUB_SECTION_LENGTH_BYTE_SIZE;
//
//    return ERROR_CODE_OK;
//}
//
//
///**
// * 结束一次压缩数据
// *
// * 在末尾插入解为标志符号
// * 更新content_length到p_content_length指针处
// *
// * @return
// */
//int _write_section_tail(struct nlogger_cache_struct *cache) {
//    *cache->p_next_write = NLOGGER_MMAP_CACHE_CONTENT_TAIL_TAG;
//    cache->p_next_write += 1;
//    return ERROR_CODE_OK;
//}

/**
 * 检查是否符合从缓存写入到日志文件到要求
 */
int _check_can_flush_after_compress(struct nlogger_cache_struct *cache) {
    int result = 0;
    if (cache->cache_mode == NLOGGER_MMAP_CACHE_MODE &&
        cache->length >= NLOGGER_FLUSH_MMAP_CACHE_SIZE_THRESHOLD) {
        result = 1;
    } else if (cache->cache_mode == NLOGGER_MEMORY_CACHE_MODE &&
               cache->length >= NLOGGER_FLUSH_MEMORY_CACHE_SIZE_THRESHOLD) {
        result = 1;
    }
    return result;
}

/**
 * 检查是否需要结束一段压缩数据
 */
int _check_can_end_section(struct nlogger_data_handler_struct *data_handler, size_t compressed_data) {
    int result = 1;
    if (data_handler->state == NLOGGER_HANDLER_STATE_HANDLING &&
        compressed_data > NLOGGER_COMPRESS_SECTION_THRESHOLD) {
        result = 1;
    }
    return result;
}

int _try_start_new_section(struct nlogger_data_handler_struct *data_handler, struct nlogger_cache_struct *cache) {
    if (data_handler->state == NLOGGER_HANDLER_STATE_IDLE) {
        //如果没有初始化，就先初始化一下
        int init_result = _prepare_compress_data(data_handler);
        if (init_result != ERROR_CODE_OK) {
            return init_result;
        }

        //压缩日志段开始，写入开始标志符号，todo 改成用魔数到形式
        *cache->p_next_write = NLOGGER_MMAP_CACHE_CONTENT_HEAD_TAG;
        cache->p_next_write += 1;
        cache->length += 1;

        //保存 p_content_length 地址，初始化section长度字段
        cache->content_length   = 0;
        cache->p_content_length = cache->p_next_write;
        _update_content_length(cache->p_content_length, cache->content_length);

        //移动写入指针，section长度字段（目前占位4byte）
        cache->p_next_write += NLOGGER_CONTENT_SUB_SECTION_LENGTH_BYTE_SIZE;
        cache->length += NLOGGER_CONTENT_SUB_SECTION_LENGTH_BYTE_SIZE;

        data_handler->state = NLOGGER_HANDLER_STATE_INIT;
    }
    return ERROR_CODE_OK;
}


/**
 * 结束一段压缩过程，并且将剩余数据全部都写入到缓存中
 */
int _try_end_section(struct nlogger_data_handler_struct *data_handler, struct nlogger_cache_struct *cache) {
    if (data_handler->state == NLOGGER_HANDLER_STATE_HANDLING) {
        //结束一次压缩流程，并且更新length，结束的时候会写入一些尾部标志位
        size_t finish_compressed_length = _finish_compress_data(data_handler, cache->p_next_write);
        cache->p_next_write += finish_compressed_length;
        cache->content_length += finish_compressed_length;
        cache->length += finish_compressed_length;
        _update_content_length(cache->p_content_length, cache->content_length);

        //压缩日志段结束，写入结束标志符号，todo 改成用魔数到形式
        *cache->p_next_write = NLOGGER_MMAP_CACHE_CONTENT_TAIL_TAG;
        cache->p_next_write += 1;
        cache->length += 1;

        data_handler->state = NLOGGER_HANDLER_STATE_IDLE;
    }
    return ERROR_CODE_OK;
}

/**
 * 写入数据片段到缓存中
 */
int _real_write(struct nlogger_cache_struct *cache, struct nlogger_data_handler_struct *data_handler, char *segment,
                size_t segment_length) {

    _try_start_new_section(data_handler, cache);
    LOGD("real_write", "start _real_write cache->p_next_write >>> %ld", cache->p_next_write);
    //加密压缩数据，尽量不去依赖缓存相关数据结构
    size_t compressed_length = _compress_and_write_data(data_handler, cache->p_next_write, segment, segment_length);
    if (compressed_length < 0) {
        return ERROR_CODE_COMPRESS_FAILED;
    }

    cache->p_next_write += compressed_length;
    cache->content_length += compressed_length;
    cache->length += compressed_length;
    _update_content_length(cache->p_content_length, cache->content_length);

    //判断是否需要flush缓存到日志文件中去
    if (_check_can_flush_after_compress(cache)) {
        LOGD("real_write", "flush cache log to log file.")
        return flush_nlogger();
    }

    //判断是否需要结束一段日志数据段
    if (_check_can_end_section(data_handler, compressed_length)) {
        _try_end_section(data_handler, cache);
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

int write_nlogger(const char *log_file_name, int flag, char *log_content, long long local_time, char *thread_name,
                  long long thread_id, int is_main) {
    if (g_nlogger == NULL || g_nlogger->state == NLOGGER_STATE_ERROR) {
        return ERROR_CODE_NEED_INIT_NLOGGER_BEFORE_ACTION;
    }

    //step1 检查日志文件
    int check_log_result = _check_log_file_healthy(&g_nlogger->log, log_file_name);

    if (check_log_result <= 0) {
        return check_log_result;
    }

    //第一次创建或者打开日志文件，需要重新指定当前mmap缓存指向的日志文件，方便下次启动将缓存flush到指定的日志文件中
    if (check_log_result == ERROR_CODE_NEW_LOG_FILE_OPENED) {
        int init_cache_result = _init_cache_on_new_log_file_opened(log_file_name, &g_nlogger->cache);
        if (init_cache_result != ERROR_CODE_OK) {
            return init_cache_result;
        }
    }

    //step2 检查缓存状态
    int check_cache_result = _check_cache_healthy(&g_nlogger->cache);
    if (check_cache_result <= 0) {
        return check_cache_result;
    }

    //mmap缓存有问题，避免remain中缓存数据影响新插入的日志，先把它清空
    if (check_cache_result == ERROR_CODE_CHANGE_CACHE_MODE_TO_MEMORY) {
        g_nlogger->data_handler.remain_data_length = 0;
    }

    //test debug
    _print_current_nlogger();

    //step3 组装日志
    char   *result_json_data;
    size_t data_size       = _malloc_and_build_json_data(g_nlogger->cache.cache_mode, flag, log_content, local_time,
                                                         thread_name, thread_id, is_main, &result_json_data);
    if (data_size == 0 || result_json_data == NULL) {
        return ERROR_CODE_BUILD_LOG_BLOCK_FAILED;
    }

    LOGI("write", "raw log data size >>> %zd ", data_size);
    LOGI("write", "raw log data >>> %s ", result_json_data)

    //step4 分段压缩写入
    int result = _write_data(&g_nlogger->cache, &g_nlogger->data_handler, result_json_data, data_size);

    return ERROR_CODE_OK;
}


int _flush_cache_to_file(char *cache, size_t cache_length, FILE *log_file) {
    //todo 写入缓存到日志文件中


}

int _reset_cache_and_data_handler();

/**
 * 解析mmap缓存文件
 *
 * @return
 */
int _parse_mmap_cache_head(struct nlogger_cache_struct *cache, char **file_name) {
    char length_array[4];
    memset(length_array, 0, 4);
    char *mmap_buffer = cache->p_buffer;
    //检查mmap缓存地址是否有效
    if (mmap_buffer == NULL) {
        return ERROR_CODE_INVALID_BUFFER_POINT_ON_PARSE_MMAP_HEADER;
    }
    //检查协议头是否正确
    if (*mmap_buffer != NLOGGER_MMAP_CACHE_HEADER_HEAD_TAG) {
        return ERROR_CODE_INVALID_HEAD_OR_TAIL_TAG_ON_PARSE_MMAP_HEADER;
    }
    mmap_buffer++;
    length_array[0] = *mmap_buffer;
    mmap_buffer++;
    length_array[1] = *mmap_buffer;
    mmap_buffer++;
    adjust_byte_order_nlogger(length_array);
    size_t header_length = (size_t) *((int *) length_array);
    LOGD("mmap_header", "parse get header length >>> %zd", header_length)

    //检查头的大小是否有效（是否有必要给头加上上限）
    if (header_length < 0 || header_length > NLOGGER_MMAP_CACHE_MAX_HEADER_CONTENT_SIZE) {
        return ERROR_CODE_INVALID_HEADER_LENGTH_ON_PARSE_MMAP_HEADER;
    }
    mmap_buffer += header_length;
    //检查协议尾部是否正确
    if (*mmap_buffer != NLOGGER_MMAP_CACHE_HEADER_TAIL_TAG) {
        return ERROR_CODE_INVALID_HEAD_OR_TAIL_TAG_ON_PARSE_MMAP_HEADER;
    }
    mmap_buffer -= header_length;

    char content[header_length];
    memset(content, 0, header_length);
    memcpy(content, mmap_buffer, header_length);
    cJSON *json_content = cJSON_Parse(content);
    if (json_content == NULL) {
        return ERROR_CODE_BAD_HEADER_CONTENT_ON_PARSE_MMAP_HEADER;
    }
    cJSON *json_file_name = cJSON_GetObjectItem(json_content, NLOGGER_MMAP_CACHE_HEADER_KEY_FILE);
    cJSON *json_version   = cJSON_GetObjectItem(json_content, NLOGGER_MMAP_CACHE_HEADER_KEY_VERSION);
    cJSON *json_date      = cJSON_GetObjectItem(json_content, NLOGGER_MMAP_CACHE_HEADER_KEY_DATE);


    if (json_file_name == NULL || json_file_name->type != cJSON_String || is_empty_string(json_file_name->valuestring) ||
        json_version == NULL || json_version->type != cJSON_Number ||
        json_date == NULL || json_date->type != cJSON_Number) {
        return ERROR_CODE_BAD_HEADER_CONTENT_ON_PARSE_MMAP_HEADER;
    }

    LOGI("mmap_header", "on parse mmap header, file_name=%s, version=%d, date=%f", json_file_name->valuestring, json_version->valueint, json_date->valuedouble);

    //返回缓存对应的日志文件名log_file_name
    size_t file_name_length = strlen(json_file_name->valuestring) + 1;
    char   *temp_file_name  = malloc(file_name_length);
    memset(temp_file_name, 0, file_name_length);
    memcpy(temp_file_name, json_file_name->valuestring, file_name_length);
    *file_name = temp_file_name;

    //记录缓存的长度地址和长度值
    mmap_buffer += header_length;
    mmap_buffer += sizeof(NLOGGER_MMAP_CACHE_HEADER_HEAD_TAG);
    cache->p_length = mmap_buffer;
    memset(length_array, 0, 4);
    for (int i = 0; i < NLOGGER_CONTENT_LENGTH_BYTE_SIZE; ++i) {
        length_array[i] = *mmap_buffer;
        mmap_buffer++;
    }
    adjust_byte_order_nlogger(length_array);
    cache->length = *((unsigned int *) length_array);

    cJSON_Delete(json_content);

    return ERROR_CODE_OK;
}

int flush_nlogger() {
    if (g_nlogger == NULL || g_nlogger->state == NLOGGER_STATE_ERROR) {
        return ERROR_CODE_NEED_INIT_NLOGGER_BEFORE_ACTION;
    }

    char *file_name;
    int  result = _parse_mmap_cache_head(&g_nlogger->cache, &file_name);
    LOGD("flush_nlogger", "flush_nlogger result >>>> %d", result);
//
//    //在没有文件名的情况下去flush的时候需要从mmap中获取文件名然后再打开，（这一步是为了兼容init的时候进行初始化的操作）
//    char *file_name = NULL;
//    if (g_nlogger->log.p_name == NULL &&
//        g_nlogger->cache.cache_mode == NLOGGER_MEMORY_CACHE_MODE) {
//        if (g_nlogger->log.p_dir == NULL) {
//            return ERROR_CODE_NEED_INIT_NLOGGER_BEFORE_ACTION;
//        }
//        //todo 解析mmap文件 需要获取 缓存文件的长度，温存文件的指针
//        //最后检查一次日志名是否存在
//    }
//
//
//    _check_log_file_healthy(&g_nlogger->log, file_name);
//    free(file_name);
//
//
//    //todo 检查一下方法的参数是否都是有效的
//    char *cache_content = g_nlogger->cache.p_length + NLOGGER_CONTENT_SUB_SECTION_LENGTH_BYTE_SIZE;
//    _flush_cache_to_file(cache_content, g_nlogger->cache.length, g_nlogger->log.p_file);
//    _reset_cache_and_data_handler();

    return ERROR_CODE_OK;
}



