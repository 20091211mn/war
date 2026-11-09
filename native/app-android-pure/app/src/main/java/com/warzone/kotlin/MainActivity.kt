package com.warzone.kotlin

import android.app.Activity
import android.media.AudioAttributes
import android.media.AudioFormat
import android.media.AudioTrack
import android.opengl.GLSurfaceView
import android.os.Build
import android.os.Bundle
import android.view.MotionEvent
import android.view.View
import android.view.WindowManager
import com.warzone.core.Game
import com.warzone.core.Sfx

/** اللعبة كلها Kotlin: المحرك (core) + OpenGL ES + لمس + صوت. بلا C++ وبلا WebView. */
class MainActivity : Activity() {
    private val game = Game()
    private val ctl = Controls(game)
    private lateinit var view: GLSurfaceView
    @Volatile private var audioRun = false
    private var audioThread: Thread? = null

    override fun onCreate(b: Bundle?) {
        super.onCreate(b)
        window.addFlags(WindowManager.LayoutParams.FLAG_KEEP_SCREEN_ON or WindowManager.LayoutParams.FLAG_FULLSCREEN)
        if (Build.VERSION.SDK_INT >= 28) window.attributes.layoutInDisplayCutoutMode = WindowManager.LayoutParams.LAYOUT_IN_DISPLAY_CUTOUT_MODE_SHORT_EDGES
        view = object : GLSurfaceView(this) {
            override fun onTouchEvent(e: MotionEvent): Boolean {
                when (e.actionMasked) {
                    MotionEvent.ACTION_DOWN, MotionEvent.ACTION_POINTER_DOWN -> { val i = e.actionIndex; ctl.touchDown(e.getPointerId(i), e.getX(i), e.getY(i)) }
                    MotionEvent.ACTION_MOVE -> for (i in 0 until e.pointerCount) ctl.touchMove(e.getPointerId(i), e.getX(i), e.getY(i))
                    MotionEvent.ACTION_UP, MotionEvent.ACTION_POINTER_UP -> { val i = e.actionIndex; ctl.touchUp(e.getPointerId(i)) }
                    MotionEvent.ACTION_CANCEL -> ctl.releaseAll()
                }
                return true
            }
        }
        view.setEGLContextClientVersion(2); view.setEGLConfigChooser(8, 8, 8, 8, 16, 0); view.preserveEGLContextOnPause = true
        view.setRenderer(GameGL(game, ctl))
        setContentView(view); immersive()
        Thread { Sfx.initBuffers() }.start()
        // تسخين: تحميل مجسّمات الأسلحة مسبقاً حتى لا يتجمّد أول إطار في المباراة
        Thread { try { for (n in arrayOf("rifle", "sniper", "dmr", "launcher")) com.warzone.core.GameData.mesh(n) } catch (e: Throwable) {} }.start()
    }
    private fun immersive() {
        @Suppress("DEPRECATION")
        window.decorView.systemUiVisibility = View.SYSTEM_UI_FLAG_IMMERSIVE_STICKY or View.SYSTEM_UI_FLAG_FULLSCREEN or View.SYSTEM_UI_FLAG_HIDE_NAVIGATION or
            View.SYSTEM_UI_FLAG_LAYOUT_FULLSCREEN or View.SYSTEM_UI_FLAG_LAYOUT_HIDE_NAVIGATION or View.SYSTEM_UI_FLAG_LAYOUT_STABLE
    }
    override fun onWindowFocusChanged(f: Boolean) { super.onWindowFocusChanged(f); if (f) immersive() }
    private var mcLock: android.net.wifi.WifiManager.MulticastLock? = null
    override fun onResume() { super.onResume(); view.onResume(); startAudio()
        try { val wm = applicationContext.getSystemService(android.content.Context.WIFI_SERVICE) as android.net.wifi.WifiManager; mcLock = wm.createMulticastLock("wz").apply { setReferenceCounted(false); acquire() } } catch (e: Throwable) {} }
    override fun onPause() {
        super.onPause(); stopAudio(); ctl.releaseAll(); try { mcLock?.release() } catch (e: Throwable) {}
        synchronized(game) { if (game.menu == 0 && !game.over && game.role == 0) game.paused = true }
        view.onPause()
    }

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
            while (audioRun) { try { Sfx.mix(buf, frames) } catch (e: Throwable) { buf.fill(0) }; track.write(buf, 0, buf.size) }
            track.stop(); track.release()
        }, "wz-audio").apply { start() }
    }
    private fun stopAudio() { audioRun = false; audioThread?.join(500); audioThread = null }
}
