
#include <sys/time.h>
#include <memory.h>
#include "console_util.h"

#define NLOGGER_BYTE_ORDER_NONE  0
#define NLOGGER_BYTE_ORDER_HIGH 1
#define NLOGGER_BYTE_ORDER_LOW 2

//获取毫秒时间戳
long long system_current_time_millis_nlogger(void) {
    struct timeval tv;
    gettimeofday(&tv, NULL);
    long long time = ((long long) tv.tv_sec) * 1000 + ((long long) tv.tv_usec) / 1000;
    return time;
}

//是否为空字符串
int is_empty_string(const char *string) {
    return string == NULL || ((int) strnlen(string, 16) == 0);
}

// 检查CPU的字节序
int _cpu_byte_order_nlogger(void) {
    static int byte_order = NLOGGER_BYTE_ORDER_NONE;
    if (byte_order == NLOGGER_BYTE_ORDER_NONE) {
        union {
            int  i;
            char c;
        } t;
        t.i = 1;
        if (t.c == 1) {
            byte_order = NLOGGER_BYTE_ORDER_LOW;
        } else {
            byte_order = NLOGGER_BYTE_ORDER_HIGH;
        }
    }
    return byte_order;
}

/**
 * 调整字节序,默认传入的字节序为低字节序,如果系统为高字节序,转化为高字节序
 * c语言字节序的写入是低字节序的,读取默认也是低字节序的
 * java语言字节序默认是高字节序的
 * @param data data
 */
void adjust_byte_order_nlogger(char data[4]) {
    if (_cpu_byte_order_nlogger() == NLOGGER_BYTE_ORDER_HIGH) {
        char *temp     = data;
        char data_temp = *temp;
        *temp       = *(temp + 3);
        *(temp + 3) = data_temp;
        data_temp = *(temp + 1);
        *(temp + 1) = *(temp + 2);
        *(temp + 2) = data_temp;
    }
}
