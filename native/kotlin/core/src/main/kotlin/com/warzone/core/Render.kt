package com.warzone.core

import kotlin.math.*

class Cam(@JvmField var x: Float, @JvmField var y: Float, @JvmField var z: Float, @JvmField var yaw: Float, @JvmField var pitch: Float, @JvmField var fov: Float)

/** Growable float buffer used to collect triangle vertices. */
class FBuf(cap: Int = 1 shl 16) {
    @JvmField var a = FloatArray(cap); @JvmField var n = 0
    fun clear() { n = 0 }
    fun ensure(extra: Int) { if (n + extra > a.size) a = a.copyOf(max(a.size * 2, n + extra)) }
    val vertexCount get() = n / Renderer.STRIDE
    val triangleCount get() = n / Renderer.STRIDE / 3
}

/**
 * Scene builder: collects colored triangles (the "GPU path" of the C++ renderer, r.gv[...]) that the Android GL layer draws.
 * Vertex layout (STRIDE floats): x y z r g b u v flags   (flags: bit0 = fog, bit1 = grid detail texture)
 * Layer 0 = world, layer 1 = first-person viewmodel (drawn after clearing depth).
 */
class Renderer {
    companion object { const val STRIDE = 9 }
    @JvmField var W = 1; @JvmField var H = 1
    @JvmField var cam = Cam(0f, 0f, 0f, 0f, 0f, 1.3f)
    @JvmField var fx = 0f; @JvmField var fy = 0f; @JvmField var fz = 0f; @JvmField var rx = 0f; @JvmField var rz = 0f; @JvmField var ux = 0f; @JvmField var uy = 0f; @JvmField var uz = 0f; @JvmField var focal = 1f
    var m: MapDef? = null
    @JvmField val fog = FloatArray(3); @JvmField var fogN = 30f; @JvmField var fogF = 100f; @JvmField val sunN = floatArrayOf(0f, 1f, 0f)
    @JvmField val layers = arrayOf(FBuf(), FBuf())
    @JvmField var zBias = 0f
    @JvmField var obbRoll = 0f
    @JvmField var cullHalf = 1f

    fun init(w: Int, h: Int) { W = w; H = h }
    fun reset() { layers[0].clear(); layers[1].clear(); zBias = 0f }

    fun setCam(c: Cam) {
        cam = c; val cy = cosf(c.yaw); val sy = sinf(c.yaw); val cp = cosf(c.pitch); val sp = sinf(c.pitch)
        fx = -sy * cp; fy = sp; fz = -cy * cp; rx = cy; rz = -sy
        ux = -rz * fy; uy = rz * fx - rx * fz; uz = rx * fy
        focal = (H * 0.5f) / tanf(c.fov * 0.5f)
    }

    /** Column-major view*projection matrix for GL. */
    fun viewProj(out: FloatArray, near: Float, far: Float) {
        val aspect = W.toFloat() / H; val t = 1f / tanf(cam.fov * 0.5f)
        // view rows: right, up, -forward
        val c = cam
        val tx = -(rx * c.x + 0f * c.y + rz * c.z); val ty = -(ux * c.x + uy * c.y + uz * c.z); val tz = (fx * c.x + fy * c.y + fz * c.z)
        // V = [r.x r.y r.z tx; u.x u.y u.z ty; -f.x -f.y -f.z tz; 0 0 0 1]
        val v = floatArrayOf(rx, 0f, rz, tx, ux, uy, uz, ty, -fx, -fy, -fz, tz, 0f, 0f, 0f, 1f)
        val pa = t / aspect; val pb = t; val pc = -(far + near) / (far - near); val pd = -2f * far * near / (far - near)
        // P rows
        for (col in 0 until 4) {
            val v0 = v[col]; val v1 = v[4 + col]; val v2 = v[8 + col]; val v3 = v[12 + col]
            out[col * 4 + 0] = pa * v0
            out[col * 4 + 1] = pb * v1
            out[col * 4 + 2] = pc * v2 + (if (col == 3) pd else 0f)
            out[col * 4 + 3] = -v2
        }
    }

    fun lightOf(nx: Float, ny: Float, nz: Float): Float {
        val d = m!!; var dt = nx * sunN[0] + ny * sunN[1] + nz * sunN[2]; if (dt < 0) dt = 0f; val hemi = 0.5f + 0.5f * ny
        return d.amb * (0.55f + 0.45f * hemi) + d.sunI * dt
    }

    fun beginWorld(mm: MapDef) {
        m = mm
        val sl = sqrtf(mm.sun[0] * mm.sun[0] + mm.sun[1] * mm.sun[1] + mm.sun[2] * mm.sun[2]); for (i in 0 until 3) sunN[i] = mm.sun[i] / sl
        for (i in 0 until 3) fog[i] = mm.fog[i]; fogN = mm.fogN; fogF = mm.fogF; zBias = 0f
    }

    private fun tri(o: FBuf, ax: Float, ay: Float, az: Float, au: Float, av: Float, bx: Float, by: Float, bz: Float, bu: Float, bv: Float,
                    cx: Float, cy: Float, cz: Float, cu: Float, cv: Float, r: Float, g: Float, b: Float, flags: Float) {
        o.ensure(27); val a = o.a; var n = o.n
        a[n++] = ax; a[n++] = ay; a[n++] = az; a[n++] = r; a[n++] = g; a[n++] = b; a[n++] = au; a[n++] = av; a[n++] = flags
        a[n++] = bx; a[n++] = by; a[n++] = bz; a[n++] = r; a[n++] = g; a[n++] = b; a[n++] = bu; a[n++] = bv; a[n++] = flags
        a[n++] = cx; a[n++] = cy; a[n++] = cz; a[n++] = r; a[n++] = g; a[n++] = b; a[n++] = cu; a[n++] = cv; a[n++] = flags
        o.n = n
    }

    /** convex polygon p (n*3 floats), optional uv (n*2) */
    fun poly(p: FloatArray, n0: Int, r: Float, g: Float, b: Float, detail: Boolean = false, uv: FloatArray? = null, useFog: Boolean = true) {
        val n = if (n0 > 8) 8 else n0
        val o = layers[if (zBias > 0) 1 else 0]
        val flags = (if (useFog) 1f else 0f) + (if (detail && uv != null) 2f else 0f)
        for (i in 1 until n - 1) {
            tri(o, p[0], p[1], p[2], uv?.get(0) ?: 0f, uv?.get(1) ?: 0f,
                p[i * 3], p[i * 3 + 1], p[i * 3 + 2], uv?.get(i * 2) ?: 0f, uv?.get(i * 2 + 1) ?: 0f,
                p[(i + 1) * 3], p[(i + 1) * 3 + 1], p[(i + 1) * 3 + 2], uv?.get((i + 1) * 2) ?: 0f, uv?.get((i + 1) * 2 + 1) ?: 0f, r, g, b, flags)
        }
    }

    private val rt = FloatArray(3)
    fun rot(yaw: Float, pitch: Float, x: Float, y: Float, z: Float, o: FloatArray) {
        val cp = cosf(pitch); val sp = sinf(pitch); val y1 = y * cp - z * sp; val z1 = y * sp + z * cp
        val c = cosf(yaw); val s = sinf(yaw); o[0] = x * c + z1 * s; o[1] = y1; o[2] = -x * s + z1 * c
    }

    private val OP = FloatArray(24); private val FN = FloatArray(3); private val FP = FloatArray(12)
    private val F = arrayOf(intArrayOf(0, 2, 3, 1), intArrayOf(4, 5, 7, 6), intArrayOf(0, 4, 6, 2), intArrayOf(1, 3, 7, 5), intArrayOf(0, 1, 5, 4), intArrayOf(2, 6, 7, 3))
    private val N = arrayOf(floatArrayOf(0f, 0f, -1f), floatArrayOf(0f, 0f, 1f), floatArrayOf(-1f, 0f, 0f), floatArrayOf(1f, 0f, 0f), floatArrayOf(0f, -1f, 0f), floatArrayOf(0f, 1f, 0f))
    fun obb(cx: Float, cy: Float, cz: Float, hx: Float, hy: Float, hz: Float, yaw: Float, pitch: Float, r: Float, g: Float, b: Float, fogOn: Boolean = true, lit: Boolean = true) {
        var rc = 1f; var rs = 0f; if (obbRoll != 0f) { rc = cosf(obbRoll); rs = sinf(obbRoll) }
        for (i in 0 until 8) {
            var lx = if (i and 1 != 0) hx else -hx; var ly = if (i and 2 != 0) hy else -hy
            if (obbRoll != 0f) { val nx = lx * rc - ly * rs; ly = lx * rs + ly * rc; lx = nx }
            rot(yaw, pitch, lx, ly, if (i and 4 != 0) hz else -hz, rt)
            OP[i * 3] = cx + rt[0]; OP[i * 3 + 1] = cy + rt[1]; OP[i * 3 + 2] = cz + rt[2]
        }
        for (f in 0 until 6) {
            var nx0 = N[f][0]; var ny0 = N[f][1]
            if (obbRoll != 0f) { val nx = nx0 * rc - ny0 * rs; ny0 = nx0 * rs + ny0 * rc; nx0 = nx }
            rot(yaw, pitch, nx0, ny0, N[f][2], FN)
            val q0 = F[f][0] * 3
            if ((cam.x - OP[q0]) * FN[0] + (cam.y - OP[q0 + 1]) * FN[1] + (cam.z - OP[q0 + 2]) * FN[2] <= 0) continue
            val l = if (lit) fmin(1.25f, lightOf(FN[0], FN[1], FN[2])) else 1f
            for (k in 0 until 4) { val s = F[f][k] * 3; FP[k * 3] = OP[s]; FP[k * 3 + 1] = OP[s + 1]; FP[k * 3 + 2] = OP[s + 2] }
            poly(FP, 4, r * l, g * l, b * l, false, null, fogOn)
        }
    }

    private val AP = FloatArray(12); private val AUV = FloatArray(8)
    private fun face(nx: Float, ny: Float, nz: Float, r: Float, g: Float, b2: Float, ax: Int,
                     a0: Float, a1: Float, a2: Float, b0: Float, b1: Float, b2_: Float, c0: Float, c1: Float, c2: Float, d0: Float, d1: Float, d2: Float) {
        AP[0] = a0; AP[1] = a1; AP[2] = a2; AP[3] = b0; AP[4] = b1; AP[5] = b2_; AP[6] = c0; AP[7] = c1; AP[8] = c2; AP[9] = d0; AP[10] = d1; AP[11] = d2
        for (k in 0 until 4) {
            val x = AP[k * 3]; val y = AP[k * 3 + 1]; val z = AP[k * 3 + 2]
            if (ax == 0) { AUV[k * 2] = z; AUV[k * 2 + 1] = y } else if (ax == 2) { AUV[k * 2] = x; AUV[k * 2 + 1] = y } else { AUV[k * 2] = x; AUV[k * 2 + 1] = z }
        }
        val l = lightOf(nx, ny, nz); poly(AP, 4, r * l, g * l, b2 * l, true, AUV)
    }
    fun aabb(b: Box, r: Float, g: Float, b2: Float) {
        val x0 = b.x0; val x1 = b.x1; val y0 = b.y0; val y1 = b.y1; val z0 = b.z0; val z1 = b.z1
        if (cam.x < x0) face(-1f, 0f, 0f, r, g, b2, 0, x0, y0, z0, x0, y0, z1, x0, y1, z1, x0, y1, z0)
        if (cam.x > x1) face(1f, 0f, 0f, r, g, b2, 0, x1, y0, z1, x1, y0, z0, x1, y1, z0, x1, y1, z1)
        if (cam.z < z0) face(0f, 0f, -1f, r, g, b2, 2, x1, y0, z0, x0, y0, z0, x0, y1, z0, x1, y1, z0)
        if (cam.z > z1) face(0f, 0f, 1f, r, g, b2, 2, x0, y0, z1, x1, y0, z1, x1, y1, z1, x0, y1, z1)
        if (cam.y > y1) face(0f, 1f, 0f, r, g, b2, 1, x0, y1, z1, x1, y1, z1, x1, y1, z0, x0, y1, z0)
        if (cam.y < y0) face(0f, -1f, 0f, r, g, b2, 1, x0, y0, z0, x1, y0, z0, x1, y0, z1, x0, y0, z1)
    }

    fun cull(b: Box): Boolean {
        val cx = (b.x0 + b.x1) * .5f; val cy = (b.y0 + b.y1) * .5f; val cz = (b.z0 + b.z1) * .5f
        val rr = 0.5f * sqrtf((b.x1 - b.x0) * (b.x1 - b.x0) + (b.y1 - b.y0) * (b.y1 - b.y0) + (b.z1 - b.z0) * (b.z1 - b.z0))
        val dx = cx - cam.x; val dy = cy - cam.y; val dz = cz - cam.z; val d = sqrtf(dx * dx + dy * dy + dz * dz)
        if (d - rr > fogF) return true
        val zf = dx * fx + dy * fy + dz * fz; if (zf + rr < 0) return true
        val half = cullHalf; val xr = dx * rx + dz * rz
        if (zf > 0 && fabsf(xr) - rr > zf * half + rr) return true
        return false
    }

    /** الخريطة الثابتة تُبنى مرة واحدة (كل الأوجه، بلا قصّ) وتُرسم من كرت الشاشة كل إطار: يوفّر معظم عمل المعالج. */
    @JvmField val statics = FBuf(1 shl 18); @JvmField var staticId = -1; @JvmField var staticVer = 0
    private fun aabbAll(b: Box, r: Float, g: Float, b2: Float) {
        val x0 = b.x0; val x1 = b.x1; val y0 = b.y0; val y1 = b.y1; val z0 = b.z0; val z1 = b.z1
        face(-1f, 0f, 0f, r, g, b2, 0, x0, y0, z0, x0, y0, z1, x0, y1, z1, x0, y1, z0)
        face(1f, 0f, 0f, r, g, b2, 0, x1, y0, z1, x1, y0, z0, x1, y1, z0, x1, y1, z1)
        face(0f, 0f, -1f, r, g, b2, 2, x1, y0, z0, x0, y0, z0, x0, y1, z0, x1, y1, z0)
        face(0f, 0f, 1f, r, g, b2, 2, x0, y0, z1, x1, y0, z1, x1, y1, z1, x0, y1, z1)
        face(0f, 1f, 0f, r, g, b2, 1, x0, y1, z1, x1, y1, z1, x1, y1, z0, x0, y1, z0)
    }
    fun ensureStatic(w: World) {
        if (staticId == w.id && staticVer == w.hashCode()) return
        val keep = layers[0]; layers[0] = statics; statics.clear(); val oz = zBias
        val mm = w.m; beginWorld(mm)
        for (b in mm.deco) { if (b.x1 == b.x0) continue; val c = mm.cols[b.k % mm.ncol]; aabbAll(b, c[0], c[1], c[2]) }
        for (b in w.boxes) { val c = mm.cols[b.k % mm.ncol]; aabbAll(b, c[0], c[1], c[2]) }
        val W2 = mm.W / 2 + 60; val D2 = mm.D / 2 + 60
        val g = floatArrayOf(-W2, 0f, -D2, -W2, 0f, D2, W2, 0f, D2, W2, 0f, -D2); val uv = floatArrayOf(-W2, -D2, -W2, D2, W2, D2, W2, -D2)
        val l = lightOf(0f, 1f, 0f); poly(g, 4, mm.ground[0] * l, mm.ground[1] * l, mm.ground[2] * l, true, uv)
        layers[0] = keep; zBias = oz; staticId = w.id; staticVer = w.hashCode(); staticDirty = true
    }
    @JvmField var staticDirty = true
    /** يجهّز الإطار: الخريطة الثابتة من الذاكرة المخزّنة؛ r.layers تحوي الديناميكي فقط. */
    fun beginFrame(w: World, c: Cam) { reset(); setCam(c); ensureStatic(w); beginWorld(w.m) }

    fun world(w: World) {
        val mm = w.m; beginWorld(mm); cullHalf = tanf(cam.fov * 0.5f) * W.toFloat() / H * 1.05f + 0.2f
        for (b in mm.deco) { if (b.x1 == b.x0 || cull(b)) continue; val c = mm.cols[b.k % mm.ncol]; aabb(b, c[0], c[1], c[2]) }
        for (b in w.boxes) { if (cull(b)) continue; val c = mm.cols[b.k % mm.ncol]; aabb(b, c[0], c[1], c[2]) }
        val W2 = mm.W / 2 + 60; val D2 = mm.D / 2 + 60
        val g = floatArrayOf(-W2, 0f, -D2, -W2, 0f, D2, W2, 0f, D2, W2, 0f, -D2); val uv = floatArrayOf(-W2, -D2, -W2, D2, W2, D2, W2, -D2)
        val l = lightOf(0f, 1f, 0f); poly(g, 4, mm.ground[0] * l, mm.ground[1] * l, mm.ground[2] * l, true, uv)
    }
}
