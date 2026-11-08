package com.warzone.core

/** Vehicle models (pickup truck + tank) emitted through the scene builder. Port of VH::draw / drawTank from veh.h. */
object VehDraw {
    private val BC = floatArrayOf(0.78f, 0.72f, 0.58f)
    private val PARTS = floatArrayOf(
        0f, 0.5f, 0f, 1.7f, 0.2f, 4.9f, 0.1f, 0.1f, 0.11f, 0f,
        0f, 0.86f, -1.7f, 1.96f, 0.46f, 1.6f, 0.78f, 0.72f, 0.58f, 0f,
        0f, 1.12f, -1.55f, 1.2f, 0.05f, 0.9f, 0.13f, 0.13f, 0.14f, 0f,
        -0.97f, 0.98f, -1.55f, 0.14f, 0.34f, 1.1f, 0.78f, 0.72f, 0.58f, 0f,
        0.97f, 0.98f, -1.55f, 0.14f, 0.34f, 1.1f, 0.78f, 0.72f, 0.58f, 0f,
        0f, 0.74f, -2.52f, 1.9f, 0.38f, 0.1f, 0.13f, 0.13f, 0.14f, 0f,
        0f, 0.5f, -2.58f, 2.0f, 0.16f, 0.14f, 0.45f, 0.46f, 0.48f, 0f,
        -0.68f, 0.94f, -2.53f, 0.26f, 0.2f, 0.06f, 1f, 0.95f, 0.75f, 1f,
        0.68f, 0.94f, -2.53f, 0.26f, 0.2f, 0.06f, 1f, 0.95f, 0.75f, 1f,
        0f, 0.72f, -0.15f, 2.0f, 0.66f, 1.5f, 0.78f, 0.72f, 0.58f, 0f,
        0f, 1.86f, -0.15f, 1.96f, 0.1f, 1.2f, 0.78f, 0.72f, 0.58f, 0f,
        -0.93f, 1.45f, -0.74f, 0.08f, 0.8f, 0.08f, 0.78f, 0.72f, 0.58f, 0f,
        0.93f, 1.45f, -0.74f, 0.08f, 0.8f, 0.08f, 0.78f, 0.72f, 0.58f, 0f,
        -0.93f, 1.45f, 0.4f, 0.08f, 0.8f, 0.1f, 0.78f, 0.72f, 0.58f, 0f,
        0.93f, 1.45f, 0.4f, 0.08f, 0.8f, 0.1f, 0.78f, 0.72f, 0.58f, 0f,
        0f, 1.46f, -0.76f, 1.8f, 0.78f, 0.04f, 0.2f, 0.3f, 0.38f, 0f,
        0f, 1.46f, 0.45f, 1.8f, 0.6f, 0.04f, 0.2f, 0.3f, 0.38f, 0f,
        -0.45f, 0.62f, -0.25f, 0.7f, 0.14f, 0.6f, 0.2f, 0.2f, 0.19f, 0f,
        0.45f, 0.62f, -0.25f, 0.7f, 0.14f, 0.6f, 0.2f, 0.2f, 0.19f, 0f,
        -0.45f, 0.95f, 0.12f, 0.7f, 0.55f, 0.12f, 0.22f, 0.22f, 0.21f, 0f,
        0.45f, 0.95f, 0.12f, 0.7f, 0.55f, 0.12f, 0.22f, 0.22f, 0.21f, 0f,
        0f, 0.98f, -0.72f, 1.7f, 0.12f, 0.2f, 0.13f, 0.13f, 0.14f, 0f,
        0f, 0.88f, 1.55f, 1.9f, 0.08f, 1.9f, 0.13f, 0.13f, 0.14f, 0f,
        -0.96f, 1.08f, 1.55f, 0.08f, 0.42f, 1.9f, 0.78f, 0.72f, 0.58f, 0f,
        0.96f, 1.08f, 1.55f, 0.08f, 0.42f, 1.9f, 0.78f, 0.72f, 0.58f, 0f,
        0f, 1.08f, 2.46f, 1.9f, 0.42f, 0.08f, 0.78f, 0.72f, 0.58f, 0f,
        0f, 1.08f, 0.66f, 1.9f, 0.42f, 0.08f, 0.78f, 0.72f, 0.58f, 0f,
        0f, 0.62f, 1.6f, 1.6f, 0.28f, 1.6f, 0.1f, 0.1f, 0.1f, 0f,
        0f, 0.7f, -2.575f, 1.1f, 0.03f, 0.03f, 0.62f, 0.64f, 0.68f, 0f,
        0f, 0.77f, -2.575f, 1.1f, 0.03f, 0.03f, 0.62f, 0.64f, 0.68f, 0f,
        0f, 0.84f, -2.575f, 1.1f, 0.03f, 0.03f, 0.62f, 0.64f, 0.68f, 0f,
        0f, 0.86f, -2.58f, 0.2f, 0.1f, 0.03f, 0.7f, 0.1f, 0.1f, 0f,
        0f, 0.37f, 2.56f, 2.0f, 0.14f, 0.12f, 0.4f, 0.41f, 0.43f, 0f,
        -0.55f, 1.1f, -1.4f, 0.6f, 0.012f, 0.45f, 0.62f, 0.56f, 0.42f, 0f,
        0.35f, 1.1f, -1.9f, 0.5f, 0.012f, 0.3f, 0.62f, 0.56f, 0.42f, 0f,
        0.1f, 1.45f, -0.3f, 0.7f, 0.012f, 0.5f, 0.62f, 0.56f, 0.42f, 0f,
        0.55f, 1.35f, 0.9f, 0.8f, 0.8f, 0.3f, 0.06f, 0.06f, 0.07f, 0f,
        0f, 1.28f, 1.7f, 0.1f, 0.8f, 0.1f, 0.07f, 0.07f, 0.07f, 0f,
        -0.55f, 1.0f, 2.0f, 0.3f, 0.2f, 0.2f, 0.3f, 0.3f, 0.24f, 0f,
        -0.7f, 0.62f, 2.52f, 0.3f, 0.12f, 0.05f, 0.9f, 0.1f, 0.1f, 1f,
        0.7f, 0.62f, 2.52f, 0.3f, 0.12f, 0.05f, 0.9f, 0.1f, 0.1f, 1f,
        -1.005f, 0.96f, -0.62f, 0.012f, 0.72f, 0.025f, 0.05f, 0.05f, 0.05f, 0f,
        1.005f, 0.96f, -0.62f, 0.012f, 0.72f, 0.025f, 0.05f, 0.05f, 0.05f, 0f,
        -1.005f, 0.96f, 0.32f, 0.012f, 0.72f, 0.025f, 0.05f, 0.05f, 0.05f, 0f,
        1.005f, 0.96f, 0.32f, 0.012f, 0.72f, 0.025f, 0.05f, 0.05f, 0.05f, 0f,
        -1.12f, 1.36f, -0.82f, 0.09f, 0.15f, 0.1f, 0.06f, 0.06f, 0.06f, 0f,
        1.12f, 1.36f, -0.82f, 0.09f, 0.15f, 0.1f, 0.06f, 0.06f, 0.06f, 0f,
        -1.05f, 0.5f, -0.15f, 0.12f, 0.05f, 1.5f, 0.08f, 0.08f, 0.08f, 0f,
        1.05f, 0.5f, -0.15f, 0.12f, 0.05f, 1.5f, 0.08f, 0.08f, 0.08f, 0f)
    private val o3 = FloatArray(3); private val o2 = FloatArray(2); private val pv = FloatArray(3); private val gg = FloatArray(3)

    fun draw(r: Renderer, v: Veh, dist: Float) {
        if (v.type == 1) { drawTank(r, v, dist); return }
        val tint = if (v.dead) 0.16f else 0.55f + 0.45f * fmax(0f, v.hp / VH.HP)
        fun P(lx: Float, y: Float, lz: Float, sx: Float, sy: Float, sz: Float, cr: Float, cg: Float, cb: Float, unlit: Boolean) {
            VH.l2w(v, lx, lz, o2); r.obb(o2[0], y, o2[1], sx * .5f, sy * .5f, sz * .5f, v.h, 0f, cr * (if (unlit) 1f else tint), cg * (if (unlit) 1f else tint), cb * (if (unlit) 1f else tint), true, !unlit)
        }
        if (dist > 60) { P(0f, 0.6f, 0f, 1.9f, 0.5f, 4.9f, BC[0], BC[1], BC[2], false); P(0f, 1.3f, -0.15f, 1.9f, 0.8f, 1.5f, BC[0], BC[1], BC[2], false) }
        else { var i = 0; while (i < PARTS.size) { P(PARTS[i], PARTS[i + 1], PARTS[i + 2], PARTS[i + 3], PARTS[i + 4], PARTS[i + 5], PARTS[i + 6], PARTS[i + 7], PARTS[i + 8], PARTS[i + 9] != 0f && !v.dead); i += 10 } }
        for (i in 0 until 4) { val bl = (v.tires shr i) and 1 != 0; P(VH.WH[i][0], if (bl) 0.3f else 0.38f, VH.WH[i][1], 0.34f, if (bl) 0.55f else 0.76f, if (bl) 0.55f else 0.76f, 0.06f, 0.06f, 0.07f, false) }
        if (dist <= 60) {
            val rel = v.ty - v.h; val pt = v.tp
            fun G(lx: Float, ly: Float, lz: Float, sx: Float, sy: Float, sz: Float, cr: Float, cg: Float, cb: Float) {
                r.rot(rel, pt, lx, ly, lz, o3); VH.l2w(v, o3[0], 1.6f + o3[2], o2)
                r.obb(o2[0], 1.72f + o3[1], o2[1], sx * .5f, sy * .5f, sz * .5f, v.h + rel, pt, cr * tint, cg * tint, cb * tint, true, true)
            }
            if (!v.mgFp) { G(0f, 0f, -0.15f, 0.16f, 0.16f, 0.7f, .1f, .1f, .11f); G(0f, 0.13f, -0.2f, 0.8f, 0.45f, 0.04f, BC[0], BC[1], BC[2]) }
            G(0f, -0.04f, -0.75f, 0.05f, 0.05f, 0.55f, .06f, .06f, .06f)
        }
    }

    private fun drawTank(r: Renderer, v: Veh, dist: Float) {
        val tint = if (v.dead) 0.16f else 0.55f + 0.45f * fmax(0f, v.hp / VH.VPS[1].hp)
        fun P(lx: Float, y: Float, lz: Float, sx: Float, sy: Float, sz: Float, cr: Float, cg: Float, cb: Float, unlit: Boolean, pitch: Float = 0f) {
            VH.l2w(v, lx, lz, o2); r.obb(o2[0], y, o2[1], sx * .5f, sy * .5f, sz * .5f, v.h, pitch, cr * (if (unlit) 1f else tint), cg * (if (unlit) 1f else tint), cb * (if (unlit) 1f else tint), true, !unlit)
        }
        val B = floatArrayOf(.34f, .39f, .25f); val rel = v.ty - v.h
        fun TU(lx: Float, y: Float, lz: Float, sx: Float, sy: Float, sz: Float, cr: Float, cg: Float, cb: Float, unlit: Boolean, pitch: Float = 0f) {
            r.rot(rel, 0f, lx, y, lz, o3); VH.l2w(v, o3[0], -0.1f + o3[2], o2)
            r.obb(o2[0], 1.3f + o3[1], o2[1], sx * .5f, sy * .5f, sz * .5f, v.h + rel, pitch, cr * (if (unlit) 1f else tint), cg * (if (unlit) 1f else tint), cb * (if (unlit) 1f else tint), true, !unlit)
        }
        fun GN(y: Float, z: Float, sx: Float, sy: Float, sz: Float, cr: Float, cg: Float, cb: Float, unlit: Boolean) {
            r.rot(rel, 0f, 0f, 0.35f, -1.7f, pv); r.rot(rel, v.tp, 0f, y, z, gg); VH.l2w(v, pv[0] + gg[0], -0.1f + pv[2] + gg[2], o2)
            r.obb(o2[0], 1.3f + pv[1] + gg[1], o2[1], sx * .5f, sy * .5f, sz * .5f, v.h + rel, v.tp, cr * (if (unlit) 1f else tint), cg * (if (unlit) 1f else tint), cb * (if (unlit) 1f else tint), true, !unlit)
        }
        P(0f, 0.78f, 0f, 2.55f, 0.62f, 6.0f, B[0], B[1], B[2], false)
        if (dist > 60) { TU(0f, 0.35f, 0.1f, 2.1f, 0.7f, 2.6f, B[0], B[1], B[2], false); GN(0f, -1.7f, 0.22f, 0.22f, 3.6f, .11f, .12f, .1f, false); return }
        for (sx in intArrayOf(-1, 1)) {
            P(sx * 1.45f, 0.5f, 0f, 0.62f, 0.85f, 6.5f, .07f, .07f, .07f, false); P(sx * 1.45f, 0.96f, 0f, 0.7f, 0.06f, 6.4f, .11f, .12f, .1f, false)
            for (i in 0 until 6) { val z = -2.5f + i * 1.0f; P(sx * 1.5f, 0.4f, z, 0.42f, 0.74f, 0.74f, .2f, .21f, .19f, false) }
        }
        P(0f, 1.0f, -2.55f, 2.5f, 0.3f, 1.5f, B[0], B[1], B[2], false, -0.5f); P(0f, 1.12f, 2.7f, 2.4f, 0.26f, 1.2f, B[0], B[1], B[2], false)
        for (i in 0 until 4) P(-0.7f + i * 0.47f, 1.28f, 2.7f, 0.3f, 0.03f, 0.9f, .11f, .12f, .1f, false)
        P(0.75f, 1.2f, -2.2f, 0.55f, 0.24f, 0.55f, .11f, .12f, .1f, false); P(0f, 0.7f, -3.08f, 2.4f, 0.4f, 0.08f, .11f, .12f, .1f, false)
        P(-0.95f, 0.88f, -3.1f, 0.32f, 0.18f, 0.06f, 1f, .95f, .75f, !v.dead); P(0.95f, 0.88f, -3.1f, 0.32f, 0.18f, 0.06f, 1f, .95f, .75f, !v.dead)
        P(-1.0f, 0.72f, 3.04f, 0.3f, 0.14f, 0.05f, .9f, .1f, .1f, !v.dead); P(1.0f, 0.72f, 3.04f, 0.3f, 0.14f, 0.05f, .9f, .1f, .1f, !v.dead)
        TU(0f, 0.32f, 0.15f, 2.3f, 0.62f, 2.9f, B[0], B[1], B[2], false); TU(0f, 0.3f, -1.4f, 1.7f, 0.54f, 0.7f, B[0], B[1], B[2], false, -0.3f); TU(0f, 0.22f, 1.6f, 1.8f, 0.4f, 0.5f, B[0], B[1], B[2], false)
        TU(0.55f, 0.72f, 0.5f, 0.75f, 0.12f, 0.75f, .11f, .12f, .1f, false); TU(-1.18f, 0.35f, 0.2f, 0.08f, 0.3f, 1.4f, .45f, .47f, .44f, false); TU(1.18f, 0.35f, 0.2f, 0.08f, 0.3f, 1.4f, .45f, .47f, .44f, false)
        val kk = v.kick * 0.45f
        TU(0f, 0.35f, -1.65f, 0.8f, 0.7f, 0.3f, .11f, .12f, .1f, false)
        GN(0f, -0.0f - 1.4f + kk, 0.24f, 0.24f, 3.2f, .11f, .12f, .1f, false)
        GN(0f, -3.15f + kk, 0.36f, 0.36f, 0.34f, .07f, .07f, .07f, false)
        if (v.kick > 0.6f) GN(0f, -3.6f, 0.7f, 0.7f, 0.8f, 1f, .8f, .3f, true)
    }
}
