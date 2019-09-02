//
// Created by fgd on 2019/5/17.
//

#include <malloc.h>
#include "nlogger_data_handler.h"
#include "../nlogger_error_code.h"
#include "../utils/nlogger_android_log.h"
#include "../nlogger_constants.h"
#include "mbedtls/aes.h"

void _real_mbedtls_encrypt(struct nlogger_data_handler_struct *data_handler, char *source, size_t length, char *destination);

size_t _aes_encrypt(struct nlogger_data_handler_struct *data_handler, char *destination, unsigned char *data, size_t data_length);

size_t _zlib_compress_with_encrypt(struct nlogger_data_handler_struct *data_handler, char *destination, char *source, size_t source_length, int type);

/**
 * 初始化加密配置，缓存加密密钥以及偏移量
 *
 * @param data_handler
 * @param encrypt_key
 * @param encrypt_iv
 * @return
 */
int init_encrypt(struct nlogger_data_handler_struct *data_handler, const char *encrypt_key, const char *encrypt_iv) {
    //配置加密相关的参数
    char *temp_encrypt_key = malloc(sizeof(char) * 16);
    memcpy(temp_encrypt_key, encrypt_key, sizeof(char) * 16);
    data_handler->p_encrypt_key = temp_encrypt_key;
    char *temp_encrypt_iv = malloc(sizeof(char) * 16);
    memcpy(temp_encrypt_iv, encrypt_iv, sizeof(char) * 16);
    data_handler->p_encrypt_iv_source  = temp_encrypt_iv;
    data_handler->p_encrypt_iv_pending = malloc(sizeof(char) * 16);
    return ERROR_CODE_OK;
}

/**
 * 初始化zlib相关的压缩配置
 *
 * @param data_handler
 * @return
 */
int init_zlib(struct nlogger_data_handler_struct *data_handler) {

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
    //初始化填充iv，一段数据可以分多次压缩但是只能填充一次偏移量
    memcpy(data_handler->p_encrypt_iv_pending, data_handler->p_encrypt_iv_source, 16);

    data_handler->state = NLOGGER_HANDLER_STATE_INIT;
    return ERROR_CODE_OK;
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
size_t compress_and_write_data(struct nlogger_data_handler_struct *data_handler, char *destination, char *source, size_t length, void (*callback)(size_t)) {
    //todo 状态检查
    LOGW("compress_data", "start compress_and_write_data, target length >>> %zd", length);
    size_t handled = 0;
    //todo 压缩数据 加密数据 不一定所有数据都会写入，会有一小部分数据缓存等待下一次写入
    handled = _zlib_compress_with_encrypt(data_handler, destination, source, length, Z_SYNC_FLUSH);
    LOGW("compress_data", "after compress_and_write_data, handled >>> %zd", handled);
    data_handler->state = NLOGGER_HANDLER_STATE_HANDLING;
    if (handled > 0) {
        (*callback)(handled);
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
size_t _zlib_compress_with_encrypt(struct nlogger_data_handler_struct *data_handler, char *destination, char *source,
                                   size_t source_length, int type) {
    size_t   handled = 0;
    z_stream *stream = data_handler->p_stream;

    unsigned char out[NLOGGER_ZLIB_COMPRESS_CHUNK_SIZE];
    unsigned int  have;
    stream->avail_in = (uInt) source_length;
    stream->next_in  = (Bytef *) source;
    do {
        stream->avail_out = NLOGGER_ZLIB_COMPRESS_CHUNK_SIZE;
        stream->next_out  = out;
        int def_res = deflate(stream, type);
        if (Z_STREAM_ERROR == def_res) {
            LOGE("zlib","compress log data on error.");
            //这里如果出现错误会导致日志错位，日志截断，怎么主动发现这个错误的日志
            deflateEnd(stream);
            data_handler->state = NLOGGER_HANDLER_STATE_IDLE;
            return (size_t) 0;
        }
        have = NLOGGER_ZLIB_COMPRESS_CHUNK_SIZE - stream->avail_out;
        handled += _aes_encrypt(data_handler, destination, out, have);
        //直接写入不进行加密
//        handled += have;
//        memcpy(destination, out, have);
        //更新下次写入的指针
        // todo 换一个变量名字
        destination += have;
//        handled += have;
    } while (0 == stream->avail_out);

    return handled;
}


/**
 * 进行aes加密，目前没有实现，只是一次copy
 *
 * @param data_handler
 * @param destination
 * @param data
 * @param data_length
 * @return
 */
size_t _aes_encrypt(struct nlogger_data_handler_struct *data_handler, char *destination, unsigned char *data, size_t data_length) {
    size_t        handled               = 0;
    size_t        unencrypt_data_length = data_length + data_handler->remain_data_length;
    size_t        handle_data_length    = (unencrypt_data_length / NLOGGER_AES_ENCRYPT_UNIT) * (size_t) NLOGGER_AES_ENCRYPT_UNIT;
    size_t        remain_data_length    = unencrypt_data_length % (size_t) NLOGGER_AES_ENCRYPT_UNIT;

//    LOGE("encrypt", "上次剩余数据长度 >>> %zd", data_handler->remain_data_length)
//    LOGE("encrypt", "计划写入总数据长度 >>> %zd", unencrypt_data_length)
//    LOGE("encrypt", "允许写入数据长度 >>> %zd", handle_data_length)
//    LOGE("encrypt", "剩余数据长度 >>> %zd", remain_data_length)

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
        _real_mbedtls_encrypt(data_handler, (char *) handle_data, handle_data_length, destination);

        handled += handle_data_length;
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
        //todo [BUG] 这块赋值不能放在里面，查了一天，在剩余数据为0的情况下就不能设置这个值了！！！
        //data_handler->remain_data_length = remain_data_length;
    }
    data_handler->remain_data_length = remain_data_length;
    return handled;
}

/**
 * 调用第三方库进行加密
 *
 * @param data_handler
 * @param source
 * @param length
 * @param destination
 */
void _real_mbedtls_encrypt(struct nlogger_data_handler_struct *data_handler, char *source, size_t length, char *destination) {
    mbedtls_aes_context context;
    mbedtls_aes_setkey_enc(&context, (unsigned char *) data_handler->p_encrypt_key, 128);
    mbedtls_aes_crypt_cbc(
            &context,
            MBEDTLS_AES_ENCRYPT,
            length,
            (unsigned char *) data_handler->p_encrypt_iv_pending,
            source,
            (unsigned char *) destination
    ); //加密
}

/**
 * 结束压缩数据
 *
 * @param data_handler
 * @param destination 写入的目的地指针
 *
 * @return 写入数据的长度
 */
size_t finish_compress_data(struct nlogger_data_handler_struct *data_handler, char *destination, void (*callback)(size_t)) {
    LOGI("finish_compress", "start finish_compress_data.")
    size_t handled = 0;
    if (data_handler->state != NLOGGER_HANDLER_STATE_HANDLING) {
        return (size_t) -1;
    }

    handled = _zlib_compress_with_encrypt(data_handler, destination, NULL, 0, Z_FINISH);
    deflateEnd(data_handler->p_stream);
    destination += handled;

    //如果还有未满16位的未加密数据，拿出先加密
    if (data_handler->remain_data_length > 0) {
        LOGD("finish_compress", "push remain data to cache remain_length >>> %zd", data_handler->remain_data_length)
        char remain[NLOGGER_AES_ENCRYPT_UNIT];
        memset(remain, '\0', NLOGGER_AES_ENCRYPT_UNIT);
        memcpy(remain, data_handler->p_remain_data, data_handler->remain_data_length);
        //加密并且把数据输出到destination目标地址中
        _real_mbedtls_encrypt(data_handler, (char *) remain, NLOGGER_AES_ENCRYPT_UNIT, destination);

//        memcpy(destination, remain, NLOGGER_AES_ENCRYPT_UNIT);
        data_handler->remain_data_length = 0;
        handled += NLOGGER_AES_ENCRYPT_UNIT;
    }
    data_handler->state = NLOGGER_HANDLER_STATE_IDLE;
    if (handled > 0) {
        (*callback)(handled);
    }
    //todo 关于remain_data写入以及delete_end等收尾工作
    return handled;
}

/**
 * 状态判断：当前是否已经进行初始化
 *
 * @param data_handler
 * @return
 */
int is_data_handler_init(struct nlogger_data_handler_struct *data_handler) {
    return data_handler->state != NLOGGER_HANDLER_STATE_IDLE;
}

/**
 * 状态判断：当前是否能进行数据压缩（加密）操作
 *
 * @param data_handler
 * @return
 */
int is_data_handler_processing(struct nlogger_data_handler_struct *data_handler) {
    return data_handler->state == NLOGGER_HANDLER_STATE_INIT || data_handler->state == NLOGGER_HANDLER_STATE_HANDLING;
}