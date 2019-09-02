package com.nier.nlogger.action

import android.os.Build.VERSION_CODES.P
import android.util.Log
import com.nier.nlogger.ILogHandler
import com.nier.nlogger.ISendTask
import com.nier.nlogger.NLogger
import okhttp3.*
import retrofit2.Retrofit
import java.io.File
import java.util.*
import java.util.concurrent.TimeUnit
import kotlin.collections.ArrayList

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
            if (!file.isDirectory) {
                logFilePaths.add(file.absolutePath)
            }
        }
        if (logFilePaths.isNullOrEmpty()) {
            return -1
        }

        return logHandler.send(logFilePaths, sender)
    }

    override fun toString(): String {
        return "Send{${hashCode()}}"
    }

    val client = OkHttpClient()

    private val sender = object : ISendTask {
        override fun doSend(file: File): Boolean {
            Log.d(TAG, "Send action [doSend] run, file >>> $file.")

            val requestBody = MultipartBody.Builder()
                .setType(MultipartBody.FORM)
                .addFormDataPart(
                    "file", file.name,
                    RequestBody.create(MediaType.parse("multipart/form-data"), file)
                )
                .addFormDataPart("app", "100")
                .addFormDataPart("tm", "6")
                .addFormDataPart("did", "asdfghjklqwer")
                .build()

            val request = Request.Builder()
                .url("")
                .post(requestBody)
                .build()

            val response = client.newCall(request).execute()
            return response.isSuccessful
        }

        override fun onFileSent(file: File, isSuccess: Boolean) {
            Log.d(TAG, "Send action [onFileSent] run， send file=${file.name}， success=${isSuccess}")
            if (isSuccess) {
                file.delete()
            }
        }

    }
}