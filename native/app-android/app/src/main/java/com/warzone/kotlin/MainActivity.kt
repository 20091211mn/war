package com.warzone.kotlin

import android.app.Activity
import android.content.Context
import android.graphics.Bitmap
import android.graphics.Paint
import android.graphics.Rect
import android.media.AudioAttributes
import android.media.AudioFormat
import android.media.AudioTrack
import android.os.Build
import android.os.Bundle
import android.os.SystemClock
import android.view.MotionEvent
import android.view.SurfaceHolder
import android.view.SurfaceView
import android.view.View
import android.view.WindowManager

/** شاشة اللعبة: تعرض صورة المحرك وتمرّر اللمس. */
class GameView(ctx: Context) : SurfaceView(ctx), SurfaceHolder.Callback {
    @Volatile private var running = false
    private var thread: Thread? = null
    private var bmp: Bitmap? = null
    private var bmpW = 0
    private var bmpH = 0
    private val paint = Paint().apply { isFilterBitmap = true }
    private val src = Rect()
    private val dst = Rect()

    init { holder.addCallback(this); isFocusable = true }

    override fun surfaceCreated(h: SurfaceHolder) {}
    override fun surfaceChanged(h: SurfaceHolder, format: Int, w: Int, hh: Int) {
        Native.init(w, hh, Runtime.getRuntime().availableProcessors())
        Native.setScreen(w, hh)
        startLoop()
    }
    override fun surfaceDestroyed(h: SurfaceHolder) { stopLoop(); Native.touch(3, 0, 0f, 0f) }

    private fun startLoop() {
        if (running) return
        running = true
        thread = Thread({
            var last = SystemClock.elapsedRealtimeNanos()
            while (running) {
                val now = SystemClock.elapsedRealtimeNanos()
                val dt = ((now - last) / 1e9).toFloat(); last = now
                Native.frame(dt)
                drawFrame()
            }
        }, "wz-render").apply { start() }
    }
    private fun stopLoop() { running = false; thread?.join(500); thread = null }

    private fun drawFrame() {
        val w = Native.width(); val h = Native.height(); val buf = Native.pixels() ?: return
        if (w <= 0 || h <= 0) return
        if (bmp == null || bmpW != w || bmpH != h) { bmp = Bitmap.createBitmap(w, h, Bitmap.Config.ARGB_8888); bmpW = w; bmpH = h }
        buf.rewind(); bmp!!.copyPixelsFromBuffer(buf)
        val c = holder.lockCanvas() ?: return
        try {
            src.set(0, 0, w, h); dst.set(0, 0, c.width, c.height)
            c.drawBitmap(bmp!!, src, dst, paint)
        } finally { holder.unlockCanvasAndPost(c) }
    }

    override fun onTouchEvent(e: MotionEvent): Boolean {
        when (e.actionMasked) {
            MotionEvent.ACTION_DOWN, MotionEvent.ACTION_POINTER_DOWN -> { val i = e.actionIndex; Native.touch(0, e.getPointerId(i), e.getX(i), e.getY(i)) }
            MotionEvent.ACTION_MOVE -> for (i in 0 until e.pointerCount) Native.touch(1, e.getPointerId(i), e.getX(i), e.getY(i))
            MotionEvent.ACTION_UP, MotionEvent.ACTION_POINTER_UP -> { val i = e.actionIndex; Native.touch(2, e.getPointerId(i), e.getX(i), e.getY(i)) }
            MotionEvent.ACTION_CANCEL -> Native.touch(3, 0, 0f, 0f)
        }
        return true
    }
}

class MainActivity : Activity() {
    private lateinit var view: GameView
    @Volatile private var audioRun = false
    private var audioThread: Thread? = null

    override fun onCreate(b: Bundle?) {
        super.onCreate(b)
        window.addFlags(WindowManager.LayoutParams.FLAG_KEEP_SCREEN_ON or WindowManager.LayoutParams.FLAG_FULLSCREEN)
        if (Build.VERSION.SDK_INT >= 28) window.attributes.layoutInDisplayCutoutMode = WindowManager.LayoutParams.LAYOUT_IN_DISPLAY_CUTOUT_MODE_SHORT_EDGES
        view = GameView(this); setContentView(view); immersive()
    }
    private fun immersive() {
        @Suppress("DEPRECATION")
        window.decorView.systemUiVisibility = View.SYSTEM_UI_FLAG_IMMERSIVE_STICKY or View.SYSTEM_UI_FLAG_FULLSCREEN or View.SYSTEM_UI_FLAG_HIDE_NAVIGATION or
            View.SYSTEM_UI_FLAG_LAYOUT_FULLSCREEN or View.SYSTEM_UI_FLAG_LAYOUT_HIDE_NAVIGATION or View.SYSTEM_UI_FLAG_LAYOUT_STABLE
    }
    override fun onWindowFocusChanged(f: Boolean) { super.onWindowFocusChanged(f); if (f) immersive() }
    override fun onResume() { super.onResume(); startAudio() }
    override fun onPause() { super.onPause(); stopAudio(); Native.pause() }

    private fun startAudio() {
        if (audioRun) return
        audioRun = true
        audioThread = Thread({
            val rate = 48000
            val minBuf = AudioTrack.getMinBufferSize(rate, AudioFormat.CHANNEL_OUT_STEREO, AudioFormat.ENCODING_PCM_16BIT)
            val frames = 512
            val track = AudioTrack.Builder()
                .setAudioAttributes(AudioAttributes.Builder().setUsage(AudioAttributes.USAGE_GAME).setContentType(AudioAttributes.CONTENT_TYPE_SONIFICATION).build())
                .setAudioFormat(AudioFormat.Builder().setSampleRate(rate).setEncoding(AudioFormat.ENCODING_PCM_16BIT).setChannelMask(AudioFormat.CHANNEL_OUT_STEREO).build())
                .setTransferMode(AudioTrack.MODE_STREAM).setBufferSizeInBytes(maxOf(minBuf, frames * 4 * 4)).build()
            track.play()
            val buf = ShortArray(frames * 2)
            while (audioRun) { Native.mix(buf, frames); track.write(buf, 0, buf.size) }
            track.stop(); track.release()
        }, "wz-audio").apply { start() }
    }
    private fun stopAudio() { audioRun = false; audioThread?.join(500); audioThread = null }
}
