package com.nier.nlogger.action

import android.util.Log
import com.nier.nlogger.ILogHandler

/**
 * Created by fgd
 * Date 2019/5/29 14:25
 *
 */
class Flush : IAction {
    private val TAG = "Nlogger_C_Flush"

    override fun run(logHandler: ILogHandler): Int {
        Log.d(TAG, "Invoke Action [Flush] run, threadId=${Thread.currentThread()}")
        return logHandler.flush()
    }

    override fun toString(): String {
        return "Flush{${hashCode()}}"
    }


}