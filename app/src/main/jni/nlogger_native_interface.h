
#ifndef ANDROID_NOVA_LOGAN_CLOGAN_PROTOCOL_H_H
#define ANDROID_NOVA_LOGAN_CLOGAN_PROTOCOL_H_H
#ifdef __cplusplus
extern "C"
{
#endif

#include <jni.h>
#include <stdlib.h>
#include <string.h>
//#include <clogan_core.h>

/**
 * JNI write interface
 */
JNIEXPORT jint JNICALL
Java_com_nier_nlogger_NLogger_nativeWrite(JNIEnv *env, jobject instance,
                                               jstring file_name,
                                               jint flag, jstring log_,
                                               jlong local_time, jstring thread_name_,
                                               jlong thread_id, jint ismain);
/**
 * JNI init interface
 */
JNIEXPORT jint JNICALL
Java_com_nier_nlogger_NLogger_nativeInit(JNIEnv *env,
                                         jobject instance,
                                         jstring cache_path_,
                                         jstring dir_path_,
                                         jlong max_file_size,
                                         jstring encrypt_key16_,
                                         jstring encrypt_iv16_);

/**
 * JNI init interface
 */
JNIEXPORT jint JNICALL
Java_com_nier_nlogger_Logger1_nativeInit1(JNIEnv *env,
                                         jobject instance,
                                         jstring cache_path_,
                                         jstring dir_path_,
                                         jlong max_file_size,
                                         jstring encrypt_key16_,
                                         jstring encrypt_iv16_);

/**
 * JNI flush interface
 */
JNIEXPORT jint JNICALL
Java_com_nier_nlogger_NLogger_nativeFlush(JNIEnv *env, jobject instance);


JNIEXPORT jstring JNICALL
Java_com_nier_nlogger_NLogger_sayHello(JNIEnv *env, jobject instance, jstring content);

#ifdef __cplusplus
}
#endif

#endif //ANDROID_NOVA_LOGAN_CLOGAN_PROTOCOL_H_H
