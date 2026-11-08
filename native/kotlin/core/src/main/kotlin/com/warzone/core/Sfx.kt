package com.warzone.core

import kotlin.math.max
import kotlin.math.min

/** Procedural sound synthesis (port of native/src/sfx.h). Pure Kotlin; the Android app pulls samples with [mix] from its audio thread. */
object Sfx {
    const val SHOT = 0; const val SHOTGUN = 1; const val EXPLODE = 2; const val HIT = 3; const val HURT = 4; const val KILL = 5
    const val EMPTY = 6; const val RELOAD = 7; const val STEP = 8; const val SNIPER = 9; const val PISTOL = 10
    @Volatile @JvmField var engSpeed = -1f
    @Volatile @JvmField var droneVol = 0f
    private var engPh = 0f; private var drPh = 0f; private var engN = 0f
    private class Voice { @JvmField var id = 0; @JvmField var pos = 0; @JvmField var len = 0; @JvmField var vol = 0f; @JvmField var pan = 0f; @JvmField var fp = 0f; @JvmField var rate = 1f }
    private const val MAXV = 24
    private val vs = Array(MAXV) { Voice() }
    private val lk = Any()
    private var rs = 12345
    /** when false, play() is a no-op (unit tests, headless) */
    @Volatile @JvmField var enabled = true
    private fun rnd(): Float { rs = rs * 1664525 + 1013904223; return ((rs ushr 9) and 0xFFFF) / 32768f - 1f }

    const val BUF_BASE = 100; const val B_SUPP = 8; const val B_MAGOUT = 9; const val B_MAGIN = 10; const val B_BOLT = 11; const val B_SLAM = 12
    const val B_SHELL = 13; const val B_PUMP = 14; const val B_SLIDE = 15; const val NBUF = 16
    private val BUFS = Array(NBUF) { FloatArray(0) }

    private class SP(val dur: Float, val bt: Float, val hi: Float, val tf: Float, val tt: Float, val ta: Float, val tail: Float, val tl: Float, val ck: Float, val ne: Int, val ec: Array<FloatArray>, val mech: Float, val bolt: Float, val pump: Float)
    private fun ec(vararg a: Float) = Array(a.size / 2) { floatArrayOf(a[it * 2], a[it * 2 + 1]) }
    private val SPAR = arrayOf(
        SP(0.75f, 0.028f, 7000f, 190f, 0.05f, 0.62f, 0.3f, 0.18f, 1.0f, 3, ec(0.055f, 0.3f, 0.13f, 0.18f, 0.26f, 0.09f), 0.5f, 0f, 0f),
        SP(1.5f, 0.06f, 6000f, 120f, 0.12f, 0.95f, 0.55f, 0.3f, 1.2f, 4, ec(0.07f, 0.35f, 0.16f, 0.25f, 0.32f, 0.15f, 0.6f, 0.08f), 0.2f, 0f, 0f),
        SP(1.1f, 0.04f, 7500f, 150f, 0.07f, 0.8f, 0.45f, 0.24f, 1.1f, 4, ec(0.06f, 0.3f, 0.14f, 0.22f, 0.3f, 0.12f, 0.55f, 0.06f), 0.8f, 0f, 0f),
        SP(0.75f, 0.026f, 8000f, 170f, 0.05f, 0.6f, 0.28f, 0.2f, 1.0f, 3, ec(0.05f, 0.3f, 0.12f, 0.18f, 0.24f, 0.08f), 0.8f, 0f, 0f),
        SP(1.3f, 0.055f, 6500f, 120f, 0.1f, 0.95f, 0.5f, 0.26f, 1.1f, 4, ec(0.06f, 0.32f, 0.15f, 0.24f, 0.32f, 0.14f, 0.6f, 0.07f), 0.9f, 0f, 0f),
        SP(1.5f, 0.07f, 6500f, 105f, 0.12f, 1.0f, 0.65f, 0.3f, 1.2f, 4, ec(0.07f, 0.34f, 0.16f, 0.25f, 0.34f, 0.14f, 0.65f, 0.07f), 0.6f, 0f, 0f),
        SP(2.2f, 0.1f, 5500f, 80f, 0.18f, 1.2f, 0.9f, 0.36f, 1.3f, 5, ec(0.09f, 0.4f, 0.2f, 0.3f, 0.4f, 0.2f, 0.8f, 0.12f, 1.3f, 0.06f), 0.0f, 0.62f, 0f),
        SP(1.5f, 0.09f, 4500f, 90f, 0.14f, 1.1f, 0.5f, 0.26f, 0.9f, 4, ec(0.07f, 0.34f, 0.16f, 0.24f, 0.34f, 0.13f, 0.6f, 0.07f), 0.0f, 0f, 0.45f))
    private const val TWO_PI = 6.2831853f
    private class Rn(var s: Int) { fun next(): Float { s = s xor (s shl 13); s = s xor (s ushr 17); s = s xor (s shl 5); return ((s.toLong() and 0xFFFFFFFFL).toFloat() / 4294967296f) * 2f - 1f } }

    private fun gRing(o: FloatArray, sr: Float, t0: Float, f: Float, tau: Float, amp: Float, parts: FloatArray, gains: FloatArray, np: Int) {
        val s0 = (t0 * sr).toInt(); val n = min(o.size - s0, (tau * 7 * sr).toInt()); if (n <= 0) return
        for (k in 0 until np) {
            val w = TWO_PI * f * parts[k] / sr; val g = amp * gains[k]; val d = expf(-1f / sr / (tau / (1 + k * 0.6f))); var e = g
            for (i in 0 until n) { o[s0 + i] += sinf(w * i) * e; e *= d }
        }
    }
    private val RP = floatArrayOf(1f, 2.76f, 5.4f); private val RG = floatArrayOf(1f, 0.5f, 0.25f)
    private fun ring2(o: FloatArray, sr: Float, t0: Float, f: Float, tau: Float, amp: Float) = gRing(o, sr, t0, f, tau, amp, RP, RG, 3)
    private fun ring2b(o: FloatArray, sr: Float, t0: Float, f: Float, tau: Float, amp: Float, p2: Float, g2: Float) = gRing(o, sr, t0, f, tau, amp, floatArrayOf(1f, p2), floatArrayOf(1f, g2), 2)
    private fun gTick(o: FloatArray, sr: Float, t0: Float, tau: Float, amp: Float, r: Rn, hp: Float = 0.5f) {
        val s0 = (t0 * sr).toInt(); val n = min(o.size - s0, (tau * 8 * sr).toInt()); if (n <= 0) return
        var p = 0f; var pw = 0f; var e = amp; val d = expf(-1f / sr / tau)
        for (i in 0 until n) { val w = r.next(); p = hp * (p + w - pw); pw = w; o[s0 + i] += p * e; e *= d }
    }
    private fun gThump(o: FloatArray, sr: Float, t0: Float, f0: Float, tau: Float, amp: Float) {
        val s0 = (t0 * sr).toInt(); val n = min(o.size - s0, (tau * 6 * sr).toInt()); if (n <= 0) return
        var ph = 0f; var fe = f0; var e = amp; var at = 1f; val df = expf(-1f / sr / (tau * 0.8f)); val de = expf(-1f / sr / tau); val da = expf(-1f / sr / 0.002f)
        for (i in 0 until n) { ph += TWO_PI * (fe + 40) / sr; o[s0 + i] += sinf(ph) * e * (1 - at); fe *= df; e *= de; at *= da }
    }
    private fun gGrate(o: FloatArray, sr: Float, t0: Float, dur: Float, amp: Float, r: Rn, rate: Float, fc: Float) {
        val s0 = (t0 * sr).toInt(); val n = min(o.size - s0, (dur * sr).toInt()); if (n <= 0) return
        var lp = 0f; val k = min(0.95f, TWO_PI * fc / sr)
        for (i in 0 until n) {
            val t = i / sr; val w = r.next(); lp += k * (w - lp)
            val am = 0.35f + 0.65f * max(0f, sinf(TWO_PI * rate * t)); val env = sinf(3.14159265f * i / n)
            o[s0 + i] += (w - lp) * 0.6f * am * env * amp + lp * 0.1f * env * amp
        }
    }
    private fun gFinish(o: FloatArray, sr: Float, drive: Float, peak: Float) {
        var mx = 0f
        for (i in o.indices) { o[i] = tanhf(o[i] * drive); mx = max(mx, fabsf(o[i])) }
        val g = peak / (if (mx > 0) mx else 1f); val fo = (0.03f * sr).toInt(); val N = o.size
        for (i in 0 until N) { o[i] *= g; if (i > N - fo) o[i] *= (N - i).toFloat() / fo }
    }

    private fun renderShot(kind: Int, seed: Int, supp: Boolean): FloatArray {
        val sr = 48000f; val P = SPAR[kind]; val rn = Rn(seed * 7919 + (kind * 104729 + 13)); if (rn.s == 0) rn.s = 1
        val N = (P.dur * sr).toInt(); val out = FloatArray(N); val core = FloatArray(N)
        val heavy = !(kind == 0 || kind == 3 || kind == 1)
        var lo = 0f; var bp = 0f; var hpP = 0f; var hpW = 0f; var ph = 0f
        val bt = if (supp) P.bt * 1.25f else P.bt; val hi = if (supp) 1800f else P.hi
        val dCk = expf(-1 / sr / 0.0012f); val dFc = expf(-1 / sr / (bt * 1.3f)); val dBl = expf(-1 / sr / bt); val dBa = expf(-1 / sr / 0.0004f)
        val dTf = expf(-1 / sr / 0.05f); val dTh = expf(-1 / sr / P.tt); val dTa = expf(-1 / sr / 0.0015f)
        var eCk = 1f; var eFc = 1f; var eBl = 1f; var eBa = 1f; var eTf = 1f; var eTh = 1f; var eTa = 1f
        val bAmp = if (supp) 0.9f else 1.7f; val tAmp = P.ta * (if (supp) 0.55f else 1f)
        for (i in 0 until N) {
            val w = rn.next(); hpP = 0.55f * (hpP + w - hpW); hpW = w; val crack = if (supp) 0f else hpP * eCk * P.ck * 1.6f
            val fc = 450 + (hi - 450) * eFc; val f = min(1.2f, TWO_PI * fc / sr); lo += f * bp; val hp2 = w - lo - 0.8f * bp; bp += f * hp2; val blast = lo * eBl * bAmp * (1 - eBa)
            ph += TWO_PI * (P.tf * eTf + 42) / sr; val th = sinf(ph) * eTh * tAmp * (1 - eTa); core[i] = crack + blast + th
            eCk *= dCk; eFc *= dFc; eBl *= dBl; eBa *= dBa; eTf *= dTf; eTh *= dTh; eTa *= dTa
        }
        var tl = 0f; val tailTau = if (supp) P.tail * 0.5f else P.tail; val tailAmp = if (supp) P.tl * 0.6f else P.tl
        val dF = expf(-1 / sr / 0.25f); val dT = expf(-1 / sr / tailTau); val dA = expf(-1 / sr / 0.01f); var eF = 1f; var eT = 1f; var eA = 1f
        for (i in 0 until N) { val fc = 300 + 900 * eF; tl += min(0.9f, TWO_PI * fc / sr) * (rn.next() - tl); out[i] = core[i] + tl * eT * tailAmp * 2.2f * (1 - eA); eF *= dF; eT *= dT; eA *= dA }
        for (e in 0 until P.ne) {
            val d = (P.ec[e][0] * sr * (0.95f + 0.1f * fabsf(rn.next()))).toInt(); val g = P.ec[e][1] * (if (supp) 0.5f else 1f); val k = max(0.06f, 0.45f - e * 0.09f); var lp = 0f
            for (i in d until N) { lp += k * (core[i - d] - lp); out[i] += lp * g * (if (e % 2 != 0) -1f else 1f) }
        }
        val mech = if (supp) max(P.mech, 0.5f) * 1.6f else P.mech
        if (mech > 0) { gTick(out, sr, 0.045f, 0.004f, 0.25f * mech, rn, 0.7f); ring2(out, sr, 0.048f, 2300f, 0.02f, 0.1f * mech) }
        if (P.bolt > 0) { val b = P.bolt; gGrate(out, sr, b, 0.1f, 0.22f, rn, 80f, 2000f); ring2(out, sr, b + 0.1f, 1300f, 0.05f, 0.16f); gTick(out, sr, b + 0.15f, 0.004f, 0.3f, rn); gThump(out, sr, b + 0.15f, 130f, 0.04f, 0.3f) }
        if (P.pump > 0) { val b = P.pump; gGrate(out, sr, b, 0.09f, 0.22f, rn, 70f, 1800f); gTick(out, sr, b + 0.16f, 0.004f, 0.35f, rn); gThump(out, sr, b + 0.16f, 110f, 0.05f, 0.4f); ring2(out, sr, b + 0.16f, 1500f, 0.04f, 0.16f) }
        val shp = floatArrayOf(0.34f, 0.47f, 0.55f); if (kind == 0 || kind == 1) { shp[0] = 0.28f; shp[1] = 0.4f }
        for (q in 0 until (if (heavy) 2 else 3)) { ring2b(out, sr, shp[q] + 0.02f * rn.next(), 4800 + 600 * rn.next(), 0.018f, (0.055f - q * 0.015f) * (if (kind == 7) 1.4f else 1f), 1.9f, 0.4f) }
        gFinish(out, sr, if (supp) 1.2f else 1.5f, 0.9f)
        return out
    }
    private val MDUR = floatArrayOf(0.35f, 0.4f, 0.3f, 0.25f, 0.22f, 0.5f, 0.45f)
    private fun renderMech(id: Int): FloatArray {
        val sr = 48000f; val r = Rn(id * 6007 + 31337); val N = (MDUR[id] * sr).toInt(); val o = FloatArray(N)
        when (id) {
            0 -> { gTick(o, sr, 0f, 0.003f, 0.7f, r, 0.6f); ring2(o, sr, 0f, 1900f, 0.025f, 0.35f); gGrate(o, sr, 0.015f, 0.11f, 0.3f, r, 60f, 1800f); ring2b(o, sr, 0.14f, 820f, 0.02f, 0.2f, 2.2f, 0.4f); gTick(o, sr, 0.15f, 0.005f, 0.25f, r, 0.3f) }
            1 -> { gGrate(o, sr, 0f, 0.1f, 0.22f, r, 55f, 1600f); gTick(o, sr, 0.1f, 0.005f, 0.8f, r, 0.4f); gThump(o, sr, 0.1f, 170f, 0.045f, 0.7f); ring2(o, sr, 0.1f, 2400f, 0.022f, 0.3f); gTick(o, sr, 0.145f, 0.002f, 0.5f, r, 0.7f); ring2(o, sr, 0.145f, 3100f, 0.012f, 0.25f) }
            2 -> { gGrate(o, sr, 0f, 0.14f, 0.34f, r, 90f, 2200f); ring2(o, sr, 0f, 1200f, 0.06f, 0.12f); gTick(o, sr, 0.15f, 0.004f, 0.35f, r, 0.6f); gThump(o, sr, 0.15f, 140f, 0.04f, 0.35f) }
            3 -> { gTick(o, sr, 0f, 0.004f, 0.9f, r, 0.5f); ring2(o, sr, 0f, 2800f, 0.04f, 0.5f); gThump(o, sr, 0f, 120f, 0.06f, 0.65f); ring2(o, sr, 0f, 1400f, 0.05f, 0.25f) }
            4 -> { gTick(o, sr, 0f, 0.003f, 0.5f, r, 0.5f); ring2(o, sr, 0f, 3600f, 0.03f, 0.3f); ring2b(o, sr, 0f, 700f, 0.025f, 0.28f, 2.4f, 0.4f); gTick(o, sr, 0.07f, 0.002f, 0.25f, r, 0.6f) }
            5 -> { gGrate(o, sr, 0f, 0.1f, 0.3f, r, 70f, 1800f); ring2(o, sr, 0f, 1000f, 0.06f, 0.1f); gTick(o, sr, 0.17f, 0.004f, 0.95f, r, 0.5f); gThump(o, sr, 0.17f, 110f, 0.06f, 0.7f); ring2(o, sr, 0.17f, 1600f, 0.05f, 0.3f) }
            else -> { gGrate(o, sr, 0f, 0.09f, 0.3f, r, 80f, 2200f); gTick(o, sr, 0.14f, 0.004f, 0.85f, r, 0.5f); ring2(o, sr, 0.14f, 2500f, 0.035f, 0.4f); gThump(o, sr, 0.14f, 150f, 0.04f, 0.5f) }
        }
        gFinish(o, sr, 1.1f, 0.85f)
        return o
    }
    @Volatile private var inited = false
    /** Heavy: renders all sample buffers. Call from the audio thread before mixing. */
    @Synchronized fun initBuffers() {
        if (inited) return
        val tmp = Array(NBUF) { FloatArray(0) }
        for (k in 0 until 8) tmp[k] = renderShot(k, 1, false)
        tmp[B_SUPP] = renderShot(2, 1, true)
        for (m in 0 until 7) tmp[B_MAGOUT + m] = renderMech(m)
        synchronized(lk) { for (i in 0 until NBUF) BUFS[i] = tmp[i] }
        inited = true
    }
    private fun lenOf(id: Int): Int = when (id) { SHOT -> 9600; SHOTGUN -> 16000; EXPLODE -> 48000; HIT -> 4800; RELOAD -> 17000; STEP -> 3000; SNIPER -> 24000; PISTOL -> 7200; HURT -> 8000; KILL -> 14400; else -> 2400 }

    private fun pickVoice(): Voice {
        for (v in vs) if (v.pos >= v.len) return v
        var b = vs[0]; for (i in 1 until MAXV) if (vs[i].pos > b.pos) b = vs[i]
        return b
    }
    @JvmStatic @JvmOverloads fun play(id: Int, vol: Float = 1f, pan: Float = 0f) {
        if (!enabled || vol < 0.02f) return
        synchronized(lk) { val v = pickVoice(); v.id = id; v.pos = 0; v.len = lenOf(id); v.vol = if (vol > 1) 1f else vol; v.pan = pan; v.fp = 0f; v.rate = 1f }
    }
    private fun startBuf(bid: Int, vol: Float, pan: Float, rate: Float) {
        if (bid < 0 || bid >= NBUF || BUFS[bid].size < 4) return
        val v = pickVoice(); v.id = BUF_BASE + bid; v.pos = 0; v.len = BUFS[bid].size; v.vol = if (vol > 1) 1f else vol; v.pan = pan; v.fp = 0f; v.rate = rate
    }
    private fun playBuf(bid: Int, vol: Float, pan: Float = 0f, rate: Float = 1f) { if (!enabled || vol < 0.02f) return; synchronized(lk) { startBuf(bid, vol, pan, rate) } }
    private class Sch { @JvmField var t = 0f; @JvmField var bid = 0; @JvmField var vol = 0f; @JvmField var pan = 0f; @JvmField var on = false }
    private val sch = Array(24) { Sch() }
    private fun later(bid: Int, delay: Float, vol: Float, pan: Float = 0f) {
        if (!enabled) return
        synchronized(lk) { for (q in sch) if (!q.on) { q.t = delay; q.bid = bid; q.vol = vol; q.pan = pan; q.on = true; break } }
    }
    @JvmStatic @JvmOverloads fun shot(kind: Int, supp: Boolean, vol: Float, pan: Float = 0f) { playBuf(if (supp) B_SUPP else (kind and 7), if (supp) vol * 0.6f else vol, pan, 0.96f + 0.08f * (rnd() * 0.5f + 0.5f)) }
    @JvmStatic fun reload(kind: Int, dur: Float, vol: Float) {
        fun E(bid: Int, at: Float) = later(bid, max(0.01f, at * dur), vol)
        if (kind == 7) { for (k in 0 until 4) E(B_SHELL, 0.14f + 0.16f * k + 0.1f); E(B_PUMP, 0.8f) }
        else if (kind == 0 || kind == 1) { E(B_MAGOUT, 0.17f); E(B_MAGIN, 0.5f); E(B_SLIDE, 0.7f) }
        else { E(B_MAGOUT, 0.17f); E(B_MAGIN, 0.5f); E(B_BOLT, 0.69f); E(B_SLAM, 0.83f) }
    }

    private val stt = FloatArray(MAXV)
    private const val PI2 = 2 * 3.14159f
    private fun sample(id: Int, n: Int, st: FloatArray, vi: Int): Float {
        val t = n / 48000f; var x = 0f; var s = st[vi]
        when (id) {
            SHOT -> { val e = expf(-t * 38f); s = 0.55f * s + 0.45f * rnd(); x = (s * 0.9f + sinf(t * PI2 * (150 - t * 400)) * 0.7f) * e }
            SHOTGUN -> { val e = expf(-t * 20f); s = 0.7f * s + 0.3f * rnd(); x = (s * 1.1f + sinf(t * PI2 * (90 - t * 150)) * 0.8f) * e }
            EXPLODE -> { val e = expf(-t * 4.5f); s = 0.97f * s + 0.03f * rnd(); x = (s * 14f + sinf(t * PI2 * (55 - t * 25)) * 0.9f) * e; if (x > 1.2f) x = 1.2f }
            HIT -> { x = sinf(t * PI2 * 1500f) * expf(-t * 50f) * 0.7f }
            HURT -> { val e = expf(-t * 18f); s = 0.85f * s + 0.15f * rnd(); x = (s * 2f + sinf(t * PI2 * 80f)) * e * 0.7f }
            KILL -> { val f = if (t < 0.1f) 880f else 1320f; x = sinf(t * PI2 * f) * expf(-t * 9f) * 0.6f }
            RELOAD -> {
                val a = expf(-t * 60f) * sinf(t * PI2 * 1800f)
                val b = if (t > 0.18f) expf(-(t - 0.18f) * 50f) * sinf((t - 0.18f) * PI2 * 1100f) else 0f
                val c = if (t > 0.27f) expf(-(t - 0.27f) * 70f) * sinf((t - 0.27f) * PI2 * 2400f) * 0.8f else 0f
                s = 0.6f * s + 0.4f * rnd(); x = (a + b) * 0.6f + c * 0.5f + s * 0.05f * (if (t < 0.01f) 1f else 0f)
            }
            STEP -> { val e = expf(-t * 45f); s = 0.8f * s + 0.2f * rnd(); x = s * e * 0.8f + sinf(t * PI2 * 70f) * e * 0.3f }
            SNIPER -> { val e = expf(-t * 9f); s = 0.4f * s + 0.6f * rnd(); x = (s * 1.2f + sinf(t * PI2 * (70 - t * 60)) * 1.0f) * e + (if (t > 0.25f) expf(-(t - 0.25f) * 40f) * sinf((t - 0.25f) * PI2 * 1400f) * 0.3f else 0f) }
            PISTOL -> { val e = expf(-t * 55f); s = 0.4f * s + 0.6f * rnd(); x = (s * 0.9f + sinf(t * PI2 * (260 - t * 500)) * 0.5f) * e }
            else -> { x = sinf(t * PI2 * 600f) * expf(-t * 80f) * 0.4f }
        }
        st[vi] = s
        return x
    }
    private val L = FloatArray(512); private val R = FloatArray(512)
    /** Fills [frames] stereo frames (interleaved L,R) into out starting at 0. frames <= 512 per call. */
    @JvmStatic fun mix(out: ShortArray, frames0: Int) {
        var frames = frames0; if (frames > 512) frames = 512
        for (i in 0 until frames) { L[i] = 0f; R[i] = 0f }
        synchronized(lk) {
            val dt = frames / 48000f
            for (q in sch) { if (!q.on) continue; q.t -= dt; if (q.t <= 0) { q.on = false; startBuf(q.bid, q.vol, q.pan, 0.97f + 0.06f * (rnd() * 0.5f + 0.5f)) } }
            for (v in 0 until MAXV) {
                val a = vs[v]; if (a.pos >= a.len) continue
                val gl = a.vol * (if (a.pan > 0) 1 - a.pan * 0.6f else 1f); val gr = a.vol * (if (a.pan < 0) 1 + a.pan * 0.6f else 1f)
                if (a.id >= BUF_BASE) {
                    val B = BUFS[a.id - BUF_BASE]
                    for (i in 0 until frames) {
                        val i0 = a.fp.toInt(); if (i0 >= a.len - 1 || i0 + 1 >= B.size) { a.pos = a.len; break }
                        val fr = a.fp - i0; val sv = B[i0] * (1 - fr) + B[i0 + 1] * fr; L[i] += sv * gl; R[i] += sv * gr; a.fp += a.rate
                    }
                    continue
                }
                if (a.pos == 0) stt[v] = 0f
                var i = 0
                while (i < frames && a.pos < a.len) { val s = sample(a.id, a.pos, stt, v); L[i] += s * gl; R[i] += s * gr; i++; a.pos++ }
            }
        }
        val es = engSpeed; val dv = droneVol
        if (es >= 0f) { val f = 38f + es * 3.2f; for (i in 0 until frames) { engPh += f / 48000f; if (engPh > 1) engPh -= 1f; engN = 0.9f * engN + 0.1f * rnd(); val sq = (if (engPh < 0.5f) 1f else -1f) * 0.5f + sinf(engPh * 6.28318f * 2) * 0.5f; val v = (sq * 0.35f + engN * 0.25f) * (0.15f + min(1f, es / 26f) * 0.25f); L[i] += v; R[i] += v } }
        if (dv > 0f) { for (i in 0 until frames) { drPh += (190f + sinf(drPh * 0.05f) * 6f) / 48000f; if (drPh > 1) drPh -= 1f; val v = ((if (drPh < 0.5f) 1f else -1f) * 0.5f + rnd() * 0.3f) * 0.07f * dv; L[i] += v; R[i] += v } }
        for (i in 0 until frames) {
            var l = L[i] * 0.5f; var r = R[i] * 0.5f; l = if (l > 1) 1f else if (l < -1) -1f else l; r = if (r > 1) 1f else if (r < -1) -1f else r
            out[2 * i] = (l * 30000).toInt().toShort(); out[2 * i + 1] = (r * 30000).toInt().toShort()
        }
    }
}
