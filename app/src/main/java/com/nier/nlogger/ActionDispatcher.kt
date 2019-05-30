package com.nier.nlogger

import android.annotation.SuppressLint
import android.os.Handler
import android.os.HandlerThread
import android.os.Message
import android.util.Log
import com.nier.nlogger.action.IAction

/**
 * Created by fgd
 * Date 2019/5/29 14:32
 */
class ActionDispatcher(val logActionHandler: ILogHandler) : HandlerThread("ActionDispatcher") {

    private val TAG = "NLogger_C_dispatcher"

    interface ActionDispatchHook {
        fun onActionDispatch(action: IAction, logHandler: ILogHandler): IAction?
    }

    interface ActionHandledResultListener {
        fun onActionHandled(action: IAction, result: Int)
    }

    private val mDispatchHandler: Handler

    private var mActionDispatchHook: ActionDispatchHook? = null

    private var mActionHandledResultListener: ActionHandledResultListener? = null

    init {
        start()
        mDispatchHandler = initHandler()
    }

    /**
     * 注册分发前的钩子，方便上层
     */
    fun registerDispatchHook(hook: ActionDispatchHook?) {
        synchronized(this) {
            mActionDispatchHook = hook
        }
    }

    fun setActionHandleResultListener(listener: ActionHandledResultListener) {
        synchronized(this) {
            mActionHandledResultListener = listener
        }
    }

    /**
     * 初始化handler
     */
    @SuppressLint("HandlerLeak")
    fun initHandler(): Handler = object : Handler(looper) {
        override fun dispatchMessage(msg: Message) {
            super.dispatchMessage(msg)
            (msg.obj as? IAction)
                ?.let { action ->
                    //找出是否有hook，hook是否有替换过Action，替换过就用hook替换过的Action
                    synchronized(this) {
                        if (mActionDispatchHook != null) {
                            mActionDispatchHook!!.onActionDispatch(action, logActionHandler)
                        } else {
                            action
                        }
                    }
                }
                ?.let { noNullAction ->
                    val result = noNullAction.run(logActionHandler)

                    synchronized(this) {
                        mActionHandledResultListener?.onActionHandled(noNullAction, result)
                    }
                } ?: Log.e(TAG, "dispatchMessage NULL action ! message obj = {${msg.obj}}, thread[${Thread.currentThread().id}]")
        }
    }

    /**
     * 调用handler发送消息
     */
    fun dispatch(action: IAction) {
        Log.d(TAG, "start dispatch action => {${action.hashCode()}}, thread[${Thread.currentThread().id}]")
        mDispatchHandler.obtainMessage()
            .apply {
                obj = action
            }
            .sendToTarget()
    }


}