/*
 * Copyright (c) 2018-present, 美团点评
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */

#include "clogan_core.h"

#include "mmap_util.h"
#include "construct_data.h"
#include "cJSON.h"
#include "json_util.h"
#include "zlib_util.h"
#include "aes_util.h"
#include "directory_util.h"
#include "base_util.h"
#include "console_util.h"
#include "clogan_status.h"

static int is_init_ok = 0;
static int is_open_ok = 0;

static unsigned char *_logan_buffer = NULL; //缓存Buffer (不释放) //mmap内存映射的内存地址指针，指向mmap文件内存映射。如果mmap失败则会指向一块内存缓存的地址

static char *_dir_path = NULL; //日志目录路径 (不释放) 在外部存储./storage/Android  //这个字段后面是否跟着‘/’的？

static char *_mmap_file_path = NULL; //mmap文件路径 (不释放)

static int buffer_length = 0; //缓存区域的大小

static unsigned char *_cache_buffer_buffer = NULL; //临时缓存文件 (不释放) //纯内存缓存，不涉及mmap，最大大小为150kb

static int buffer_type; //缓存区块的类型

static long max_file_len = LOGAN_LOGFILE_MAXLENGTH; //日志文件缓存大小默认10mb

static cLogan_model *logan_model = NULL; //(不释放)

/**
 * 打开日志文件的目录，并且记录当前日志文件的情况，已经写入的大小等
 *    logan_model->file = file_temp; //持有FILE结构体，到时候需要手动关闭
 *    logan_model->file_len = longBytes; //记录打开文件当前的长度
 *    logan_model->file_stream_type = LOGAN_FILE_OPEN; //标记当前文件的状态
 */
int init_file_clogan(cLogan_model *logan_model) {
    int is_ok = 0;
    if (LOGAN_FILE_OPEN == logan_model->file_stream_type) {
        return 1;
    } else {
    	//创建文件
        FILE *file_temp = fopen(logan_model->file_path, "ab+");//fopen 以附加的方式打开只写文件。若文件不存在，则会建立该文件，如果文件存在，写入的数据会被加到文件尾，即文件原先的内容会被保留。（EOF符保留）
        if (NULL != file_temp) {  //初始化文件流开启
            logan_model->file = file_temp;
            fseek(file_temp, 0, SEEK_END);
            long longBytes = ftell(file_temp);
            logan_model->file_len = longBytes;
            logan_model->file_stream_type = LOGAN_FILE_OPEN;
            is_ok = 1;
        } else {
            logan_model->file_stream_type = LOGAN_FILE_NONE;
        }
    }
    return is_ok;
}

void init_encrypt_key_clogan(cLogan_model *logan_model) {
    aes_inflate_iv_clogan(logan_model->aes_iv);
}

void write_mmap_data_clogan(char *path, unsigned char *temp) {
    logan_model->total_point = temp;
    logan_model->file_path = path;
    char len_array[] = {'\0', '\0', '\0', '\0'};
    len_array[0] = *temp;
    temp++;
    len_array[1] = *temp;
    temp++;
    len_array[2] = *temp;

    adjust_byteorder_clogan(len_array);//调整字节序,默认为低字节序,在读取的地方处理

    int *total_len = (int *) len_array;

    int t = *total_len;
    printf_clogan("write_mmapdata_clogan > buffer total length %d\n", t);
    if (t > LOGAN_WRITEPROTOCOL_HEAER_LENGTH && t < LOGAN_MMAP_LENGTH) {
        logan_model->total_len = t;
        if (NULL != logan_model) {
            if (init_file_clogan(logan_model)) {
                logan_model->is_ok = 1;
                logan_model->zlib_type = LOGAN_ZLIB_NONE;
                clogan_flush();
                fclose(logan_model->file);
                logan_model->file_stream_type = LOGAN_FILE_CLOSE;

            }
        }
    } else {
        logan_model->file_stream_type = LOGAN_FILE_NONE;
    }
    logan_model->total_len = 0;
    logan_model->file_path = NULL;
}


/**
 * 读取mmap文件中的数据，并且写入到外部存储的日志文件中 
 *
 * path_dirs 日志文件的目录(/storage/emulated/0/Android/data/test.logan.dianping.com.logan/files/logan_v1)
 *
 */
void read_mmap_data_clogan(const char *path_dirs) {
    if (buffer_type == LOGAN_MMAP_MMAP) {
        unsigned char *temp = _logan_buffer;
        unsigned char *temp2 = NULL;
        char i = *temp;
        if (LOGAN_MMAP_HEADER_PROTOCOL == i) {
            temp++;
            char len_array[] = {'\0', '\0', '\0', '\0'};
            len_array[0] = *temp;
            temp++;
            len_array[1] = *temp;
            adjust_byteorder_clogan(len_array);
            int *len_p = (int *) len_array;
            temp++;
            temp2 = temp;
            int len = *len_p;

            printf_clogan("read_mmapdata_clogan > path's json length : %d\n", len);

            if (len > 0 && len < 1024) {
                temp += len;
                i = *temp;
                if (LOGAN_MMAP_TAIL_PROTOCOL == i) {
                    char dir_json[len];
                    memset(dir_json, 0, len);
                    memcpy(dir_json, temp2, len);
                    printf_clogan("dir_json %s\n", dir_json);
                    cJSON *cjson = cJSON_Parse(dir_json);

                    if (NULL != cjson) {
						//version 测试机上显示的是3
                        cJSON *dir_str = cJSON_GetObjectItem(cjson,
                                                             LOGAN_VERSION_KEY);  //删除json根元素释放 //什么意思?
						//文件的名称，目前是时间戳命名的
                        cJSON *path_str = cJSON_GetObjectItem(cjson, LOGAN_PATH_KEY);
                        if ((NULL != dir_str && cJSON_Number == dir_str->type &&
                             CLOGAN_VERSION_NUMBER == dir_str->valuedouble) &&
                            (NULL != path_str && path_str->type == cJSON_String &&
                             !is_string_empty_clogan(path_str->valuestring))) {

                            printf_clogan(
                                    "read_mmapdata_clogan > dir , path and version : %s || %s || %lf\n",
                                    path_dirs, path_str->valuestring, dir_str->valuedouble);

                            size_t dir_len = strlen(path_dirs);//日志文件的路径长度
                            size_t path_len = strlen(path_str->valuestring);//文件名的长度
                            size_t length = dir_len + path_len + 1;
                            char file_path[length];
                            memset(file_path, 0, length);
                            memcpy(file_path, path_dirs, dir_len);
                            strcat(file_path, path_str->valuestring);
                            temp++;
							//temp目前指向的是header过后的数据，file_path为文件路径
                            write_mmap_data_clogan(file_path, temp);
                        }
                        cJSON_Delete(cjson);
                    }
                }
            }
        }
    }
}

/**
 * Logan初始化
 * @param cachedirs 缓存路径，在内部/data/user目录下面(/data/user/0/test.logan.dianping.com.logan/files)
 * @param pathdirs  日志文件目录路径,在外部存储控件(/storage/emulated/0/Android/data/test.logan.dianping.com.logan/files/logan_v1)
 * @param max_file  日志文件最大值
 */
int
clogan_init(const char *cache_dirs, const char *path_dirs, int max_file, const char *encrypt_key16,
            const char *encrypt_iv16) {
    int back = CLOGAN_INIT_FAIL_HEADER;
    if (is_init_ok ||
        NULL == cache_dirs || strnlen(cache_dirs, 11) == 0 ||
        NULL == path_dirs || strnlen(path_dirs, 11) == 0 ||
        NULL == encrypt_key16 || NULL == encrypt_iv16) {
        back = CLOGAN_INIT_FAIL_HEADER;
        return back;
    }

    if (max_file > 0) {
        max_file_len = max_file;
    } else {
        max_file_len = LOGAN_LOGFILE_MAXLENGTH;
    }

    if (NULL != _dir_path) { // 初始化时 , _dir_path和_mmap_file_path是非空值,先释放,再NULL
        free(_dir_path);
        _dir_path = NULL;
    }
    if (NULL != _mmap_file_path) {
        free(_mmap_file_path);
        _mmap_file_path = NULL;
    }

	//赋值一下aes需要的密钥key 以及 偏移量
    aes_init_key_iv(encrypt_key16, encrypt_iv16);

	
	//>>>>>>>>>>>>> 创建mmap缓存目录 START >>>>>>>>>>>>> 
    size_t path1 = strlen(cache_dirs);
    size_t path2 = strlen(LOGAN_CACHE_DIR);
    size_t path3 = strlen(LOGAN_CACHE_FILE);
    size_t path4 = strlen(LOGAN_DIVIDE_SYMBOL);

    int isAddDivede = 0;
    char d = *(cache_dirs + path1 - 1);
    if (d != '/') {
        isAddDivede = 1;
    }
	//计算mmap文件路径的长度
    size_t total = path1 + (isAddDivede ? path4 : 0) + path2 + path4 + path3 + 1;
    char *cache_path = malloc(total);
    if (NULL != cache_path) {
        _mmap_file_path = cache_path; //保持mmap文件路径,如果初始化失败,注意释放_mmap_file_path
    } else {
        is_init_ok = 0;
        printf_clogan("clogan_init > malloc memory fail for mmap_file_path \n");
        back = CLOGAN_INIT_FAIL_NOMALLOC;
        return back;
    }

    memset(cache_path, 0, total);
    strcpy(cache_path, cache_dirs);
    if (isAddDivede)
        strcat(cache_path, LOGAN_DIVIDE_SYMBOL);

    strcat(cache_path, LOGAN_CACHE_DIR);
    strcat(cache_path, LOGAN_DIVIDE_SYMBOL);
	
	 //创建保存mmap文件的目录（文件还没有创建）
    makedir_clogan(cache_path);

    strcat(cache_path, LOGAN_CACHE_FILE);

	//<<<<<<<<<<<<< 创建mmap缓存目录 END <<<<<<<<<<<<<


	//>>>>>>>>>>>>> 创建日志缓存目录 START >>>>>>>>>>>>> 
    size_t dirLength = strlen(path_dirs);

    isAddDivede = 0;
    d = *(path_dirs + dirLength - 1);
    if (d != '/') {
        isAddDivede = 1;
    }
    total = dirLength + (isAddDivede ? path4 : 0) + 1;

    char *dirs = (char *) malloc(total); //缓存文件目录

    if (NULL != dirs) {
        _dir_path = dirs; //日志写入的文件目录
    } else {
        is_init_ok = 0;
        printf_clogan("clogan_init > malloc memory fail for _dir_path \n");
        back = CLOGAN_INIT_FAIL_NOMALLOC;
        return back;
    }
    memset(dirs, 0, total);
    memcpy(dirs, path_dirs, dirLength);
    if (isAddDivede)
        strcat(dirs, LOGAN_DIVIDE_SYMBOL);
	
    makedir_clogan(_dir_path); //创建缓存目录,如果初始化失败,注意释放_dir_path
	//<<<<<<<<<<<<< 创建日志缓存目录 END <<<<<<<<<<<<<

	// 打开mmap内存映射并且保存内存映射的地址到_logan_buffer 
    int flag = LOGAN_MMAP_FAIL;
    if (NULL == _logan_buffer) {
        if (NULL == _cache_buffer_buffer) {
            /**
             * cache_path: mmap文件地址
             * _logan_buffer: 指向mmap内存映射的地址指针，会被方法赋值
             * _cache_buffer_buffer: 内存缓存的地址指针，会在内部创建一个150kb的内存缓存
             */
            flag = open_mmap_file_clogan(cache_path, &_logan_buffer, &_cache_buffer_buffer);//_cache_buffer_buffer这个东西有什么用
        } else {
            flag = LOGAN_MMAP_MEMORY;
        }
    } else {
        flag = LOGAN_MMAP_MMAP;
    }

    if (flag == LOGAN_MMAP_MMAP) {
        buffer_length = LOGAN_MMAP_LENGTH;
        buffer_type = LOGAN_MMAP_MMAP;
        is_init_ok = 1;
        back = CLOGAN_INIT_SUCCESS_MMAP;
    } else if (flag == LOGAN_MMAP_MEMORY) {
        buffer_length = LOGAN_MEMORY_LENGTH;
        buffer_type = LOGAN_MMAP_MEMORY;
        is_init_ok = 1;
        back = CLOGAN_INIT_SUCCESS_MEMORY;
    } else if (flag == LOGAN_MMAP_FAIL) {
        is_init_ok = 0;
        back = CLOGAN_INIT_FAIL_NOCACHE;
    }

    if (is_init_ok) {
		//创建日志处理结构体 logan_model
        if (NULL == logan_model) {
            logan_model = malloc(sizeof(cLogan_model));
            if (NULL != logan_model) { //堆非空判断 , 如果为null , 就失败
                memset(logan_model, 0, sizeof(cLogan_model));
            } else {
                is_init_ok = 0;
                printf_clogan("clogan_init > malloc memory fail for logan_model\n");
                back = CLOGAN_INIT_FAIL_NOMALLOC;
                return back;
            }
        }
        if (flag == LOGAN_MMAP_MMAP) //MMAP的缓存模式,从缓存的MMAP中读取数据
            //mmap文件刚刚映射好，里面都是用空数据填充的为什么还要取读一次？
            //在mmap文件中有数据的情况下是不会去用空字符来填充的，这个时候就要去尝试去读取一次

		    //尝试把之前没有被缓存到
            read_mmap_data_clogan(_dir_path);
        printf_clogan("clogan_init > logan init success\n");
    } else {
        printf_clogan("clogan_open > logan init fail\n");
        // 初始化失败，删除所有路径
        if (NULL == _dir_path) {
            free(_dir_path);
            _dir_path = NULL;
        }
        if (NULL == _mmap_file_path) {
            free(_mmap_file_path);
            _mmap_file_path = NULL;
        }
    }
    return back;
}

/*
 * 对mmap添加header和确定总长度位置
 */
void add_mmap_header_clogan(char *content, cLogan_model *model) {
    size_t content_len = strlen(content) + 1;
    size_t total_len = content_len;
    char *temp = (char *) model->buffer_point;
    *temp = LOGAN_MMAP_HEADER_PROTOCOL;
    temp++;
    *temp = total_len;
    temp++;
    *temp = total_len >> 8;
    printf_clogan("\n add_mmap_header_clogan len %d\n", total_len);
    temp++;
    memcpy(temp, content, content_len);
    temp += content_len;
    *temp = LOGAN_MMAP_TAIL_PROTOCOL;
    temp++;
    model->total_point = (unsigned char *) temp; // 总数据的total_length的指针位置
    model->total_len = 0;//避免之前有脏数据没有覆盖完全
}

/**
 * 确立最后的长度指针位置和最后的写入指针位置
 */
void restore_last_position_clogan(cLogan_model *model) {
    unsigned char *temp = model->last_point;
    *temp = LOGAN_WRITE_PROTOCOL_HEADER;
    model->total_len++;
    temp++;
    model->content_lent_point = temp; // 除去开头Head标志符的数据地址，也就是java层content长度的指针，估计是到时候为了修改
    //计算并且写入数据体的长度 START （高位在前）
    *temp = model->content_len >> 24;
    model->total_len++;
    temp++;
    *temp = model->content_len >> 16;
    model->total_len++;
    temp++;
    *temp = model->content_len >> 8;
    model->total_len++;
    temp++;
    *temp = model->content_len >> 0;
	//计算并且写入数据体的长度 END
    model->total_len++;
    temp++;
    model->last_point = temp;

    printf_clogan("restore_last_position_clogan > content_len : %d\n", model->content_len);
}

/**
 * 这个方法有什么作用
 *
 * pathname 日志文件的文件名
 *
 */
int clogan_open(const char *pathname) {
    int back = CLOGAN_OPEN_FAIL_NOINIT;
    if (!is_init_ok) {
        back = CLOGAN_OPEN_FAIL_NOINIT;
        return back;
    }

    is_open_ok = 0;
	//path name 128 是什么情况 127个char?
    if (NULL == pathname || 0 == strnlen(pathname, 128) || NULL == _logan_buffer ||
        NULL == _dir_path ||
        0 == strnlen(_dir_path, 128)) {
        back = CLOGAN_OPEN_FAIL_HEADER;
        return back;
    }

	//>>>>>>>>>> 将可能的缓存写入到文件 并且重置logan_model >>>>>>>>>>
	
    if (NULL != logan_model) { //回写到日志中
    	//在已经init的情况下，但是时间发生了改变，这个时候就需要先把可能的缓存全部写入到文件中
        if (logan_model->total_len > LOGAN_WRITEPROTOCOL_HEAER_LENGTH) {// head(1) + length(3) + tail(1)
            clogan_flush();
        }
        if (logan_model->file_stream_type == LOGAN_FILE_OPEN) {
            fclose(logan_model->file);
            logan_model->file_stream_type = LOGAN_FILE_CLOSE;
        }
        if (NULL != logan_model->file_path) {
            free(logan_model->file_path);
            logan_model->file_path = NULL;
        }
        logan_model->total_len = 0;
    } else {
        logan_model = malloc(sizeof(cLogan_model));
        if (NULL != logan_model) {
            memset(logan_model, 0, sizeof(cLogan_model));
        } else {
            logan_model = NULL; //初始化Logan_model失败,直接退出
            is_open_ok = 0;
            back = CLOGAN_OPEN_FAIL_MALLOC;
            return back;
        }
    }
	
	//<<<<<<<<<< 将可能的缓存写入到文件 并且重置logan_model <<<<<<<<<<

    char *temp = NULL;//将指向新文件的path 字符串地址

    size_t file_path_len = strlen(_dir_path) + strlen(pathname) + 1;
    char *temp_file = malloc(file_path_len); // 日志文件路径
    if (NULL != temp_file) {
        memset(temp_file, 0, file_path_len);
        temp = temp_file;
        memcpy(temp, _dir_path, strlen(_dir_path));
        temp += strlen(_dir_path);
        memcpy(temp, pathname, strlen(pathname)); //创建文件路径
        logan_model->file_path = temp_file;
		//创建日志文件
        if (!init_file_clogan(logan_model)) {  //初始化文件IO和文件大小
            is_open_ok = 0;
            back = CLOGAN_OPEN_FAIL_IO;
            return back;
        }
		//配置 zstream 调用 deflateInit2 来初始化
        if (init_zlib_clogan(logan_model) != Z_OK) { //初始化zlib压缩
            is_open_ok = 0;
            back = CLOGAN_OPEN_FAIL_ZLIB;
            return back;
        }

        logan_model->buffer_point = _logan_buffer;

        if (buffer_type == LOGAN_MMAP_MMAP) {  //如果是MMAP,缓存文件目录和文件名称
            cJSON *root = NULL;
            Json_map_logan *map = NULL;
            root = cJSON_CreateObject();
            map = create_json_map_logan();
            char *back_data = NULL;
            if (NULL != root) {
                if (NULL != map) {
                    add_item_number_clogan(map, LOGAN_VERSION_KEY, CLOGAN_VERSION_NUMBER);
                    add_item_string_clogan(map, LOGAN_PATH_KEY, pathname);
                    inflate_json_by_map_clogan(root, map);
                    back_data = cJSON_PrintUnformatted(root);
                }
                cJSON_Delete(root);
                if (NULL != back_data) {
                    add_mmap_header_clogan(back_data, logan_model);
                    free(back_data);
                } else {
                    logan_model->total_point = _logan_buffer;
                    logan_model->total_len = 0;
                }
            } else {
                logan_model->total_point = _logan_buffer;
                logan_model->total_len = 0;
            }
			//到目前为止total_point指向mmap文件头的末尾，total_len=0
            logan_model->last_point = logan_model->total_point + LOGAN_MMAP_TOTALLEN;//这里为什么还有+3？预先跳过3位标识长度的字节？不应该跳或者说跳的不是长度字节，后面还要重新去读取的//确实是预留位

            if (NULL != map) {
                delete_json_map_clogan(map);
            }
        } else {
            logan_model->total_point = _logan_buffer;
            logan_model->total_len = 0;
            logan_model->last_point = logan_model->total_point + LOGAN_MMAP_TOTALLEN;
        }
        restore_last_position_clogan(logan_model);
        init_encrypt_key_clogan(logan_model);
        logan_model->is_ok = 1;
        is_open_ok = 1;
    } else {
        is_open_ok = 0;
        back = CLOGAN_OPEN_FAIL_MALLOC;
        printf_clogan("clogan_open > malloc memory fail\n");
    }

    if (is_open_ok) {
        back = CLOGAN_OPEN_SUCCESS;
        printf_clogan("clogan_open > logan open success\n");
    } else {
        printf_clogan("clogan_open > logan open fail\n");
    }
    return back;
}


//更新总数据和最后的count的数据到内存中
void update_length_clogan(cLogan_model *model) {
    unsigned char *temp = NULL;
    if (NULL != model->total_point) {
        temp = model->total_point;
        *temp = model->total_len;
        temp++;
        *temp = model->total_len >> 8;
        temp++;
        *temp = model->total_len >> 16;
    }

    if (NULL != model->content_lent_point) {
        temp = model->content_lent_point;
        // 为了兼容java,采用高字节序
        *temp = model->content_len >> 24;
        temp++;
        *temp = model->content_len >> 16;
        temp++;
        *temp = model->content_len >> 8;
        temp++;
        *temp = model->content_len;
    }
}

//对clogan_model数据做还原
void clear_clogan(cLogan_model *logan_model) {
    logan_model->total_len = 0;

    if (logan_model->zlib_type == LOGAN_ZLIB_END) { //因为只有ZLIB_END才会释放掉内存,才能再次初始化
        memset(logan_model->strm, 0, sizeof(z_stream));
        logan_model->zlib_type = LOGAN_ZLIB_NONE;
        init_zlib_clogan(logan_model);
    }
    logan_model->remain_data_len = 0;
    logan_model->content_len = 0;
    logan_model->last_point = logan_model->total_point + LOGAN_MMAP_TOTALLEN;
    restore_last_position_clogan(logan_model);
    init_encrypt_key_clogan(logan_model);
    logan_model->total_len = 0;
    update_length_clogan(logan_model);
    logan_model->total_len = LOGAN_WRITEPROTOCOL_HEAER_LENGTH;
}

/**
 * 向日志文件中插入协议头
 * 对空的文件插入一行头文件做标示
 */
void insert_header_file_clogan(cLogan_model *loganModel) {
    char *log = "clogan header";
    int flag = 1;
    long long local_time = get_system_current_clogan();
    char *thread_name = "clogan";
    long long thread_id = 1;
    int ismain = 1;
    Construct_Data_cLogan *data = construct_json_data_clogan(log, flag, local_time, thread_name,
                                                             thread_id, ismain);
    if (NULL == data) {
        return;
    }
    cLogan_model temp_model; //临时的clogan_model //这里为什么要有一个临时的model
    int status_header = 1;
    memset(&temp_model, 0, sizeof(cLogan_model));
    if (Z_OK != init_zlib_clogan(&temp_model)) {
        status_header = 0;
    }

    if (status_header) {
        init_encrypt_key_clogan(&temp_model);
        int length = data->data_len * 10;// 这里为什么要乘以10
        unsigned char temp_memory[length];
        memset(temp_memory, 0, length);
        temp_model.total_len = 0;
        temp_model.last_point = temp_memory;
		//数据内存缓存（是头文件json字符传的10倍大小）
		//初始换内存 head_tag[1] + section_length[4] last_point指向content_length之后
        restore_last_position_clogan(&temp_model);
		//压缩加密数据
        clogan_zlib_compress(&temp_model, data->data, data->data_len);
		//资源回收//这里应该不是资源回收
		clogan_zlib_end_compress(&temp_model);
		//更新数据长度
		update_length_clogan(&temp_model);

        fwrite(temp_memory, sizeof(char), temp_model.total_len, loganModel->file);//写入到文件中
        fflush(logan_model->file);
        loganModel->file_len += temp_model.total_len; //修改文件大小
    }

    if (temp_model.is_malloc_zlib) {
        free(temp_model.strm);
        temp_model.is_malloc_zlib = 0;
    }
    construct_data_delete_clogan(data);
}

/*
 * 文件写入磁盘、更新文件大小
 * 
 * point  mmap中缓存日志的起始地址（包含日志文件的长度信息，占3位）
 * size   一个char的大小，应该是1字节
 * length mmap中缓存日志的大小 
 *
 */
 void write_dest_clogan(void *point, size_t size, size_t length, cLogan_model *loganModel) {
    if (!is_file_exist_clogan(loganModel->file_path)) { //如果文件被删除,再创建一个文件
        if (logan_model->file_stream_type == LOGAN_FILE_OPEN) {
            fclose(logan_model->file);
            logan_model->file_stream_type = LOGAN_FILE_CLOSE;
        }
        if (NULL != _dir_path) {//这个文件是当前的时间戳的路径
            if (!is_file_exist_clogan(_dir_path)) {
                makedir_clogan(_dir_path);
            }
            init_file_clogan(logan_model);
            printf_clogan("clogan_write > create log file , restore open file stream \n");
        }
    }
    if (CLOGAN_EMPTY_FILE == loganModel->file_len) { //如果是空文件插入一行CLogan的头文件//这个头文件有什么用呢
        insert_header_file_clogan(loganModel);
    }
    fwrite(point, sizeof(char), logan_model->total_len, logan_model->file);//写入到文件中
    fflush(logan_model->file);
    loganModel->file_len += loganModel->total_len; //修改文件大小
}

void write_flush_clogan() {
    if (logan_model->zlib_type == LOGAN_ZLIB_ING) {
        //什么情况下会走到这一步呢？
        //write的情况下，并且是在mmap文件占用超过三分之一时会在没有调end的情况下flush，然后进入到这一步
        //这一步不是很合理！！！应该放回到write的地方去
        clogan_zlib_end_compress(logan_model);
        update_length_clogan(logan_model);
    }
    if (logan_model->total_len > LOGAN_WRITEPROTOCOL_HEAER_LENGTH) {
        unsigned char *point = logan_model->total_point;
        point += LOGAN_MMAP_TOTALLEN;//前三位记录长度
        write_dest_clogan(point, sizeof(char), logan_model->total_len, logan_model);
        printf_clogan("write_flush_clogan > logan total len : %d \n", logan_model->total_len);
        clear_clogan(logan_model);
    }
}

void clogan_write2(char *data, int length) {
    if (NULL != logan_model && logan_model->is_ok) {
		//压缩加密
		clogan_zlib_compress(logan_model, data, length);
        update_length_clogan(logan_model); //有数据操作,要更新数据长度到缓存中
        int is_gzip_end = 0;

        if (!logan_model->file_len ||
            logan_model->content_len >= LOGAN_MAX_GZIP_UTIL) { //是否一个压缩单元结束//压缩单元有什么意义吗？//20K压缩到5k？或者40k压缩到5k？压缩完成的超过5k就准备写入
            clogan_zlib_end_compress(logan_model);
            is_gzip_end = 1;
            update_length_clogan(logan_model);
        }


		//什么时候向mmap文件中写入数据
        int isWrite = 0;
        if (!logan_model->file_len && is_gzip_end) { //如果是个空文件、第一条日志写入
            isWrite = 1;
            printf_clogan("clogan_write2 > write type empty file \n");
        } else if (buffer_type == LOGAN_MMAP_MEMORY && is_gzip_end) { //如果是内存缓存直接写入文件
            isWrite = 1;
            printf_clogan("clogan_write2 > write type memory \n");
        } else if (buffer_type == LOGAN_MMAP_MMAP &&
                   logan_model->total_len >=
                   buffer_length / LOGAN_WRITEPROTOCOL_DEVIDE_VALUE) { //如果是MMAP 且 文件长度已经超过三分之一
            isWrite = 1;
            printf_clogan("clogan_write2 > write type MMAP \n");
        }
        if (isWrite) { //写入
            //有什么情况会在没有end的情况下走进到这一步里面
            //如果是MMAP 且 文件长度已经超过三分之一
            write_flush_clogan();
        } else if (is_gzip_end) { //如果是mmap类型,不回写IO,初始化下一步
            //什么情况下会走到这一步呢？ 文件为空  或者  压缩大小超过5kb，
            logan_model->content_len = 0;
            logan_model->remain_data_len = 0;//这里为什么可以是0？  end过后补位补满了？
            init_zlib_clogan(logan_model);
            restore_last_position_clogan(logan_model);
            init_encrypt_key_clogan(logan_model);
        }
    }
}

//如果数据流非常大,切割数据,分片写入
//按照这边的逻辑来看基本都会去做分片写入数据的操作
//分片操作有什么优势吗，一次性写入和分开写入会有性能上的差异吗？
void clogan_write_section(char *data, int length) {
    int size = LOGAN_WRITE_SECTION;
    int times = length / size;
    int remain_len = length % size;
    char *temp = data;
    int i = 0;
    for (i = 0; i < times; i++) {
        clogan_write2(temp, size);
        temp += size;
    }
    if (remain_len) {
        clogan_write2(temp, remain_len);
    }
}

/**
 @brief 写入数据 按照顺序和类型传值(强调、强调、强调)
 @param flag 日志类型 (int)
 @param log 日志内容 (char*)
 @param local_time 日志发生的本地时间，形如1502100065601 (long long)
 @param thread_name 线程名称 (char*)
 @param thread_id 线程id (long long) 为了兼容JAVA
 @param ismain 是否为主线程，0为是主线程，1位非主线程 (int)
 */
int
clogan_write(int flag, char *log, long long local_time, char *thread_name, long long thread_id,
             int is_main) {
    int back = CLOGAN_WRITE_FAIL_HEADER;
    if (!is_init_ok || NULL == logan_model || !is_open_ok) {
        back = CLOGAN_WRITE_FAIL_HEADER;
        return back;
    }


    //1. 检查目标的日志文件是否存在，如果不存在则需要再做创建
    if (is_file_exist_clogan(logan_model->file_path)) {
        //当文件超过最大大小的时候会直接忽略，这里是否可以加入一些策略,是否可以加入一些优先级的判断
        if (logan_model->file_len > max_file_len) {
            printf_clogan("clogan_write > beyond max file , cant write log\n");
            back = CLOAGN_WRITE_FAIL_MAXFILE;
            return back;
        }
    } else {
        //这套逻辑又是在哪里看到过
        //关闭已经打开的文件，重新创建一个新的文件
        if (logan_model->file_stream_type == LOGAN_FILE_OPEN) {
            fclose(logan_model->file);
            logan_model->file_stream_type = LOGAN_FILE_CLOSE;
        }
        if (NULL != _dir_path) {
            if (!is_file_exist_clogan(_dir_path)) {
                makedir_clogan(_dir_path);
            }
            init_file_clogan(logan_model);
            printf_clogan("clogan_write > create log file , restore open file stream \n");
        }
    }

    //2. 判断MMAP文件是否存在,如果被删除,用内存缓存  难道是让流氓软件清掉了,应该不会出现这种情况，就算出现这种情况是否重新打开mmap来进行补救？
    if (buffer_type == LOGAN_MMAP_MMAP && !is_file_exist_clogan(_mmap_file_path)) {
        if (NULL != _cache_buffer_buffer) {
            buffer_type = LOGAN_MMAP_MEMORY;
            buffer_length = LOGAN_MEMORY_LENGTH;

            printf_clogan("clogan_write > change to memory buffer");

            _logan_buffer = _cache_buffer_buffer;
            logan_model->total_point = _logan_buffer;
            logan_model->total_len = 0;
            logan_model->content_len = 0;
            logan_model->remain_data_len = 0;
            //这里为什么要重新init一次
            if (logan_model->zlib_type == LOGAN_ZLIB_INIT) {
                clogan_zlib_delete_stream(logan_model); //关闭已开的流
            }

            logan_model->last_point = logan_model->total_point + LOGAN_MMAP_TOTALLEN;
            restore_last_position_clogan(logan_model);
            init_zlib_clogan(logan_model);
            init_encrypt_key_clogan(logan_model);
            logan_model->is_ok = 1;
        } else {
            //这块为什么不直接返回，没有缓存怎么处理日志 应该return 虽然很少有可能会在mmap失败 盛情内存也失败的情况出现
            buffer_type = LOGAN_MMAP_FAIL;
            is_init_ok = 0;
            is_open_ok = 0;
            _logan_buffer = NULL;
        }
    }

	//3. 组装一个json格式的日志数据体
    Construct_Data_cLogan *data = construct_json_data_clogan(log, flag, local_time, thread_name,
                                                             thread_id, is_main);
    if (NULL != data) {
        //4. 分段写入到mmap或者内存缓存中
        clogan_write_section(data->data, data->data_len);
        construct_data_delete_clogan(data);
        back = CLOGAN_WRITE_SUCCESS;
    } else {
        back = CLOGAN_WRITE_FAIL_MALLOC;
    }
    return back;
}

int clogan_flush(void) {
    int back = CLOGAN_FLUSH_FAIL_INIT;
    if (!is_init_ok || NULL == logan_model) {
        return back;
    }
    write_flush_clogan();
    back = CLOGAN_FLUSH_SUCCESS;
    printf_clogan(" clogan_flush > write flush\n");
    return back;
}

void clogan_debug(int debug) {
    set_debug_clogan(debug);
}
