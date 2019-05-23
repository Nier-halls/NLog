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
#include "cJSON.h"
#include "nlogger_cache.h"
#include "nlogger_android_log.h"
#include "nlogger_error_code.h"
#include "nlogger_constants.h"
#include "nlogger_utils.h"
#include "nlogger_protocol.h"

int _open_mmap(char *mmap_cache_file_path, char **mmap_cache_buffer) {
    int result = INIT_FAILED;
    //step1 打开文件获取一个文件描述符fd
    int fd     = open(mmap_cache_file_path,
                      O_RDWR | O_CREAT | O_CLOEXEC, //以读写的模式或者创建的模式打开，文件不存在则创建，存在则读写
                      S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP); //赋予用户及用户组读写的权限

    if (fd != -1) {
        //step2 创建一个文件流，检查文件的大小，保证把150kb的数据填充满
        int  is_file_ok = 0;
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
        LOGW(TAG, "open file failed.");
    }
    return result;
}

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

int create_cache_nlogger(char *mmap_cache_file_path, char **mmap_cache_buffer) {
    int result;
    //首先尝试用mmap的形式作为缓存
    if (mmap_cache_file_path == NULL || strnlen(mmap_cache_file_path, 16) == 0) {
        LOGW(TAG, "argument mmap cache file is invalid.")
        result = NLOGGER_MEMORY_CACHE_MODE;
    } else if (_open_mmap(mmap_cache_file_path, mmap_cache_buffer) == INIT_OK) {
        result = NLOGGER_MMAP_CACHE_MODE;
    } else {
        result = NLOGGER_MEMORY_CACHE_MODE;
    }
    //mmap失败则使用内存来作为缓存方式
    if (result == NLOGGER_MEMORY_CACHE_MODE && _init_memory_cache(mmap_cache_buffer) != INIT_OK) {
        result = NLOGGER_INIT_CACHE_FAILED;
    }
    return result;
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

int parse_mmap_cache_content_length(char *p_content_length) {
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
 * 写入文件名到mmap中
 * 如果写入文件名失败，则采取内存映射（此时mmap的优势已经没有了，如果重新打开mmap中的内容不知道恢复到哪里去）
 *
 * 此方法会赋值
 *
 */
size_t write_mmap_cache_header(char *cache_buffer, char *log_file_name) {
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
 * 解析mmap缓存文件
 * todo 分解开
 *
 * @return
 */
int parse_mmap_cache_head(char *cache_buffer, char **file_name) {
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
 * 更新内存中的压缩数据段的长度到缓存中
 *
 * @param p_content_length
 * @param length
 * @return
 */
size_t update_cache_section_length(char *section_length, unsigned int length) {
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
size_t update_cache_content_length(char *content_length, unsigned int length) {
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


