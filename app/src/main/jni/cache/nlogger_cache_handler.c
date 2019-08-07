//
// Created by Nie R on 2019/5/1.
//

#ifndef TAG
#define TAG "cache" //150k
#endif

#ifndef INIT_OK
#define INIT_OK 1
#endif

#ifndef INIT_FAILED
#define INIT_FAILED 0
#endif


#include <stddef.h>
#include <memory.h>
#include <fcntl.h>
#include <stdio.h>
#include <sys/mman.h>
#include <zconf.h>
#include <malloc.h>
#include "../utils/cJSON.h"
#include "nlogger_cache_handler.h"
#include "../utils/nlogger_android_log.h"
#include "../nlogger_error_code.h"
#include "../nlogger_constants.h"
#include "../utils/nlogger_utils.h"
#include "nlogger_protocol.h"
#include "../nlogger.h"
#include "../utils/nlogger_file_utils.h"

int _create_cache_file_path(const char *cache_file_dir, char **result_path);

size_t _update_cache_section_length(char *section_length, unsigned int length);

size_t _update_cache_content_length(char *content_length, unsigned int length);

int _create_cache_nlogger(struct nlogger_cache_struct *cache);

int _open_mmap(char *mmap_cache_file_path, char **mmap_cache_buffer);

int _init_memory_cache(char **mmap_cache_buffer);

int _parse_log_file_name_from_header(char *mmap_cache_header, char **file_name);

size_t _write_mmap_cache_header(char *cache_buffer, char *log_file_name);

int _parse_mmap_cache_head(char *cache_buffer, char **file_name);

int _parse_mmap_cache_content_length(char *p_content_length);

/**
 * 创建缓存文件所在的目录，打开mmap或者memeory cache
 *
 * @param cache
 * @param cache_file_dir
 * @return
 */
int init_cache(struct nlogger_cache_struct *cache, const char *cache_file_dir) {
    //创建mmap缓存文件的目录
    char *final_mmap_cache_path;
    _create_cache_file_path(cache_file_dir, &final_mmap_cache_path);
    LOGD("init", "finish create log cache path>>>> %s ", final_mmap_cache_path);
    cache->p_file_path = final_mmap_cache_path;

    //初始化缓存文件，首先采用mmap缓存，失败则用内存缓存
    int create_cache_result = _create_cache_nlogger(cache);

    if (create_cache_result == NLOGGER_INIT_CACHE_FAILED) {
        return ERROR_CODE_CREATE_CACHE_FAILED;
    }

    return ERROR_CODE_OK;
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
    if (make_dir_nlogger(*result_path) < 0) {
        return ERROR_CODE_CREATE_LOG_CACHE_DIR_FAILED;
    }
    LOGD("init", "create mmap cache dir_string >>> %s", *result_path);
    strcat(*result_path, NLOGGER_CACHE_FILE_NAME);
    return ERROR_CODE_OK;
}

/**
 * 创建缓存，优先尝试创建mmap缓存，无法创建则创建memory缓存
 *
 * @param cache
 * @return
 */
int _create_cache_nlogger(struct nlogger_cache_struct *cache) {
    int result;
    //首先尝试用mmap的形式作为缓存
    if (cache->p_file_path == NULL || strnlen(cache->p_file_path, 16) == 0) {
        LOGW(TAG, "argument mmap cache file is invalid.")
        result = NLOGGER_MEMORY_CACHE_MODE;
    } else if (_open_mmap(cache->p_file_path, &cache->p_buffer) == INIT_OK) {
        result = NLOGGER_MMAP_CACHE_MODE;
    } else {
        result = NLOGGER_MEMORY_CACHE_MODE;
    }
    //mmap失败则使用内存来作为缓存方式
    if (result == NLOGGER_MEMORY_CACHE_MODE && _init_memory_cache(&cache->p_buffer) != INIT_OK) {
        result = NLOGGER_INIT_CACHE_FAILED;
    }

    if (result != NLOGGER_INIT_CACHE_FAILED) {
        cache->cache_mode = result;
    }

    return result;
}

/**
 * 打开mmap内存映射
 *
 * @param mmap_cache_file_path 创建内存映射的文件
 * @param mmap_cache_buffer 内存映射的地址
 * @return
 */
int _open_mmap(char *mmap_cache_file_path, char **mmap_cache_buffer) {
    int result = INIT_FAILED;
    //step1 打开文件获取一个文件描述符fd，之后openMMap时会用到
    int fd     = open(mmap_cache_file_path,
                      O_RDWR | O_CREAT | O_CLOEXEC, //以读写的模式或者创建的模式打开，文件不存在则创建，存在则读写
                      S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP); //赋予用户及用户组读写的权限

    if (fd != -1) {
        //step2 创建一个文件流，检查文件的大小，保证把150kb的数据填充满
        int  is_file_ok = 0;
        //file用于数据填充检查
        FILE *file      = fopen(mmap_cache_file_path, "rb+");
        if (file != NULL) {
            fseek(file, 0, SEEK_END);//将文件指针移动到文件的末尾
            long file_length = ftell(file);//获取当前文件指针的位置

            LOGE(TAG, "mmap file length >>> %ld", file_length);

            //如果文件的内容少于指定的缓存大小，则覆盖'/0'直至放满为止
            if (file_length < NLOGGER_MMAP_CACHE_SIZE) {
                fseek(file, 0, SEEK_SET);
                char null_data[NLOGGER_MMAP_CACHE_SIZE];
                memset(null_data, 0, NLOGGER_MMAP_CACHE_SIZE);
                size_t written = fwrite(null_data, sizeof(char), NLOGGER_MMAP_CACHE_SIZE, file);
                fflush(file);
                LOGD(TAG, "pending data， %d, written %zd", NLOGGER_MMAP_CACHE_SIZE, written);
                if (written == NLOGGER_MMAP_CACHE_SIZE) {
                    is_file_ok = 1;
                } else {
                    LOGW(TAG, "pending data failed.expect %d, written %zd", NLOGGER_MMAP_CACHE_SIZE, written);
                }
            } else {
                is_file_ok = 1;
            }
            //检查完文件的大小关闭文件流
            fclose(file);
        } else {
            LOGW(TAG, "mmap cache file fopen failed");
        }
        //mmap映射的内存地址
        char *temp_mmap_buffer;
        //step3 建立mmap文件内存映射（在映射前要保证文件的内容被填满吗？）
        if (is_file_ok) {
            temp_mmap_buffer = mmap(NULL,//mmap内存映射到的地址，如果传NULL由内核自己决定映射到哪里
                                    NLOGGER_MMAP_CACHE_SIZE,//需要映射的文件长度
                                    PROT_READ | PROT_WRITE,//mapping映射的权限
                                    MAP_SHARED,//映射后更新会同时生效到文件显示在别的进程中的映射
                                    fd, 0);//
            if (temp_mmap_buffer != MAP_FAILED && temp_mmap_buffer != NULL) {
                result = INIT_OK;
                LOGD(TAG, "mmap success.");
            } else {
                LOGW(TAG, "mmap failed.");
            }
            //完成mmap映射后关闭文件描述符，mmap能在fd关闭的情况下仍然进行映射
            close(fd);

            //将mmap执行的内存地址让指针持有
            if (result == INIT_OK) {
                if (access(mmap_cache_file_path, F_OK) != -1) {
                    *mmap_cache_buffer = temp_mmap_buffer;
                    LOGD(TAG, "nlogger access mmap file success.");
                } else {
                    //文件有问题即使切换至内存缓存模式
                    result = INIT_FAILED;
                    LOGW(TAG,
                         "nlogger build mmap cache failed, access >> %s failed",
                         mmap_cache_file_path);
                }
            }
            //mmap失败时及时回收资源
            if (result == INIT_FAILED && temp_mmap_buffer != NULL) {
                munmap(temp_mmap_buffer, NLOGGER_MMAP_CACHE_SIZE);
            }
        }
    } else {
        LOGW(TAG, "open file(%s) failed.", mmap_cache_file_path);
    }
    return result;
}

/**
 * 初始化内存缓存
 *
 * @param mmap_cache_buffer 内存缓存的地址
 * @return
 */
int _init_memory_cache(char **mmap_cache_buffer) {
    int  result       = INIT_FAILED;
    char *temp_buffer = malloc(NLOGGER_MEMORY_CACHE_SIZE);
    if (temp_buffer != NULL) {
        *mmap_cache_buffer = temp_buffer;
        result = INIT_OK;
    } else {
        LOGW(TAG, "on init memory, malloc failed.");
    }
    return result;
}

/**
 * 写入缓存日志段的头(magic num)，并且初始化日志段的长度（4byte）块
 *
 * @param cache
 * @return
 */
int write_cache_content_header_tag_and_length_block(struct nlogger_cache_struct *cache) {
    int written_length = add_section_head_tag(cache->p_next_write);
    cache->p_next_write += written_length;
    cache->content_length += written_length;

    //保存 p_section_length 地址，初始化section长度字段
    cache->section_length   = 0;
    cache->p_section_length = cache->p_next_write;

    //移动写入指针，section长度字段（目前占位4byte）
    cache->p_next_write += NLOGGER_CONTENT_SUB_SECTION_LENGTH_BYTE_SIZE;
    cache->content_length += NLOGGER_CONTENT_SUB_SECTION_LENGTH_BYTE_SIZE;

    _update_cache_section_length(cache->p_section_length, cache->section_length);
    _update_cache_content_length(cache->p_content_length, cache->content_length);
    return ERROR_CODE_OK;
}

/**
 * 当有日志写入时更新结构体中保存的指针
 *
 * @param cache
 * @param written_length
 */
void on_cache_written(struct nlogger_cache_struct *cache, size_t written_length) {
    cache->p_next_write += written_length;
    cache->section_length += written_length;
    cache->content_length += written_length;
    _update_cache_section_length(cache->p_section_length, cache->section_length);
    _update_cache_content_length(cache->p_content_length, cache->content_length);
}

/**
 * 写入日志尾部标志（magic num）
 *
 * @param cache
 * @return
 */
int write_cache_content_tail_tag_block(struct nlogger_cache_struct *cache) {
    int written_length = add_section_tail_tag(cache->p_next_write);
    cache->p_next_write += written_length;
    cache->content_length += written_length;
    _update_cache_section_length(cache->p_section_length, cache->section_length);
    _update_cache_content_length(cache->p_content_length, cache->content_length);
    return ERROR_CODE_OK;
}


/**
 * 检查是否符合从缓存写入到日志文件到要求
 */
int is_cache_overflow(struct nlogger_cache_struct *cache) {
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

/**
 * 检查缓存是否健康，如果mmap缓存有问题及时切换成memory缓存
 *
 * @param cache
 * @return
 */
int check_cache_healthy(struct nlogger_cache_struct *cache) {
    int result = ERROR_CODE_OK;
    if (cache->cache_mode == NLOGGER_MMAP_CACHE_MODE &&
        !is_file_exist_nlogger(cache->p_file_path)) {
        //mmap缓存文件不见了，这个时候主动切换成Memory缓存
        if (cache->p_buffer != NULL) {
            munmap(cache->p_buffer, NLOGGER_MMAP_CACHE_SIZE);
        }
        if (cache->p_file_path != NULL) {
            free(cache->p_file_path);
            cache->p_file_path = NULL;
        }
        //路径传空，强制Memory缓存
        if (NLOGGER_INIT_CACHE_FAILED == _create_cache_nlogger(cache)) {
            return ERROR_CODE_INIT_CACHE_FAILED;
        }
        cache->p_content_length = cache->p_buffer;
        //其中有3byte是用来保存缓存日志长度的，预先空出来
        cache->p_next_write     = cache->p_content_length + NLOGGER_CONTENT_LENGTH_BYTE_SIZE;
        cache->content_length   = 0;
        cache->section_length   = 0;
        result = ERROR_CODE_CHANGE_CACHE_MODE_TO_MEMORY;
    }

    //最后检查一下缓存指针是否存在，如果不存在则直接报错返回
    if (cache->p_buffer == NULL) {
        result = ERROR_CODE_INIT_CACHE_FAILED;
    }

    //在检查一下关键字段是否为空 todo
//    if (cache->p_next_write == NULL) {
//        result = ERROR_CODE_INIT_CACHE_FAILED;
//    }
    return result;
}


/**
 * 完成mmap缓存文件和，日志文件的映射（写入一个缓存对应的日志文件的信息头到cache中，方便以外中断恢复缓存日志到日志文件中）
 *
 * @param cache
 * @param log_file_name
 * @return
 */
int map_log_file_with_cache(struct nlogger_cache_struct *cache, const char *log_file_name) {
    if (cache->cache_mode == NLOGGER_MMAP_CACHE_MODE) {
        //直接忽略旧的日志头，覆盖写入新的日志头，指定当前mmap文件映射的log file name
        size_t written_header_length = _write_mmap_cache_header(cache->p_buffer, log_file_name);

        if (written_header_length > 0) {
            cache->p_content_length = cache->p_buffer + written_header_length;
        } else {
            //写入失败则切换成Memory缓存模式
            //（不能建立关系就失去了意外中断导致日志丢失的优势，这和内存缓存没有区别）
            if (cache->p_buffer != NULL) {
                munmap(cache->p_buffer, NLOGGER_MMAP_CACHE_SIZE);
            }
            if (cache->p_file_path != NULL) {
                free(cache->p_file_path);
                cache->p_file_path = NULL;
            }
            //cache->p_file_path = NULL 会主动创建memory cache
            if (NLOGGER_INIT_CACHE_FAILED == _create_cache_nlogger(cache)) {
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
 * 写入文件名到mmap中
 * 如果写入文件名失败，则采取内存映射（此时mmap的优势已经没有了，如果重新打开mmap中的内容不知道恢复到哪里去）
 *
 * 此方法会赋值
 *
 */
size_t _write_mmap_cache_header(char *cache_buffer, char *log_file_name) {
    size_t write_size = (size_t) -1;
    char   *result_header_json;

    int result = malloc_and_build_cache_header_json_data(log_file_name, &result_header_json);


    if (result_header_json != NULL && result == ERROR_CODE_OK) {
        write_size = 0;
        LOGD("mmap_header", "build json finished. header >>> %s ", result_header_json);
        size_t content_length = strlen(result_header_json) + 1;

        LOGD("mmap_header", "build json finished. header length >>> %zd ", content_length);
        char *mmap_buffer = cache_buffer;


//        *mmap_buffer = NLOGGER_MMAP_CACHE_HEADER_HEAD_TAG;
//        write_size++;
//        mmap_buffer++;

        int written_tag_size = add_mmap_head_tag(mmap_buffer);
        write_size += written_tag_size;
        mmap_buffer += written_tag_size;

        *mmap_buffer = content_length;
        write_size++;
        mmap_buffer++;

        *mmap_buffer = content_length >> 8;
        write_size++;
        mmap_buffer++;

        memcpy(mmap_buffer, result_header_json, content_length);
        mmap_buffer += content_length;
        write_size += content_length;

//        *mmap_buffer = NLOGGER_MMAP_CACHE_HEADER_TAIL_TAG;
//        write_size++;

        written_tag_size = add_mmap_tail_tag(mmap_buffer);
        write_size += written_tag_size;

    }

    if (result_header_json != NULL) {
        free(result_header_json);
    }

    return write_size;
}

/**
 * 创建一个保存日志的协议结构体（json）
 *
 * @param cache
 * @param flag 日志的标志
 * @param log_content 日志内容
 * @param local_time 日志创建时间戳
 * @param thread_name 日志发生的线程名
 * @param thread_id 日志的id
 * @param is_main 是否是主线程
 * @param result_json_data 用来返回结果的二级指针
 * @return
 */
int build_log_json_data(
        struct nlogger_cache_struct *cache,
        int flag,
        char *log_content,
        long long local_time,
        char *thread_name,
        long long thread_id,
        int is_main,
        char **result_json_data) {
    int cache_mode = cache->cache_mode;
    return malloc_and_build_log_json_data(cache_mode, flag, log_content, local_time, thread_name, thread_id, is_main, result_json_data);
}


/**
 * 解析mmap缓存的header，根据header来
 * 初始化缓存的几个重要指针，以及返回 日志名 指针
 *
 * @param cache
 * @param file_name
 * @return
 */
int init_cache_from_mmap_buffer(struct nlogger_cache_struct *cache, char **log_file_name) {
    int error_code = ERROR_CODE_CAN_NOT_PARSE_ON_MEMORY_CACHE_MODE;
    if (cache->cache_mode == NLOGGER_MMAP_CACHE_MODE) {
        //解析日志文件名以及cache header的长度，如果结果大于0代表成功，结果数值就是header的长度
        int parse_result = _parse_mmap_cache_head(cache->p_buffer, log_file_name);
        if (parse_result > 0) {
            cache->p_content_length = cache->p_buffer + parse_result;
            //解析日志cache content的长度，如果结果大于0代表成功，结果数值就是content的长度
            parse_result = _parse_mmap_cache_content_length(cache->p_content_length);
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
 * 解析mmap缓存文件
 * todo 分解开
 *
 * @return
 */
int _parse_mmap_cache_head(char *cache_buffer, char **file_name) {
    char length_array[4];
    memset(length_array, 0, 4);
    int handled_length = 0;

    char *mmap_buffer = cache_buffer;
    //检查mmap缓存地址是否有效
    if (mmap_buffer == NULL) {
        return ERROR_CODE_INVALID_BUFFER_POINT_ON_PARSE_MMAP_HEADER;
    }
    //检查协议头是否正确
//    if (*mmap_buffer != NLOGGER_MMAP_CACHE_HEADER_HEAD_TAG) {
//        return ERROR_CODE_INVALID_HEAD_OR_TAIL_TAG_ON_PARSE_MMAP_HEADER;
//    }

    int head_tag_size = check_mmap_head_tag(mmap_buffer);
    //check_mmap_head_tag 返回当前head tag所占用的长度，如果返回0则代表失败
    if (!head_tag_size) {
        return ERROR_CODE_INVALID_HEAD_OR_TAIL_TAG_ON_PARSE_MMAP_HEADER;
    }

    mmap_buffer += head_tag_size;
    handled_length += head_tag_size;

    length_array[0] = *mmap_buffer;
    mmap_buffer++;
    handled_length++;

    length_array[1] = *mmap_buffer;
    mmap_buffer++;
    handled_length++;
    adjust_byte_order_nlogger(length_array);
    size_t header_content_length = (size_t) *((int *) length_array);
    LOGD("parse_mmap_h", "parse get header length >>> %zd", header_content_length)


    //检查头的大小是否有效（是否有必要给头加上上限）
    if (header_content_length < 0 || header_content_length > NLOGGER_MMAP_CACHE_MAX_HEADER_CONTENT_SIZE) {
        return ERROR_CODE_INVALID_HEADER_LENGTH_ON_PARSE_MMAP_HEADER;
    }
    LOGD("parse_mmap_h", "parse get head next >>> %c", *mmap_buffer)
    mmap_buffer += header_content_length;
    //检查协议尾部是否正确
//    if (*mmap_buffer != NLOGGER_MMAP_CACHE_HEADER_TAIL_TAG) {
//        LOGD("parse_mmap_h", "parse get tail >>> %c", *mmap_buffer )
//        LOGD("parse_mmap_h", "parse get tail >>> %c", *(mmap_buffer -3))
//        return ERROR_CODE_INVALID_HEAD_OR_TAIL_TAG_ON_PARSE_MMAP_HEADER;
//    }

    int tail_tag_size = check_mmap_tail_tag(mmap_buffer);
    if (!tail_tag_size) {
        return ERROR_CODE_INVALID_HEAD_OR_TAIL_TAG_ON_PARSE_MMAP_HEADER;
    }

    mmap_buffer -= header_content_length;

    char content[header_content_length];
    memset(content, 0, header_content_length);
    memcpy(content, mmap_buffer, header_content_length);
    int parse_result = _parse_log_file_name_from_header(content, file_name);
    if (parse_result != ERROR_CODE_OK) {
        return parse_result;
    }

    //记录缓存的长度地址和长度值
//    mmap_buffer += header_content_length;
    handled_length += header_content_length;

    //加上尾部标志的长度
    handled_length += tail_tag_size;

    return handled_length;
}


/**
 * 解析mmap缓存文件头中的日志文件名
 *
 * @param mmap_cache_header
 * @param file_name
 * @return
 */
int _parse_log_file_name_from_header(char *mmap_cache_header, char **file_name) {
    char *file_name_from_cache;
    int  *cache_version;
    long *cache_date;

    int parse_result = parse_header_json_data(mmap_cache_header, &file_name_from_cache, &cache_version, &cache_date);
    if (parse_result != ERROR_CODE_OK) {
        return parse_result;
    }

    LOGI("parse_mmap_h", "on parse mmap header, file_name=%s, version=%d, date=%f", file_name_from_cache, *cache_version, *cache_date);

    //返回缓存对应的日志文件名log_file_name
    size_t file_name_length = strlen(file_name_from_cache) + 1;
    char   *temp_file_name  = malloc(file_name_length);
    memset(temp_file_name, 0, file_name_length);
    memcpy(temp_file_name, file_name_from_cache, file_name_length);
    *file_name = temp_file_name;


    if (file_name_from_cache != NULL) {
        free(file_name_from_cache);
    }
    if (cache_version != NULL) {
        free(cache_version);
    }
    if (cache_date != NULL) {
        free(cache_date);
    }
//    cJSON_Delete(json_content);
    return ERROR_CODE_OK;
}

/**
 * 解析获取日志缓存内容的长度
 *
 * @param p_content_length 指向缓存内容长度的指针
 * @return
 */
int _parse_mmap_cache_content_length(char *p_content_length) {
    char length_array[4];
    //从content长度地址中获取记录的长度
    memset(length_array, 0, 4);
    for (int i = 0; i < NLOGGER_CONTENT_LENGTH_BYTE_SIZE; ++i) {
        length_array[i] = *p_content_length;
        p_content_length++;
    }
    adjust_byte_order_nlogger(length_array);
    int content_length = *((unsigned int *) length_array);
    LOGD("parse_mmap_h", "parse content length from header, cache->length >>> %d", content_length)
    return content_length;
}


/**
 * 获取缓存日志的头指针
 *
 * @param cache
 * @return
 */
char *cache_content_head(struct nlogger_cache_struct *cache) {
    return cache->p_content_length + NLOGGER_CONTENT_LENGTH_BYTE_SIZE;
}

/**
 * 获取缓存日志的长度
 *
 * @param cache
 * @return
 */
size_t cache_content_length(struct nlogger_cache_struct *cache) {
    return cache->content_length;
}

/**
 * 重制cache缓存状态
 *
 * @param cache
 * @return
 */
int reset_nlogger_cache(struct nlogger_cache_struct *cache) {
    cache->content_length = 0;
    cache->p_next_write   = cache->p_content_length + NLOGGER_CONTENT_LENGTH_BYTE_SIZE;
    _update_cache_content_length(cache->p_content_length, cache->content_length);

    return ERROR_CODE_OK;
}

/**
 * 获取当前写入的指针
 *
 * @param cache
 * @return
 */
char *obtain_cache_next_write(struct nlogger_cache_struct *cache) {
    return cache->p_next_write;
}


/**
 * 更新内存中的压缩数据段的长度到缓存中
 *
 * @param p_content_length
 * @param length
 * @return
 */
size_t _update_cache_section_length(char *section_length, unsigned int length) {
    size_t handled = 0;
    // 为了兼容java,采用高字节序
    *section_length = length >> 24;
    section_length++;
    handled++;
    *section_length = length >> 16;
    section_length++;
    handled++;
    *section_length = length >> 8;
    section_length++;
    handled++;
    *section_length = length;
    handled++;
    LOGD("up_sect_len", "_update_section_length length >>> %d ", length);
    return handled;
}

/**
 * 更新内存中的日志体的长度
 *
 * @param content_length
 * @param length
 * @return
 */
size_t _update_cache_content_length(char *content_length, unsigned int length) {
    size_t handled = 0;
    // 为了兼容java,采用高字节序
    *content_length = length >> 0;
    content_length++;
    handled++;
    *content_length = length >> 8;
    content_length++;
    handled++;
    *content_length = length >> 16;
    handled++;
    LOGD("up_cont_len", "_update_content_length length >>> %d ", length);
    return handled;
}



