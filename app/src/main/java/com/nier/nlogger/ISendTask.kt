package com.nier.nlogger

import java.io.File

/**
 * Created by fgd
 * Date 2019/8/30 15:33
 */
interface ISendTask {
    /**
     * 发送，必须要同步的发送请求
     */
    fun doSend(file: File)

    fun finish()
}