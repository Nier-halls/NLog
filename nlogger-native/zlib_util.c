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


#include "zlib_util.h"
#include "aes_util.h"

int init_zlib_clogan(cLogan_model *model) {
    int ret = 1;
    if (model->zlib_type == LOGAN_ZLIB_INIT) { //如果是init的状态则不需要init
        return Z_OK;
    }
    z_stream *temp_zlib = NULL;
    if (!model->is_malloc_zlib) {
        temp_zlib = malloc(sizeof(z_stream));
    } else {
        temp_zlib = model->strm;
    }

    if (NULL != temp_zlib) {
        model->is_malloc_zlib = 1; //表示已经 malloc 一个zlib
        memset(temp_zlib, 0, sizeof(z_stream));
        model->strm = temp_zlib;
        temp_zlib->zalloc = Z_NULL;
        temp_zlib->zfree = Z_NULL;
        temp_zlib->opaque = Z_NULL;
        ret = deflateInit2(
                temp_zlib, //z_stream
                Z_BEST_COMPRESSION, // gives best compression压缩模式
                Z_DEFLATED, // It must be Z_DEFLATED in this version of the library 写死的不用管
                (15 + 16), //不知道干啥用的 默认15，16会加上与i个header
                8,//默认值表明会使用较大的内存来提高速度
                Z_DEFAULT_STRATEGY//压缩算法，影响加密比率
        );
        if (ret == Z_OK) {
            model->is_ready_gzip = 1;
            model->zlib_type = LOGAN_ZLIB_INIT;
        } else {
            model->is_ready_gzip = 0;
            model->zlib_type = LOGAN_ZLIB_FAIL;
        }
    } else {
        model->is_malloc_zlib = 0;
        model->is_ready_gzip = 0;
        model->zlib_type = LOGAN_ZLIB_FAIL;

    }
    return ret;
}

/**
 * 压缩加密数据，并非完全都加密，满16位的数据才做加密
 * 
 * data 数据的地址
 * data_len 数据的长度
 *
 */
void clogan_zlib(cLogan_model *model, char *data, int data_len, int type) {
    int is_gzip = model->is_ready_gzip;
    int ret;
    //测试, 强行不做GZIP压缩 fgd
    if (is_gzip && 0) {
        // >>>>>>>> gzip压缩的情况 >>>>>>>>>
        unsigned int have; // 这个字段干什么用的
        unsigned char out[LOGAN_CHUNK];
        z_stream *strm = model->strm;

        strm->avail_in = (uInt) data_len;
        strm->next_in = (unsigned char *) data;
        //压缩加密的模块，压缩后的数据分16的倍数去加密，不满16的倍数缓存着
        //循环继续压缩判断16的倍数加密，16的倍数完全是为了加密考虑(内存压缩
        //完成后立即加密是出于什么考虑，为了不能压缩完成以后再做加密?)
        do {
            strm->avail_out = LOGAN_CHUNK;//压缩后数据存放的最大长度  这个长度会慢慢变小
            strm->next_out = (unsigned char *) out;
            ret = deflate(strm, type);
            if (Z_STREAM_ERROR == ret) {
                deflateEnd(model->strm);

                model->is_ready_gzip = 0;
                model->zlib_type = LOGAN_ZLIB_END;
            } else {
                have = LOGAN_CHUNK - strm->avail_out;
                int total_len = model->remain_data_len + have;//剩余未加密的数据
                unsigned char *temp = NULL;
                int handler_len = (total_len / 16) * 16;//找出16整数倍的压缩数据，也就是需要加密的数据块，包含以前剩余的和新压缩好的
                int remain_len = total_len % 16;//找出不满16位的数据
                if (handler_len) {//压缩的数据长度有超过16位
                    int copy_data_len = handler_len - model->remain_data_len;
                    char gzip_data[handler_len];
                    temp = (unsigned char *) gzip_data;//temp的大小为 剩余 + 压缩 数据和（不满16的部分会去掉）的长度
                    //把之前位处的数据拷贝到temp中
                    if (model->remain_data_len) {
                        memcpy(temp, model->remain_data, model->remain_data_len);
                        temp += model->remain_data_len;
                    }
                    memcpy(temp, out, copy_data_len); //填充剩余数据和压缩数据 //把新压缩好的数据放到temp中

                    //加密16的整数倍数据，说到底还是向内存缓存中写入加密数据
                    aes_encrypt_clogan((unsigned char *) gzip_data, model->last_point, handler_len,
                                       (unsigned char *) model->aes_iv); //把加密数据写入缓存
                    //整理数据的大小
                    model->total_len += handler_len;
                    model->content_len += handler_len;
                    model->last_point += handler_len;
                }

                if (remain_len) {//压缩数据不超过16位的
                    if (handler_len) {
                        int copy_data_len = handler_len - model->remain_data_len;
                        temp = (unsigned char *) out;
                        temp += copy_data_len;
                        //拷贝剩余不足16位的数据
                        memcpy(model->remain_data, temp, remain_len); //填充剩余数据和压缩数据
                    } else {
                        //把剩余数据拷贝到remain_data中，感觉效果差不多
                        temp = (unsigned char *) model->remain_data;
                        temp += model->remain_data_len;
                        memcpy(temp, out, have);
                    }
                }
                //这个长度位除16的余数一般不满16
                model->remain_data_len = remain_len;
            }
        } while (strm->avail_out == 0);
    } else {
        // >>>>>>>> 非gzip压缩的情况 >>>>>>>>>
        int total_len = model->remain_data_len + data_len;
        unsigned char *temp = NULL;
        int handler_len = (total_len / 16) * 16;
        int remain_len = total_len % 16;
        //每满16就加密
        if (handler_len) {
            int copy_data_len = handler_len - model->remain_data_len;
            char gzip_data[handler_len];
            temp = (unsigned char *) gzip_data; //这里的temp是不是没有什么用???
            if (model->remain_data_len) {
                memcpy(temp, model->remain_data, model->remain_data_len);
                temp += model->remain_data_len;
            }
            memcpy(temp, data, copy_data_len); //填充剩余数据和压缩数据
            aes_encrypt_clogan((unsigned char *) gzip_data, model->last_point, handler_len,
                              (unsigned char *) model->aes_iv);
//            memcpy(model->last_point, gzip_data, handler_len);

            model->total_len += handler_len;
            model->content_len += handler_len;
            model->last_point += handler_len;
        }
        //未满16的缓存在内存中
        if (remain_len) {
            //为了把不满16的数据再重新放入到remain中
            if (handler_len) {
                //data 是新加入的数据，remain_data_len是之前剩余的数据
                //remain_data + data 是总的数据
                //handler_len 本次处理的数据
                //要得到data中处理了多少数据，用处理的数据减去remain_data的数据就可以知道
                int copy_data_len = handler_len - model->remain_data_len;
                temp = (unsigned char *) data;
                temp += copy_data_len;
                memcpy(model->remain_data, temp, remain_len); //填充剩余数据和压缩数据
            } else {
                temp = (unsigned char *) model->remain_data;
                temp += model->remain_data_len;
                memcpy(temp, data, data_len);
            }
        }
        model->remain_data_len = remain_len;
    }
}

/**
 * 关闭zstream，回收zlib申请的资源，计算总长度以及内容长度
 * model->zlib_type = LOGAN_ZLIB_END;
 * model->is_ready_gzip = 0;
 */
void clogan_zlib_end_compress(cLogan_model *model) {
    clogan_zlib(model, NULL, 0, Z_FINISH);
    (void) deflateEnd(model->strm);
    int val = 16 - model->remain_data_len;//这一步是什么意思
    char data[16];
    // [fgd_debug] < 2019/4/8 22:24 > 不加密的情况下看看是什么 用空字符来处理多余数据
    memset(data, '\0', 16);//这里为什么不用0来做初始化
    if (model->remain_data_len) {
        memcpy(data, model->remain_data, model->remain_data_len);
    }
    aes_encrypt_clogan((unsigned char *) data, model->last_point, 16,
                       (unsigned char *) model->aes_iv); //把加密数据写入缓存
    //上面操作都是把未加密剩余不满16位的加密填满

    model->last_point += 16;
    *(model->last_point) = LOGAN_WRITE_PROTOCOL_TAIL;
    model->last_point++;
    model->remain_data_len = 0;
    model->total_len += 17;//难道是内容填充的16 + 结尾标志的1
    model->content_len += 16; //为了兼容之前协议content_len,只包含内容,不包含结尾符
    model->zlib_type = LOGAN_ZLIB_END;
    model->is_ready_gzip = 0;
}

void clogan_zlib_compress(cLogan_model *model, char *data, int data_len) {
    if (model->zlib_type == LOGAN_ZLIB_ING || model->zlib_type == LOGAN_ZLIB_INIT) {
        model->zlib_type = LOGAN_ZLIB_ING;
        clogan_zlib(model, data, data_len, Z_SYNC_FLUSH);
    } else {
        //什么时候会走到这一步呢?
        init_zlib_clogan(model);
    }
}

void clogan_zlib_delete_stream(cLogan_model *model) {
    (void) deflateEnd(model->strm);
    model->zlib_type = LOGAN_ZLIB_END;
    model->is_ready_gzip = 0;
}
