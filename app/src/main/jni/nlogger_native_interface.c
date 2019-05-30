
#include "nlogger_native_interface.h"
#include "utils/nlogger_android_log.h"
#include "nlogger.h"

JNIEXPORT jint JNICALL
Java_com_nier_nlogger_NLogger_nativeWrite(JNIEnv *env, jobject instance,
                                               jstring file_name_, jint flag,
                                               jstring log_, jlong local_time,
                                               jstring thread_name_, jlong thread_id,
                                               jint is_main) {
    const char *file_name   = (*env)->GetStringUTFChars(env, file_name_, 0);
    const char *log         = (*env)->GetStringUTFChars(env, log_, 0);
    const char *thread_name = (*env)->GetStringUTFChars(env, thread_name_, 0);

//    jint code = (jint) clogan_write(flag, log, local_time, thread_name, thread_id, is_main);

    jint code = (jint) write_nlogger(file_name, flag, log, local_time, thread_name, thread_id, is_main);

    (*env)->ReleaseStringUTFChars(env, log_, log);
    (*env)->ReleaseStringUTFChars(env, thread_name_, thread_name);
    (*env)->ReleaseStringUTFChars(env, file_name_, file_name);
    return code;

}

JNIEXPORT jint JNICALL
Java_com_nier_nlogger_NLogger_nativeInit(JNIEnv *env,
                                              jobject instance,
                                              jstring cache_path_,
                                              jstring dir_path_,
                                              jint max_file,
                                              jstring encrypt_key16_,
                                              jstring encrypt_iv16_) {
    const char *dir_path      = (*env)->GetStringUTFChars(env, dir_path_, 0);
    const char *cache_path    = (*env)->GetStringUTFChars(env, cache_path_, 0);
    const char *encrypt_key16 = (*env)->GetStringUTFChars(env, encrypt_key16_, 0);
    const char *encrypt_iv16  = (*env)->GetStringUTFChars(env, encrypt_iv16_, 0);

    jint code = (jint) init_nlogger(dir_path, cache_path, encrypt_key16, encrypt_iv16);

    (*env)->ReleaseStringUTFChars(env, dir_path_, dir_path);
    (*env)->ReleaseStringUTFChars(env, cache_path_, cache_path);
    (*env)->ReleaseStringUTFChars(env, encrypt_key16_, encrypt_key16);
    (*env)->ReleaseStringUTFChars(env, encrypt_iv16_, encrypt_iv16);
    return code;
}

JNIEXPORT jint JNICALL
Java_com_nier_nlogger_NLogger_nativeFlush(JNIEnv *env, jobject instance) {
    return flush_nlogger();
}
