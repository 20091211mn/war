package com.warzone.core
import kotlin.test.Test
import kotlin.test.assertTrue
class SmokeTest {
    @Test fun menuThenPlay() {
        val g = Game(); g.modeCount = 1; g.menu = 1; g.init(1280, 600)
        for (m in 0 until g.MAP_COUNT) { g.menuSel = m; g.renderMenu(); assertTrue(g.r.statics.n > 0, "menu map $m") }
        g.menuSel = 0; val i = Input(); i.confirm = true; g.update(1f / 60, i); assertTrue(g.menu == 0)
        var vmSeen = 0
        for (k in 0 until 600) { val s = Input(); s.my = 1f; s.fire = k % 20 < 10; s.lx = 0.01f; g.update(1f / 60, s); g.render(); vmSeen = maxOf(vmSeen, g.r.layers[1].n) }
        assertTrue(g.r.statics.n > 1000 && vmSeen > 0, "scene tris ${g.r.layers[0].n} vm ${g.r.layers[1].n}")
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
            assertTrue(nan == 0, "NaN verts in viewmodel wpn=$wi")
            println("VM wpn=$wi alive=${g.e[0].alive} drive=${g.drive} verts=${L.vertexCount} nan=$nan inFront=$inV ndc x[$mnx,$mxx] y[$mny,$mxy] minW=$mnw")
        }
        run { g.startMatch(0); g.menu = 0; for (k in 0 until 200) { g.update(1f / 60, Input()); g.render() }
            var tu = 0L; var tr = 0L; var tg = 0L; val N = 300
            for (k in 0 until N) { val a = System.nanoTime(); g.update(1f / 60, Input()); val b = System.nanoTime(); g.render(); val c = System.nanoTime(); tu += b - a; tr += c - b }
            println("PERF update=${tu / N / 1000} us render=${tr / N / 1000} us tris=${g.r.statics.triangleCount}+${g.r.layers[0].triangleCount} vm=${g.r.layers[1].triangleCount} wpn=${g.e[g.meI].wpn}") }
        println("SMOKE OK maps=${g.MAP_COUNT} tris=${g.r.statics.triangleCount}+${g.r.layers[0].triangleCount}")
    }

    @Test fun lanLoopback() {
        val h = Game(); h.menu = 1; h.init(640, 360); h.mode = 1
        val c = Game(); c.menu = 1; c.init(640, 360)
        val ci = Input(); ci.confirm = true; h.update(1f / 60, ci); ci.confirm = false; h.update(1f / 60, ci); ci.confirm = true; h.update(1f / 60, ci); assertTrue(h.role == 1 && h.menu == 0, "host started")
        assertTrue(c.joinIp("127.0.0.1"))
        var ok = false
        for (k in 0 until 400) {
            val a = Input(); a.my = 1f; a.fire = k % 30 < 5; h.update(1f / 60, a)
            val b = Input(); b.my = 1f; b.lx = 0.2f; b.fire = k % 40 < 5; c.update(1f / 60, b)
            Thread.sleep(8)
            if (c.role == 2 && c.menu == 0 && c.e.size == h.e.size && c.e[c.meI].alive && h.humans() >= 2) ok = true
        }
        println("NET host=${h.role} client=${c.role} slot=${c.meI} ents=${c.e.size}/${h.e.size} meAlive=${c.e.getOrNull(c.meI)?.alive} hostSeenByClient=${c.e[0].b.x}/${h.e[0].b.x} cliPosOnHost=${h.e.getOrNull(c.meI)?.b?.x}/${c.e.getOrNull(c.meI)?.b?.x}")
        assertTrue(ok, "client joined and synced")
        assertTrue(h.e[c.meI].remote, "slot remote on host")
        val cx = c.e[c.meI].b.x; val hx = h.e[c.meI].b.x; assertTrue(Math.abs(cx - hx) < 3f, "client pos tracked on host $cx vs $hx")
        // المسح المباشر: المضيف يردّ على T_PROBE بإشارة (beacon) تلقائياً
        java.net.DatagramSocket().use { ds -> ds.soTimeout = 20
            ds.send(java.net.DatagramPacket(byteArrayOf(7, 3), 2, java.net.InetAddress.getByName("127.0.0.1"), 9330))
            var got = false; val buf = ByteArray(64)
            for (k in 0 until 100) { if (got) break; h.update(1f / 60, Input()); try { val p = java.net.DatagramPacket(buf, buf.size); ds.receive(p); if (p.length >= 4 && buf[0].toInt() == 6) got = true } catch (_: Exception) {} }
            println("PROBE reply=$got"); assertTrue(got, "host answers probe")
        }
        c.netClose(); h.netClose()
    }

    @Test fun airWar() {
        val g = Game(); g.modeCount = 1; g.menu = 1; g.init(640, 360); g.menu = 0; g.startMatch(8)
        assertTrue(g.air && g.jets.size >= 16, "air active jets=${g.jets.size}")
        val dt = 1f / 60
        for (k in 0 until 60 * 120) { g.update(dt, Input()); if (k % 120 == 0) g.render()
            for (J in g.jets) assertTrue(!J.x.isNaN() && !J.y.isNaN() && !J.z.isNaN(), "NaN jet at $k") }
        val j0 = g.myJet(); println("AIR score=${g.score[0]}/${g.score[1]} myJet=$j0 jets=${g.jets.count { it.alive }}")
        // الإقلاع: طائرة اللاعب على المدرج
        g.startMatch(8); val me = g.e[g.meI]
        val sj = g.jetNew(me.team, false, 0); val S = g.jets[sj]; S.x = me.b.x + 5f; S.z = me.b.z; S.y = 0f; S.gnd = true; S.spare = false
        // اجعل اللاعب راكباً على أرض: اخرج ثم ادخل
        var jj = g.myJet(); val e1 = Input(); e1.tri = true
        // الخروج من الطائرة الجوية (قفز) ثم نزول RPG
        g.update(dt, e1); assertTrue(g.myJet() < 0 && me.mags == 5, "exited with RPG mags=${me.mags}")
        for (k in 0 until 10) g.update(dt, Input())
        println("AIR exit ok rpg=${g.WEAPONS[me.wpn].id}")
    }

    @Test fun fuzzAllMaps() {
        val rnd = java.util.Random(7)
        for (m in 0 until GameData.let { Game().also { it.menu = 1; it.init(160, 90) }.MAP_COUNT }) {
            val g = Game(); g.modeCount = 1; g.menu = 1; g.init(160, 90); g.menu = 0; g.diff = rnd.nextInt(3); g.startMatch(m)
            val inp = Input()
            for (f in 0 until 60 * 30) {
                if (f % 20 == 0) {
                    inp.mx = (rnd.nextInt(21) - 10) / 10f; inp.my = (rnd.nextInt(21) - 10) / 10f; inp.lx = (rnd.nextInt(21) - 10) / 4f; inp.ly = (rnd.nextInt(21) - 10) / 8f
                    inp.fire = rnd.nextInt(3) == 0; inp.ads = rnd.nextInt(4) == 0; inp.jump = rnd.nextInt(5) == 0; inp.crouch = rnd.nextInt(6) == 0; inp.sprint = rnd.nextInt(4) == 0
                    inp.reload = rnd.nextInt(8) == 0; inp.grenade = rnd.nextInt(12) == 0; inp.drone = rnd.nextInt(40) == 0; inp.c4 = rnd.nextInt(15) == 0; inp.det = rnd.nextInt(15) == 0
                    inp.tri = rnd.nextInt(6) == 0; inp.use = rnd.nextInt(6) == 0; inp.seat = rnd.nextInt(10) == 0; inp.swapP = rnd.nextInt(10) == 0; inp.swapN = rnd.nextInt(10) == 0
                    inp.pick = if (rnd.nextInt(4) == 0) rnd.nextInt(g.WEAPON_COUNT) else -1
                } else { inp.tri = false; inp.swapP = false; inp.swapN = false; inp.pick = -1; inp.grenade = false; inp.c4 = false; inp.det = false }
                g.update(1f / 60, inp); if (f % 30 == 0) g.render()
                for (p in g.e) assertTrue(!p.b.x.isNaN() && !p.b.y.isNaN() && !p.b.z.isNaN() && !p.hp.isNaN(), "NaN ent map $m frame $f")
                for (J in g.jets) assertTrue(!J.x.isNaN() && !J.y.isNaN() && !J.vx.isNaN(), "NaN jet map $m frame $f")
                if (g.over) { inp.confirm = true; g.update(1f / 60, inp); inp.confirm = false }
            }
        }
    }

    private fun crossBin(): String? = System.getenv("WZ_CROSS")?.takeIf { java.io.File(it).canExecute() }

    /** مضيف C++ حقيقي (كود PS4/iOS) ← عميل Kotlin (أندرويد) */
    @Test fun crossCppHostKotlinClient() {
        val bin = crossBin() ?: return
        val pr = ProcessBuilder(bin, "host").redirectErrorStream(true).start()
        try {
            Thread.sleep(1500)
            val c = Game(); c.menu = 1; c.init(640, 360); assertTrue(c.joinIp("127.0.0.1"))
            var ok = 0
            for (k in 0 until 600) { val b = Input(); b.my = 1f; c.update(1f / 60, b); Thread.sleep(16); if (c.role == 2 && c.menu == 0 && c.e.size >= 2 && c.e[c.meI].alive) ok++ }
            println("CROSS kotlinClient->cppHost role=${c.role} slot=${c.meI} ents=${c.e.size} ok=$ok")
            assertTrue(ok > 200, "kotlin client joined C++ host and stayed synced ok=$ok")
            c.netClose()
        } finally { pr.destroyForcibly() }
    }

    /** مضيف Kotlin ← عميل C++ حقيقي */
    @Test fun crossKotlinHostCppClient() {
        val bin = crossBin() ?: return
        Thread.sleep(1500)
        val h = Game(); h.menu = 1; h.init(640, 360); h.mode = 1; val ci = Input(); ci.confirm = true; h.update(1f / 60, ci); ci.confirm = false; h.update(1f / 60, ci); ci.confirm = true; h.update(1f / 60, ci); assertTrue(h.role == 1)
        val pr = ProcessBuilder(bin, "join").redirectErrorStream(true).start()
        try {
            var ok = 0
            for (k in 0 until 900) { h.update(1f / 60, Input()); Thread.sleep(16); if (h.humans() >= 2) ok++; if (ok > 300) break }
            val out = try { pr.inputStream.bufferedReader().let { if (pr.waitFor(5, java.util.concurrent.TimeUnit.SECONDS)) it.readText() else "" } } catch (_: Exception) { "" }
            println("CROSS cppClient->kotlinHost ok=$ok out=${out.trim()}")
            assertTrue(ok > 200, "C++ client joined kotlin host ok=$ok")
            h.netClose()
        } finally { pr.destroyForcibly() }
    }
}
