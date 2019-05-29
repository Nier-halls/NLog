package com.nier.nlogger

import android.content.Context
import android.util.Log
import com.nier.nlogger.action.Flush
import com.nier.nlogger.action.IAction
import com.nier.nlogger.action.Send
import com.nier.nlogger.action.Write
import java.io.File
import java.lang.Exception

/**
 * Created by fgd
 * Date 2019/5/29 20:43
 *
 * todo 测试一下 多线程写入是否能够收拢
 * todo 测试一下回调是否有用
 */
class NLogger : ILogHandler {

    companion object {
        private val TAG = "NLogger_client"
        private var isLoadLibrarySuccess = false //可以考虑以后以java实现

        init {
            try {
                System.loadLibrary("nlogger")
                isLoadLibrarySuccess = true
            } catch (e: Exception) {
                isLoadLibrarySuccess = false
                e.printStackTrace()
            }
        }

        private val mInstance = NLogger()
        private val mDispatcher = ActionDispatcher(mInstance)


        fun init(config: NLoggerConfig) {
            mInstance.init(config)

            mDispatcher.registerDispatchHook(object : ActionDispatcher.ActionDispatchHook {
                override fun onActionDispatch(action: IAction, logHandler: ILogHandler): IAction? {
                    Log.d(TAG, "Call onActionDispatch >>> $action")
                    return action
                }
            })

            mDispatcher.setACtionHandleResultListener(object : ActionDispatcher.ActionHandledResultListener {
                override fun onActionHandled(action: IAction, result: Int) {
                    Log.d(TAG, "Call onActionHandled >>> $action , result=$result")
                }
            })
        }

        fun write(content: String) {
            if (isLoadLibrarySuccess) {
                val action = Write(content)
                Log.d(TAG, "Run [write] >>> $action")
                mDispatcher.dispatch(action)
            }
        }

        fun flush() {
            if (isLoadLibrarySuccess) {
                val action = Flush()
                Log.d(TAG, "Run [flush] >>> $action")
                mDispatcher.dispatch(action)
            }
        }

        fun send() {
            if (isLoadLibrarySuccess) {
                val action = Send()
                Log.d(TAG, "Run [send] >>> $action")
                mDispatcher.dispatch(action)
            }
        }

    }

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

    fun init(config: NLoggerConfig) {
        if (!isInit) {
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

        val fileName = "niers_log"

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

    override fun send(handler: (logFiles: List<File>) -> Unit): Int {
        if (!isInit) {
            return -1 //todo 统一ErrorCode
        }
        return 0
    }

}