package com.nier.nlogger

import java.io.File

/**
 * Created by fgd
 * Date 2019/5/29 14:27
 */
interface ILogHandler {
    /**
     * 写入日志文件
     */
    fun write(log: LogEntry): Int

    /**
     * 将日志从缓冲区（mmap或者memory）中推到日志文件
     */
    fun flush(): Int

    /**
     * 输入文件路径然后截取当前对应路径的日志文件的快照，
     * 通过sendHandler发送快照，确保不会因为发送的同时再同时写入日志
     * 导致异常
     */
    fun send(filePaths: List<String>, sendTask: ISendTask): Int
}