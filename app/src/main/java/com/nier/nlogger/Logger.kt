package com.nier.nlogger

import android.content.Context
import android.util.Log
import com.nier.nlogger.action.Flush
import com.nier.nlogger.action.IAction
import com.nier.nlogger.action.Send
import com.nier.nlogger.action.Write
import java.lang.Exception

/**
 * Created by fgd
 * Date 2019/8/30 15:07
 */
class Logger {
    companion object {
        private val TAG = "NLogger_C_client"
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
        private var fileName: String = System.currentTimeMillis().toString()

        fun init(config: NLoggerConfig) {
            mInstance.init(config)

            mDispatcher.registerDispatchHook(object : ActionDispatcher.ActionDispatchHook {
                override fun onActionDispatch(action: IAction, logHandler: ILogHandler): IAction? {
                    Log.d(TAG, "Call onActionDispatch >>> {${action.hashCode()}}, thread[${Thread.currentThread().id}]")
                    return action
                }
            })

            mDispatcher.setActionHandleResultListener(object : ActionDispatcher.ActionHandledResultListener {
                override fun onActionHandled(action: IAction, result: Int) {
                    Log.d(TAG, "Call onActionHandled >>>{${action.hashCode()}}, result=$result ,thread[${Thread.currentThread().id}]")
                }
            })
        }

        fun init(context: Context) {
            this.init(NLoggerConfig.DEFAULT_CONFIG(context))
        }


        fun write(content: String) {
            if (isLoadLibrarySuccess) {
                synchronized(this) {
                    val action = Write(content, fileName())
                    Log.d(TAG, "Run [write] >>> $action, thread[${Thread.currentThread().id}]")
                    mDispatcher.dispatch(action)
                }
            }
        }

        fun flush() {
            if (isLoadLibrarySuccess) {
                val action = Flush()
                Log.d(TAG, "Run [flush] >>> $action, thread[${Thread.currentThread().id}]")
                mDispatcher.dispatch(action)
            }
        }

        fun send() {
            if (isLoadLibrarySuccess) {
                synchronized(this) {
                    val action = Send()
                    //在确定发送的文件后新起一个文件来记录后面的日志
                    refreshFileName()
                    Log.d(TAG, "Run [send] >>> $action, thread[${Thread.currentThread().id}]")
                    mDispatcher.dispatch(action)
                }
            }
        }

        private fun fileName(): String {
            synchronized(this) {
                return fileName
            }
        }

        private fun refreshFileName(): String {
            synchronized(this) {
                val last = fileName
                fileName = System.currentTimeMillis().toString()
                return last
            }
        }

    }
}