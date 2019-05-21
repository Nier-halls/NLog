import java.io.ByteArrayInputStream
import java.io.ByteArrayOutputStream
import java.io.File
import java.io.RandomAccessFile
import java.nio.ByteBuffer
import java.nio.ByteOrder
import java.nio.channels.FileChannel
import java.util.zip.GZIPInputStream

/**
 * Created by fgd
 * Date 2019/5/9
 */
//NLOGGER_MMAP_CACHE_CONTENT_HEAD_MAGIC 0x003397A8//nier>>3381160
//NLOGGER_MMAP_CACHE_CONTENT_TAIL_MAGIC 0x00018BC3//fgd>>101315
//NLOGGER_MMAP_CACHE_HEADER_HEAD_MAGIC 0x0031AA22//java>>3254818
//NLOGGER_MMAP_CACHE_HEADER_TAIL_MAGIC 0xBCE91901//kotlin>>-1125574399

fun main(args: Array<String>) {
    Gzip().inflate()
}

//const val FILE_NAME = "niers_log"
const val FILE_NAME = "cache.mmap"

class Gzip {


    fun inflate() {
        val cacheFile = File("./$FILE_NAME")
        val temp = RandomAccessFile(cacheFile, "r")
        val fileChannel = temp.channel
        val headTagBuffer = ByteBuffer.allocate(4)
        val headTag = 0x003397A8;

        for (i in 0..(fileChannel.size() - 1)) {
            fileChannel.read(headTagBuffer, i)
            headTagBuffer.order(ByteOrder.LITTLE_ENDIAN)
            if (headTagBuffer.getInt(0) == headTag) {
                headTagBuffer.clear()
                println("tartget Index  start >>> $i")
                parse(fileChannel, i)
            }
            headTagBuffer.clear()
        }
    }


    fun parse(fileChannel: FileChannel, headTagIndex: Long) {
        var contentIndex = 0L
        val lengthBuffer = ByteBuffer.allocate(4)
        fileChannel.read(lengthBuffer, headTagIndex + 4)
        lengthBuffer.order(ByteOrder.BIG_ENDIAN)
        val length = lengthBuffer.getInt(0)
        if (length < 0 || length >= 10000) {
            println("!!! content length >>> $length")
            return
        } else {
            println("content length >>> $length")
        }
//        if (headTagIndex != 0L) {
        contentIndex = headTagIndex + 8L
//        }

        val contentBuffer = ByteBuffer.allocate(length)


        fileChannel.read(contentBuffer, contentIndex)
        println("contentIndex.toInt() >>>${contentIndex.toInt()}")

//        for (i in 0..(length - 1)) {
//            println(">>>> index $i:${0xff and contentBuffer[i].toInt()}")
//        }

        try {
            val byteIn = ByteArrayInputStream(contentBuffer.array())

            val gzipIn = GZIPInputStream(byteIn)

            val byteOut = ByteArrayOutputStream()

            val buffer = ByteArray(256)
            var n: Int

            n = gzipIn.read(buffer)

            while (n >= 0) {
                byteOut.write(buffer, 0, n)
                n = gzipIn.read(buffer)
            }

            println("byte out start with $headTagIndex >>> ${String(byteOut.toByteArray())}")

            byteOut.close()
        } catch (e: Exception) {
            e.printStackTrace()
        }
    }

}