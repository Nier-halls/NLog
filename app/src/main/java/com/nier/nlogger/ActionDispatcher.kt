package com.nier.nlogger

import android.annotation.SuppressLint
import android.os.Handler
import android.os.HandlerThread
import android.os.Message
import com.nier.nlogger.action.IAction

/**
 * Created by fgd
 * Date 2019/5/29 14:32
 */
class ActionDispatcher(val logActionHandler: ILogHandler) : HandlerThread("ActionDispatcher") {

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
        mDispatchHandler = Handler(looper)
    }

    fun registerDispatchHook(hook: ActionDispatchHook?) {
        synchronized(this) {
            mActionDispatchHook = hook
        }
    }

    fun setACtionHandleResultListener(listener: ActionHandledResultListener) {
        synchronized(this) {
            mActionHandledResultListener = listener
        }
    }

    @SuppressLint("HandlerLeak")
    fun initHandler(): Handler = object : Handler() {
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
                }
        }
    }


    fun dispatch(action: IAction) {
        mDispatchHandler.obtainMessage()
            .apply {
                obj = action
            }
            .sendToTarget()
    }


}