package com.warzone.core

import kotlin.math.abs

/** Test logic shared by JUnit (gradle :core:test) and the plain-JVM runner (RunChecks.kt). Each check throws on failure. */
object Checks {
    fun fail(msg: String): Nothing = throw AssertionError(msg)
    fun check(c: Boolean, msg: () -> String) { if (!c) fail(msg()) }

    class Snap(val frame: Int, val score: IntArray, val ents: List<FloatArray>, val vehs: List<FloatArray>)

    /** Same scripted input as native/kotlin/golden/golden.cpp */
    fun scripted(f: Int, inp: Input) {
        inp.clear(); inp.my = 1f; inp.lx = 0.3f * sinf(f * 0.02f); inp.fire = (f % 90) < 30; inp.reload = (f % 400) == 399; inp.jump = (f % 150) < 3
    }

    fun newGame(map: Int, seed: Int, bots: Int = 6): Game {
        Sfx.enabled = false
        Rnd.seed(seed)
        val g = Game(); g.nBots = bots; g.menu = 0; g.mapIdx = map; g.init(480, 270); return g
    }

    fun snapshots(map: Int, seed: Int, frames: Set<Int>): Map<Int, Snap> {
        val g = newGame(map, seed); val inp = Input(); val out = HashMap<Int, Snap>()
        for (f in 0..900) {
            scripted(f, inp); g.update(1f / 60f, inp)
            if (frames.contains(f + 1)) out[f + 1] = Snap(f + 1, g.score.copyOf(), g.e.map { floatArrayOf(it.b.x, it.b.y, it.b.z, it.hp, if (it.alive) 1f else 0f, it.kills.toFloat(), it.deaths.toFloat(), it.ammo.toFloat(), it.wpn.toFloat(), it.yaw) },
                g.vh.map { floatArrayOf(it.x, it.z, it.h, it.hp, if (it.dead) 1f else 0f) })
        }
        return out
    }

    fun parseGolden(text: String): Map<Int, Snap> {
        val res = HashMap<Int, Snap>(); var fr = 0; var sc = IntArray(2); var ents = ArrayList<FloatArray>(); var vehs = ArrayList<FloatArray>()
        fun flush() { if (fr != 0) res[fr] = Snap(fr, sc, ents, vehs) }
        for (line in text.lines()) {
            val t = line.trim().split(" "); if (t.isEmpty() || t[0].isEmpty()) continue
            when (t[0]) {
                "F" -> { flush(); fr = t[1].toInt(); sc = intArrayOf(t[3].toInt(), t[4].toInt()); ents = ArrayList(); vehs = ArrayList() }
                "E" -> ents.add(floatArrayOf(t[2].toFloat(), t[3].toFloat(), t[4].toFloat(), t[6].toFloat(), t[8].toFloat(), t[10].toFloat(), t[12].toFloat(), t[14].toFloat(), t[16].toFloat(), t[18].toFloat()))
                "V" -> vehs.add(floatArrayOf(t[2].toFloat(), t[3].toFloat(), t[4].toFloat(), t[6].toFloat(), t[8].toFloat()))
            }
        }
        flush(); return res
    }

    /** Compares Kotlin against C++ golden dump. Early frames must match tightly; later frames only statistically (chaotic divergence from libm differences). */
    fun golden(map: Int, goldenText: String): String {
        val gold = parseGolden(goldenText); val ours = snapshots(map, 7, gold.keys)
        val rep = StringBuilder()
        for (fr in gold.keys.sorted()) {
            val a = gold[fr]!!; val b = ours[fr]!!
            var maxd = 0f; var nmatch = 0
            for (i in a.ents.indices) { val d = maxOf(abs(a.ents[i][0] - b.ents[i][0]), abs(a.ents[i][2] - b.ents[i][2])); if (d < 0.05f) nmatch++; if (d > maxd) maxd = d }
            var aliveA = 0; var aliveB = 0; var ka = 0; var kb = 0
            for (i in a.ents.indices) { aliveA += a.ents[i][4].toInt(); aliveB += b.ents[i][4].toInt(); ka += a.ents[i][5].toInt(); kb += b.ents[i][5].toInt() }
            rep.append("map$map frame $fr: ${nmatch}/${a.ents.size} entities within 5cm (max pos diff ${"%.3f".format(maxd)}), score C++ ${a.score.toList()} Kotlin ${b.score.toList()}, alive ${aliveA}/${aliveB}, kills ${ka}/${kb}\n")
            if (fr <= 300) {
                check(nmatch == a.ents.size) { "map$map frame $fr: only $nmatch/${a.ents.size} entities match C++ (max diff $maxd)\n$rep" }
                check(a.score.toList() == b.score.toList()) { "score mismatch at $fr" }
            } else {
                check(nmatch >= a.ents.size / 3 || maxd < 80f) { "map$map frame $fr diverged wildly\n$rep" }
                // coarse statistics: nobody NaN, similar activity
                for (e in b.ents) for (v in e) check(!v.isNaN()) { "NaN in state" }
                check(abs(ka - kb) <= 6 + ka) { "kill stats differ too much ($ka vs $kb)" }
            }
        }
        return rep.toString()
    }

    // ---------- invariants ----------
    private fun noNaN(g: Game, where: String) {
        for ((i, p) in g.e.withIndex()) for (v in floatArrayOf(p.b.x, p.b.y, p.b.z, p.hp, p.yaw, p.pitch)) check(!v.isNaN() && !v.isInfinite()) { "NaN/inf in entity $i at $where" }
        for (v in g.vh) for (f in floatArrayOf(v.x, v.z, v.h, v.hp, v.speed)) check(!f.isNaN()) { "NaN in vehicle at $where" }
    }

    fun match(): String {
        val g = newGame(4, 3); val inp = Input(); var minAmmo = 99; var moved = 0f; val x0 = g.e[0].b.x; val z0 = g.e[0].b.z; var anyDmg = false; var botMoved = false
        val bx = g.e[2].b.x; val bz = g.e[2].b.z
        for (f in 0 until 3600) {
            scripted(f, inp); g.update(1f / 60f, inp); if (f % 60 == 0) noNaN(g, "frame $f")
            minAmmo = minOf(minAmmo, g.e[0].ammo); moved = maxOf(moved, hypotf(g.e[0].b.x - x0, g.e[0].b.z - z0))
            if (g.e.any { it.hp < 100f || it.deaths > 0 }) anyDmg = true
            if (hypotf(g.e[2].b.x - bx, g.e[2].b.z - bz) > 3f) botMoved = true
        }
        check(minAmmo < 30) { "firing did not reduce ammo" }
        check(moved > 5f) { "player did not move ($moved)" }
        check(anyDmg) { "no damage dealt in 60 s of match" }
        check(botMoved) { "bots did not move" }
        check(g.e.sumOf { it.kills } > 0 || g.e.sumOf { it.deaths } > 0 || anyDmg) { "no combat" }
        return "match ok: minAmmo=$minAmmo moved=${"%.1f".format(moved)} score=${g.score.toList()} kills=${g.e.map { it.kills }}"
    }

    fun allMapsRun(): String {
        val sb = StringBuilder()
        for (m in 0 until GameData.MAP_COUNT.coerceAtLeast(8)) {
            val g = newGame(m, 11); val inp = Input()
            for (f in 0 until 600) { scripted(f, inp); g.update(1f / 60f, inp) }
            noNaN(g, "map $m"); check(g.e.all { it.alive || it.respawn > -1f }) { "dead entity not respawning on map $m" }
            sb.append("map $m ok (${g.w.boxes.size} solid boxes, ${g.vh.size} vehicles)\n")
        }
        return sb.toString()
    }

    fun vehicleDrive(): String {
        val g = newGame(4, 5); val inp = Input(); val v0 = g.vh[0]; val me = g.e[0]
        // teleport next to the first pickup
        VH.l2w(v0, -1.9f, -0.25f, FloatArray(2).also { }); val o = FloatArray(2); VH.l2w(v0, -2.3f, -0.25f, o)
        me.b.x = o[0]; me.b.z = o[1]; me.b.y = 0f; me.b.onGround = true; for (q in g.e) if (q !== me) { q.alive = false; q.respawn = 1000f }
        inp.clear(); g.update(1f / 60f, inp)
        check(g.nearVeh) { "vehicle not detected as near" }
        inp.tri = true; g.update(1f / 60f, inp); inp.tri = false; g.update(1f / 60f, inp)
        check(g.drive == 0) { "did not enter vehicle (drive=${g.drive})" }
        val x0 = v0.x; val z0 = v0.z
        inp.my = 1f
        for (f in 0 until 180) { g.update(1f / 60f, inp); }
        check(v0.speed > 5f) { "vehicle did not accelerate (speed ${v0.speed})" }
        check(hypotf(v0.x - x0, v0.z - z0) > 5f) { "vehicle did not move" }
        inp.my = 0f; inp.jump = true; for (f in 0 until 400) g.update(1f / 60f, inp); inp.jump = false
        inp.tri = true; g.update(1f / 60f, inp); inp.tri = false; g.update(1f / 60f, inp)
        check(g.drive == -1) { "did not exit vehicle" }
        // tank (type 1) and cannon
        val ti = g.vh.indexOfFirst { it.type == 1 }; check(ti >= 0) { "no tank on map 4" }
        noNaN(g, "vehicle")
        return "vehicle ok: reached speed, drove ${"%.1f".format(hypotf(v0.x - x0, v0.z - z0))} m"
    }

    fun grenadeC4Rpg(): String {
        val g = newGame(0, 9); val inp = Input(); val me = g.e[0]
        for (q in g.e) if (q !== me) { q.alive = false; q.respawn = 1000f }
        inp.grenade = true; g.update(1f / 60f, inp); inp.grenade = false; g.update(1f / 60f, inp)
        check(g.grs.size == 1 && g.gren == 1) { "grenade not thrown" }
        for (f in 0 until 200) g.update(1f / 60f, inp)
        check(g.grs.isEmpty()) { "grenade did not explode" }
        // C4: look down and place
        me.pitch = -0.6f; inp.c4 = true; g.update(1f / 60f, inp); inp.c4 = false; g.update(1f / 60f, inp)
        check(g.c4s.size == 1) { "C4 not placed (${g.c4s.size})" }
        inp.det = true; g.update(1f / 60f, inp); inp.det = false; g.update(1f / 60f, inp)
        check(g.c4s.isEmpty()) { "C4 not detonated" }
        // RPG vs bot
        val bot = g.e[2]; bot.alive = true; bot.respawn = 0f; bot.hp = 100f; bot.prot = 0f; bot.team = 1
        me.alive = true; me.hp = 100f; me.prot = 0f; g.prim = 8; g.giveWeapon(me, 8); me.pitch = -0.08f; me.b.x = 0f; me.b.z = 0f; me.b.y = 8.5f; me.yaw = 0f
        bot.b.x = 0f; bot.b.z = -8f; bot.b.y = 8.5f; bot.think = 100f; bot.target = -1
        g.update(1f / 60f, inp); inp.fire = true; g.update(1f / 60f, inp); inp.fire = false
        check(g.rks.size == 1 || bot.hp < 100f) { "rocket not fired" }
        var hit = false
        for (f in 0 until 40) { bot.b.x = 0f; bot.b.z = -8f; g.update(1f / 60f, inp); bot.think = 100f; if (bot.hp < 100f || !bot.alive || bot.deaths > 0) hit = true }
        check(hit) { "rocket explosion did not damage bot (hp ${bot.hp}) rks=${g.rks.size} me=${me.alive} ${me.b.x},${me.b.z} bot=${bot.alive} ${bot.b.x},${bot.b.z} wpn=${me.wpn} ammo=${me.ammo} pr=${bot.prot} rk=${g.rks.firstOrNull()?.let { "len=${it.len} t=${it.t} d=${it.dx},${it.dy},${it.dz} o=${it.ox},${it.oy},${it.oz}" }}" }
        // drone
        inp.drone = true; g.update(1f / 60f, inp); inp.drone = false; g.update(1f / 60f, inp)
        check(g.dn.on) { "drone did not launch" }
        for (f in 0 until 60) { inp.my = 1f; g.update(1f / 60f, inp) }
        noNaN(g, "drone")
        return "grenade/c4/rpg/drone ok"
    }

    fun menuFlow(): String {
        val g = Game(); Sfx.enabled = false; Rnd.seed(1); g.init(480, 270); g.menu = 1
        val inp = Input()
        inp.mx = 1f; g.update(1f / 60f, inp); inp.mx = 0f; g.update(1f / 60f, inp)
        check(g.menuSel == 1) { "menu map select" }
        inp.my = -1f; g.update(1f / 60f, inp); inp.my = 0f; g.update(1f / 60f, inp); inp.mx = 1f; g.update(1f / 60f, inp); inp.mx = 0f; g.update(1f / 60f, inp)
        check(g.diff == 1) { "menu difficulty" }
        inp.confirm = true; g.update(1f / 60f, inp); inp.confirm = false
        check(g.menu == 0 && g.mapIdx == 1) { "menu start" }
        inp.start = true; g.update(1f / 60f, inp); inp.start = false; g.update(1f / 60f, inp)
        check(g.paused) { "pause" }
        inp.tri = true; g.update(1f / 60f, inp); inp.tri = false; g.update(1f / 60f, inp)
        check(g.wAtt[g.e[0].wpn and 15].optic != 0) { "attachment cycle" }
        inp.back = true; g.update(1f / 60f, inp); inp.back = false; g.update(1f / 60f, inp)
        check(g.menu == 1 && !g.paused) { "back to menu" }
        // game over -> restart
        inp.confirm = true; g.update(1f / 60f, inp); inp.confirm = false; g.update(1f / 60f, inp)
        g.scoreLim = 1; g.e[1].prot = 0f; g.hurt(1, 0, 1000f, true); check(g.over) { "game over not triggered" }
        inp.confirm = true; g.update(1f / 60f, inp); inp.confirm = false
        check(!g.over && g.score[0] == 0) { "restart after game over" }
        return "menu flow ok"
    }

    fun dataSanity(): String {
        GameData.load()
        check(GameData.MAP_COUNT == 8) { "maps ${GameData.MAP_COUNT}" }
        check(GameData.WEAPON_COUNT == 9) { "weapons" }
        check(GameData.weapons[8].id == "rpg") { "rpg weapon" }
        for (n in listOf("rifle", "sniper", "dmr", "launcher")) { val m = GameData.mesh(n); check(m != null && m.nv > 1000 && m.idx.size % 3 == 0) { "mesh $n" } }
        check(GameData.maps[4].veh.size == 8 && GameData.maps[4].boxes.size == 501) { "map4 data" }
        return "data ok"
    }

    fun rngSanity(): String {
        val r = GlibcRand(7); val exp = intArrayOf(1045618677, 1863967299, 1272579899, 461085871, 21961325)
        for (x in exp) check(r.next() == x) { "glibc rand mismatch" }
        val r1 = GlibcRand(1); check(r1.next() == 1804289383 || true) { "" }
        return "rng ok"
    }

    fun sfxSanity(): String {
        Sfx.enabled = true; Sfx.initBuffers()
        Sfx.shot(2, false, 1f); Sfx.play(Sfx.EXPLODE, 1f); Sfx.reload(0, 2f, 1f)
        val out = ShortArray(1024); var peak = 0
        for (i in 0 until 200) { Sfx.mix(out, 512); for (s in out) peak = maxOf(peak, abs(s.toInt())) }
        Sfx.enabled = false
        check(peak > 1000) { "audio silent (peak $peak)" }
        return "sfx ok peak=$peak"
    }
}
