package com.warzone.core

import kotlin.math.*

object PK {
    const val r = 0.35f; const val h = 1.75f; const val hc = 1.2f; const val eye = 1.62f; const val eyec = 1.1f
    const val step = 0.6f; const val headH = 0.3f; const val walk = 4.6f; const val sprint = 7.0f; const val crouch = 2.4f
    const val jump = 6.2f; const val grav = 18f
}

class Body { @JvmField var x = 0f; @JvmField var y = 0f; @JvmField var z = 0f; @JvmField var vy = 0f; @JvmField var onGround = false }

class World {
    lateinit var m: MapDef
    var id = 0
    var boxes: Array<Box> = emptyArray()
    var ox = 0f; var oz = 0f; var nx = 0; var nz = 0
    var cells: Array<IntArray> = emptyArray()
    var mark = IntArray(0); var stamp = 0

    fun load(idx: Int) {
        id = idx; m = GameData.maps[idx]; boxes = m.boxes
        ox = -m.W / 2 - 3; oz = -m.D / 2 - 3
        nx = ceil((m.W + 6) / CS).toInt() + 1; nz = ceil((m.D + 6) / CS).toInt() + 1
        val tmp = Array(nx * nz) { ArrayList<Int>() }
        mark = IntArray(boxes.size); stamp = 0
        fun ci(v: Float, o: Float, n: Int): Int = max(0, min(n - 1, floor((v - o) / CS).toInt()))
        for (k in boxes.indices) {
            val b = boxes[k]
            val i0 = ci(b.x0 - 0.6f, ox, nx); val i1 = ci(b.x1 + 0.6f, ox, nx); val j0 = ci(b.z0 - 0.6f, oz, nz); val j1 = ci(b.z1 + 0.6f, oz, nz)
            for (j in j0..j1) for (i in i0..i1) tmp[j * nx + i].add(k)
        }
        cells = Array(nx * nz) { tmp[it].toIntArray() }
    }

    fun cellAt(x: Float, z: Float): IntArray {
        val i = max(0, min(nx - 1, floor((x - ox) / CS).toInt())); val j = max(0, min(nz - 1, floor((z - oz) / CS).toInt()))
        return cells[j * nx + i]
    }

    companion object { const val CS = 8 }
}

const val FAR = 1e30f

fun rayBox(ox: Float, oy: Float, oz: Float, dx: Float, dy: Float, dz: Float, x0: Float, y0: Float, z0: Float, x1: Float, y1: Float, z1: Float): Float {
    var tmin = 0f; var tmax = 1e30f
    // x
    if (abs(dx) < 1e-9f) { if (ox < x0 || ox > x1) return FAR }
    else { var t1 = (x0 - ox) / dx; var t2 = (x1 - ox) / dx; if (t1 > t2) { val t = t1; t1 = t2; t2 = t }
        if (t1 > tmin) tmin = t1; if (t2 < tmax) tmax = t2; if (tmin > tmax) return FAR }
    if (abs(dy) < 1e-9f) { if (oy < y0 || oy > y1) return FAR }
    else { var t1 = (y0 - oy) / dy; var t2 = (y1 - oy) / dy; if (t1 > t2) { val t = t1; t1 = t2; t2 = t }
        if (t1 > tmin) tmin = t1; if (t2 < tmax) tmax = t2; if (tmin > tmax) return FAR }
    if (abs(dz) < 1e-9f) { if (oz < z0 || oz > z1) return FAR }
    else { var t1 = (z0 - oz) / dz; var t2 = (z1 - oz) / dz; if (t1 > t2) { val t = t1; t1 = t2; t2 = t }
        if (t1 > tmin) tmin = t1; if (t2 < tmax) tmax = t2; if (tmin > tmax) return FAR }
    return tmin
}

fun raycastMap(w: World, ox: Float, oy: Float, oz: Float, dx: Float, dy: Float, dz: Float, maxT: Float): Float {
    var best = maxT; val CS = World.CS
    var cx = max(0, min(w.nx - 1, floor((ox - w.ox) / CS).toInt())); var cz = max(0, min(w.nz - 1, floor((oz - w.oz) / CS).toInt()))
    val sx = if (dx > 0) 1 else -1; val sz = if (dz > 0) 1 else -1
    val tdx = if (abs(dx) > 1e-9f) CS / abs(dx) else 1e30f; val tdz = if (abs(dz) > 1e-9f) CS / abs(dz) else 1e30f
    var tmx = if (abs(dx) > 1e-9f) (((if (dx > 0) (cx + 1) * CS + w.ox else cx * CS + w.ox)) - ox) / dx else 1e30f
    var tmz = if (abs(dz) > 1e-9f) (((if (dz > 0) (cz + 1) * CS + w.oz else cz * CS + w.oz)) - oz) / dz else 1e30f
    val id = ++w.stamp
    for (g in 0 until 400) {
        for (k in w.cells[cz * w.nx + cx]) {
            if (w.mark[k] == id) continue; w.mark[k] = id; val b = w.boxes[k]
            val t = rayBox(ox, oy, oz, dx, dy, dz, b.x0, b.y0, b.z0, b.x1, b.y1, b.z1); if (t < best) best = t
        }
        val tn = min(tmx, tmz)
        if (best <= tn || tn >= maxT) break
        if (tmx < tmz) { cx += sx; tmx += tdx; if (cx < 0 || cx >= w.nx) break } else { cz += sz; tmz += tdz; if (cz < 0 || cz >= w.nz) break }
    }
    return if (best >= maxT) 1e30f else best
}

class RayHit(@JvmField val t: Float, @JvmField val head: Boolean, @JvmField val ok: Boolean)
private val NOHIT = RayHit(0f, false, false)

fun rayPlayer(px: Float, py: Float, pz: Float, crouch: Boolean, ox: Float, oy: Float, oz: Float, dx: Float, dy: Float, dz: Float, maxT: Float): RayHit {
    val hh = if (crouch) PK.hc else PK.h; val yb = py + hh - PK.headH
    val tb = rayBox(ox, oy, oz, dx, dy, dz, px - 0.34f, py, pz - 0.34f, px + 0.34f, yb, pz + 0.34f)
    val th = rayBox(ox, oy, oz, dx, dy, dz, px - 0.2f, yb, pz - 0.2f, px + 0.2f, py + hh, pz + 0.2f)
    if (th <= tb && th < maxT) return RayHit(th, true, true)
    if (tb < maxT) return RayHit(tb, false, true)
    return NOHIT
}

fun blocked(w: World, x: Float, y: Float, z: Float, hh: Float): Boolean {
    val r = PK.r; val st = PK.step
    for (k in w.cellAt(x, z)) {
        val b = w.boxes[k]
        if (x + r > b.x0 && x - r < b.x1 && z + r > b.z0 && z - r < b.z1 && y + hh > b.y0 && y + st < b.y1) return true
    }
    return false
}

fun groundAt(w: World, x: Float, z: Float, y: Float): Float {
    val r = PK.r; val lim = y + PK.step + 1e-4f; var g = 0f
    for (k in w.cellAt(x, z)) {
        val b = w.boxes[k]
        if (b.y1 <= lim && b.y1 > g && x + r > b.x0 && x - r < b.x1 && z + r > b.z0 && z - r < b.z1) g = b.y1
    }
    return g
}

fun moveBody(w: World, p: Body, vx: Float, vz: Float, dt: Float, jump: Boolean, crouched: Boolean) {
    val hh = if (crouched) PK.hc else PK.h
    val limx = w.m.W / 2 - PK.r - 0.05f; val limz = w.m.D / 2 - PK.r - 0.05f
    var nx = p.x + vx * dt; nx = max(-limx, min(limx, nx)); if (!blocked(w, nx, p.y, p.z, hh)) p.x = nx
    var nz = p.z + vz * dt; nz = max(-limz, min(limz, nz)); if (!blocked(w, p.x, p.y, nz, hh)) p.z = nz
    if (jump && p.onGround) { p.vy = PK.jump; p.onGround = false }
    p.vy -= PK.grav * dt; if (p.vy < -40) p.vy = -40f
    var ny = p.y + p.vy * dt
    if (p.vy > 0) {
        val r = PK.r
        for (k in w.cellAt(p.x, p.z)) {
            val b = w.boxes[k]
            if (b.y0 >= p.y + hh - 0.01f && b.y0 < ny + hh && p.x + r > b.x0 && p.x - r < b.x1 && p.z + r > b.z0 && p.z - r < b.z1) { ny = b.y0 - hh; p.vy = 0f }
        }
        p.onGround = false
    } else {
        val g = groundAt(w, p.x, p.z, p.y); if (ny <= g) { ny = g; p.vy = 0f; p.onGround = true } else p.onGround = false
    }
    p.y = ny
}

// ---- float math helpers matching C's sinf/cosf etc ----
@Suppress("NOTHING_TO_INLINE") inline fun sinf(x: Float): Float = Math.sin(x.toDouble()).toFloat()
@Suppress("NOTHING_TO_INLINE") inline fun cosf(x: Float): Float = Math.cos(x.toDouble()).toFloat()
@Suppress("NOTHING_TO_INLINE") inline fun tanf(x: Float): Float = Math.tan(x.toDouble()).toFloat()
@Suppress("NOTHING_TO_INLINE") inline fun sqrtf(x: Float): Float = Math.sqrt(x.toDouble()).toFloat()
@Suppress("NOTHING_TO_INLINE") inline fun atan2f(y: Float, x: Float): Float = Math.atan2(y.toDouble(), x.toDouble()).toFloat()
@Suppress("NOTHING_TO_INLINE") inline fun asinf(x: Float): Float = Math.asin(x.toDouble()).toFloat()
@Suppress("NOTHING_TO_INLINE") inline fun powf(x: Float, y: Float): Float = Math.pow(x.toDouble(), y.toDouble()).toFloat()
@Suppress("NOTHING_TO_INLINE") inline fun expf(x: Float): Float = Math.exp(x.toDouble()).toFloat()
@Suppress("NOTHING_TO_INLINE") inline fun tanhf(x: Float): Float = Math.tanh(x.toDouble()).toFloat()
@Suppress("NOTHING_TO_INLINE") inline fun hypotf(x: Float, y: Float): Float = Math.hypot(x.toDouble(), y.toDouble()).toFloat()
@Suppress("NOTHING_TO_INLINE") inline fun fabsf(x: Float): Float = if (x < 0f) -x else x
@Suppress("NOTHING_TO_INLINE") inline fun fmin(a: Float, b: Float): Float = if (b < a) b else a
@Suppress("NOTHING_TO_INLINE") inline fun fmax(a: Float, b: Float): Float = if (a < b) b else a
