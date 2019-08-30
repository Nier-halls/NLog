package com.nier.nlogger

import android.content.Context
import android.util.Log
import com.nier.nlogger.action.Flush
import com.nier.nlogger.action.IAction
import com.nier.nlogger.action.Send
import com.nier.nlogger.action.Write
import java.io.*
import java.lang.Exception

/**
 * Created by fgd
 * Date 2019/5/29 20:43
 *
 * todo 单线程是否可用
 * todo 测试一下 多线程写入是否能够收拢
 * todo 测试一下回调是否有用
 */
class NLogger : ILogHandler {


    private external fun nativeInit(
        cache_path: String, dir_path: String, max_file: Int, encrypt_key_16: String,
        encrypt_iv_16: String
    ): Int

    private external fun nativeWrite(
        file_name: String, flag: Int, log: String, local_time: Long, thread_name: String,
        thread_id: Long, is_main: Int
    ): Int

    private external fun nativeFlush(): Int

    private var isInit = false

//    private lateinit var cacheFilePath: String
//    private lateinit var logFileDir: String
//    private lateinit var encryptKey: String
//    private lateinit var encryptIV: String
//    private var maxLogFileSize: Long = 0

    var currentInitConfig: NLoggerConfig? = null


    fun init(config: NLoggerConfig) {
        if (!isInit) {
            currentInitConfig = config
//            cacheFilePath = config.cacheFilePath
//            logFileDir = config.logFileDir
//            encryptKey = config.encryptKey
//            encryptIV = config.encryptIV
//            maxLogFileSize = config.maxLogFileSize
            nativeInit(config.cacheFilePath, config.logFileDir, config.maxLogFileSize.toInt(), config.encryptKey, config.encryptIV)
            isInit = true
        }
    }

    override fun write(log: LogEntry): Int {
        if (!isInit) {
            return -1 //todo 统一ErrorCode
        }
        return log.run {
            nativeWrite(
                file_name = fileName,
                flag = type,
                log = content,
                local_time = date,
                thread_name = threadName,
                thread_id = threadId,
                is_main = if (isMain) {
                    1
                } else {
                    0
                }
            )
        }
    }

    override fun flush(): Int {
        if (!isInit) {
            return -1 //todo 统一ErrorCode
        }
        return nativeFlush()
    }

    override fun send(filePaths: List<String>, sendTask: ISendTask): Int {
        if (!isInit) {
            return -1 //todo 统一ErrorCode
        }

        //1.Copy所有未发送的日志文件
        if (filePaths.isEmpty()) {
            return -1
        }

        //2.逐个发送,真正发送的具体逻辑交给外部处理，包括删发送以后的逻辑
        return try {
            copyFiles(filePaths).forEach {
                sendTask.doSend(it)
            }
            sendTask.finish()
            0
        } catch (e: Exception) {
            e.printStackTrace()
            -1
        }
    }

    private fun copyFiles(filePaths: List<String>): List<File> {
        val result = ArrayList<File>()
        filePaths.forEach { source ->
            var inputStream: FileInputStream? = null
            var outputStream: FileOutputStream? = null
            val targetFile = File("$source.temp")
            try {
                inputStream = FileInputStream(File(source))
                outputStream = FileOutputStream(targetFile)
                val buffer = ByteArray(1024)
                var i: Int = inputStream.read(buffer)
                while (i >= 0) {
                    outputStream.write(buffer, 0, i)
                    outputStream.flush()
                    i = inputStream.read(buffer)
                }
                result.add(targetFile)
            } catch (e: FileNotFoundException) {
                e.printStackTrace()
            } catch (e: IOException) {
                e.printStackTrace()
            } finally {
                try {
                    inputStream?.close()
                } catch (e: Exception) {
                    e.printStackTrace()
                }
                try {
                    outputStream?.close()
                } catch (e: Exception) {
                    e.printStackTrace()
                }
            }
        }
        return result
    }


}