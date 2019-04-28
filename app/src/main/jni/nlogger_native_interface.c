
#include "nlogger_native_interface.h"
#include "nlogger_android_log_util.h"
#include "nlogger.h"

JNIEXPORT jint JNICALL
Java_com_nier_nlogger_NLoggerProxy_nativeWrite(JNIEnv *env, jobject instance, jint flag,
                                               jstring log_, jlong local_time,
                                               jstring thread_name_, jlong thread_id,
                                               jint is_main) {
    const char *log = (*env)->GetStringUTFChars(env, log_, 0);
    const char *thread_name = (*env)->GetStringUTFChars(env, thread_name_, 0);

    jint code = (jint) clogan_write(flag, log, local_time, thread_name, thread_id, is_main);

    (*env)->ReleaseStringUTFChars(env, log_, log);
    (*env)->ReleaseStringUTFChars(env, thread_name_, thread_name);
    return code;

}

JNIEXPORT jint JNICALL
Java_com_nier_nlogger_NLoggerProxy_nativeOpen(JNIEnv *env, jobject instance,
                                              jstring file_name_) {
    const char *file_name = (*env)->GetStringUTFChars(env, file_name_, 0);

    jint code = (jint) clogan_open(file_name);

    (*env)->ReleaseStringUTFChars(env, file_name_, file_name);
    return code;
}

JNIEXPORT jint JNICALL
Java_com_nier_nlogger_NLoggerProxy_nativeInit(JNIEnv *env, jobject instance,
                                              jstring cache_path_,
                                              jstring dir_path_, jint max_file,
                                              jstring encrypt_key16_,
                                              jstring encrypt_iv16_) {
    const char *dir_path = (*env)->GetStringUTFChars(env, dir_path_, 0);
    const char *cache_path = (*env)->GetStringUTFChars(env, cache_path_, 0);
    const char *encrypt_key16 = (*env)->GetStringUTFChars(env, encrypt_key16_, 0);
    const char *encrypt_iv16 = (*env)->GetStringUTFChars(env, encrypt_iv16_, 0);

    /**
     * todo
     * 1. 检查目录合法性log_dir, cache_dir
     * 2. free之前在堆上的问题件名
     * 3. 配置encryptKey和iv
     * 4. 拼装mmap_cache_file的完整地址
     * 5. 拼装并且创建log_dir
     * 6. open mmap
     */

//    jint code = (jint) clogan_init(cache_path, dir_path, max_file, encrypt_key16, encrypt_iv16);

    LOGD("JNI", "\ndir_path >>> %s\n, cache_path >>> %s\n, encrypt_key16 >>> %s\n, encrypt_iv16 >>> %s", dir_path,
         cache_path, encrypt_key16, encrypt_iv16);

    (*env)->ReleaseStringUTFChars(env, dir_path_, dir_path);
    (*env)->ReleaseStringUTFChars(env, cache_path_, cache_path);
    (*env)->ReleaseStringUTFChars(env, encrypt_key16_, encrypt_key16);
    (*env)->ReleaseStringUTFChars(env, encrypt_iv16_, encrypt_iv16);
    return 0;
}

JNIEXPORT jint JNICALL
Java_com_nier_nlogger_NLoggerProxy_nativeFlush(JNIEnv *env, jobject instance) {
    clogan_flush();
}

JNIEXPORT jstring JNICALL
Java_com_nier_nlogger_NLoggerProxy_sayHello(JNIEnv *env, jobject instance, jstring content) {
    const char *arg = (*env)->GetStringUTFChars(env, content, 0);
    char *result = ", hello log native...";
//    LOGD("result -> %s", result);

    size_t total_size = strlen(arg) + strlen(result);

    char *final = malloc(total_size + 1);
    memset(final, 0, total_size + 1);
    strcat(final, arg);
    strcat(final, result);

//    LOGD("final result >>>> %s.", final);

    return (*env)->NewStringUTF(env, final);
}
