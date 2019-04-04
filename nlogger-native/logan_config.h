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

#ifndef CLOGAN_LOGAN_CONFIG_H
#define CLOGAN_LOGAN_CONFIG_H

#include <zlib.h>
#include <stdio.h>

#define LOGAN_VERSION_KEY "nlogger_version"
#define LOGAN_PATH_KEY "file_path"

#define  LOGAN_WRITE_PROTOCOL_HEADER '^'
#define  LOGAN_WRITE_PROTOCOL_TAIL '&'

#define LOGAN_CACHE_DIR "nlogger_cache"
#define LOGAN_CACHE_FILE "nlogger_mmap"

#define LOGAN_MMAP_HEADER_PROTOCOL '^' //MMAP的头文件标识符
#define LOGAN_MMAP_TAIL_PROTOCOL '&' //MMAP尾文件标识符
#define LOGAN_MMAP_TOTALLEN  3 //MMAP文件长度

#define LOGAN_MAX_GZIP_UTIL 5 * 1024 //压缩单元的大小

#define LOGAN_WRITEPROTOCOL_HEAER_LENGTH 5 //Logan写入协议的头和写入数据的总长度

#define LOGAN_WRITEPROTOCOL_DEVIDE_VALUE 3 //多少分之一写入

#define LOGAN_DIVIDE_SYMBOL "/"

#define LOGAN_LOGFILE_MAXLENGTH 10 * 1024 * 1024

#define LOGAN_WRITE_SECTION 20 * 1024 //多大长度做分片 20kb

#define LOGAN_RETURN_SYMBOL "\n"

#define LOGAN_FILE_NONE 0
#define LOGAN_FILE_OPEN 1
#define LOGAN_FILE_CLOSE 2

#define CLOGAN_EMPTY_FILE 0

#define CLOGAN_VERSION_NUMBER 3 //Logan的版本号(2)版本

typedef struct logan_model_struct {
    int total_len; //mmap文件中日志文件的数据长度  是卸载header后面的3位的那个数字吗？
    char *file_path; //日志的文件的路径，一般在外部storage目录下面

    int is_malloc_zlib;
    z_stream *strm; //压缩的时候 z_stream 的地址 
    int zlib_type; //压缩类型
    char remain_data[16]; //剩余空间
    int remain_data_len; //剩余空间长度

    int is_ready_gzip; //是否可以gzip

    int file_stream_type; //文件流类型
    FILE *file; //日志文件流

    long file_len; //日志文件当前的文件大小

    unsigned char *buffer_point; //缓存的指针 (不变)
    unsigned char *last_point; //最后写入位置的指针
    unsigned char *total_point; //mmap中总数的指针 (可能变) , 给c看,低字节//为什么可能变？？？
    unsigned char *content_lent_point;//协议内容长度指针 , 给java看,高字节//为什么要给java看？ 难道java需要通过JNI来读取这个字段？
    int content_len; //内容的大小

    unsigned char aes_iv[16]; //aes_iv
    int is_ok;

} cLogan_model;

#endif //CLOGAN_LOGAN_CONFIG_H
