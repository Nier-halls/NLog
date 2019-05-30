package com.nier.nlogger.action

import android.util.Log
import com.nier.nlogger.ILogHandler

/**
 * Created by fgd
 * Date 2019/5/29 14:25
 */

class Send : IAction {
    private val TAG = "Nlogger_C_Send"

    override fun run(logHandler: ILogHandler): Int {
        Log.d(TAG, "Invoke Action [Send] run, threadId=${Thread.currentThread()}")

        return logHandler.send { logFiles ->
            logFiles.forEach {
                Log.d(TAG, "Start send ${it.path}")
            }
        }
    }

    override fun toString(): String {
        return "Send{${hashCode()}}"
    }


}