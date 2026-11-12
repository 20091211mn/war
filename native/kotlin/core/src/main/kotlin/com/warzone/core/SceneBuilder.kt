package com.warzone.core

import kotlin.math.abs
import kotlin.math.max
import kotlin.math.min

/** Builds the 3D scene (world, vehicles, avatars, effects, first-person gun) as colored triangles in [Renderer]. Port of Game::render() and friends. */
class SceneBuilder(private val g: Game) {
    private val r get() = g.r
    private var gsc = 1f
    private var vmMinD = 0f
    private val vmF = floatArrayOf(0f, 0f, -1f)
    private var rlp: RPose? = null
    private val ONE = floatArrayOf(1f, 1f, 1f); private val FAST = floatArrayOf(1.5f, 0.95f, 0.5f)
    private var bob = 0f
    private val o3 = FloatArray(3); private val n3 = FloatArray(3); private val pts9 = FloatArray(9)

    private val Tc = arrayOf(floatArrayOf(.14f, .14f, .16f), floatArrayOf(.34f, .35f, .38f), floatArrayOf(.05f, .05f, .06f), floatArrayOf(.42f, .27f, .13f), floatArrayOf(.5f, .45f, .32f), floatArrayOf(.9f, .5f, .1f), floatArrayOf(.86f, .68f, .55f))
    private fun gcol(c: Char, o: FloatArray) { val k = when (c) { 'd' -> 0; 'g' -> 1; 'b' -> 2; 'w' -> 3; 't' -> 4; 'o' -> 5; else -> 6 }; o[0] = Tc[k][0]; o[1] = Tc[k][1]; o[2] = Tc[k][2] }

    private fun vmLight(nx: Float, ny: Float, nz: Float): Float {
        val fx = vmF[0]; val fy = vmF[1]; val fz = vmF[2]; var rx = -fz; var rz = fx; val rl = sqrtf(rx * rx + rz * rz) + 1e-6f; rx /= rl; rz /= rl
        val ux = -fy * rz; val uy = rz * fx - rx * fz; val uz = rx * fy
        val Lx = -fx * 0.55f - rx * 0.45f + ux * 0.75f; val Ly = -fy * 0.55f + uy * 0.75f; val Lz = -fz * 0.55f - rz * 0.45f + uz * 0.75f; val ll = 1f / sqrtf(Lx * Lx + Ly * Ly + Lz * Lz)
        val d = (nx * Lx + ny * Ly + nz * Lz) * ll; return 0.42f + 0.78f * fmax(0f, d)
    }

    private class MeshWork(nv: Int) { val w = FloatArray(nv * 6); val done = IntArray(nv); var stamp = 0 }
    private val work = HashMap<GunMesh, MeshWork>()

    private fun drawAwmGroup(wi: Int, grp: Int, ox: Float, oy: Float, oz: Float, yaw: Float, pitch: Float, roll: Float, dz: Float, k: Float, lit: Boolean, tint: FloatArray, dx: Float = 0f, dy: Float = 0f) {
        val M = GameData.meshOf(wi) ?: return
        var off = 0; for (i in 0 until grp) off += M.grp[i]; val cnt = M.grp[grp]; if (cnt <= 0) return
        val mw = work.getOrPut(M) { MeshWork(M.nv) }; mw.stamp++; val W = mw.w; val done = mw.done; val st = mw.stamp
        val rc = cosf(roll); val rs = sinf(roll)
        fun xf(vi: Int) {
            if (done[vi] == st) return; done[vi] = st
            var lx = (M.vx[vi] + dx) * gsc; var ly = (M.vy[vi] + dy) * gsc
            if (roll != 0f) { val t = lx * rc - ly * rs; ly = lx * rs + ly * rc; lx = t }
            r.rot(yaw, pitch, lx, ly, (M.vz[vi] + dz) * gsc, o3)
            var nx = M.nx[vi]; var ny = M.ny[vi]; if (roll != 0f) { val t = nx * rc - ny * rs; ny = nx * rs + ny * rc; nx = t }
            r.rot(yaw, pitch, nx, ny, M.nz[vi], n3)
            val q = vi * 6; W[q] = ox + o3[0]; W[q + 1] = oy + o3[1]; W[q + 2] = oz + o3[2]; W[q + 3] = n3[0]; W[q + 4] = n3[1]; W[q + 5] = n3[2]
        }
        val cam = r.cam
        var t = 0; var backface: Boolean
        while (t < cnt) {
            val a = M.index(off + t); val b = M.index(off + t + 1); val c = M.index(off + t + 2); t += 3
            xf(a); xf(b); xf(c); val A = a * 6; val B = b * 6; val C = c * 6
            val nx = W[A + 3] + W[B + 3] + W[C + 3]; val ny = W[A + 4] + W[B + 4] + W[C + 4]; val nz = W[A + 5] + W[B + 5] + W[C + 5]
            val cx = (W[A] + W[B] + W[C]) * (1f / 3); val cy = (W[A + 1] + W[B + 1] + W[C + 1]) * (1f / 3); val cz = (W[A + 2] + W[B + 2] + W[C + 2]) * (1f / 3)
            run {
                val e1x = W[B] - W[A]; val e1y = W[B + 1] - W[A + 1]; val e1z = W[B + 2] - W[A + 2]; val e2x = W[C] - W[A]; val e2y = W[C + 1] - W[A + 1]; val e2z = W[C + 2] - W[A + 2]
                var fx = e1y * e2z - e1z * e2y; var fy = e1z * e2x - e1x * e2z; var fz = e1x * e2y - e1y * e2x
                if (fx * nx + fy * ny + fz * nz < 0) { fx = -fx; fy = -fy; fz = -fz }
                backface = vmMinD <= 0 && (cam.x - cx) * fx + (cam.y - cy) * fy + (cam.z - cz) * fz <= 0
            }
            if (backface) continue
            if (vmMinD > 0) {
                val d0 = (W[A] - cam.x) * vmF[0] + (W[A + 1] - cam.y) * vmF[1] + (W[A + 2] - cam.z) * vmF[2]
                val d1 = (W[B] - cam.x) * vmF[0] + (W[B + 1] - cam.y) * vmF[1] + (W[B + 2] - cam.z) * vmF[2]
                val d2 = (W[C] - cam.x) * vmF[0] + (W[C + 1] - cam.y) * vmF[1] + (W[C + 2] - cam.z) * vmF[2]
                if (fmin(d0, fmin(d1, d2)) < vmMinD) continue
            }
            val il = 1f / fmax(1e-5f, sqrtf(nx * nx + ny * ny + nz * nz))
            var l = if (lit) fmin(1.25f, r.lightOf(nx * il, ny * il, nz * il)) else (if (vmMinD > 0) vmLight(nx * il, ny * il, nz * il) else 1f); l *= k
            val cr = (M.cr[a] + M.cr[b] + M.cr[c]) * (1f / 3) * l * tint[0]; val cg = (M.cg[a] + M.cg[b] + M.cg[c]) * (1f / 3) * l * tint[1]; val cb = (M.cb[a] + M.cb[b] + M.cb[c]) * (1f / 3) * l * tint[2]
            pts9[0] = W[A]; pts9[1] = W[A + 1]; pts9[2] = W[A + 2]; pts9[3] = W[B]; pts9[4] = W[B + 1]; pts9[5] = W[B + 2]; pts9[6] = W[C]; pts9[7] = W[C + 1]; pts9[8] = W[C + 2]
            r.poly(pts9, 3, cr, cg, cb, false, null, true)
        }
    }

    private val cc3 = FloatArray(3); private val cb3 = FloatArray(3); private val cd3 = FloatArray(3)
    fun drawGunAt(ox: Float, oy: Float, oz: Float, yaw: Float, pitch: Float, wi: Int, flash: Float, lit: Boolean, k: Float, at: GAtt?, hideOptic: Boolean) {
        val gd = GameData.gundefs[wi]
        val aOptic = if (hideOptic) 0 else (at?.optic ?: 0); val aMuz = at?.muz ?: 0; val aUnder = at?.under ?: 0; val aMag = at?.mag ?: 0
        val R = rlp; val rl = R?.rz ?: 0f; r.obbRoll = rl
        fun part(x: Float, y: Float, z: Float, sx: Float, sy: Float, sz: Float, c: FloatArray) {
            var xx = x; var yy = y; if (rl != 0f) { val c_ = cosf(rl); val s_ = sinf(rl); xx = x * c_ - y * s_; yy = x * s_ + y * c_ }
            r.rot(yaw, pitch, xx, yy, z, o3); r.obb(ox + o3[0], oy + o3[1] + 0f, oz + o3[2], sx * .5f, sy * .5f, sz * .5f, yaw, pitch, c[0] * k, c[1] * k, c[2] * k, true, lit)
        }
        val hasMesh = GameData.meshOf(wi) != null
        val awm = hasMesh && (vmMinD > 0 || run { val dx = ox - r.cam.x; val dy = oy - r.cam.y; val dz = oz - r.cam.z; dx * dx + dy * dy + dz * dz < 20f * 20f })
        if (awm) {
            drawAwmGroup(wi, 0, ox, oy, oz, yaw, pitch, rl, 0f, k, lit, ONE)
            if (!(R != null && R.hideRear)) drawAwmGroup(wi, 3, ox, oy, oz, yaw, pitch, rl, 0f, k, lit, ONE)
            drawAwmGroup(wi, 2, ox, oy, oz, yaw, pitch, rl, R?.bolt ?: 0f, k, lit, ONE)
            if (!(R != null && R.hideMag)) drawAwmGroup(wi, 1, ox, oy, oz, yaw, pitch, rl, 0f, k, lit, if (aMag == 2) FAST else ONE)
            if (R != null && R.hasRocket) drawAwmGroup(wi, 1, ox, oy, oz, yaw, pitch, rl, R.rk[2], k, lit, ONE, R.rk[0], R.rk[1])
        }
        if (!awm) for (i in 0 until gd.np) {
            gcol(gd.pc[i], cc3)
            if (R != null && R.hideRear && i != 0 && gd.pz(i) - gd.psz(i) * 0.5f > 0.1f) continue
            val dz = if (R != null && (i == R.shiftIdx || i == R.shiftIdx2)) R.shiftV else 0f
            part(gd.px(i), gd.py(i), gd.pz(i) + dz, gd.psx(i), gd.psy(i), gd.psz(i), cc3)
        }
        gcol('b', cb3); gcol('d', cd3)
        if (!awm && gd.hasMag && !(R != null && R.hideMag)) {
            val kk = if (aMag == 1) 1.4f else 1f; val c = FloatArray(3); if (aMag == 2) { c[0] = 0.9f; c[1] = 0.5f; c[2] = 0.1f } else gcol('b', c)
            part(gd.mag[0], gd.mag[1] - (kk - 1) * gd.mag[4] * 0.5f, gd.mag[2], gd.mag[3], gd.mag[4] * kk, gd.mag[5], c)
        }
        if (!awm && aOptic > 0) {
            val OP = arrayOf(floatArrayOf(0f, 0f, 0f, 0f), floatArrayOf(0.03f, 0.035f, 0.05f, 0.075f), floatArrayOf(0.034f, 0.034f, 0.12f, 0.08f), floatArrayOf(0.04f, 0.04f, 0.18f, 0.085f), floatArrayOf(0.045f, 0.045f, 0.24f, 0.09f))
            val o = OP[aOptic]; val zc = -0.04f; val top = o[3]
            part(0f, 0.044f, zc, 0.024f, 0.014f, o[2] * 0.6f + 0.04f, cb3)
            if (aOptic == 1) {
                val gl = floatArrayOf(0.5f, 0.8f, 1f); val rd = floatArrayOf(1f, 0.1f, 0.1f)
                part(0f, top - 0.02f, zc, 0.034f, 0.04f, 0.05f, cb3); part(0f, top - 0.02f, zc - 0.026f, 0.026f, 0.028f, 0.004f, gl); part(0f, top - 0.018f, zc - 0.028f, 0.006f, 0.006f, 0.003f, rd)
            } else {
                val Rr = o[0]; val L = o[2]; val cy = top - Rr / 2 - 0.004f; val gl = floatArrayOf(0.3f, 0.55f, 0.9f)
                part(0f, cy, zc, Rr, Rr, L, cb3); part(0f, cy, zc - L / 2 - 0.012f, Rr * 1.35f, Rr * 1.35f, 0.03f, cd3); part(0f, cy, zc + L / 2 + 0.01f, Rr * 1.2f, Rr * 1.2f, 0.025f, cd3); part(0f, cy, zc - L / 2 - 0.028f, Rr * 1.05f, Rr * 1.05f, 0.004f, gl)
            }
        }
        var mzOff = 0f
        if (aMuz == 1) { part(0f, 0.005f, gd.mz - 0.07f, 0.036f, 0.036f, 0.2f, cb3); for (q in 0 until 4) part(0f, 0.005f, gd.mz - 0.01f - q * 0.045f, 0.04f, 0.04f, 0.006f, cd3); mzOff = -0.16f }
        else if (aMuz == 2) { part(0f, 0.005f, gd.mz - 0.01f, 0.036f, 0.036f, 0.06f, cd3); part(0.019f, 0.005f, gd.mz - 0.01f, 0.004f, 0.02f, 0.03f, cb3); part(-0.019f, 0.005f, gd.mz - 0.01f, 0.004f, 0.02f, 0.03f, cb3); part(0f, 0.021f, gd.mz - 0.01f, 0.02f, 0.004f, 0.03f, cb3) }
        if (aUnder == 1) { part(0f, -0.075f, gd.mz * 0.5f - 0.02f, 0.028f, 0.09f, 0.03f, cd3); part(0f, -0.032f, gd.mz * 0.5f - 0.02f, 0.04f, 0.012f, 0.05f, cb3); part(0f, -0.118f, gd.mz * 0.5f - 0.02f, 0.032f, 0.01f, 0.034f, cb3) }
        else if (aUnder == 2) { val rd = floatArrayOf(1f, 0.1f, 0.1f); part(0.03f, -0.015f, gd.mz * 0.5f, 0.022f, 0.026f, 0.07f, cb3); part(0.03f, -0.015f, gd.mz * 0.5f - 0.04f, 0.012f, 0.012f, 0.012f, rd) }
        if (flash > 0) {
            val c = floatArrayOf(1f, 0.85f, 0.4f); val w2 = floatArrayOf(1f, 1f, 0.85f); val sz = (if (aMuz == 1) 0.35f else 1f) * (0.06f + 0.05f * flash); val zz = gd.mz + mzOff - 0.03f
            part(0f, 0.005f, zz, sz, sz, sz * 2.2f, w2); part(0f, 0.005f, zz - 0.01f, sz * 3.2f, sz * 0.35f, sz * 0.5f, c); part(0f, 0.005f, zz - 0.01f, sz * 0.35f, sz * 3.2f, sz * 0.5f, c); part(0f, 0.005f, zz - 0.12f, sz * 0.5f, sz * 0.5f, sz * 3f, c)
        }
        if (R != null) {
            val mcol = FloatArray(3); if (aMag == 2) { mcol[0] = 0.9f; mcol[1] = 0.5f; mcol[2] = 0.1f } else gcol('b', mcol)
            for (q in 0 until R.nm) part(R.mg[q][0], R.mg[q][1], R.mg[q][2], R.ms[0], R.ms[1], R.ms[2], mcol)
            if (R.ns > 0) { val br = floatArrayOf(0.85f, 0.62f, 0.18f); val cp = floatArrayOf(0.7f, 0.1f, 0.08f); part(R.sh[0], R.sh[1], R.sh[2], 0.02f, 0.02f, 0.07f, br); part(R.sh[0], R.sh[1], R.sh[2] + 0.03f, 0.021f, 0.021f, 0.018f, cp) }
            if (R.handle && !awm) { val gg = FloatArray(3); gcol('g', gg); val sn = (wi == 3); part(if (sn) 0.062f else 0.03f, if (sn) 0f else 0.032f, 0.05f + R.bolt, 0.014f, 0.014f, 0.04f, gg) }
        }
        r.obbRoll = 0f
    }

    private val pants = floatArrayOf(.17f, .18f, .15f); private val boot = floatArrayOf(.08f, .07f, .07f); private val vest = floatArrayOf(.2f, .22f, .2f); private val plate = floatArrayOf(.26f, .28f, .25f)
    private val pocket = floatArrayOf(.16f, .17f, .15f); private val pack = floatArrayOf(.18f, .2f, .16f); private val belt = floatArrayOf(.1f, .1f, .11f); private val skin = floatArrayOf(.86f, .68f, .55f)
    private val glove = floatArrayOf(.1f, .1f, .11f); private val visor = floatArrayOf(.05f, .06f, .08f); private val mask = floatArrayOf(.12f, .12f, .12f)
    private val tcol = arrayOf(floatArrayOf(0.82f, 0.14f, 0.14f), floatArrayOf(0.15f, 0.36f, 0.9f)); private val tdk = arrayOf(floatArrayOf(0.5f, 0.08f, 0.08f), floatArrayOf(0.08f, 0.2f, 0.55f))

    // ---- أشجار تتمايل مع الريح + نوافذ وإفريز للمباني القريبة (جودة عالية فأعلى) ----
    private class Tree(val x: Float, val z: Float, val h: Float, val ph: Float)
    private var trees = ArrayList<Tree>(); private var treeMap = -1
    private fun buildTrees() {
        treeMap = g.w.id; trees = ArrayList(); val w = g.w; if (w.id >= 8) return
        var sd = (0x9E3779B9L * (w.id + 7)).toInt()
        fun rnd(): Float { sd = sd * 1664525 + 1013904223; return ((sd ushr 8) and 0xFFFFFF) / 16777216f }
        var n = 0; var tries = 0
        while (n < 60 && tries < 900) { tries++
            val x = (rnd() - 0.5f) * w.m.W * 0.96f; val z = (rnd() - 0.5f) * w.m.D * 0.96f; var bad = false
            var dx = -1.6f; while (dx <= 1.6f && !bad) { var dz = -1.6f; while (dz <= 1.6f && !bad) { for (k in w.cellAt(x + dx, z + dz)) { val b = w.boxes[k]; if (x + dx > b.x0 - 0.6f && x + dx < b.x1 + 0.6f && z + dz > b.z0 - 0.6f && z + dz < b.z1 + 0.6f && b.y1 > 0.3f) { bad = true; break } }; dz += 1.6f }; dx += 1.6f }
            for (p in w.m.red) { val ax = x - p.x; val az = z - p.z; if (ax * ax + az * az < 100f) bad = true }
            for (p in w.m.blue) { val ax = x - p.x; val az = z - p.z; if (ax * ax + az * az < 100f) bad = true }
            if (bad) continue; trees.add(Tree(x, z, 3.2f + rnd() * 2.6f, rnd() * 6.28f)); n++ }
    }
    private fun drawScenery(c: Cam) {
        val q = Lang.quality; if (q < 2) return; val w = g.w; if (treeMap != w.id) buildTrees(); val t = g.t
        val nt = min(trees.size, intArrayOf(0, 0, 10, 28, 60)[q.coerceIn(0, 4)]); val snow = w.id == 3; val desert = w.id == 1 || w.id == 4 || w.id == 5 || w.id == 7
        for (i in 0 until nt) { val T = trees[i]; val dx = T.x - c.x; val dz = T.z - c.z; val dd = dx * dx + dz * dz; if (dd > r.fogF * r.fogF || dd > 110f * 110f) continue
            val sw = sinf(t * 1.25f + T.ph) * 0.09f + sinf(t * 3.1f + T.ph * 2) * 0.025f; val h = T.h; val tx = T.x + sw * h * 0.35f; val tz = T.z + sw * h * 0.2f
            r.obb((T.x + tx) * 0.5f, h * 0.25f, (T.z + tz) * 0.5f, 0.14f, h * 0.25f, 0.14f, 0f, 0f, 0.33f, 0.22f, 0.12f); r.obb(tx, h * 0.62f, tz, 0.11f, h * 0.14f, 0.11f, 0f, 0f, 0.30f, 0.2f, 0.11f)
            val fx = T.x + sw * h * 0.9f; val fz = T.z + sw * h * 0.5f; val cr = if (snow) 0.16f else if (desert) 0.3f else 0.14f; val cg = if (snow) 0.34f else if (desert) 0.5f else 0.42f; val cb = if (snow) 0.2f else if (desert) 0.16f else 0.16f
            if (desert) { for (k in 0 until 6) { val a = k * 1.047f + T.ph; r.obb(fx + cosf(a), h * 0.82f - 0.1f, fz + sinf(a), 0.9f, 0.05f, 0.16f, a + sw * 2, 0.35f, cr, cg, cb) } }
            else { r.obb(fx - sw * h * 0.2f, h * 0.74f, fz, 1.35f, 0.5f, 1.35f, sw, 0f, cr, cg, cb); r.obb(fx, h * 0.74f + 0.65f, fz, 1f, 0.5f, 1f, sw + 0.4f, 0f, cr * 1.1f, cg * 1.1f, cb * 1.1f); r.obb(fx + sw * h * 0.2f, h * 0.74f + 1.2f, fz, 0.6f, 0.45f, 0.6f, sw + 0.8f, 0f, cr * 1.2f, cg * 1.2f, cb * 1.2f) } }
        if (q >= 3) { val budget = if (q >= 4) 300 else 150; val rD = if (q >= 4) 45f else 32f; var used = 0
            for (b in w.boxes) { if (used >= budget) break; val bw = b.x1 - b.x0; val bd = b.z1 - b.z0; val bh = b.y1 - b.y0; if (bh < 4f || b.y0 > 2f || max(bw, bd) > 50f || min(bw, bd) < 2.5f) continue
                val cx = (b.x0 + b.x1) * 0.5f; val cz = (b.z0 + b.z1) * 0.5f; val ddx = cx - c.x; val ddz = cz - c.z; if (ddx * ddx + ddz * ddz > rD * rD) continue
                r.obb(cx, b.y1 + 0.09f, cz, bw * 0.5f + 0.12f, 0.09f, bd * 0.5f + 0.12f, 0f, 0f, 0.22f, 0.22f, 0.24f)
                for (f in 0 until 4) { if (used >= budget) break; val zf = f < 2; val face = if (f == 0) b.z0 else if (f == 1) b.z1 else if (f == 2) b.x0 else b.x1; val cam = if (zf) c.z else c.x
                    if (if ((f and 1) == 0) cam > face else cam < face) continue; val len = if (zf) bw else bd; val nw = max(1, (len / 2.6f).toInt()); val step = len / nw
                    for (i2 in 0 until nw) { var wy = b.y0 + 1.6f; while (wy < b.y1 - 1.2f && used < budget) { val o = (i2 + 0.5f) * step - len * 0.5f
                        val wx = if (zf) cx + o else face + (if (f == 2) -0.03f else 0.03f); val wz = if (zf) face + (if (f == 0) -0.03f else 0.03f) else cz + o
                        val lit = ((i2 * 7 + (wy * 3).toInt() + w.id) % 5) == 0
                        r.obb(wx, wy + 0.7f, wz, if (zf) 0.55f else 0.04f, 0.7f, if (zf) 0.04f else 0.55f, 0f, 0f, if (lit) 0.9f else 0.1f, if (lit) 0.8f else 0.2f, if (lit) 0.45f else 0.32f); used++; wy += 3.2f } } } } }
    }

    fun drawAvatar(a: Ent, camDist: Float) {
        val c = tcol[a.team]; val d = tdk[a.team]; val x = a.b.x; val y = a.b.y; val z = a.b.z; val yw = a.yaw; val t = g.t
        if (!a.alive) {
            var dt_ = fmax(0f, fmin(1f, (4f - a.respawn) / 0.45f)); if (a.respawn > 4f || a.respawn <= 0) dt_ = 1f; val k = 1 - (1 - dt_) * (1 - dt_); val ph = -1.5708f * k
            fun D(lx: Float, ly: Float, lz: Float, hx: Float, hy: Float, hz: Float, rx_: Float, col: FloatArray) {
                val y1 = ly * cosf(ph) - lz * sinf(ph); val z1 = ly * sinf(ph) + lz * cosf(ph) + 0.0f; r.rot(yw, 0f, lx, y1 + 0.17f * k, z1, o3)
                r.obb(x + o3[0], y + o3[1], z + o3[2], hx * .5f, hy * .5f, hz * .5f, yw, rx_ + ph, col[0] * 0.6f, col[1] * 0.6f, col[2] * 0.6f)
            }
            val sp = 0.12f * k
            D(-0.12f - sp, 0.4f, 0.02f, .2f, .8f, .22f, 0f, pants); D(0.12f + sp, 0.4f, 0.0f, .2f, .8f, .22f, 0f, pants); D(0f, 1.1f, 0f, .44f, .58f, .25f, 0f, c); D(0f, 1.13f, 0.015f, .5f, .46f, .29f, 0f, vest); D(0f, 1.0f, 0.2f, .36f, .42f, .14f, 0f, pack)
            D(0f, 1.58f, 0f, .21f, .23f, .22f, 0f, skin); D(0f, 1.66f, 0.01f, .34f, .2f, .35f, 0f, d); D(0.36f + sp * 2, 1.2f, 0f, .12f, .54f, .12f, 0f, d); D(-0.36f - sp * 2, 1.2f, 0f, .12f, .54f, .12f, 0f, d)
            return
        }
        val avLod = floatArrayOf(14f, 24f, 38f, 60f, 95f)[Lang.quality.coerceIn(0, 4)]
        if (Lang.quality >= 2 && camDist < avLod) { r.obb(x, y + 0.03f, z, 0.42f, 0.012f, 0.42f, yw, 0f, 0.10f, 0.10f, 0.11f, false, false); r.obb(x, y + 0.035f, z, 0.42f, 0.012f, 0.42f, yw + 0.785f, 0f, 0.10f, 0.10f, 0.11f, false, false) }
        if (camDist > avLod) {
            r.obb(x, y + 0.42f, z, .2f, .42f, .12f, yw, 0f, pants[0], pants[1], pants[2]); r.obb(x, y + 1.1f, z, .25f, .3f, .15f, yw, 0f, c[0], c[1], c[2]); r.obb(x, y + 1.58f, z, .12f, .13f, .12f, yw, 0f, d[0], d[1], d[2]); return
        }
        val mv = if (a.moving) 1f else 0f; val cr = a.crouch; val amp = (if (a.sprint) 0.95f else 0.68f) * (if (cr) 0.45f else 1f) * mv
        val hipY = 0.84f - (if (cr) 0.34f else 0f); val bobv = fabsf(sinf(a.phase)) * 0.032f * mv * (if (a.sprint) 1.7f else 1f) * (if (cr) 0.5f else 1f)
        val L = (if (a.sprint) 0.2f else 0f) + (if (cr) 0.1f else 0f) + 0.02f * sinf(a.phase * 2) * mv; val cl = cosf(L); val sl = sinf(L)
        fun B(lx: Float, ly: Float, lz: Float, hx: Float, hy: Float, hz: Float, rx_: Float, col: FloatArray) { r.rot(yw, 0f, lx, ly, lz, o3); r.obb(x + o3[0], y + o3[1], z + o3[2], hx * .5f, hy * .5f, hz * .5f, yw, rx_, col[0], col[1], col[2]) }
        fun UP(lx: Float, ly: Float, lz: Float, hx: Float, hy: Float, hz: Float, col: FloatArray) { val oy = ly - 0.84f; val y2 = oy * cl - lz * sl; val z2 = oy * sl + lz * cl; B(lx, hipY + bobv + y2, z2, hx, hy, hz, L, col) }
        val up3 = FloatArray(3)
        fun UPp(lx: Float, ly: Float, lz: Float, o: FloatArray) { val oy = ly - 0.84f; o[0] = lx; o[1] = hipY + bobv + oy * cl - lz * sl; o[2] = oy * sl + lz * cl }
        for (s2 in 0 until 2) {
            val lx = if (s2 != 0) 0.12f else -0.12f; val ph = a.phase + (if (s2 != 0) 3.14159f else 0f)
            val th = sinf(ph) * amp + (if (cr) 0.85f else 0f); val flex = fmax(0f, cosf(ph)) * 0.85f * mv * (if (cr) 0.5f else 1f) + (if (cr) 1.7f else 0f); val sa = th - flex
            val hy = hipY + bobv * 0.5f; val kx = hy - 0.42f * cosf(th); val kz = -0.42f * sinf(th)
            B(lx, hy - 0.21f * cosf(th), -0.21f * sinf(th), 0.2f, 0.44f, 0.22f, th, pants)
            B(lx, kx - 0.21f * cosf(sa), kz - 0.21f * sinf(sa), 0.18f, 0.44f, 0.2f, sa, pants)
            val ay = kx - 0.42f * cosf(sa); val az = kz - 0.42f * sinf(sa); B(lx, ay - 0.05f, az - 0.08f, 0.21f, 0.14f, 0.3f, sa * 0.35f, boot)
        }
        UP(0f, 1.1f, 0f, .44f, .58f, .25f, c); UP(0f, 1.13f, 0.015f, .5f, .46f, .29f, vest); UP(0f, 1.22f, -0.15f, .36f, .2f, .05f, plate)
        for (px in floatArrayOf(-0.14f, 0f, 0.14f)) UP(px, 0.98f, -0.16f, .1f, .12f, .06f, pocket)
        UP(0f, 1.0f, 0.2f, .36f, .42f, .14f, pack); UP(0f, 0.92f, 0f, .5f, .1f, .3f, belt); UP(0f, 1.44f, 0f, .11f, .1f, .11f, skin)
        run {
            val nk = FloatArray(3); UPp(0f, 1.58f, 0f, nk); val hp = a.pitch * 0.5f
            fun H(ox: Float, oy: Float, oz: Float, sx: Float, sy: Float, sz: Float, col: FloatArray) { val cx = cosf(hp); val sx2 = sinf(hp); B(nk[0] + ox, nk[1] + oy * cx - oz * sx2, nk[2] + oy * sx2 + oz * cx, sx, sy, sz, hp, col) }
            H(0f, 0f, 0f, .21f, .23f, .22f, skin); val hel = floatArrayOf(d[0] * .9f, d[1] * .9f, d[2] * .9f); H(0f, 0.08f, 0.01f, .34f, .2f, .35f, hel); H(0f, 0.02f, -0.115f, .23f, .07f, .03f, visor); H(0f, -0.1f, -0.08f, .18f, .05f, .08f, mask)
        }
        val ap = (if (a.sprint) 0.2f else a.pitch) + (if (Lang.quality >= 2) 0.025f * sinf(a.phase * 2f) * (if (a.moving) 1f else 0f) + 0.012f * sinf(t * 1.6f + a.b.x) else 0f); val base = if (a.sprint) 0.9f else 1.5708f; val br = 0.015f * sinf(a.phase * 0.5f + t * 2f)
        val gs = FloatArray(3); UPp(0.12f, 1.3f, 0f, gs); val cA = cosf(ap); val sA = sinf(ap); val lau = g.isExpl(a.wpn); val gdy = if (lau) -0.13f else 0.05f; val gdz = if (lau) 0.14f else 0.38f; val wo = FloatArray(3)
        r.rot(yw, 0f, gs[0], gs[1] - gdy * cA + gdz * sA, gs[2] - gdy * sA - gdz * cA, wo)
        gsc = if (GameData.meshOf(a.wpn) != null && GameData.wkind[a.wpn % g.WEAPON_COUNT] > 1) 1.5f else 1f
        drawGunAt(x + wo[0], y + wo[1], z + wo[2], yw, ap, a.wpn, if (a.flash > 0) 1f else 0f, true, 1f, null, false); gsc = 1f
        // ذراعان بمفصل مرفق (IK): اليمنى على قبضة الزناد واليسرى على المقدّمة
        run {
            val gsK = if (GameData.meshOf(a.wpn) != null && GameData.wkind[a.wpn % g.WEAPON_COUNT] > 1) 1.5f else 1f; val gd = GameData.gundefs[a.wpn]; val pistol = a.wpn == 6 || a.wpn == 7
            val gl = floatArrayOf(gs[0], gs[1] - gdy * cA + gdz * sA, gs[2] - gdy * sA - gdz * cA)
            fun g2l(hx0: Float, hy0: Float, hz0: Float): FloatArray { val hx = hx0 * gsK; val hy = hy0 * gsK; val hz = hz0 * gsK; return floatArrayOf(gl[0] + hx, gl[1] + hy * cA - hz * sA, gl[2] + hy * sA + hz * cA) }
            fun seg(p0: FloatArray, p1: FloatArray, th: Float, col: FloatArray) { val w0 = FloatArray(3); val w1 = FloatArray(3); r.rot(yw, 0f, p0[0], p0[1], p0[2], w0); r.rot(yw, 0f, p1[0], p1[1], p1[2], w1)
                val dx = w1[0] - w0[0]; val dy = w1[1] - w0[1]; val dz = w1[2] - w0[2]; val l = sqrtf(dx * dx + dy * dy + dz * dz); if (l < 0.01f) return
                r.obb(x + (w0[0] + w1[0]) * 0.5f, y + (w0[1] + w1[1]) * 0.5f, z + (w0[2] + w1[2]) * 0.5f, th * .5f, th * .5f, l * .5f, atan2f(-dx, -dz), asinf(dy / l), col[0], col[1], col[2]) }
            fun limb(side: Float, hand: FloatArray) {
                val S = FloatArray(3); UPp(side * 0.27f, 1.36f, 0f, S); val L1 = 0.3f; val L2 = 0.29f
                val dv = floatArrayOf(hand[0] - S[0], hand[1] - S[1], hand[2] - S[2]); var D = fmax(0.12f, sqrtf(dv[0] * dv[0] + dv[1] * dv[1] + dv[2] * dv[2]))
                val k2 = if (D > L1 + L2 - 0.02f) (L1 + L2 - 0.02f) / D else 1f; val H = floatArrayOf(S[0] + dv[0] * k2, S[1] + dv[1] * k2, S[2] + dv[2] * k2); D *= k2
                val ux = (H[0] - S[0]) / D; val uy = (H[1] - S[1]) / D; val uz = (H[2] - S[2]) / D
                val pv = floatArrayOf(side * 0.55f, -1f, 0.25f); val dp = pv[0] * ux + pv[1] * uy + pv[2] * uz; pv[0] -= ux * dp; pv[1] -= uy * dp; pv[2] -= uz * dp
                var pl = sqrtf(pv[0] * pv[0] + pv[1] * pv[1] + pv[2] * pv[2]); if (pl < 1e-3f) { pv[0] = side; pv[1] = 0f; pv[2] = 0f; pl = 1f }; for (q in 0 until 3) pv[q] /= pl
                val aa = (L1 * L1 - L2 * L2 + D * D) / (2 * D); val hh = sqrtf(fmax(0f, L1 * L1 - aa * aa))
                val E = floatArrayOf(S[0] + ux * aa + pv[0] * hh, S[1] + uy * aa + pv[1] * hh, S[2] + uz * aa + pv[2] * hh)
                seg(S, E, 0.115f, d); seg(E, H, 0.1f, d); val hw = FloatArray(3); r.rot(yw, 0f, H[0], H[1], H[2], hw); r.obb(x + hw[0], y + hw[1], z + hw[2], .05f, .05f, .055f, yw, ap, glove[0], glove[1], glove[2])
            }
            val mzz = gd.mz
            val rh: FloatArray; val lh: FloatArray
            if (lau) { rh = g2l(0.02f, -0.06f, mzz * 0.10f); lh = g2l(-0.01f, 0f, mzz * 0.55f) }
            else if (pistol) { rh = g2l(0f, -0.055f, mzz * 0.20f); lh = g2l(-0.025f, -0.075f, mzz * 0.18f) }
            else { rh = g2l(-0.005f, -0.06f, mzz * 0.32f); lh = g2l(-0.03f, -0.07f, mzz * 0.72f) }
            limb(1f, rh); limb(-1f, lh)
        }
    }

    /** Camera used for the last frame (also read by HUD for projections). */
    private val JP = floatArrayOf(0f,0f,0.5f,0.95f,0.8f,4.8f,0f, 0f,0f,-5.6f,0.75f,0.62f,1.4f,0f, 0f,-0.05f,-7.3f,0.4f,0.35f,0.7f,0f, 0f,0.85f,-2.6f,0.5f,0.38f,1.5f,2f,
        -1f,-0.1f,-2.2f,0.4f,0.5f,1.5f,0f, 1f,-0.1f,-2.2f,0.4f,0.5f,1.5f,0f, -2.9f,-0.2f,1.4f,1.9f,0.07f,2.1f,0f, 2.9f,-0.2f,1.4f,1.9f,0.07f,2.1f,0f,
        -5f,-0.2f,2.6f,1f,0.06f,1.2f,0f, 5f,-0.2f,2.6f,1f,0.06f,1.2f,0f, -2.2f,-0.1f,5.9f,1.3f,0.06f,0.9f,0f, 2.2f,-0.1f,5.9f,1.3f,0.06f,0.9f,0f,
        -1f,1.5f,5.1f,0.06f,1.1f,1.1f,1f, 1f,1.5f,5.1f,0.06f,1.1f,1.1f,1f, 0f,0f,6.2f,0.6f,0.55f,0.5f,3f)
    private val jo = FloatArray(3)
    fun drawJets(c: Cam) {
        for (J in g.jets) { if (!J.alive) continue; val dx = J.x - c.x; val dz = J.z - c.z; if (dx * dx + dz * dz > r.fogF * r.fogF) continue
            val cr = cosf(J.roll); val sr = sinf(J.roll); r.obbRoll = J.roll; val t0 = if (J.team != 0) 0.2f else 0.85f; val t1 = if (J.team != 0) 0.4f else 0.18f; val t2 = if (J.team != 0) 0.9f else 0.18f
            for (k in 0 until 15) { val b = k * 7; val px = JP[b]; val py = JP[b + 1]; val lx = px * cr - py * sr; val ly = px * sr + py * cr; r.rot(J.yaw, J.pitch, lx, ly, JP[b + 2], jo)
                val cc = JP[b + 6].toInt(); val R: Float; val G: Float; val B: Float
                when (cc) { 1 -> { R = t0; G = t1; B = t2 }; 2 -> { R = 0.12f; G = 0.2f; B = 0.3f }; 3 -> { R = 0.1f; G = 0.1f; B = 0.11f }; else -> { R = 0.48f; G = 0.52f; B = 0.57f } }
                r.obb(J.x + jo[0], J.y + jo[1], J.z + jo[2], JP[b + 3], JP[b + 4], JP[b + 5], J.yaw, J.pitch, R, G, B, true, true) }
            if (J.ab > 0.3f) { r.rot(J.yaw, J.pitch, 0f, 0f, 7.2f, jo); r.obb(J.x + jo[0], J.y + jo[1], J.z + jo[2], 0.35f, 0.35f, 0.9f * J.ab, J.yaw, J.pitch, 1f, 0.6f, 0.15f, false, false) }
            r.obbRoll = 0f }
    }
    fun render() {
        val me = g.e[g.meI]; val eyeH = if (me.alive) (me.b.y + (if (me.crouch) PK.eyec else PK.eye)) else me.b.y + 0.6f
        var fov = 1.3f * g.fovMul; if (g.adsHeld && me.alive) { fov = 1.3f * g.fovMul / g.zoomOf(me) }
        var c = Cam(me.b.x, eyeH, me.b.z, me.yaw, me.pitch, fov)
        if (g.drive >= 0 && me.alive && !g.passenger()) {
            val v = g.vh[g.drive]; val cp = cosf(me.pitch); val fxw = -sinf(me.yaw) * cp; val fyw = sinf(me.pitch); val fzw = -cosf(me.yaw) * cp
            val cx0 = v.x; val cy0 = if (v.type == 1) 2.3f else 2.2f; val cz0 = v.z; var D = if (v.type == 1) 11f else 8f
            val tw2 = raycastMap(g.w, cx0, cy0, cz0, -fxw, -fyw, -fzw, D); D = fmin(D, tw2 - 0.3f); D = fmax(if (v.type == 1) 6.5f else 1.5f, D)
            c.x = cx0 - fxw * D; c.y = fmax(0.5f, cy0 - fyw * D); c.z = cz0 - fzw * D; g.camX = c.x; g.camY = c.y; g.camZ = c.z
        }
        if (g.air && g.myJet() >= 0 && me.alive) { val J = g.jets[g.myJet()]; val cp = cosf(J.pitch); val fx = -sinf(J.yaw) * cp; val fy = sinf(J.pitch); val fz = -cosf(J.yaw) * cp
            c = Cam(J.x - fx * 26f, fmax(2.5f, J.y - fy * 26f + 7f), J.z - fz * 26f, J.yaw, J.pitch * 0.9f - 0.12f, 1.3f * g.fovMul); g.camX = c.x; g.camY = c.y; g.camZ = c.z }
        if (g.dn.on && me.alive) c = Cam(g.dn.x, g.dn.y, g.dn.z, g.dn.yaw, g.dn.pitch, 1.3f)
        r.beginFrame(g.w, c)
        val t = g.t
        for (v in g.vh) { val dx = v.x - c.x; val dz = v.z - c.z; val dd = sqrtf(dx * dx + dz * dz); if (dd < r.fogF) VehDraw.draw(r, v, dd) }
        for (s in g.shells) r.obb(s.x, s.y, s.z, 0.012f, 0.012f, 0.03f, t * 20 + s.x * 9, 0f, 0.85f, 0.62f, 0.15f, false, false)
        for (q in g.grs) r.obb(q.x, q.y, q.z, 0.09f, 0.12f, 0.09f, t * 6, 0f, 0.2f, 0.3f, 0.2f, false, false)
        for (q in g.rks) {
            val yw = atan2f(-q.dx, -q.dz); val pt = asinf(q.dy); val px = q.ox + q.dx * q.t - q.dx * 0.475f; val py = q.oy + q.dy * q.t - q.dy * 0.475f; val pz = q.oz + q.dz * q.t - q.dz * 0.475f
            drawAwmGroup(8, 1, px, py, pz, yw, pt, 0f, 0f, 1f, true, ONE)
        }
        for (q in g.c4s) {
            r.obb(q.x, q.y + 0.04f, q.z, 0.1f, 0.04f, 0.065f, 0f, 0f, 0.22f, 0.22f, 0.2f, true, true); r.obb(q.x, q.y + 0.085f, q.z, 0.085f, 0.008f, 0.05f, 0f, 0f, 0.45f, 0.4f, 0.3f, true, true)
            val on = ((t * 2.2f).toInt()) % 2 == 0; val k = if (on) 1f else 0.15f; r.obb(q.x + 0.05f, q.y + 0.1f, q.z, 0.015f, 0.015f, 0.015f, 0f, 0f, if (q.team != 0) 0.1f * k else k, 0.1f * k, if (q.team != 0) k else 0.1f * k, false, false)
        }
        for (q in g.puffs) {
            val k = fmin(1f, q.t)
            if (q.fire) r.obb(q.x, q.y, q.z, q.sz * .4f, q.sz * .4f, q.sz * .4f, 0f, 0f, 1.0f, 0.55f * k + 0.2f, 0.1f, false, false)
            else r.obb(q.x, q.y, q.z, q.sz * .5f, q.sz * .5f, q.sz * .5f, 0f, 0f, 0.12f + 0.1f * k, 0.12f + 0.1f * k, 0.12f + 0.1f * k, false, false)
        }
        if (g.air) drawJets(c)
        for (i in 0 until g.e.size) {
            if ((i == g.meI && !g.dn.on) || g.e[i].gone || (g.air && g.entJet.getOrElse(i) { -1 } >= 0)) continue; val a = g.e[i]; val dx = a.b.x - c.x; val dz = a.b.z - c.z
            if (dx * dx + dz * dz > r.fogF * r.fogF || (dx * dx + dz * dz < 0.5f && i != g.meI)) continue
            drawAvatar(a, sqrtf(dx * dx + dz * dz))
        }
        drawScenery(c)
        for (q in g.tr) {
            val dx = q.x1 - q.x0; val dy = q.y1 - q.y0; val dz = q.z1 - q.z0; val l = sqrtf(dx * dx + dy * dy + dz * dz); if (l < 0.1f) continue
            val p0 = fmin(1f, fmax(0f, 1f - q.t / 0.09f)); val hd = p0 * l; val tl = fmax(0f, hd - fmin(5f, l * 0.7f)); val sg = hd - tl; if (sg < 0.05f) continue
            val yw = atan2f(-dx, -dz); val pt = asinf(dy / l); val m = (hd + tl) * 0.5f / l; val cx = q.x0 + dx * m; val cy = q.y0 + dy * m; val cz = q.z0 + dz * m; val fa = 1f - p0 * 0.5f
            r.obb(cx, cy, cz, 0.022f, 0.022f, sg / 2, yw, pt, 1.0f * fa, 0.62f * fa, 0.2f * fa, false, false); r.obb(cx, cy, cz, 0.01f, 0.01f, sg / 2 + 0.01f, yw, pt, 1f, 0.98f, 0.85f, false, false)
            val hx = q.x0 + dx * hd / l; val hy = q.y0 + dy * hd / l; val hz = q.z0 + dz * hd / l; r.obb(hx, hy, hz, 0.035f, 0.035f, 0.06f, yw, pt, 1f, 1f, 0.9f, false, false)
        }
        for (k in g.sparks) { val a = fmin(1f, k.t * 5f); r.obb(k.x, k.y, k.z, 0.025f, 0.025f, 0.025f, 0f, 0f, 1f, 0.7f * a + 0.2f, 0.2f, false, false) }
        // first-person gun
        if (me.alive && !(g.air && g.myJet() >= 0) && (g.drive < 0 || g.passenger()) && !g.dn.on && !(g.adsHeld && (g.zoomOf(me) >= 2f || (GameData.meshOf(me.wpn) != null && me.wpn != 8)))) {
            r.zBias = 1000f; val gd = GameData.gundefs[me.wpn]; val k = if (g.adsHeld) 1f else 0f; val top = gd.sy
            bob += if (me.moving) (if (me.sprint) 0.17f else 0.11f) else 0f
            val bx = sinf(bob) * 0.008f * (if (me.moving) 1 else 0); val by = fabsf(cosf(bob)) * 0.008f * (if (me.moving) 1 else 0)
            var x = 0.17f * (1 - k) + 0 * k + bx; var y = -0.17f * (1 - k) + (-top - 0.016f) * k + by; var z = -0.33f * (1 - k) - 0.36f * k; val kick = if (me.flash > 0) 0.045f else 0f; z += kick; var rx = kick * 1.2f; var ry = 0.07f * (1 - k)
            y += sinf(t * 1.7f) * 0.0025f * (1 - k * 0.7f); x += sinf(t * 0.9f) * 0.0015f
            val RP = RPose(); var inRl = false
            if (me.reloadT >= 0) {
                val tt = fmin(1f, me.reloadT / (g.WEAPONS[me.wpn].reload * me.relM)); val wk = GameData.wkind[me.wpn % g.WEAPON_COUNT]
                RL.pose(gd, wk <= 1, wk == 7, tt, RP, g.isExpl(me.wpn)); inRl = true; x += RP.x; y += RP.y; z += RP.z; rx += RP.rx; ry += RP.ry
            }
            if (me.sprint) { y -= 0.06f; rx -= 0.35f; ry = 0.4f; x -= 0.04f }
            val O = FloatArray(3); r.rot(me.yaw, me.pitch, x, y, z, O); val gy = me.yaw + ry; val gp = me.pitch + rx; val ox = c.x + O[0]; val oy = c.y + O[1]; val oz = c.z + O[2]
            run { val cp = cosf(me.pitch); vmF[0] = -sinf(me.yaw) * cp; vmF[1] = sinf(me.pitch); vmF[2] = -cosf(me.yaw) * cp; vmMinD = 0.2f }
            rlp = if (inRl) RP else null
            drawGunAt(ox, oy, oz, gy, gp, me.wpn, if (me.flash > 0) fmin(1f, me.flash * 16f) else 0f, false, 0.95f, g.wAtt[me.wpn and 15], g.adsHeld); rlp = null; vmMinD = 0f
            val sl = floatArrayOf(if (me.team != 0) 0.08f else 0.5f, if (me.team != 0) 0.2f else 0.08f, if (me.team != 0) 0.55f else 0.08f); val sk = floatArrayOf(.86f, .68f, .55f)
            val hrl = if (inRl) RP.rz else 0f
            fun hp(px: Float, py: Float, pz: Float, sx: Float, sy: Float, sz: Float, cc: FloatArray) {
                var xx = px; var yy = py; if (hrl != 0f) { val c_ = cosf(hrl); val s_ = sinf(hrl); xx = px * c_ - py * s_; yy = px * s_ + py * c_ }
                r.rot(gy, gp, xx, yy, pz, o3); r.obbRoll = hrl; r.obb(ox + o3[0], oy + o3[1], oz + o3[2], sx * .5f, sy * .5f, sz * .5f, gy, gp, cc[0] * 0.95f, cc[1] * 0.95f, cc[2] * 0.95f, false, false); r.obbRoll = 0f
            }
            val pk = GameData.wkind[me.wpn % g.WEAPON_COUNT] <= 1; val gz = if (pk) 0.04f else 0.06f
            hp(0f, -0.075f, gz, .05f, .05f, .06f, sk); hp(0.015f, -0.12f, gz + 0.12f, .065f, .06f, .28f, sl)
            if (inRl) { hp(RP.hand[0], RP.hand[1], RP.hand[2], .055f, .05f, .07f, sk); hp(RP.hand[0] - 0.03f, RP.hand[1] - 0.05f, RP.hand[2] + 0.13f, .06f, .055f, .18f, sl) }
            else if (!pk) { hp(-0.005f, -0.05f, gd.mz * 0.42f, .055f, .05f, .07f, sk); hp(-0.03f, -0.09f, gd.mz * 0.42f + 0.2f, .06f, .055f, .3f, sl) }
            else hp(-0.03f, -0.075f, gz + 0.01f, .05f, .05f, .06f, sk)
            r.zBias = 0f
        }
    }

    /** Menu background: slowly rotating camera over the selected map. */
    private var mt = 0f
    fun renderMenu() {
        mt += 0.016f; val showMap = if (g.menu == 1) g.menuSel else g.mapIdx
        if (g.w.id != showMap) g.w.load(showMap)
        r.beginFrame(g.w, Cam(0f, 6f, 0f, mt * 0.2f, -0.15f, 1.2f))
        g.camMenuYaw = mt * 0.2f
    }
}
