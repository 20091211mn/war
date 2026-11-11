package com.warzone.kotlin

import com.warzone.core.Game
import com.warzone.core.myJet
import com.warzone.core.jetSpeed
import com.warzone.core.Input
import kotlin.math.*

/** زر شاشة: دائرة (r>0) أو مستطيل (w,h). الإحداثيات بكسلات الشاشة. */
class Btn(val id: Int, val x: Float, val y: Float, val r: Float, val label: String, val w: Float = 0f, val h: Float = 0f) {
    fun hit(px: Float, py: Float): Boolean = if (w > 0f) (px >= x - w / 2 && px <= x + w / 2 && py >= y - h / 2 && py <= y + h / 2)
        else { val dx = px - x; val dy = py - y; val rr = r * 1.25f; dx * dx + dy * dy <= rr * rr }
}

object B {
    const val FIRE = 1; const val ADS = 2; const val JUMP = 3; const val CROUCH = 4; const val RELOAD = 5; const val GREN = 6; const val USE = 7; const val DRONE = 8
    const val C4 = 9; const val DET = 10; const val SEAT = 11; const val PREV = 12; const val NEXT = 13; const val PAUSE = 14
    const val OK = 20; const val BACK = 21; const val SENS_UP = 22; const val SENS_DN = 23
    const val P_OPT = 24; const val P_MUZ = 25; const val P_GRIP = 26; const val P_MAG = 27
    const val ROW_MAP = 30; const val ROW_DIFF = 31; const val ROW_MODE = 32; const val START = 33
    const val WEAPON = 15; const val W0 = 50; const val H0 = 70; const val RM0 = 80; const val RMGO = 86
    const val NB = 96
}

/** يحوّل اللمس إلى مدخلات اللعبة ويحمل تخطيط الأزرار (Kotlin فقط). */
class Controls(private val g: Game) {
    private class Tc { var id = -1; var x = 0f; var y = 0f; var ox = 0f; var oy = 0f; var role = 0; var btn = 0 } // role: 1 حركة 2 نظر 3 زر
    private val tc = Array(12) { Tc() }
    val held = BooleanArray(B.NB)
    var adsL = false; var crL = false
    private var pdx = 0f; private var pdy = 0f; var stX = 0f; var stY = 0f
    var sw = 1f; var sh = 1f
    var btns: List<Btn> = emptyList(); private set
    private var lastMode = -1
    var wheelOpen = false; private var pickReq = -1; @Volatile private var lobbyConf = false
    val u get() = sh / 540f
    val moveStick: Pair<Float, Float>? get() { for (t in tc) if (t.id >= 0 && t.role == 1) return Pair(t.ox, t.oy); return null }

    @Synchronized fun layout() {
        val u = u; val w = sw; val h = sh; val l = ArrayList<Btn>()
        fun A(id: Int, x: Float, y: Float, r: Float, s: String) { l.add(Btn(id, x, y, r * u, s)) }
        fun R(id: Int, x: Float, y: Float, bw: Float, bh: Float, s: String) { l.add(Btn(id, x, y, 0f, s, bw * u, bh * u)) }
        if (g.menu == 1) {
            val cx = w * 0.30f
            R(B.ROW_MAP, cx, h * 0.26f, 330f, 52f, "map"); R(B.ROW_DIFF, cx, h * 0.26f + 70 * u, 330f, 52f, "diff"); R(B.ROW_MODE, cx, h * 0.26f + 140 * u, 330f, 52f, "mode")
            R(B.START, cx, h * 0.26f + 232 * u, 330f, 64f, "start")
        } else if (g.menu == 4) {
            val cx = w * 0.5f; for (k in 0 until 6) R(B.RM0 + k, cx, h * 0.18f + k * 54 * u, 420f, 46f, "row$k")
            R(B.RMGO, cx, h * 0.18f + 6 * 54 * u + 8 * u, 420f, 54f, "go"); R(B.BACK, 90 * u, h - 40 * u, 140f, 44f, "back")
        } else if (g.menu == 2) {
            for (i in 0 until minOf(5, g.ns.hosts.size)) R(B.H0 + i, w * 0.5f, h * 0.30f + i * 62 * u, 420f, 54f, "host")
            R(B.BACK, w * 0.5f, h - 56 * u, 220f, 50f, "back")
        } else if (g.paused) {
            R(B.PAUSE, w * 0.5f, h * 0.30f, 260f, 50f, "resume"); R(B.BACK, w * 0.5f, h * 0.30f + 64 * u, 260f, 50f, "menu")
            R(B.SENS_DN, w * 0.5f - 90 * u, h * 0.30f + 134 * u, 120f, 44f, "-"); R(B.SENS_UP, w * 0.5f + 90 * u, h * 0.30f + 134 * u, 120f, 44f, "+")
            R(B.P_OPT, w * 0.5f - 135 * u, h * 0.30f + 200 * u, 126f, 44f, "optic"); R(B.P_MUZ, w * 0.5f, h * 0.30f + 200 * u, 126f, 44f, "muzzle")
            R(B.P_GRIP, w * 0.5f + 135 * u, h * 0.30f + 200 * u, 126f, 44f, "grip"); R(B.P_MAG, w * 0.5f, h * 0.30f + 252 * u, 126f, 44f, "mag")
        } else if (g.air && g.myJet() >= 0) {
            A(B.FIRE, w - 120 * u, h - 130 * u, 64f, "GUN"); A(B.ADS, w - 236 * u, h - 70 * u, 40f, "BURN"); A(B.JUMP, w - 62 * u, h - 250 * u, 36f, "THR+")
            A(B.CROUCH, w - 196 * u, h - 200 * u, 34f, "THR-"); A(B.USE, w - 46 * u, 150 * u, 28f, "EXIT"); A(B.PAUSE, w / 2 + 150 * u, 30 * u, 20f, "II")
            if (g.over) R(B.OK, w / 2, h - 40 * u, 200f, 52f, "OK")
        } else {
            // يمين: النار والتصويب والقفز والانبطاح والتعبئة — بلا تداخل
            A(B.FIRE, w - 120 * u, h - 130 * u, 64f, "FIRE"); A(B.ADS, w - 236 * u, h - 70 * u, 40f, "ADS"); A(B.JUMP, w - 62 * u, h - 250 * u, 36f, "JUMP")
            A(B.CROUCH, w - 196 * u, h - 200 * u, 34f, "CROUCH"); A(B.RELOAD, w - 290 * u, h - 140 * u, 32f, "R")
            A(B.GREN, w - 46 * u, 150 * u, 25f, "G"); A(B.USE, w - 46 * u, 208 * u, 25f, "CAR")
            // يسار: أدوات
            A(B.C4, 44 * u, 170 * u, 25f, "C4"); A(B.DET, 44 * u, 228 * u, 25f, "DET"); A(B.SEAT, 44 * u, 286 * u, 25f, "SEAT"); A(B.DRONE, 44 * u, 344 * u, 25f, "UAV")
            A(B.PAUSE, w / 2 + 150 * u, 30 * u, 20f, "II")
            R(B.WEAPON, w / 2 + 10 * u, h - 44 * u, 250f, 66f, "weapon")
            if (g.over) R(B.OK, w / 2, h - 40 * u, 200f, 52f, "OK")
            if (wheelOpen) {
                val tw = 196f; val th = 64f; val gx = 10f; val cx0 = w / 2 - (tw + gx) * u; val cy0 = h / 2 - (th + gx) * u - 6 * u
                for (i in 0 until g.WEAPON_COUNT) R(B.W0 + i, cx0 + (i % 3) * (tw + gx) * u, cy0 + (i / 3) * (th + gx) * u, tw, th, "w$i")
            }
        }
        btns = l
    }
    private fun hit(x: Float, y: Float): Btn? { for (i in btns.indices.reversed()) if (btns[i].hit(x, y)) return btns[i]; return null }

    private fun menuTap(b: Btn, x: Float) {
        val dir = if (x < b.x) -1 else 1
        when (b.id) {
            B.ROW_MAP -> g.menuSel = (g.menuSel + dir + g.MAP_COUNT) % g.MAP_COUNT
            B.ROW_DIFF -> g.diff = (g.diff + dir + 3) % 3
            B.ROW_MODE -> g.mode = (g.mode + dir + g.modeCount) % g.modeCount
        }
    }

    @Synchronized fun touchDown(id: Int, x: Float, y: Float) {
        if (tc.any { it.id == id }) return
        val t = tc.firstOrNull { it.id < 0 } ?: return
        layout(); val b = hit(x, y)
        if (wheelOpen) { t.id = id; t.role = 4; t.btn = 0; if (b != null && b.id >= B.W0 && b.id < B.W0 + 20) pickReq = b.id - B.W0; wheelOpen = false; return }
        t.id = id; t.x = x; t.y = y; t.ox = x; t.oy = y; t.btn = 0; t.role = 0
        if (b != null) {
            if (g.menu == 1 && (b.id == B.ROW_MAP || b.id == B.ROW_DIFF || b.id == B.ROW_MODE)) { menuTap(b, x); t.role = 4; return }
            if (g.menu == 4 && b.id >= B.RM0 && b.id < B.RM0 + 6) { g.rmRow = b.id - B.RM0; g.roomAdj(g.rmRow, if (x < b.x) -1 else 1); t.role = 4; return }
            if (g.menu == 2 && b.id >= B.H0 && b.id < B.H0 + 5) { g.ns.hostSel = b.id - B.H0; lobbyConf = true; t.role = 4; return }
            if (b.id == B.WEAPON) { t.role = 4; t.btn = 0; wheelOpen = true; return }
            t.role = 3; t.btn = b.id; held[b.id] = true
            if (b.id == B.ADS) adsL = !adsL else if (b.id == B.CROUCH && !(g.air && g.myJet() >= 0)) crL = !crL
            return
        }
        if (g.menu != 0 || g.paused) return
        val haveMove = tc.any { it.id >= 0 && it.role == 1 }
        if (x < sw * 0.38f && !haveMove) { t.role = 1; stX = 0f; stY = 0f } else t.role = 2
    }
    @Synchronized fun touchMove(id: Int, x: Float, y: Float) {
        val t = tc.firstOrNull { it.id == id } ?: return
        if (t.role == 2 || (t.role == 3 && t.btn == B.FIRE)) { pdx += x - t.x; pdy += y - t.y }
        else if (t.role == 1) { val R = 75f * u; var dx = (x - t.ox) / R; var dy = (y - t.oy) / R; val m = sqrt(dx * dx + dy * dy); if (m > 1) { dx /= m; dy /= m }; stX = dx; stY = -dy }
        t.x = x; t.y = y
    }
    @Synchronized fun touchUp(id: Int) {
        val t = tc.firstOrNull { it.id == id } ?: return
        if (t.role == 3 && t.btn != 0) { if (tc.none { it !== t && it.id >= 0 && it.role == 3 && it.btn == t.btn }) held[t.btn] = false }
        if (t.role == 1) { stX = 0f; stY = 0f }
        t.id = -1; t.role = 0; t.btn = 0
    }
    @Synchronized fun releaseAll() { for (t in tc) { t.id = -1; t.role = 0; t.btn = 0 }; held.fill(false); stX = 0f; stY = 0f }

    /** مدخلات هذا الإطار. */
    @Synchronized fun build(): Input {
        val md = (if (g.menu != 0) 1 else 0) or (if (g.paused) 2 else 0) or (if (g.over) 4 else 0)
        if (md != lastMode) { lastMode = md; releaseAll(); wheelOpen = false }
        val i = Input(); i.pick = pickReq; pickReq = -1
        if (g.menu == 1) { i.confirm = held[B.START]; return i }
        if (g.menu == 4) { i.confirm = held[B.RMGO]; i.back = held[B.BACK]; return i }
        if (g.menu == 2) { i.back = held[B.BACK]; i.confirm = lobbyConf; lobbyConf = false; return i }
        if (g.paused) {
            i.start = held[B.PAUSE]; i.back = held[B.BACK]
            i.mx = if (held[B.SENS_UP]) 1f else if (held[B.SENS_DN]) -1f else 0f
            i.tri = held[B.P_OPT]; i.reload = held[B.P_MUZ]; i.swapP = held[B.P_GRIP]; i.fire = held[B.P_MAG]
            return i
        }
        var sx = stX; var sy = stY; val m = sqrt(sx * sx + sy * sy); val D = 0.14f
        if (m < D) { sx = 0f; sy = 0f } else { val k = min(1f, (m - D) / (1 - D)); sx = sx / m * k; sy = sy / m * k }
        i.mx = sx; i.my = sy; i.sprint = sy > 0.85f && m > 0.95f
        i.fire = held[B.FIRE]; i.ads = adsL; i.jump = held[B.JUMP]; i.crouch = if (g.air && g.myJet() >= 0) held[B.CROUCH] else crL; i.reload = held[B.RELOAD]
        i.grenade = held[B.GREN]; i.tri = held[B.USE]; i.use = held[B.USE]; i.drone = held[B.DRONE]; i.c4 = held[B.C4]; i.det = held[B.DET]; i.seat = held[B.SEAT]
        i.start = held[B.PAUSE]; i.confirm = g.over && held[B.OK]
        return i
    }
    /** دوران النظر المتراكم منذ آخر استدعاء (x يمين، y أعلى) بوحدات اللعبة. */
    @Synchronized fun takeLook(steps: Int, out: FloatArray) {
        if (steps > 0 && g.menu == 0 && !g.paused) { val K = 0.0036f * (540f / sh); out[0] = pdx * K * 60f / steps; out[1] = -pdy * K * 0.85f * 60f / steps }
        else { out[0] = 0f; out[1] = 0f }
        pdx = 0f; pdy = 0f
    }
}
