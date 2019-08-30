package com.nier.nlogger.action

import android.os.Build.VERSION_CODES.P
import android.util.Log
import com.nier.nlogger.ILogHandler
import com.nier.nlogger.ISendTask
import com.nier.nlogger.NLogger
import java.io.File

/**
 * Created by fgd
 * Date 2019/5/29 14:25
 */

class Send : IAction {
    private val TAG = "Nlogger_C_Send"

    override fun run(logHandler: ILogHandler): Int {
        Log.d(TAG, "Invoke Action [Send] run, threadId=${Thread.currentThread()}")
        val sendHandler: (file: File) -> Int = sendHandler@{
            Log.d(TAG, "Send file ${it.absolutePath}")
            return@sendHandler 0
        }
        val fileDirString = (logHandler as NLogger).currentInitConfig?.logFileDir
        if (fileDirString.isNullOrEmpty()) {
            Log.e(TAG, "Send file on invalid file dir $fileDirString")
            return -1
        }
        val logFileDir = File(fileDirString)
        if (!logFileDir.exists() || !logFileDir.isDirectory) {
            Log.e(TAG, "Send file on invalid file dir $fileDirString")
            return -1
        }
        val logFilePaths = ArrayList<String>()
        logFileDir.listFiles().forEach { file ->
            if (!file.isDirectory){
                logFilePaths.add(file.absolutePath)
            }
        }
        if (logFilePaths.isNullOrEmpty()){
            return -1
        }

        return logHandler.send(logFilePaths, object : ISendTask{
            override fun doSend(file: File) {
                Log.d(TAG, "Send action [doSend] run, file >>> $file.")
            }

            override fun finish() {
                Log.d(TAG, "Send action [finish] run.")
            }

        })
    }

    override fun toString(): String {
        return "Send{${hashCode()}}"
    }


}