package com.warzone.core

import kotlin.math.*

class VP(@JvmField val hp: Float, @JvmField val L: Float, @JvmField val hw: Float, @JvmField val hl: Float, @JvmField val maxFwd: Float, @JvmField val maxRev: Float,
         @JvmField val accel: Float, @JvmField val brake: Float, @JvmField val maxSteer: Float, @JvmField val crashMin: Float, @JvmField val armor: Float,
         @JvmField val nseats: Int, @JvmField val tracked: Boolean)

class Veh {
    @JvmField var occ = 0; @JvmField var type = 0; @JvmField var kick = 0f; @JvmField var ty = 0f; @JvmField var tp = 0f; @JvmField var cannonT = 0f
    @JvmField var mgFp = false
    @JvmField var x = 0f; @JvmField var z = 0f; @JvmField var h = 0f; @JvmField var vx = 0f; @JvmField var vz = 0f; @JvmField var steer = 0f; @JvmField var speed = 0f
    @JvmField var hp = 450f
    @JvmField val tireHp = floatArrayOf(30f, 30f, 30f, 30f)
    @JvmField var tires = 0; @JvmField var driver = -1; @JvmField var team = -1
    @JvmField var dead = false; @JvmField var burn = false
    @JvmField var respawnT = 0f; @JvmField var sx = 0f; @JvmField var sz = 0f; @JvmField var sh = 0f; @JvmField var smoke = 0f; @JvmField var crashT = 0f
    @JvmField var tx = 0f; @JvmField var tz = 0f; @JvmField var th = 0f

    fun copyFrom(o: Veh) {
        occ = o.occ; type = o.type; kick = o.kick; ty = o.ty; tp = o.tp; cannonT = o.cannonT; mgFp = o.mgFp
        x = o.x; z = o.z; h = o.h; vx = o.vx; vz = o.vz; steer = o.steer; speed = o.speed; hp = o.hp
        for (i in 0 until 4) tireHp[i] = o.tireHp[i]
        tires = o.tires; driver = o.driver; team = o.team; dead = o.dead; burn = o.burn
        respawnT = o.respawnT; sx = o.sx; sz = o.sz; sh = o.sh; smoke = o.smoke; crashT = o.crashT; tx = o.tx; tz = o.tz; th = o.th
    }
}

class BB(@JvmField val x0: Float, @JvmField val y0: Float, @JvmField val z0: Float, @JvmField val x1: Float, @JvmField val y1: Float, @JvmField val z1: Float, @JvmField val part: Int)

object VH {
    const val HP = 450f; const val MAXREV = 7f
    @JvmField val VPS = arrayOf(
        VP(450f, 3.0f, 1.0f, 2.5f, 26f, 7f, 7.5f, 16f, 0.52f, 4f, 1f, 5, false),
        VP(1100f, 3.6f, 1.5f, 3.1f, 11f, 5f, 3.8f, 12f, 1f, 6f, 0.14f, 2, true))

    @JvmStatic fun l2w(v: Veh, lx: Float, lz: Float, o: FloatArray) { val c = cosf(v.h); val s = sinf(v.h); o[0] = v.x + c * lx + s * lz; o[1] = v.z - s * lx + c * lz }
    @JvmStatic fun w2l(v: Veh, wx: Float, wz: Float, o: FloatArray) { val c = cosf(v.h); val s = sinf(v.h); val dx = wx - v.x; val dz = wz - v.z; o[0] = c * dx - s * dz; o[1] = s * dx + c * dz }
    @JvmStatic fun vp(v: Veh): VP = VPS[if (v.type == 1) 1 else 0]
    @JvmStatic fun tiresBlown(m: Int) = (m and 1) + ((m shr 1) and 1) + ((m shr 2) and 1) + ((m shr 3) and 1)

    private fun bb(vararg a: Float): Array<BB> = Array(a.size / 7) { BB(a[it * 7], a[it * 7 + 1], a[it * 7 + 2], a[it * 7 + 3], a[it * 7 + 4], a[it * 7 + 5], a[it * 7 + 6].toInt()) }
    @JvmField val BODY = bb(
        -1f, 0.3f, -2.5f, 1f, 1.1f, -0.9f, 0f, -1f, 0.3f, -0.9f, 1f, 1.05f, 0.6f, 0f, -0.95f, 1.8f, -0.7f, 0.95f, 1.9f, 0.4f, 0f, -1f, 0.3f, 0.6f, 1f, 0.85f, 2.5f, 1f,
        -1f, 0.85f, 0.6f, -0.9f, 1.25f, 2.5f, 0f, 0.9f, 0.85f, 0.6f, 1f, 1.25f, 2.5f, 0f, -1f, 0.85f, 2.4f, 1f, 1.25f, 2.5f, 0f, -1f, 0.85f, 0.6f, 1f, 1.25f, 0.7f, 0f)
    @JvmField val BODY_T = bb(
        -1.45f, 0.35f, -3.1f, 1.45f, 1.3f, 3.1f, 0f, -1.0f, 1.3f, -1.5f, 1.0f, 1.95f, 1.3f, 0f, -0.12f, 1.5f, -4.8f, 0.12f, 1.8f, -1.5f, 0f,
        -1.2f, 1.0f, 1.4f, 1.2f, 1.35f, 3.0f, 1f, -1.62f, 0.1f, -3.0f, -1.2f, 0.9f, 3.0f, 0f, 1.2f, 0.1f, -3.0f, 1.62f, 0.9f, 3.0f, 0f)
    @JvmField val WH = arrayOf(floatArrayOf(-0.95f, -1.55f), floatArrayOf(0.95f, -1.55f), floatArrayOf(-0.95f, 1.5f), floatArrayOf(0.95f, 1.5f))

    private val tl = FloatArray(2)
    /** ray vs vehicle: returns t or 1e30, and part in partOut[0] */
    @JvmStatic fun rayVeh(v: Veh, ox: Float, oy: Float, oz: Float, dx: Float, dy: Float, dz: Float, maxT: Float, partOut: IntArray): Float {
        val c = cosf(v.h); val s = sinf(v.h); val px = ox - v.x; val pz = oz - v.z
        val lox = c * px - s * pz; val loz = s * px + c * pz; val ldx = c * dx - s * dz; val ldz = s * dx + c * dz
        var best = maxT; var part = -1
        if (v.type == 1) {
            for (b in BODY_T) { val t = rayBox(lox, oy, loz, ldx, dy, ldz, b.x0, b.y0, b.z0, b.x1, b.y1, b.z1); if (t < best) { best = t; part = b.part } }
            partOut[0] = part
            return if (part >= 0) best else 1e30f
        }
        for (b in BODY) { val t = rayBox(lox, oy, loz, ldx, dy, ldz, b.x0, b.y0, b.z0, b.x1, b.y1, b.z1); if (t < best) { best = t; part = b.part } }
        for (i in 0 until 4) {
            val t = rayBox(lox, oy, loz, ldx, dy, ldz, WH[i][0] - 0.2f, 0f, WH[i][1] - 0.4f, WH[i][0] + 0.2f, 0.76f, WH[i][1] + 0.4f)
            if (t < best + 0.02f && t < maxT) { best = fmin(best, t); part = 10 + i }
        }
        partOut[0] = part
        return if (part >= 0) best else 1e30f
    }

    @JvmStatic fun circleHit(w: World, cx: Float, cz: Float, r: Float, minH: Float = 0.55f): Boolean {
        val lx = w.m.W / 2 - r; val lz = w.m.D / 2 - r; if (cx > lx || cx < -lx || cz > lz || cz < -lz) return true
        for (b in w.boxes) {
            if (b.y1 <= minH || b.y0 >= 1.6f) continue
            if (cx < b.x0 - r || cx > b.x1 + r || cz < b.z0 - r || cz > b.z1 + r) continue
            val nx = if (cx < b.x0) b.x0 else if (cx > b.x1) b.x1 else cx; val nz = if (cz < b.z0) b.z0 else if (cz > b.z1) b.z1 else cz
            val dx = cx - nx; val dz = cz - nz; if (dx * dx + dz * dz < r * r) return true
        }
        return false
    }

    private val SS = FloatArray(5)
    @JvmStatic fun collides(w: World, x: Float, z: Float, h: Float, all: List<Veh>, self: Veh?): Boolean {
        val P = if (self != null) vp(self) else VPS[0]; val bx = sinf(h); val bz = cosf(h); val e = P.hl - 0.3f
        SS[0] = -e; SS[1] = -e / 2; SS[2] = 0f; SS[3] = e / 2; SS[4] = e
        for (q in 0 until 5) {
            val k = SS[q]; val cx = x + bx * k; val cz = z + bz * k
            if (circleHit(w, cx, cz, P.hw, if (P.tracked) 1.15f else 0.55f)) return true
            for (o in all) {
                if (o === self || o.dead) continue
                val Q = vp(o); val ob = sinf(o.h); val oc = cosf(o.h); val lim = (P.hw + Q.hw) * (P.hw + Q.hw) * 0.82f
                for (jj in 0 until 3) {
                    val j = if (jj == 0) -Q.hl * 0.64f else if (jj == 1) 0f else Q.hl * 0.64f
                    val dx = cx - (o.x + ob * j); val dz = cz - (o.z + oc * j); if (dx * dx + dz * dz < lim) return true
                }
            }
        }
        return false
    }

    /** pushes an entity out of the body; io[0]=x io[1]=z in/out. */
    @JvmStatic fun pushOut(v: Veh, io: FloatArray, py: Float): Boolean {
        if (py > 1.85f || py < -1) return false
        w2l(v, io[0], io[1], tl); val lx = tl[0]; val lz = tl[1]; val P = vp(v); val hx = P.hw + 0.38f; val hz = P.hl + 0.38f
        if (fabsf(lx) >= hx || fabsf(lz) >= hz) return false
        val a = hx - fabsf(lx); val b = hz - fabsf(lz); var nx = lx; var nz = lz
        if (a < b) nx = (if (lx < 0) -1f else 1f) * hx else nz = (if (lz < 0) -1f else 1f) * hz
        l2w(v, nx, nz, io); return true
    }

    private val D4 = arrayOf(intArrayOf(1, 0), intArrayOf(-1, 0), intArrayOf(0, 1), intArrayOf(0, -1))
    @JvmStatic fun step(w: World, v: Veh, all: List<Veh>, thr: Float, steer: Float, hb: Boolean, dt: Float): Float {
        val fx = -sinf(v.h); val fz = -cosf(v.h); val rx = cosf(v.h); val rz = -sinf(v.h)
        var vf = v.vx * fx + v.vz * fz; var vl = v.vx * rx + v.vz * rz; val P = vp(v); val T = P.tracked; val nb = if (T) 0 else tiresBlown(v.tires)
        val maxFwd = fmax(5f, P.maxFwd * (1 - 0.17f * nb)); val accel = P.accel * (1 - 0.1f * nb)
        if (thr > 0) { if (vf < -0.5f) vf += P.brake * dt else vf += accel * thr * dt * fmax(0f, 1 - vf / maxFwd) }
        else if (thr < 0) { if (vf > 0.5f) vf -= P.brake * dt else vf -= accel * 0.6f * -thr * dt * fmax(0f, 1 - (-vf) / MAXREV) }
        else { val roll = (1.5f + 0.012f * vf * vf) * dt; vf -= (if (vf > 0) 1f else -1f) * fmin(fabsf(vf), roll) }
        if (vf > maxFwd) vf = fmax(maxFwd, vf - 12 * dt)
        if (hb) vf -= (if (vf > 0) 1f else -1f) * fmin(fabsf(vf), 9 * dt)
        val grip = if (T) 14f else (if (hb) 1.4f else 7f) * (1 - 0.12f * nb); vl *= expf(-grip * dt)
        var yaw: Float
        if (T) { v.steer += (steer * 0.5f - v.steer) * fmin(1f, dt * 8); yaw = steer * 0.85f * (1 - fmin(0.5f, fabsf(vf) / 24)) }
        else {
            val maxS = P.maxSteer * (1 - fmin(0.72f, fabsf(vf) / 38)); v.steer += (steer * maxS - v.steer) * fmin(1f, dt * 6)
            yaw = vf / P.L * tanf(v.steer) * (if (hb) 1.35f else 1f)
            val pull = (((v.tires shr 1 and 1) + (v.tires shr 3 and 1)) - ((v.tires and 1) + (v.tires shr 2 and 1))).toFloat()
            yaw += pull * 0.09f * fmin(1f, fabsf(vf) / 10) * (if (vf < 0) -1f else 1f)
        }
        val nh = v.h - yaw * dt; var crash = 0f
        if (!collides(w, v.x, v.z, nh, all, v)) v.h = nh else if (fabsf(vf) > 1) crash = fmax(crash, fabsf(vf) * 0.4f)
        val nfx = -sinf(v.h); val nfz = -cosf(v.h); val nrx = cosf(v.h); val nrz = -sinf(v.h); var nvx = nfx * vf + nrx * vl; var nvz = nfz * vf + nrz * vl
        val nx = v.x + nvx * dt; val nz = v.z + nvz * dt; var hit = false
        if (!collides(w, nx, v.z, v.h, all, v)) v.x = nx else { hit = true; nvx *= -0.2f }
        if (!collides(w, v.x, nz, v.h, all, v)) v.z = nz else { hit = true; nvz *= -0.2f }
        if (hit) { crash = fmax(crash, fabsf(vf)); nvx *= 0.6f; nvz *= 0.6f }
        v.vx = nvx; v.vz = nvz; v.speed = nvx * nfx + nvz * nfz
        if (collides(w, v.x, v.z, v.h, all, v)) {
            var done = false
            var k = 1
            while (k <= 12 && !done) { for (d in D4) { if (!collides(w, v.x + d[0] * k * 0.25f, v.z + d[1] * k * 0.25f, v.h, all, v)) { v.x += d[0] * k * 0.25f; v.z += d[1] * k * 0.25f; done = true; break } }; k++ }
        }
        return crash
    }
}
