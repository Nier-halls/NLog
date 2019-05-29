package com.nier.nlogger

import java.io.File

/**
 * Created by fgd
 * Date 2019/5/29 14:27
 */
interface ILogHandler {
    fun write(log: LogEntry): Int

    fun flush(): Int

    fun send(handler: (logFiles: List<File>) -> Unit): Int
}