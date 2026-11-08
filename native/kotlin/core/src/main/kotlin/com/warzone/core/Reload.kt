package com.warzone.core

import kotlin.math.max
import kotlin.math.min

/** Reload animation pose (port of native/src/reload.h) */
class RPose {
    @JvmField var x = 0f; @JvmField var y = 0f; @JvmField var z = 0f; @JvmField var rx = 0f; @JvmField var ry = 0f; @JvmField var rz = 0f; @JvmField var tilt = 0f; @JvmField var bolt = 0f
    @JvmField val hand = FloatArray(3)
    @JvmField var hideMag = false; @JvmField var hideRear = true; @JvmField var handle = false
    @JvmField var nm = 0; @JvmField val mg = Array(3) { FloatArray(3) }; @JvmField val ms = FloatArray(3)
    @JvmField var ns = 0; @JvmField val sh = FloatArray(3)
    @JvmField var shiftIdx = -1; @JvmField var shiftV = 0f; @JvmField var shiftIdx2 = -1
    @JvmField var hasRocket = false; @JvmField val rk = FloatArray(3)
}

object RL {
    private fun ez(x: Float) = if (x <= 0) 0f else if (x >= 1) 1f else x * x * (3 - 2 * x)
    private fun sg(t: Float, a: Float, b: Float) = ez((t - a) / (b - a))
    private fun mix3(a: FloatArray, b: FloatArray, k: Float, o: FloatArray) { for (i in 0 until 3) o[i] = a[i] + (b[i] - a[i]) * k }
    private fun set3(o: FloatArray, a: Float, b: Float, c: Float) { o[0] = a; o[1] = b; o[2] = c }
    private val PT = arrayOf(floatArrayOf(0.00f, -0.26f, -0.32f, 0.80f), floatArrayOf(0.17f, -0.24f, -0.16f, 0.45f), floatArrayOf(0.32f, -0.30f, -0.06f, -0.10f), floatArrayOf(0.45f, -0.16f, 0.0f, -0.42f), floatArrayOf(0.55f, 0.0f, 0.0f, -0.36f))

    private fun launcherPose(t: Float, P: RPose) {
        val tilt = sg(t, 0f, 0.14f) - sg(t, 0.9f, 1f); P.tilt = tilt; P.hideRear = false; P.y = 0.05f * tilt; P.x = -0.03f * tilt; P.z = -0.06f * tilt; P.rx = 0.42f * tilt; P.ry = 0f; P.rz = -0.32f * tilt; P.hideMag = t < 0.74f
        val H0 = floatArrayOf(-0.03f, -0.13f, -0.08f); val off = floatArrayOf(0f, 0f, 0f); var have = false
        if (t >= 0.07f && t < 0.55f) {
            var i = 0; while (i < 3 && t >= PT[i + 1][0]) i++
            val k = if (i != 0) sg(t, PT[i][0], PT[i + 1][0]) else sg(t, 0.07f, PT[1][0])
            val a = floatArrayOf(PT[i][1], PT[i][2], PT[i][3]); val b = floatArrayOf(PT[i + 1][1], PT[i + 1][2], PT[i + 1][3]); mix3(a, b, k, off); have = true
        } else if (t >= 0.55f && t < 0.74f) { val k = sg(t, 0.6f, 0.72f); off[0] = 0f; off[1] = 0f; off[2] = -0.36f * (1 - k); have = true }
        P.hasRocket = have; for (i in 0 until 3) P.rk[i] = off[i]
        val hand = floatArrayOf(H0[0], H0[1], H0[2])
        if (t < 0.07f) { val a = floatArrayOf(-0.2f, -0.3f, 0.6f); mix3(H0, a, sg(t, 0f, 0.07f), hand) }
        else if (have && t < 0.74f) { set3(hand, off[0] - 0.02f, off[1] - 0.06f, -0.46f + off[2] + 0.1f) }
        else if (t < 0.9f) { val a = floatArrayOf(0f, -0.06f, -0.26f); mix3(a, H0, sg(t, 0.78f, 0.9f), hand) }
        for (i in 0 until 3) P.hand[i] = hand[i]
        P.z += 0.03f * max(0f, sinf(min(1f, max(0f, (t - 0.7f) / 0.08f)) * 3.14159265f))
    }

    fun pose(g: GunDef, pistol: Boolean, shotgun: Boolean, t: Float, P: RPose, launcher: Boolean = false) {
        if (launcher) { launcherPose(t, P); return }
        val M = floatArrayOf(0f, -0.08f, 0.05f); val MS = floatArrayOf(0.028f, 0.09f, 0.04f)
        if (g.hasMag) for (i in 0 until 3) { M[i] = g.mag[i]; MS[i] = g.mag[3 + i] }
        for (i in 0 until 3) P.ms[i] = MS[i]
        val H0 = FloatArray(3); if (pistol) set3(H0, -0.03f, -0.075f, 0.05f) else set3(H0, -0.005f, -0.05f, g.mz * 0.42f)
        val tilt = sg(t, 0f, 0.13f) - sg(t, 0.88f, 1f); P.tilt = tilt
        P.y = 0.09f * tilt; P.x = -0.1f * tilt; P.z = -0.12f * tilt; P.rx = -0.2f * tilt; P.ry = 0f; P.rz = -0.5f * tilt; P.hideRear = true
        for (i in 0 until 3) P.hand[i] = H0[i]
        val pouch = floatArrayOf(-0.14f, -0.17f, 0.06f)
        if (shotgun) {
            val W = floatArrayOf(0.14f, 0.30f, 0.46f, 0.62f); val hand = floatArrayOf(H0[0], H0[1], H0[2]); val port = floatArrayOf(0f, -0.035f, -0.02f); val mid = floatArrayOf(-0.05f, -0.2f, 0.02f); var cur = false; val cp = floatArrayOf(0f, 0f, 0f)
            for (k in 0 until 4) {
                val a = W[k]; val b = a + 0.14f
                if (t >= a && t < b + 0.02f) {
                    val u = (t - a) / (b - a); val pos = FloatArray(3)
                    if (u < 0.45f) mix3(pouch, mid, sg(u, 0f, 0.45f), pos) else mix3(mid, port, sg(u, 0.45f, 0.9f), pos)
                    if (u < 0.92f) { cur = true; for (i in 0 until 3) cp[i] = pos[i] }; set3(hand, pos[0] - 0.01f, pos[1] - 0.045f, pos[2])
                }
            }
            if (cur) { P.ns = 1; for (i in 0 until 3) P.sh[i] = cp[i] }
            val pump = sg(t, 0.78f, 0.84f) - sg(t, 0.84f, 0.9f)
            if (t >= 0.76f && t < 0.92f) { P.shiftIdx = 2; P.shiftIdx2 = 3; P.shiftV = 0.12f * pump; set3(hand, -0.005f, -0.04f, g.mz * 0.42f + 0.12f * pump) }
            else if (t >= 0.9f) { val a = floatArrayOf(-0.005f, -0.04f, g.mz * 0.42f); mix3(a, H0, sg(t, 0.9f, 0.97f), hand) }
            else if (t < 0.14f) { mix3(H0, pouch, sg(t, 0.04f, 0.14f), hand) }
            for (i in 0 until 3) P.hand[i] = hand[i]; return
        }
        val Mb = floatArrayOf(M[0], M[1] - MS[1] * 0.5f - 0.02f, M[2])
        val hOld = sg(t, 0.14f, 0.30f)
        P.hideMag = t < 0.58f; P.nm = 0
        if (t < 0.52f) {
            if (t < 0.30f) { set3(P.mg[P.nm++], M[0] - 0.01f * hOld, M[1] - 0.17f * hOld, M[2]) }
            else { val q = t - 0.30f; if (q < 0.17f) set3(P.mg[P.nm++], M[0] - 0.01f - q * 0.5f, M[1] - 0.17f - 12 * q * q, M[2] + q * 0.3f) }
        }
        val hand = floatArrayOf(H0[0], H0[1], H0[2]); var haveNew = false; val np = floatArrayOf(0f, 0f, 0f)
        val hp1 = floatArrayOf(0f, 0.025f, 0.06f); val hr1 = floatArrayOf(0f, 0.05f, 0.05f); val HC = if (pistol) hp1 else hr1
        if (t < 0.14f) { mix3(H0, Mb, sg(t, 0f, 0.14f), hand) }
        else if (t < 0.30f) { set3(hand, Mb[0] - 0.01f * hOld, Mb[1] - 0.17f * hOld, Mb[2]) }
        else if (t < 0.40f) { val a = floatArrayOf(Mb[0] - 0.01f, Mb[1] - 0.17f, Mb[2]); mix3(a, pouch, sg(t, 0.30f, 0.40f), hand) }
        else if (t < 0.58f) {
            val k = if (t < 0.52f) sg(t, 0.40f, 0.52f) else 1f; val slam = sg(t, 0.52f, 0.575f); val tgt = floatArrayOf(Mb[0], Mb[1] - 0.04f, Mb[2]); val a = FloatArray(3); mix3(pouch, tgt, k, a)
            if (t < 0.52f) { for (i in 0 until 3) np[i] = a[i] } else mix3(tgt, M, slam, np)
            haveNew = true; set3(hand, np[0] - 0.015f, np[1] - MS[1] * 0.5f - 0.03f, np[2])
        }
        else if (t < 0.69f) { val a = floatArrayOf(M[0] - 0.015f, M[1] - 0.07f, M[2]); mix3(a, HC, sg(t, 0.585f, 0.69f), hand) }
        else if (t < 0.84f) { for (i in 0 until 3) hand[i] = HC[i] }
        else { mix3(HC, H0, sg(t, 0.84f, 0.95f), hand) }
        if (haveNew) { for (i in 0 until 3) P.mg[P.nm][i] = np[i]; P.nm++ }
        fun c01(v: Float) = max(0f, min(1f, v))
        P.z += 0.022f * max(0f, sinf(c01((t - 0.57f) / 0.06f) * 3.14159265f)) - 0.012f * max(0f, sinf(c01((t - 0.82f) / 0.05f) * 3.14159265f))
        val b = sg(t, 0.69f, 0.78f) - sg(t, 0.8f, 0.83f); P.bolt = 0.06f * b
        if (pistol) { P.shiftIdx = 0; P.shiftV = P.bolt; if (t >= 0.69f && t < 0.84f) set3(hand, 0f, 0.025f, 0.06f + P.bolt) }
        else { P.handle = (t >= 0.58f && t < 0.9f); if (t >= 0.69f && t < 0.84f) set3(hand, 0f, 0.05f, 0.05f + P.bolt) }
        for (i in 0 until 3) P.hand[i] = hand[i]
    }
}
