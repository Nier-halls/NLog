//
// Created by fgd on 2019/4/28.
//

#include <android/log.h>

#ifndef NLOGGER_ANDROID_LOG_UTIL_H
#define NLOGGER_ANDROID_LOG_UTIL_H

#endif //NLOGGER_ANDROID_LOG_UTIL_H





// Windows 和 Linux 这两个宏是在 CMakeLists.txt 通过 ADD_DEFINITIONS 定义的
#ifdef Windows
#define __FILENAME__ (strrchr(__FILE__, '\\') + 1) // Windows下文件目录层级是'\\'
#elif Linux
#define __FILENAME__ (strrchr(__FILE__, '/') + 1) // Linux下文件目录层级是'/'
#else
#define __FILENAME__ (strrchr(__FILE__, '/') + 1) // 默认使用这种方式
#endif


#define LOGV(TAG, format, ...) __android_log_print(ANDROID_LOG_VERBOSE, "NLogger_"TAG,\
        "[%s:%d] " format, __FILENAME__, __LINE__, ##__VA_ARGS__);

#define LOGD(TAG, format, ...) __android_log_print(ANDROID_LOG_DEBUG, "NLogger_"TAG,\
        "[%s:%d] " format, __FILENAME__, __LINE__, ##__VA_ARGS__);

#define LOGI(TAG, format, ...) __android_log_print(ANDROID_LOG_INFO, "NLogger_"TAG,\
        "[%s:%d] " format, __FILENAME__, __LINE__, ##__VA_ARGS__);

#define LOGW(TAG, format, ...) __android_log_print(ANDROID_LOG_WARN, "NLogger_"TAG,\
        "[%s:%d] " format, __FILENAME__, __LINE__, ##__VA_ARGS__);

#define LOGE(TAG, format, ...) __android_log_print(ANDROID_LOG_ERROR, "NLogger_"TAG,\
        "[%s:%d] " format, __FILENAME__, __LINE__, ##__VA_ARGS__);

