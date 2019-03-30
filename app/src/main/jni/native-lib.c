//
// Created by fgd on 2019/3/21.
//

#include <jni.h>
#include <stdlib.h>
#include <string.h>
#include<android/log.h>

#define LOG_TAG  "[NierLogger]"
#define LOGD(...)  __android_log_print(ANDROID_LOG_DEBUG, LOG_TAG, __VA_ARGS__)

JNIEXPORT jstring JNICALL
Java_com_nier_nlogger_MainActivity_sayHello(JNIEnv *env, jobject instance, jstring content) {
    const char *arg = (*env)->GetStringUTFChars(env, content, 0);
    char *result = ", hello...";
    LOGD("result -> %s", result);

    size_t total_size = strlen(arg) + strlen(result);

    char *final = malloc(total_size + 1);
    memset(final, 0, total_size + 1);
    strcat(final, arg);
    strcat(final, result);

    LOGD("final result >>>> %s.", final);

    return (*env)->NewStringUTF(env, final);
}


