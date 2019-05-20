package com.nier.nlogger

import android.os.Bundle
import android.support.v7.app.AppCompatActivity
import android.widget.TextView

class MainActivity : AppCompatActivity() {

    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)
        setContentView(R.layout.activity_main)
        findViewById<TextView>(R.id.tv_result).text = NLoggerProxy.instance.test("Nier-Automata-test")

        findViewById<TextView>(R.id.btn_init).setOnClickListener {
            NLoggerProxy.instance.init(this)
        }

        findViewById<TextView>(R.id.btn_write).setOnClickListener {
            NLoggerProxy.instance.write("")

        }

        findViewById<TextView>(R.id.btn_flush).setOnClickListener {
            NLoggerProxy.instance.flush()
        }

        NLoggerProxy.instance.init(this)
        NLoggerProxy.instance.write("")
        NLoggerProxy.instance.write("")
        NLoggerProxy.instance.write("")
        NLoggerProxy.instance.write("")
        NLoggerProxy.instance.flush()
    }
}
