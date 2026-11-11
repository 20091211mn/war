package com.warzone.kotlin

import android.graphics.Bitmap
import android.graphics.Canvas
import android.opengl.GLES20.*
import android.opengl.GLSurfaceView
import android.opengl.GLUtils
import android.os.SystemClock
import com.warzone.core.FBuf
import com.warzone.core.Game
import com.warzone.core.Input
import com.warzone.core.Renderer
import java.nio.ByteBuffer
import java.nio.ByteOrder
import java.nio.FloatBuffer
import javax.microedition.khronos.egl.EGLConfig
import javax.microedition.khronos.opengles.GL10
import kotlin.math.*

private const val VS = """
attribute vec3 aPos; attribute vec3 aCol; attribute vec2 aUv; attribute float aFlag;
uniform mat4 uVP;
varying vec3 vCol; varying vec2 vUv; varying float vFlag; varying vec3 vPos;
void main(){ vCol=aCol; vUv=aUv; vFlag=aFlag; vPos=aPos; gl_Position=uVP*vec4(aPos,1.0); }
"""
private const val FS = """
precision mediump float;
varying vec3 vCol; varying vec2 vUv; varying float vFlag; varying vec3 vPos;
uniform vec3 uCam; uniform vec3 uFog; uniform vec2 uFogR;
void main(){
  vec3 c=vCol; float d=length(vPos-uCam);
  float det=step(1.5,vFlag);
  if(det>0.5){ vec2 f=abs(fract(vUv)-0.5); float line=step(0.485,max(f.x,f.y)); c*=1.0-0.12*line*(1.0-clamp(d/40.0,0.0,1.0)); }
  float fogOn=mod(vFlag,2.0);
  float t=clamp((d-uFogR.x)/max(1.0,uFogR.y-uFogR.x),0.0,1.0)*fogOn;
  gl_FragColor=vec4(mix(c,uFog,t),1.0);
}
"""
private const val SVS = "attribute vec2 aP; varying vec2 vN; void main(){ vN=aP; gl_Position=vec4(aP,0.999,1.0); }"
private const val SFS = """
precision mediump float;
varying vec2 vN; uniform vec3 uSky; uniform vec3 uHor; uniform vec3 uGnd; uniform float uH;
void main(){ float y=vN.y; vec3 c;
  if(y>uH){ float t=clamp((y-uH)/max(0.3,1.0-uH),0.0,1.0); c=mix(uHor,uSky,pow(t,0.65)); } else { float t=clamp((uH-y)*1.6,0.0,1.0); c=mix(uHor,uGnd,t); }
  gl_FragColor=vec4(c,1.0); }
"""
private const val OVS = "attribute vec2 aP; varying vec2 vT; void main(){ vT=vec2(aP.x*0.5+0.5, 0.5-aP.y*0.5); gl_Position=vec4(aP,0.0,1.0); }"
private const val OFS = "precision mediump float; varying vec2 vT; uniform sampler2D uTex; void main(){ gl_FragColor=texture2D(uTex,vT); }"

/** رسم Kotlin كامل: مثلثات المحرك -> OpenGL ES 2 ، والواجهة Canvas -> نسيج شفاف. */
class GameGL(val g: Game, val ctl: Controls) : GLSurfaceView.Renderer {
    private var sw = 1; private var sh = 1
    private var prog = 0; private var sprog = 0; private var oprog = 0
    private val vbo = IntArray(1); private var tex = 0
    private var uVP = 0; private var uCam = 0; private var uFog = 0; private var uFogR = 0
    private var aPos = 0; private var aCol = 0; private var aUv = 0; private var aFlag = 0
    private var cap = 0; private var fbuf: FloatBuffer? = null
    private val vp = FloatArray(16)
    private var last = 0L; private var acc = 0f
    private var hudBmp: Bitmap? = null; private var hudCv: Canvas? = null; private var hudW = 1; private var hudH = 1
    private var hud: Hud? = null
    private val quad: FloatBuffer = ByteBuffer.allocateDirect(8 * 4).order(ByteOrder.nativeOrder()).asFloatBuffer().apply { put(floatArrayOf(-1f, -1f, 1f, -1f, -1f, 1f, 1f, 1f)); position(0) }
    private val look = FloatArray(2)
    private var err: String? = null; private var tUpd = 0f; private var tScn = 0f; private var fpsAvg = 60f
    @Volatile var ready = false

    private fun shader(t: Int, src: String): Int { val s = glCreateShader(t); glShaderSource(s, src); glCompileShader(s)
        val ok = IntArray(1); glGetShaderiv(s, GL_COMPILE_STATUS, ok, 0); if (ok[0] == 0) throw RuntimeException("shader: " + glGetShaderInfoLog(s)); return s }
    private fun program(v: String, f: String): Int { val p = glCreateProgram(); glAttachShader(p, shader(GL_VERTEX_SHADER, v)); glAttachShader(p, shader(GL_FRAGMENT_SHADER, f)); glLinkProgram(p)
        val ok = IntArray(1); glGetProgramiv(p, GL_LINK_STATUS, ok, 0); if (ok[0] == 0) throw RuntimeException("link: " + glGetProgramInfoLog(p)); return p }

    override fun onSurfaceCreated(gl: GL10?, c: EGLConfig?) {
        prog = program(VS, FS); sprog = program(SVS, SFS); oprog = program(OVS, OFS)
        uVP = glGetUniformLocation(prog, "uVP"); uCam = glGetUniformLocation(prog, "uCam"); uFog = glGetUniformLocation(prog, "uFog"); uFogR = glGetUniformLocation(prog, "uFogR")
        aPos = glGetAttribLocation(prog, "aPos"); aCol = glGetAttribLocation(prog, "aCol"); aUv = glGetAttribLocation(prog, "aUv"); aFlag = glGetAttribLocation(prog, "aFlag")
        glGenBuffers(1, vbo, 0); val t = IntArray(1); glGenTextures(1, t, 0); tex = t[0]
        glBindTexture(GL_TEXTURE_2D, tex); glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR); glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR)
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE); glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE)
        hudBmp = null; last = 0L; cap = 0; sBuf = 0; g.r.staticDirty = true
        synchronized(g) { if (!ready) { g.menu = 1; g.nBots = 6; g.init(sw.coerceAtLeast(2), sh.coerceAtLeast(2)); hud = Hud(g, ctl); ready = true } }
    }

    override fun onSurfaceChanged(gl: GL10?, w: Int, h: Int) {
        sw = w; sh = h; glViewport(0, 0, w, h); ctl.sw = w.toFloat(); ctl.sh = h.toFloat(); g.r.init(w, h)
        val sc = min(1f, 1100f / w); hudW = max(2, (w * sc).toInt()); hudH = max(2, (h * sc).toInt()); hudBmp = Bitmap.createBitmap(hudW, hudH, Bitmap.Config.ARGB_8888); hudCv = Canvas(hudBmp!!)
        glBindTexture(GL_TEXTURE_2D, tex); GLUtils.texImage2D(GL_TEXTURE_2D, 0, hudBmp, 0)
    }

    private var sBuf = 0; private var sN = 0
    private fun bindAttribs() { val st = Renderer.STRIDE * 4
        glEnableVertexAttribArray(aPos); glVertexAttribPointer(aPos, 3, GL_FLOAT, false, st, 0)
        glEnableVertexAttribArray(aCol); glVertexAttribPointer(aCol, 3, GL_FLOAT, false, st, 12)
        glEnableVertexAttribArray(aUv); glVertexAttribPointer(aUv, 2, GL_FLOAT, false, st, 24)
        glEnableVertexAttribArray(aFlag); glVertexAttribPointer(aFlag, 1, GL_FLOAT, false, st, 32) }
    /** الخريطة الثابتة: تُرفع إلى كرت الشاشة مرة واحدة فقط ثم تُرسم كل إطار بلا عمل معالج. */
    private fun upload(b: FBuf, static: Boolean) {
        if (b.n <= 0) return
        if (sBuf == 0) { val t = IntArray(1); glGenBuffers(1, t, 0); sBuf = t[0] }
        glBindBuffer(GL_ARRAY_BUFFER, sBuf)
        if (g.r.staticDirty) { val fb = ByteBuffer.allocateDirect(b.n * 4).order(ByteOrder.nativeOrder()).asFloatBuffer(); fb.put(b.a, 0, b.n); fb.position(0); glBufferData(GL_ARRAY_BUFFER, b.n * 4, fb, GL_STATIC_DRAW); sN = b.n; g.r.staticDirty = false }
        bindAttribs(); glDrawArrays(GL_TRIANGLES, 0, sN / Renderer.STRIDE)
    }
    private fun upload(b: FBuf) {
        val n = b.n; if (n <= 0) return
        if (fbuf == null || cap < n) { cap = max(n, cap * 2).coerceAtLeast(1 shl 16); fbuf = ByteBuffer.allocateDirect(cap * 4).order(ByteOrder.nativeOrder()).asFloatBuffer() }
        val fb = fbuf!!; fb.clear(); fb.put(b.a, 0, n); fb.position(0)
        glBindBuffer(GL_ARRAY_BUFFER, vbo[0]); glBufferData(GL_ARRAY_BUFFER, n * 4, fb, GL_STREAM_DRAW)
        val st = Renderer.STRIDE * 4
        glEnableVertexAttribArray(aPos); glVertexAttribPointer(aPos, 3, GL_FLOAT, false, st, 0)
        glEnableVertexAttribArray(aCol); glVertexAttribPointer(aCol, 3, GL_FLOAT, false, st, 12)
        glEnableVertexAttribArray(aUv); glVertexAttribPointer(aUv, 2, GL_FLOAT, false, st, 24)
        glEnableVertexAttribArray(aFlag); glVertexAttribPointer(aFlag, 1, GL_FLOAT, false, st, 32)
        glDrawArrays(GL_TRIANGLES, 0, n / Renderer.STRIDE)
    }

    private val step = FloatArray(2)
    @Volatile var onQuality: ((Int) -> Unit)? = null; private var appliedQ = -1
    override fun onDrawFrame(gl: GL10?) {
        if (appliedQ != com.warzone.core.Lang.quality) { appliedQ = com.warzone.core.Lang.quality; onQuality?.invoke(appliedQ) }
        if (!ready) return
        val now = SystemClock.elapsedRealtimeNanos(); var dt = if (last == 0L) 1f / 60 else ((now - last) / 1e9f); last = now; dt = min(0.1f, max(0.001f, dt))
        synchronized(g) {
            val t0 = SystemClock.elapsedRealtimeNanos()
            if (err == null) try {
                val inp = ctl.build(); acc += dt
                // خطوة أو خطوتان فقط بزمن متغيّر (بدل 4 خطوات ثابتة): لا حلقة موت على الجهاز البطيء
                val n = if (acc > 1f / 45f) 2 else 1; val sd = min(1f / 30f, acc / n); ctl.takeLook(n, look)
                for (k in 0 until n) { val i = Input(); i.set(inp); i.lx = look[0]; i.ly = look[1]; g.update(sd, i) }; acc = 0f
                tUpd += (((SystemClock.elapsedRealtimeNanos() - t0) / 1e6f) - tUpd) * 0.1f
                val t1 = SystemClock.elapsedRealtimeNanos()
                if (g.menu != 0) g.renderMenu() else g.render()
                tScn += (((SystemClock.elapsedRealtimeNanos() - t1) / 1e6f) - tScn) * 0.1f
            } catch (e: Throwable) { err = e.toString() + "\n" + e.stackTrace.take(6).joinToString("\n") { "  at " + it }; android.util.Log.e("WZ", "game error", e) }
            fpsAvg += ((1f / dt) - fpsAvg) * 0.05f
            HudDiag.line = String.format("FPS %.0f  update %.1f ms  scene %.1f ms  tris %d", fpsAvg, tUpd, tScn, g.r.statics.triangleCount + g.r.layers[0].triangleCount + g.r.layers[1].triangleCount)
            HudDiag.err = err
            val r = g.r; val m = r.m
            glViewport(0, 0, sw, sh); glDisable(GL_BLEND); glDisable(GL_CULL_FACE)
            glClearColor(0.5f, 0.6f, 0.8f, 1f); glClear(GL_COLOR_BUFFER_BIT or GL_DEPTH_BUFFER_BIT)
            if (m != null) {
                // السماء
                glDisable(GL_DEPTH_TEST); glUseProgram(sprog)
                glUniform3f(glGetUniformLocation(sprog, "uSky"), m.sky[0], m.sky[1], m.sky[2]); glUniform3f(glGetUniformLocation(sprog, "uHor"), m.hor[0], m.hor[1], m.hor[2]); glUniform3f(glGetUniformLocation(sprog, "uGnd"), m.ground[0], m.ground[1], m.ground[2])
                val th = tan(r.cam.fov * 0.5f); glUniform1f(glGetUniformLocation(sprog, "uH"), (-tan(r.cam.pitch) / th).coerceIn(-3f, 3f))
                val ap = glGetAttribLocation(sprog, "aP"); glEnableVertexAttribArray(ap); glBindBuffer(GL_ARRAY_BUFFER, 0); glVertexAttribPointer(ap, 2, GL_FLOAT, false, 0, quad); glDrawArrays(GL_TRIANGLE_STRIP, 0, 4)
                // العالم
                glEnable(GL_DEPTH_TEST); glDepthFunc(GL_LEQUAL); glUseProgram(prog)
                r.viewProj(vp, 0.05f, max(300f, r.fogF * 1.5f)); glUniformMatrix4fv(uVP, 1, false, vp, 0)
                glUniform3f(uCam, r.cam.x, r.cam.y, r.cam.z); glUniform3f(uFog, r.fog[0], r.fog[1], r.fog[2]); glUniform2f(uFogR, r.fogN, r.fogF)
                upload(r.statics, true)
                upload(r.layers[0])
                if (r.layers[1].n > 0) { glClear(GL_DEPTH_BUFFER_BIT); upload(r.layers[1]) }
                glDisableVertexAttribArray(aPos); glDisableVertexAttribArray(aCol); glDisableVertexAttribArray(aUv); glDisableVertexAttribArray(aFlag)
            }
            // الواجهة
            val bmp = hudBmp; val cv = hudCv
            if (bmp != null && cv != null) {
                bmp.eraseColor(0); cv.save(); cv.scale(hudW.toFloat() / sw, hudH.toFloat() / sh); hud!!.draw(cv, sw.toFloat(), sh.toFloat()); cv.restore()
                glDisable(GL_DEPTH_TEST); glEnable(GL_BLEND); glBlendFunc(GL_ONE, GL_ONE_MINUS_SRC_ALPHA)
                glUseProgram(oprog); glActiveTexture(GL_TEXTURE0); glBindTexture(GL_TEXTURE_2D, tex); GLUtils.texImage2D(GL_TEXTURE_2D, 0, bmp, 0)
                glUniform1i(glGetUniformLocation(oprog, "uTex"), 0)
                val ap = glGetAttribLocation(oprog, "aP"); glEnableVertexAttribArray(ap); glBindBuffer(GL_ARRAY_BUFFER, 0); glVertexAttribPointer(ap, 2, GL_FLOAT, false, 0, quad); glDrawArrays(GL_TRIANGLE_STRIP, 0, 4); glDisableVertexAttribArray(ap)
            }
        }
    }
}
