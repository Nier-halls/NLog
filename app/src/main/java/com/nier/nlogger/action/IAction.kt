package com.nier.nlogger.action

import com.nier.nlogger.ILogHandler

/**
 * Created by fgd
 * Date 2019/5/29 14:18
 */
interface IAction {
    fun run(logHandler: ILogHandler):Int
}