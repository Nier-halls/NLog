package com.nier.nlogger

import android.content.Context
import java.io.File

/**
 * Created by fgd
 * Date 2019/4/2
 */
class NLoggerProxy {
    companion object {
        init {
            System.loadLibrary("nlogger")
        }

        val instance = NLoggerProxy()
        var testIndex = 0
    }

    private external fun nativeWrite(
        fileName: String, flag: Int, log: String, local_time: Long, thread_name: String,
        thread_id: Long, is_main: Int
    ): Int

    private external fun nativeFlush(): Int

    private external fun nativeInit(
        cache_path: String, dir_path: String, max_file: Int, encrypt_key_16: String,
        encrypt_iv_16: String
    ): Int

    private external fun nativeOpen(fileName: String): Int

    private external fun sayHello(content: String): String


    fun init(context: Context) {
//        thread {
        nativeInit(
            context.applicationContext.filesDir.absolutePath,
            context.applicationContext.getExternalFilesDir(null).absolutePath + File.separator + "nlogger",
            10 * 1024 * 1024,
            "nier12345678auto",
            "nier12345678auto"
        )
//        }.start()
    }

    fun open() {
        nativeOpen("niers_log")
    }

    fun write(content: String) {
        nativeWrite(
            "niers_log",
            0,
            "${System.currentTimeMillis()} write a log test heiheihei 2223333 666666 888888 !@#$%^&*()<>?. 试试中文 index=$testIndex",
            System.currentTimeMillis(),
            "main_nier",
            Thread.currentThread().id,
            1
        )

        testIndex++
    }

    fun flush() {
        nativeFlush()
    }


    fun test(string: String): String {
        val result = sayHello(string)
        return result
    }


}