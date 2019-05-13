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
fun main(args: Array<String>) {
    Gzip().inflate()
}

class Gzip {


    fun inflate() {
        val cacheFile = File("./cache.mmap")
        val temp = RandomAccessFile(cacheFile, "r")
        val fileChannel = temp.channel
        val headTagBuffer = ByteBuffer.allocate(1)
        val headTag: Byte = 1

        for (i in 0..(fileChannel.size() - 1)) {
            fileChannel.read(headTagBuffer, i)
            if (headTagBuffer.get(0) == headTag) {
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
        fileChannel.read(lengthBuffer, headTagIndex + 1)
        lengthBuffer.order(ByteOrder.BIG_ENDIAN)
        val length = lengthBuffer.getInt(0)
        println("content length >>> $length")
        if (length < 0) {
            println("!!! content length >>> $length")
            return
        }
        if (headTagIndex != 0L) {
            contentIndex = headTagIndex + 5L
        }

        val contentBuffer = ByteBuffer.allocate(length)

        fileChannel.read(contentBuffer, contentIndex)
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
    }

}