package com.nier.nlogger

import android.os.Looper

/**
 * Created by fgd
 * Date 2019/5/29 14:29
 */
class LogEntry(val content: String, val fileName: String) {
    private val DEFAULT_LOG_TYPE = 0

    val threadName: String = Thread.currentThread().name//记录日志的线程名字

    val threadId: Long = Thread.currentThread().id//记录日志的线程名字

    var priority = 10//权重，可以根据权重来做一些操作，如直接发送，缓存满是做筛选(权重越小 优先级越高)

    val isMain: Boolean = Looper.getMainLooper() == Looper.myLooper() //日志记录是否在主线程

    val type = DEFAULT_LOG_TYPE//日志类型

    val date: Long = System.currentTimeMillis()//记录日志的时间

    val sdkVersion = android.os.Build.VERSION.SDK_INT//android版本

    override fun toString(): String {
        return "LogEntry{" +
                "content='$content', " +
                "DEFAULT_LOG_TYPE=$DEFAULT_LOG_TYPE, " +
                "threadName='$threadName', " +
                "threadId=$threadId, " +
                "priority=$priority, " +
                "isMain=$isMain, " +
                "type=$type, " +
                "date=$date, " +
                "sdkVersion=$sdkVersion" +
                "}"
    }


}