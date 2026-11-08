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
        println("SMOKE OK maps=${g.MAP_COUNT} tris=${g.r.layers[0].triangleCount}")
    }
}
