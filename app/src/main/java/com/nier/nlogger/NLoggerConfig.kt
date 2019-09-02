package com.nier.nlogger

import android.content.Context
import java.io.File
import java.security.AccessControlContext

/**
 * Created by fgd
 * Date 2019/5/29 21:29
 */
class NLoggerConfig(val cacheFilePath: String, val logFileDir: String) {
    private val DEFAULT_ENCRYPT_KEY = "hexinapp20190807"
    private val DEFAULT_ENCRYPT_IV = "nierhexinlognier"
    private val DEFAULT_MAX_LOG_FILE_SIZE = 10L * 1024L * 1024L

    var encryptKey: String = DEFAULT_ENCRYPT_KEY
    var encryptIV: String = DEFAULT_ENCRYPT_IV
    var maxLogFileSize: Long = DEFAULT_MAX_LOG_FILE_SIZE

    companion object {
        fun DEFAULT_CONFIG(context: Context): NLoggerConfig {
            return NLoggerConfig(
                context.applicationContext.filesDir.absolutePath,
                context.applicationContext.getExternalFilesDir(null)!!.absolutePath + File.separator + "nlogger"
            )
        }
    }
}