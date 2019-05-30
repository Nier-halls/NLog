package com.nier.nlogger.action

import android.util.Log
import com.nier.nlogger.ILogHandler
import com.nier.nlogger.LogEntry

/**
 * Created by fgd
 * Date 2019/5/29 14:25
 */
class Write(val log: String) : IAction {
    private val TAG = "Nlogger_C_Write"
    private val mLogEntry = LogEntry(log)


    override fun run(logHandler: ILogHandler): Int {
        Log.d(TAG, "Invoke Action [Write] run, threadId=${Thread.currentThread()}")

        return if (log.isEmpty() || log.isBlank()) {
            Log.e(TAG, "Empty or blank log.")
            -1
        } else {
            logHandler.write(mLogEntry)
        }
    }

    override fun toString(): String {
        return "Write{${hashCode()} mLogEntry=$mLogEntry}"
    }


}