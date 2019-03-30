package com.nier.nlogger

import android.support.v7.app.AppCompatActivity
import android.os.Bundle
import android.widget.TextView

class MainActivity : AppCompatActivity() {

    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)
        setContentView(R.layout.activity_main)
        findViewById<TextView>(R.id.tv_result).text = sayHello("Nier-Auto")
    }

    external fun sayHello(content: String): String

    companion object {
        init {
            System.loadLibrary("nier-native")
        }
    }
}
