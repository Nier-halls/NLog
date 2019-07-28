
#ifndef NLOGGER_BASE_UTIL_H
#define NLOGGER_BASE_UTIL_H

//获取时间毫秒currentTimeMillis
long long system_current_time_millis_nlogger(void);

/**
 * 调整字节序,默认传入的字节序为低字节序,如果系统为高字节序,转化为高字节序
 * c语言字节序的写入是低字节序的,读取默认也是低字节序的
 * java语言字节序默认是高字节序的
 * @param data
 */
void adjust_byte_order_nlogger(char item[4]);

int is_empty_string(const char *string);

#endif //NLOGGER_BASE_UTIL_H
