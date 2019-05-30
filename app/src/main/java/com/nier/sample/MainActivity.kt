package com.nier.sample

import android.os.Bundle
import android.support.v7.app.AppCompatActivity
import android.widget.EditText
import android.widget.TextView
import com.nier.nlogger.NLogger
import com.nier.nlogger.R

class MainActivity : AppCompatActivity() {

    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)
        setContentView(R.layout.activity_main)

        NLogger.init(this)

        findViewById<EditText>(R.id.tv_result).setText("Hello World QWERTYUIOPASDFGHJKL[]{};':ZXCVBNM<>?,./123456789!@#\\\$%^*()!")

        findViewById<TextView>(R.id.btn_init).setOnClickListener {
            //            NLogger.init(this)
            testMutiThread(50)
        }

        findViewById<TextView>(R.id.btn_write).setOnClickListener {
            val text = findViewById<EditText>(R.id.tv_result).text
            NLogger.write("date[${System.currentTimeMillis()}] content[$text]")

        }

        findViewById<TextView>(R.id.btn_flush).setOnClickListener {
            NLogger.flush()
        }


//        NLoggerProxy.instance.init(this)
//        NLoggerProxy.instance.write("")
//        NLoggerProxy.instance.write("")
//        NLoggerProxy.instance.write("")
//        NLoggerProxy.instance.write("")
//        NLoggerProxy.instance.flush()
    }


    private fun testMutiThread(threadNum: Int) {
        for (i in 0..threadNum) {
            Thread {
                val result = i % 3
                when (result) {
                    0 -> NLogger.write("Hello World QWERTYUIOPASDFGHJKL[]{};':ZXCVBNM<>?,./123456789!@#\\\$%^*()!")
                    1 -> NLogger.send()
                    2 -> NLogger.flush()
                }
            }.start()
        }
    }
}
