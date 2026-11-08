package com.warzone.core
import kotlin.test.Test
import kotlin.test.assertTrue
class SmokeTest {
    @Test fun menuThenPlay() {
        val g = Game(); g.modeCount = 1; g.menu = 1; g.init(1280, 600)
        for (m in 0 until g.MAP_COUNT) { g.menuSel = m; g.renderMenu(); assertTrue(g.r.layers[0].n > 0, "menu map $m") }
        val i = Input(); i.confirm = true; g.update(1f / 60, i); assertTrue(g.menu == 0)
        for (k in 0 until 600) { val s = Input(); s.my = 1f; s.fire = k % 20 < 10; s.lx = 0.01f; g.update(1f / 60, s); g.render() }
        assertTrue(g.r.layers[0].n > 1000 && g.r.layers[1].n > 0, "scene tris ${g.r.layers[0].n} vm ${g.r.layers[1].n}")
        for (m in 0 until g.MAP_COUNT) { g.startMatch(m); for (k in 0 until 120) { g.update(1f / 60, Input()); g.render() } }
        for (wi in 0 until g.WEAPON_COUNT) {
            g.startMatch(0); g.menu = 0; g.e[0].wpn = wi; g.e[0].alive = true; for (k in 0 until 30) { g.update(1f / 60, Input()) }; g.e[0].wpn = wi; g.render()
            val L = g.r.layers[1]; val vp = FloatArray(16); g.r.viewProj(vp, 0.05f, 300f)
            var nan = 0; var inV = 0; var mnx = 9f; var mxx = -9f; var mny = 9f; var mxy = -9f; var mnw = 1e9f
            for (i in 0 until L.vertexCount) { val o = i * Renderer.STRIDE; val x = L.a[o]; val y = L.a[o + 1]; val z = L.a[o + 2]
                if (x.isNaN() || y.isNaN() || z.isNaN() || L.a[o + 3].isNaN()) { nan++; continue }
                val cx = vp[0] * x + vp[4] * y + vp[8] * z + vp[12]; val cy = vp[1] * x + vp[5] * y + vp[9] * z + vp[13]; val cw = vp[3] * x + vp[7] * y + vp[11] * z + vp[15]
                if (cw > 0.05f) { inV++; val nx = cx / cw; val ny = cy / cw; mnx = minOf(mnx, nx); mxx = maxOf(mxx, nx); mny = minOf(mny, ny); mxy = maxOf(mxy, ny); mnw = minOf(mnw, cw) } }
            if (nan > 0) { val M = GameData.meshOf(wi); var fi = -1; for (i in 0 until L.vertexCount) { val o = i * Renderer.STRIDE; if (L.a[o].isNaN() || L.a[o + 1].isNaN() || L.a[o + 2].isNaN() || L.a[o + 3].isNaN()) { fi = i; break } }
                val o = fi * Renderer.STRIDE; println("VMNAN wpn=$wi first=$fi vals=" + (0 until Renderer.STRIDE).joinToString(",") { L.a[o + it].toString() } + " mesh=" + (M != null) + " lo=" + M?.lo?.toList() + " rng=" + M?.rng?.toList() + " vx0=" + M?.vx?.get(0) + " nx0=" + M?.nx?.get(0) + " cr0=" + M?.cr?.get(0)) }
            println("VM wpn=$wi alive=${g.e[0].alive} drive=${g.drive} verts=${L.vertexCount} nan=$nan inFront=$inV ndc x[$mnx,$mxx] y[$mny,$mxy] minW=$mnw")
        }
        println("SMOKE OK maps=${g.MAP_COUNT} tris=${g.r.layers[0].triangleCount}")
    }
}
