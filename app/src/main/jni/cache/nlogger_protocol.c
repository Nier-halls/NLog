//
// Created by fgd on 2019/5/17.
//

#include <malloc.h>
#include "nlogger_protocol.h"
#include "../utils/nlogger_utils.h"
#include "../nlogger_error_code.h"
#include "../utils/nlogger_android_log.h"
#include "../nlogger_constants.h"


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
int malloc_and_build_log_json_data(int cache_type, int flag, char *log_content, long long local_time,
                                   char *thread_name, long long thread_id, int is_main, char **result_json_data) {
//    size_t result_size = 0;
    int result = ERROR_CODE_BUILD_LOG_JSON_DATA_FAILED;

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
            LOGW("protocol_log", "log result_json >>> %s", result_json);
            size_t content_length = strlen(result_json);
            size_t final_length   = content_length + 1; //加上末尾的换行符\n
            char   *log_data      = malloc(final_length);
            memset(log_data, 0, final_length);
            memcpy(log_data, result_json, content_length);
//            memcpy(log_data + content_length, "\n", 1);
            *result_json_data = log_data;
            //这里是否有必要释放内存
            free(result_json);
            result = ERROR_CODE_OK;
        }
        cJSON_Delete(root);
    }
    if (map != NULL) {
        delete_json_map_nlogger(map);
    }

    return result;
}


int malloc_and_build_cache_header_json_data(char *log_file_name, char **result_json_data) {

    int              result       = ERROR_CODE_BUILD_CACHE_HEADER_JSON_DATA_FAILED;
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
            result      = ERROR_CODE_OK;
        }

        if (result_json != NULL) {
            LOGW("protocol_log", "header result_json >>> %s", result_json);
            size_t content_length = strlen(result_json);
            //todo bug 末尾再加上一个空字符，避免取长度出现问题
//            size_t final_length = content_length + 1; //加上末尾的换行符\n
            size_t final_length = content_length + 1 + 1; //加上末尾的换行符\n
            char   *log_data      = malloc(final_length);
            memset(log_data, 0, final_length);
            memcpy(log_data, result_json, content_length);
//            memcpy(log_data + content_length, "\n", 1);
            *result_json_data = log_data;
            //这里是否有必要释放内存
            free(result_json);
            result = ERROR_CODE_OK;
        }
        cJSON_Delete(root);
    }

    if (map != NULL) {
        delete_json_map_nlogger(map);
    }
    return result;
}


int parse_header_json_data(char *header_json_data, char **result_file_name, int **result_version, long **result_date) {
    int   result        = ERROR_CODE_BAD_HEADER_CONTENT_ON_PARSE_MMAP_HEADER;
    cJSON *json_content = cJSON_Parse(header_json_data);
    if (json_content != NULL) {
        cJSON *json_file_name = cJSON_GetObjectItem(json_content, NLOGGER_MMAP_CACHE_HEADER_KEY_FILE);
        cJSON *json_version   = cJSON_GetObjectItem(json_content, NLOGGER_MMAP_CACHE_HEADER_KEY_VERSION);
        cJSON *json_date      = cJSON_GetObjectItem(json_content, NLOGGER_MMAP_CACHE_HEADER_KEY_DATE);
        if (json_file_name != NULL && json_file_name->type == cJSON_String && !is_empty_string(json_file_name->valuestring)
            && json_version != NULL && json_version->type == cJSON_Number
            && json_date != NULL && json_date->type == cJSON_Number) {
            size_t file_name_length = strlen(json_file_name->valuestring) + 1;
            char   *file_name;
            int    *version;
            long   *date;

            file_name = malloc(file_name_length);
            if (file_name == NULL) {
                return ERROR_CODE_MALLOC_FAILED_ON_PARSE_MMAP_HEADER;
            }
            memset(file_name, 0, file_name_length);
            memcpy(file_name, json_file_name->valuestring, file_name_length);
            *result_file_name = file_name;

            size_t version_length = sizeof(int);
            version = malloc(version_length);
            if (version == NULL) {
                return ERROR_CODE_MALLOC_FAILED_ON_PARSE_MMAP_HEADER;
            }
            memset(version, 0, version_length);
            memcpy(version, &json_version->valueint, version_length);
            *result_version = version;


            size_t date_length = sizeof(long);
            date = malloc(date_length);
            if (date == NULL) {
                return ERROR_CODE_MALLOC_FAILED_ON_PARSE_MMAP_HEADER;
            }
            memset(date, 0, date_length);
            memcpy(date, &json_date->valuedouble, date_length);
            *result_date = date;

            result = ERROR_CODE_OK;
        }
        cJSON_Delete(json_content);
    }
    return result;
}

int _default_write_magic_tag(char *desc, int magic) {
    *desc = magic >> 0;
    desc++;
    *desc = magic >> 8;
    desc++;
    *desc = magic >> 16;
    desc++;
    *desc = magic >> 24;
    return 4;
}

int add_mmap_head_tag(char *cache) {
    return _default_write_magic_tag(cache, NLOGGER_MMAP_CACHE_HEADER_HEAD_MAGIC);
}

int add_mmap_tail_tag(char *cache) {
    return _default_write_magic_tag(cache, NLOGGER_MMAP_CACHE_HEADER_TAIL_MAGIC);
}

int add_section_head_tag(char *cache) {
    return _default_write_magic_tag(cache, NLOGGER_MMAP_CACHE_CONTENT_HEAD_MAGIC);
}

int add_section_tail_tag(char *cache) {
    return _default_write_magic_tag(cache, NLOGGER_MMAP_CACHE_CONTENT_TAIL_MAGIC);
}

int _default_read_magic_tag(char *desc) {
    char tag_array[4];
    tag_array[0] = *desc;
    desc++;
    tag_array[1] = *desc;
    desc++;
    tag_array[2] = *desc;
    desc++;
    tag_array[3] = *desc;
    adjust_byte_order_nlogger(tag_array);
    return *((int *) tag_array);
}

int check_mmap_head_tag(char *tag_point) {
    if (_default_read_magic_tag(tag_point) == NLOGGER_MMAP_CACHE_HEADER_HEAD_MAGIC) {
        return 4;
    }
    return 0;
}

int check_mmap_tail_tag(char *tag_point) {
    if (_default_read_magic_tag(tag_point) == NLOGGER_MMAP_CACHE_HEADER_TAIL_MAGIC) {
        return 4;
    }
    return 0;
}