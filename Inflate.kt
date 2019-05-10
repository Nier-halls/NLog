import java.io.ByteArrayInputStream
import java.io.ByteArrayOutputStream
import java.io.File
import java.io.RandomAccessFile
import java.nio.ByteBuffer
import java.nio.ByteOrder
import java.util.zip.GZIPInputStream
import java.util.zip.GZIPOutputStream
import kotlin.experimental.and

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

        val tempBuffer = ByteBuffer.allocate(1)
        val fileSize = fileChannel.size()

        var start = 0L
//        var end = 0L

        val headTag: Byte = 1
//        val endTag: Byte = 16


        for (i in 0..(fileSize - 1)) {
            fileChannel.read(tempBuffer, i)
            if (tempBuffer.get(0) == headTag) {
                start = i
                break
            }
            tempBuffer.clear()
        }
        println("tartget Index  start >>> $start")
        tempBuffer.clear()


        val lengthBuffer = ByteBuffer.allocate(4)
        fileChannel.read(lengthBuffer, start + 1)
        lengthBuffer.order(ByteOrder.BIG_ENDIAN)
        val length = lengthBuffer.getInt(0)
        println("content length >>> $length")

//        for (i in 0..(fileSize - 1)) {
//            fileChannel.read(tempBuffer, i)
//            if (tempBuffer.get(0) == endTag) {
//                end = i
//                break
//            }
//            tempBuffer.clear()
//        }
//        println("tartget Index  end >>> $end")
//        tempBuffer.clear()

        if (start != 0L) {
            start += 5L
        }

        var contentLength = length
        val contentBuffer = ByteBuffer.allocate(contentLength)

        fileChannel.read(contentBuffer, start)
        var byteIn = ByteArrayInputStream(contentBuffer.array())


//        println("result >>>>>" + GZIPUtils.uncompressToString(contentBuffer.array()))

        var gzipIn = GZIPInputStream(byteIn)

        var byteOut = ByteArrayOutputStream()
//        var gzipOut = GZIPOutputStream(byteOut)
        val buffer = ByteArray(256)
        var n: Int

        n = gzipIn.read(buffer)

        while (n >= 0) {
            byteOut.write(buffer, 0, n)
            n = gzipIn.read(buffer)
        }
//        do {
//            n = gzipIn.read(buffer)
//            byteOut.write(buffer, 0, n)
//        } while (n >= 0)


        println("byte out >>> ${String(byteOut.toByteArray())}")

        byteOut.close()
//        gzipIn.close()
        fileChannel.close()
    }

}