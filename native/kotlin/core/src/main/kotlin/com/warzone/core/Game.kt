package com.warzone.core

import kotlin.math.*

class Input {
    @JvmField var tri = false; @JvmField var seat = false; @JvmField var c4 = false; @JvmField var det = false
    @JvmField var mx = 0f; @JvmField var my = 0f   // move: x right, y forward
    @JvmField var lx = 0f; @JvmField var ly = 0f   // look: x right, y up (scaled)
    @JvmField var fire = false; @JvmField var ads = false; @JvmField var jump = false; @JvmField var crouch = false; @JvmField var reload = false; @JvmField var sprint = false
    @JvmField var swapP = false; @JvmField var swapN = false; @JvmField var grenade = false; @JvmField var drone = false; @JvmField var use = false
    @JvmField var start = false; @JvmField var confirm = false; @JvmField var back = false
    @JvmField var pick = -1   // اختيار سلاح مباشر (عجلة الأسلحة): رقم السلاح أو -1
    fun set(o: Input) {
        tri = o.tri; seat = o.seat; c4 = o.c4; det = o.det; mx = o.mx; my = o.my; lx = o.lx; ly = o.ly; fire = o.fire; ads = o.ads; jump = o.jump; crouch = o.crouch
        reload = o.reload; sprint = o.sprint; swapP = o.swapP; swapN = o.swapN; grenade = o.grenade; drone = o.drone; use = o.use; start = o.start; confirm = o.confirm; back = o.back; pick = o.pick
    }
    fun clear() { set(Input()) }
}

class Ent {
    @JvmField var mnt = false; @JvmField var mvi = -1
    @JvmField val b = Body(); @JvmField var yaw = 0f; @JvmField var pitch = 0f; @JvmField var team = 0
    @JvmField var bot = true; @JvmField var alive = false; @JvmField var crouch = false; @JvmField var sprint = false; @JvmField var moving = false
    @JvmField var magM = 1f; @JvmField var relM = 1f; @JvmField var reveal = 0f; @JvmField var hp = 100f; @JvmField var prot = 0f; @JvmField var respawn = 0f
    @JvmField var phase = 0f; @JvmField var flash = 0f; @JvmField var cd = 0f; @JvmField var reloadT = -1f; @JvmField var rkP = 0f; @JvmField var rkY = 0f
    @JvmField var wpn = 0; @JvmField var ammo = 0; @JvmField var mags = 0; @JvmField var kills = 0; @JvmField var deaths = 0
    @JvmField var think = 0f; @JvmField var tYaw = 0f; @JvmField var sx = 0f; @JvmField var sz = 0f; @JvmField var stuck = 0f; @JvmField var turnT = 0f; @JvmField var turnDir = 1f
    @JvmField var burst = 0f; @JvmField var react = 0f; @JvmField var strafe = 1f; @JvmField var strafeT = 0f; @JvmField var target = -1
    @JvmField var lastX = 0f; @JvmField var lastZ = 0f; @JvmField var lastDmgT = 0f; @JvmField var lastHitBy = -1
    @JvmField var revealT = 0f; @JvmField var c4Left = 2; @JvmField var c4Refill = 0f
    @JvmField var name = ""; @JvmField var remote = false; @JvmField var gone = false; @JvmField var spawnSeq = 0
    @JvmField var rin: NetIn? = null; @JvmField var rc4Init = false; @JvmField var rc4p = 0; @JvmField var rc4d = 0
    @JvmField var tx = 0f; @JvmField var ty = 0f; @JvmField var tz = 0f; @JvmField var tyaw = 0f
}

class C4s(@JvmField val x: Float, @JvmField val y: Float, @JvmField val z: Float, @JvmField val team: Int, @JvmField val owner: Int)
class Tracer(@JvmField val x0: Float, @JvmField val y0: Float, @JvmField val z0: Float, @JvmField val x1: Float, @JvmField val y1: Float, @JvmField val z1: Float, @JvmField var t: Float)
class Puff(@JvmField var x: Float, @JvmField var y: Float, @JvmField var z: Float, @JvmField var vy: Float, @JvmField var t: Float, @JvmField var sz: Float, @JvmField val fire: Boolean)
class Shell(@JvmField var x: Float, @JvmField var y: Float, @JvmField var z: Float, @JvmField var vx: Float, @JvmField var vy: Float, @JvmField var vz: Float, @JvmField var t: Float)
class Gr(@JvmField var x: Float, @JvmField var y: Float, @JvmField var z: Float, @JvmField var vx: Float, @JvmField var vy: Float, @JvmField var vz: Float, @JvmField var t: Float)
class Rk(@JvmField val ox: Float, @JvmField val oy: Float, @JvmField val oz: Float, @JvmField val dx: Float, @JvmField val dy: Float, @JvmField val dz: Float, @JvmField val len: Float, @JvmField var t: Float, @JvmField val by: Float)
class Spark(@JvmField var x: Float, @JvmField var y: Float, @JvmField var z: Float, @JvmField var vx: Float, @JvmField var vy: Float, @JvmField var vz: Float, @JvmField var t: Float)
class Drn { @JvmField var on = false; @JvmField var x = 0f; @JvmField var y = 0f; @JvmField var z = 0f; @JvmField var yaw = 0f; @JvmField var pitch = 0f
    @JvmField var vx = 0f; @JvmField var vy = 0f; @JvmField var vz = 0f; @JvmField var t = 0f }
class GAtt { @JvmField var optic = 0; @JvmField var muz = 0; @JvmField var under = 0; @JvmField var mag = 0 }

private fun frand(): Float = Rnd.frand()
private fun rand(): Int = Rnd.rand()

class Game {
    @JvmField val w = World()
    @JvmField val r = Renderer()
    @JvmField var e: ArrayList<Ent> = ArrayList()
    @JvmField val tr = ArrayList<Tracer>()
    @JvmField var mapIdx = 0; @JvmField var scoreLim = 30; @JvmField val score = IntArray(2); @JvmField var nBots = 6; @JvmField var diff = 0
    @JvmField var sens = 1.0f
    @JvmField var t = 0f; @JvmField var hitMark = 0f; @JvmField var dmgFlash = 0f
    @JvmField var over = false; @JvmField var winner = 0; @JvmField var streak = 0; @JvmField var glCnt = 2; @JvmField var hitAng = 0f; @JvmField var hitAngT = 0f
    @JvmField var fpsShow = 0f
    @JvmField var vh = ArrayList<Veh>(); @JvmField var drive = -1; @JvmField var lookIdle = 0f; @JvmField var nearVeh = false
    @JvmField val puffs = ArrayList<Puff>()
    @JvmField var dn = Drn(); @JvmField var dnCd = 0f; @JvmField var dnSpot = 0f; @JvmField var pdn = false; @JvmField var pfireD = false
    @JvmField val shells = ArrayList<Shell>(); @JvmField val grs = ArrayList<Gr>(); @JvmField val rks = ArrayList<Rk>(); @JvmField var gren = 2; @JvmField var pg = false
    @JvmField val sparks = ArrayList<Spark>()
    @JvmField var prim = 0; @JvmField val prev = Input(); @JvmField var paused = false; @JvmField var msgT = 0f; @JvmField var msg = ""
    @JvmField val feed = Array(4) { "" }; @JvmField val feedT = FloatArray(4)
    @JvmField var menu = 1; @JvmField var menuSel = 0; @JvmField var menuRow = 0; @JvmField var mode = 0
    @JvmField var modeCount = 3
    @JvmField var air = false; @JvmField val jets = ArrayList<Jet>(); @JvmField var entJet = IntArray(0); @JvmField val airPrev = Input()
    @JvmField var footT = 0f; @JvmField var jetMsg = ""; @JvmField var jetMsgT = 0f; @JvmField var rpgIdx = 8
    @JvmField var meI = 0; @JvmField var role = 0; @JvmField val ns = NetState(); @JvmField var overT = 0f; @JvmField var netPc4 = false; @JvmField var netPdet = false
    @JvmField var spawnCtr = 0
    @JvmField var camX = 0f; @JvmField var camY = 3f; @JvmField var camZ = 0f
    @JvmField val c4s = ArrayList<C4s>(); @JvmField var pc4 = false; @JvmField var pdet = false
    @JvmField var gunner = false; @JvmField var pSeat = false
    @JvmField var adsHeld = false; @JvmField var optZ = 0f
    @JvmField val wAtt = Array(16) { GAtt() }
    @JvmField var lastPh = 0f
    @JvmField var fovMul = 1f; @JvmField var hudK = 1f
    @JvmField var frameCount = 0
    @JvmField val scene = SceneBuilder(this)
    @JvmField var camMenuYaw = 0f
    fun render() = scene.render()
    fun renderMenu() = scene.renderMenu()
    private val use = Input()
    private val tmp2 = FloatArray(2)

    val WEAPONS get() = GameData.weapons
    val WEAPON_COUNT get() = GameData.WEAPON_COUNT
    val MAP_COUNT get() = GameData.MAP_COUNT

    fun passenger() = drive >= 0 && gunner
    fun nm(i: Int): String = if (i == meI) "YOU" else if (i == 0 && role == 2) "HOST" else if (i < e.size && e[i].remote) "P$i" else "BOT$i"
    fun init(rw: Int, rh: Int) { GameData.load(); r.init(rw, rh); startMatch(mapIdx) }
    fun say(s: String) { msg = s; msgT = 2.5f }
    fun addFeed(s: String) { for (i in 3 downTo 1) { feed[i] = feed[i - 1]; feedT[i] = feedT[i - 1] }; feed[0] = s; feedT[0] = 5f }

    fun startMatch(idx: Int) {
        mapIdx = idx % MAP_COUNT; w.load(mapIdx)
        val total = if (idx % MAP_COUNT == 8 && role == 0) 16 else 1 + nBots * 2 - 1
        e = ArrayList(total); for (i in 0 until total) e.add(Ent()); c4s.clear()
        for (i in 0 until total) { val p = e[i]; p.bot = i > 0; p.team = if (i == 0) 0 else (if (i and 1 != 0) 1 else 0); p.name = nm(i); p.wpn = if (i == 0) prim else rand() % WEAPON_COUNT }
        dn.on = false; dnCd = 0f; grs.clear(); rks.clear(); vh = ArrayList(); drive = -1; puffs.clear()
        for (k in 0 until w.m.nv) vh.add(makeVeh(k))
        score[0] = 0; score[1] = 0; over = false; t = 0f; tr.clear()
        if (role == 1) { ns.matchId = (ns.matchId + 1) and 255; ns.evq.clear(); for (c in ns.cli) if (c.on && c.ent in 0 until e.size) { val p = e[c.ent]; p.remote = true; p.bot = false; p.rin = null; p.rc4Init = false } }
        air = mapIdx == 8 && role == 0
        if (air) airInit() else { jets.clear(); entJet = IntArray(0) }
        if (!air) for (i in 0 until e.size) spawn(i)
    }

    fun zoomOf(p: Ent): Float = if (optZ > 0 && p === e[meI]) optZ else WEAPONS[p.wpn].zoom
    fun cap(p: Ent): Int = (WEAPONS[p.wpn].mag * p.magM + 0.5f).toInt()
    fun giveWeapon(p: Ent, wi: Int) {
        p.wpn = wi; p.magM = 1f; p.relM = 1f; if (p === e[meI]) optZ = 0f
        p.ammo = WEAPONS[wi].mag; p.mags = WEAPONS[wi].mags; p.reloadT = -1f; p.cd = 0f; if (p === e[meI]) applyWAtt(p)
    }
    fun spawn(ix: Int) {
        val p = e[ix]
        if (e.isNotEmpty()) c4Clear(ix)
        val sp = if (p.team != 0) w.m.blue else w.m.red; val n = sp.size; var s = sp[rand() % n]
        var bestD = -1f
        for (k in 0 until 4) {
            val c = sp[rand() % n]; var md = 1e9f
            for (o in e) if (o.alive && o.team != p.team) md = fmin(md, (o.b.x - c.x) * (o.b.x - c.x) + (o.b.z - c.z) * (o.b.z - c.z))
            if (md > bestD) { bestD = md; s = c }
        }
        p.b.x = s.x; p.b.z = s.z; p.b.y = 20f; p.b.vy = 0f; p.b.onGround = false
        for (i in 0 until 120) { moveBody(w, p.b, 0f, 0f, 0.04f, false, false); if (p.b.onGround) break }
        p.pitch = 0f
        p.yaw = atan2f(-(0 - p.b.x), -(0 - p.b.z)); if (!p.bot && !p.remote) gren = 2; p.alive = true; p.hp = 100f; p.prot = 2f; p.crouch = false
        p.spawnSeq = (++spawnCtr) and 255
        giveWeapon(p, if (p.bot || p.remote) p.wpn else prim)
    }

    fun dmgAt(d: WeaponDef, dist: Float): Float {
        if (dist <= d.r0) return d.dmg; if (dist >= d.r1) return d.dmg * d.minMul
        return d.dmg * (1 - (1 - d.minMul) * (dist - d.r0) / (d.r1 - d.r0))
    }
    fun visible(a: Ent, b2: Ent): Boolean {
        val ax = a.b.x; val ay = a.b.y + PK.eye; val az = a.b.z; val bx = b2.b.x; val by = b2.b.y + 1.2f; val bz = b2.b.z
        val dx = bx - ax; val dy = by - ay; val dz = bz - az; val d = sqrtf(dx * dx + dy * dy + dz * dz); if (d < 0.01f) return true
        return raycastMap(w, ax, ay, az, dx / d, dy / d, dz / d, d) >= 1e29f
    }

    fun hurt(vi: Int, ai: Int, dmg: Float, head: Boolean) {
        if (role == 2) return
        val v = e[vi]; if (!v.alive || v.prot > 0) return
        v.hp -= dmg; v.lastHitBy = ai; v.lastDmgT = t
        if (vi == meI) { dmgFlash = 0.5f; Sfx.play(Sfx.HURT, 0.9f); if (ai >= 0 && ai != vi) { hitAng = atan2f(-(e[ai].b.x - v.b.x), -(e[ai].b.z - v.b.z)) - v.yaw; hitAngT = 1.2f } }
        if (ai == meI) { hitMark = 0.25f; Sfx.play(Sfx.HIT, 0.7f) }
        if (v.hp <= 0) {
            v.alive = false; v.deaths++; v.respawn = 4f; c4Clear(vi)
            if (vi == meI) streak = 0
            if (ai == meI && vi != meI) {
                Sfx.play(Sfx.KILL, 0.8f); streak++
                if (head) say("تصويبة رأس!") else if (streak == 2) say("قتلتان متتاليتان") else if (streak == 3) say("ثلاث قتلات!") else if (streak == 5) say("سلسلة قتل!") else if (streak == 8) say("لا يمكن إيقافك!")
            }
            if (role == 1 && ai >= 0 && ai != vi) { ns.evtId = (ns.evtId + 1) and 255; if (ns.evtId == 0) ns.evtId = 1; ns.evq.add(NetEvt(ns.evtId, ai, vi, if (head) 1 else 0)); if (ns.evq.size > 6) ns.evq.removeAt(0) }
            if (!air && ai >= 0 && ai != vi) { val k = e[ai]; k.kills++; score[k.team]++; addFeed("${nm(ai)} > ${nm(vi)}${if (head) " HS" else ""}") }
            if (!air && (score[0] >= scoreLim || score[1] >= scoreLim)) { over = true; winner = if (score[0] >= scoreLim) 0 else 1 }
        }
    }

    // ---- vehicles ----
    fun explodeVeh(v: Veh, by: Int) {
        Sfx.play(Sfx.EXPLODE, fmax(0.2f, 1f - sqrtf((v.x - e[meI].b.x) * (v.x - e[meI].b.x) + (v.z - e[meI].b.z) * (v.z - e[meI].b.z)) / 80f)); v.dead = true; v.respawnT = 30f; v.burn = false
        if (v.driver >= 0) { if (v.driver == meI) drive = -1; v.driver = -1 }
        for (k in 0 until 14) { val a = v.x + (frand() - 0.5f) * 3; val b = 0.6f + frand() * 1.5f; val c = v.z + (frand() - 0.5f) * 3; val d = 1.5f + frand() * 2f; val f = 1.2f + frand() * 1.2f; val g = 1.2f + frand() * 1.5f; puffs.add(Puff(a, b, c, d, f, g, k < 8)) }
        for (i in 0 until e.size) { if (!e[i].alive) continue; val dx = e[i].b.x - v.x; val dz = e[i].b.z - v.z; val dd = sqrtf(dx * dx + dz * dz); if (dd < 7f) hurt(i, by, 260 * powf(1 - dd / 7f, 1.1f), false) }
        if (by == meI) hitMark = 0.4f; say("دُمِّرت المركبة!")
    }
    fun hurtVeh(v: Veh, amt: Float, by: Int) { if (role == 2) return; if (v.dead || amt <= 0) return; v.hp -= amt; if (v.hp <= 0) explodeVeh(v, by) else if (v.hp < 150) v.burn = true }
    fun damageVeh(v: Veh, part: Int, d: Float, by: Int) {
        if (v.dead) return
        if (part >= 10) { val i = part - 10; if ((v.tires shr i) and 1 != 0) return; v.tireHp[i] -= d; if (v.tireHp[i] <= 0) v.tires = v.tires or (1 shl i); return }
        hurtVeh(v, (if (part == 1) d * 1.3f else d * 0.35f) * VH.vp(v).armor, by); if (by == meI) hitMark = 0.25f
    }
    fun puffsBoom(x: Float, z: Float) {
        for (k in 0 until 14) { val a = x + (frand() - 0.5f) * 3; val b = 0.6f + frand() * 1.5f; val c = z + (frand() - 0.5f) * 3; val d = 1.5f + frand() * 2f; val f = 1.2f + frand() * 1.2f; val g = 1.2f + frand() * 1.5f; puffs.add(Puff(a, b, c, d, f, g, k < 8)) }
        Sfx.play(Sfx.EXPLODE, fmax(0.2f, 1f - hypotf(x - e[meI].b.x, z - e[meI].b.z) / 80f))
    }
    fun makeVeh(k: Int): Veh {
        val v = Veh()
        if (k < w.m.nv) { val d = w.m.veh[k]; v.x = d.x; v.sx = d.x; v.tx = d.x; v.z = d.z; v.sz = d.z; v.tz = d.z; v.h = d.h; v.sh = d.h; v.th = d.h; v.type = d.t }
        v.hp = VH.VPS[if (v.type == 1) 1 else 0].hp; v.ty = v.h; return v
    }
    private fun puffExplosion(gx: Float, gy: Float, gz: Float, n: Int, lim: Int) {
        for (j in 0 until n) { val a = gx + (frand() - 0.5f) * 2; val b = gy + frand() * 1.2f; val c = gz + (frand() - 0.5f) * 2; val d = 1.2f + frand() * 1.5f; val f = 0.7f + frand() * 0.5f; val g = 0.9f + frand() * 0.9f; puffs.add(Puff(a, b, c, d, f, g, j < lim)) }
    }
    fun sparkAt(x: Float, y: Float, z: Float, nx: Float, ny: Float, nz: Float, n: Int) {
        var k = 0
        while (k < n && sparks.size < 120) { val a = -nx * 1.5f + (frand() - 0.5f) * 3f; val b = -ny * 1.5f + frand() * 2.5f; val c = -nz * 1.5f + (frand() - 0.5f) * 3f; val d = 0.22f + frand() * 0.15f; sparks.add(Spark(x, y, z, a, b, c, d)); k++ }
    }
    private val aim = FloatArray(3); private val pOut = IntArray(1)
    fun cannonFx(v: Veh, dx: Float, dy: Float, dz: Float, dmg: Boolean, by: Int) {
        val ox = v.x + dx * 4.1f; val oy = 1.65f + dy * 4.1f; val oz = v.z + dz * 4.1f; var best = fmin(raycastMap(w, ox, oy, oz, dx, dy, dz, 160f), 160f)
        for (o in vh) { if (o === v || o.dead) continue; val tv = VH.rayVeh(o, ox, oy, oz, dx, dy, dz, best, pOut); if (tv < best) best = tv }
        val ex = ox + dx * best; val ey = fmax(0.1f, oy + dy * best); val ez = oz + dz * best; v.cannonT = 2.5f; v.kick = 1f
        if (tr.size < 64) tr.add(Tracer(ox, oy, oz, ex, ey, ez, 0.12f))
        for (j in 0 until 6) { val a = ox + (frand() - 0.5f) * 0.6f; val c = oz + (frand() - 0.5f) * 0.6f; val d = 0.8f + frand(); val f = 0.5f + frand() * 0.4f; puffs.add(Puff(a, oy, c, d, f, 0.5f, false)) }
        for (j in 0 until 12) { val a = ex + (frand() - 0.5f) * 3; val b = ey + frand() * 1.5f; val c = ez + (frand() - 0.5f) * 3; val d = 1.2f + frand() * 1.8f; val f = 0.8f + frand() * 0.6f; val g = 1f + frand() * 1.2f; puffs.add(Puff(a, b, c, d, f, g, j < 6)) }
        sparkAt(ex, ey, ez, 0f, -1f, 0f, 40); Sfx.play(Sfx.EXPLODE, 1f); if (by == meI) hitMark = 0.3f
        if (!dmg) return; val R = 6.5f
        for (i in 0 until e.size) {
            if (!e[i].alive) continue; if (e[i].team == e[by].team && i != by) continue
            val qx = e[i].b.x - ex; val qy = e[i].b.y + 1f - ey; val qz = e[i].b.z - ez; val d = sqrtf(qx * qx + qy * qy + qz * qz); if (d < R) hurt(i, by, 230f * powf(1 - d / R, 0.9f), false)
        }
        for (o in vh) { if (o.dead) continue; val qx = o.x - ex; val qz = o.z - ez; val d = sqrtf(qx * qx + qz * qz); if (d < R + 1.5f) hurtVeh(o, 420f * (1 - d / (R + 1.5f)), by) }
    }
    fun tankAimDir(v: Veh, yaw: Float, pitch: Float, d: FloatArray) {
        val cp = cosf(pitch); val ddx = -sinf(yaw) * cp; val ddy = sinf(pitch); val ddz = -cosf(yaw) * cp
        val t = fmin(raycastMap(w, camX, camY, camZ, ddx, ddy, ddz, 160f), 160f); val px = camX + ddx * t; val py = camY + ddy * t; val pz = camZ + ddz * t
        val ax = px - v.x; val ay = py - 1.65f; val az = pz - v.z; val l = sqrtf(ax * ax + ay * ay + az * az) + 1e-5f; d[0] = ax / l; d[1] = ay / l; d[2] = az / l
    }

    // ---- C4 ----
    fun c4Place(i: Int, dx0: Float, dy0: Float, dz0: Float): Boolean {
        var dx = dx0; var dy = dy0; var dz = dz0
        val p = e[i]; if (!p.alive || p.c4Left <= 0) return false; var mine = 0; for (c in c4s) if (c.owner == i) mine++; if (mine >= 3) return false
        val l = sqrtf(dx * dx + dy * dy + dz * dz); if (l < 0.5f) return false; dx /= l; dy /= l; dz /= l; val ey = p.b.y + (if (p.crouch) PK.eyec else PK.eye); val R = 4.5f
        var t = raycastMap(w, p.b.x, ey, p.b.z, dx, dy, dz, R)
        if (t >= R) { if (dy >= -0.05f) return false; val tt = (ey - 0.05f) / -dy; if (tt > R) return false; t = tt }
        val x = p.b.x + dx * (t - 0.06f); val y = fmax(0.05f, ey + dy * (t - 0.06f)); val z = p.b.z + dz * (t - 0.06f)
        p.c4Left--; c4s.add(C4s(x, y, z, p.team, i)); if (i == meI) Sfx.play(Sfx.EMPTY, 0.8f); return true
    }
    fun blastAt(x: Float, y: Float, z: Float, owner: Int, team: Int, R: Float, pd: Float, vd: Float) {
        for (k in 0 until 16) { val a = x + (frand() - 0.5f) * 3; val b = y + frand() * 1.5f; val c = z + (frand() - 0.5f) * 3; val d = 1.4f + frand() * 2f; val f = 1f + frand() * 1f; val g = 1.2f + frand() * 1.4f; puffs.add(Puff(a, b, c, d, f, g, k < 9)) }
        sparkAt(x, y, z, 0f, -1f, 0f, 40)
        run { val ddx = x - e[meI].b.x; val ddz = z - e[meI].b.z; Sfx.play(Sfx.EXPLODE, fmax(0.25f, 1f - sqrtf(ddx * ddx + ddz * ddz) / 70f)) }
        for (i in 0 until e.size) {
            if (!e[i].alive) continue; if (e[i].team == team && i != owner) continue
            val qx = e[i].b.x - x; val qy = e[i].b.y + 1f - y; val qz = e[i].b.z - z; val d = sqrtf(qx * qx + qy * qy + qz * qz); if (d > R) continue
            val tw = raycastMap(w, x, y + 0.2f, z, qx / (d + 1e-5f), (qy - 0.2f) / (d + 1e-5f), qz / (d + 1e-5f), d); if (tw < d - 0.3f) continue
            hurt(i, owner, pd * powf(1 - d / R, 1.2f), false)
        }
        for (o in vh) { if (o.dead) continue; val qx = o.x - x; val qz = o.z - z; val d = fmax(0f, sqrtf(qx * qx + (y - 0.8f) * (y - 0.8f) + qz * qz) - 1.6f); if (d < R) hurtVeh(o, vd * powf(1 - d / R, 1.1f), owner) }
    }
    fun c4Det(i: Int) {
        val mine = ArrayList<C4s>(); var k = 0
        while (k < c4s.size) { if (c4s[k].owner == i) { mine.add(c4s[k]); c4s.removeAt(k) } else k++ }
        if (mine.isEmpty()) return; e[i].c4Refill = 20f; for (c in mine) blastAt(c.x, c.y, c.z, i, c.team, 8f, 210f, 380f)
    }
    fun c4Clear(i: Int) { var k = 0; while (k < c4s.size) { if (c4s[k].owner == i) c4s.removeAt(k) else k++ }; e[i].c4Left = 2; e[i].c4Refill = 0f }
    fun c4Tick(dt: Float) {
        for (i in 0 until e.size) { val p = e[i]; if (p.alive && p.c4Left < 2 && p.c4Refill > 0) { p.c4Refill -= dt; var any = false; for (c in c4s) if (c.owner == i) any = true; if (p.c4Refill <= 0 && !any) { p.c4Left = 2; p.c4Refill = 0f } } }
    }

    fun exitVeh() {
        gunner = false; if (drive < 0) return; val v = vh[drive]; val p = e[meI]
        val sps = arrayOf(floatArrayOf(-1.9f, -0.25f), floatArrayOf(1.9f, -0.25f), floatArrayOf(0f, 3.6f), floatArrayOf(0f, -3.6f))
        val spt = arrayOf(floatArrayOf(2.6f, 0f), floatArrayOf(-2.6f, 0f), floatArrayOf(0f, 4.2f), floatArrayOf(0f, -4.2f))
        val SP = if (v.type == 1) spt else sps
        var bx = v.x; var bz = v.z
        for (q in 0 until 4) { val s = SP[q]; VH.l2w(v, s[0], s[1], tmp2); if (!blocked(w, tmp2[0], 0.05f, tmp2[1], PK.h)) { bx = tmp2[0]; bz = tmp2[1]; break } }
        p.b.x = bx; p.b.z = bz; p.b.y = 0.3f; p.b.vy = 0f; p.b.onGround = false; for (k in 0 until 30) { moveBody(w, p.b, 0f, 0f, 0.04f, false, false); if (p.b.onGround) break }
        p.pitch = 0f; v.driver = -1; drive = -1
    }

    private fun wrapPi(a0: Float): Float { var a = a0; while (a > 3.14159f) a -= 6.28318f; while (a < -3.14159f) a += 6.28318f; return a }
    private fun clampf(v: Float, lo: Float, hi: Float) = fmax(lo, fmin(hi, v))

    fun updateVeh(dt: Float, inp: Input) {
        val me = e[meI]; nearVeh = false; val tri = inp.tri && !prev.tri
        if (drive >= 0 && (!me.alive || vh[drive].dead)) { vh[drive].driver = -1; drive = -1 }
        if (me.alive && drive < 0) {
            var best = -1; var bd = 2.8f
            for (k in 0 until vh.size) {
                val v = vh[k]; if (v.dead) continue; VH.w2l(v, me.b.x, me.b.z, tmp2); val lx = tmp2[0]; val lz = tmp2[1]; val P = VH.vp(v)
                val dd = hypotf(fmax(0f, fabsf(lx) - P.hw), fmax(0f, fabsf(lz) - P.hl))
                if (dd < bd && me.b.y < 2.3f && v.driver < 0 && (v.team < 0 || v.team == me.team)) { bd = dd; best = k }
            }
            nearVeh = best >= 0
            if (nearVeh && tri) { drive = best; vh[best].driver = meI; vh[best].team = me.team; me.pitch = -0.18f; me.crouch = false; me.reloadT = -1f; lookIdle = 0f }
        } else if (me.alive && drive >= 0 && tri) exitVeh()
        for (k in 0 until vh.size) {
            val v = vh[k]
            if (v.dead) { v.respawnT -= dt; if (v.respawnT <= 0) { val n = Veh(); n.type = v.type; n.hp = VH.vp(v).hp; n.x = v.sx; n.sx = v.sx; n.z = v.sz; n.sz = v.sz; n.h = v.sh; n.sh = v.sh; n.ty = v.sh; v.copyFrom(n) }; continue }
            val dr = drive == k && me.alive
            if (dr) { if (inp.seat && !pSeat) gunner = !gunner; pSeat = inp.seat }
            var thr = 0f; var st = 0f; var hb = false
            if (dr && gunner) { hb = true }
            else if (dr) { thr = if (v.type == 1) fmax(-1f, fmin(1f, inp.my - (if (inp.ads) 1f else 0f))) else fmax(-1f, fmin(1f, inp.my + (if (inp.fire) 1f else 0f) - (if (inp.ads) 1f else 0f))); st = inp.mx; hb = inp.jump }
            if (v.type != 1) {
                var gi = -1; for (j in 0 until e.size) if (e[j].mnt && e[j].mvi == k && e[j].alive) { gi = j; break }
                v.mgFp = (gi == meI && drive == k)
                if (gi >= 0) { val dy = wrapPi(e[gi].yaw - v.ty); v.ty += dy * fmin(1f, dt * 12); v.tp = fmax(-0.5f, fmin(0.9f, e[gi].pitch)) } else { v.ty = v.h; v.tp = 0f }
            }
            if (v.type == 1) {
                if (v.cannonT > 0) v.cannonT -= dt; v.kick = fmax(0f, v.kick - dt * 3); val di = if (dr) meI else v.driver
                if (di >= 0 && di < e.size) { v.ty = e[di].yaw; v.tp = fmax(-0.35f, fmin(0.5f, e[di].pitch)) } else v.ty = v.h
                if (dr && !gunner && inp.fire && v.cannonT <= 0) { tankAimDir(v, me.yaw, me.pitch, aim); cannonFx(v, aim[0], aim[1], aim[2], true, meI) }
            }
            if (!dr) hb = fabsf(v.speed) < 2f
            val crash = VH.step(w, v, vh, thr, st, hb, dt)
            if (crash > VH.vp(v).crashMin && v.crashT <= 0) { v.crashT = 0.45f; hurtVeh(v, (crash - VH.vp(v).crashMin) * 9, v.driver); if (dr) dmgFlash = fmax(dmgFlash, 0.15f) }
            if (v.crashT > 0) v.crashT -= dt
            if (v.dead) continue
            if (v.burn) {
                v.hp -= 6 * dt; v.smoke -= dt
                if (v.smoke <= 0) { v.smoke = 0.12f; val a = v.x + (frand() - 0.5f); val c = v.z + (frand() - 0.5f); puffs.add(Puff(a, 1.3f, c, 1.6f, 0.9f, 0.5f, frand() < 0.3f)) }
                if (v.hp <= 0) { explodeVeh(v, v.driver); continue }
            }
            for (i in 0 until e.size) {
                val p = e[i]; if (!p.alive || (dr && i == meI) || p.gone) continue
                tmp2[0] = p.b.x; tmp2[1] = p.b.z; if (VH.pushOut(v, tmp2, p.b.y)) { p.b.x = tmp2[0]; p.b.z = tmp2[1] }
                if (dr && fabsf(v.speed) > 5f && p.team != me.team) {
                    VH.w2l(v, p.b.x, p.b.z, tmp2); val lx = tmp2[0]; val lz = tmp2[1]
                    if (fabsf(lx) < VH.vp(v).hw + 0.45f && fabsf(lz) < VH.vp(v).hl + 0.45f && p.b.y < 1.8f && p.prot <= 0) hurt(i, meI, fabsf(v.speed) * (if (v.type == 1) 24 else 14), false)
                }
            }
            if (dr) {
                var sx: Float; var sz: Float
                if (gunner) { if (v.type == 1) VH.l2w(v, 0f, 2.0f, tmp2) else VH.l2w(v, 0f, 1.9f, tmp2); me.b.y = if (v.type == 1) 1.55f else 0.85f }
                else if (v.type == 1) { VH.l2w(v, 0f, -0.5f, tmp2); me.b.y = 1.0f }
                else { VH.l2w(v, -0.45f, -0.25f, tmp2); me.b.y = 0.35f }
                sx = tmp2[0]; sz = tmp2[1]
                me.b.x = sx; me.b.z = sz; me.b.vy = 0f; me.moving = false
            }
        }
        run { for (q in puffs) { q.t -= dt; q.y += q.vy * dt; q.sz += dt * 0.8f }; puffs.removeAll { it.t <= 0 } }
    }

    fun isExpl(wi: Int) = WEAPONS[wi].id == "rpg"
    fun explosionAt(gx: Float, gy: Float, gz: Float, by: Int, R: Float, pd: Float, vd: Float) {
        if (air) for (j in jets.indices) { val J = jets[j]; if (!J.alive || J.team == e[by].team) continue; val dd = sqrtf((J.x - gx) * (J.x - gx) + (J.y + 1 - gy) * (J.y + 1 - gy) + (J.z - gz) * (J.z - gz)); if (dd < R + 8f) jetHurt(j, pd * 1.3f * (1f - dd / (R + 8f)) + 40f, by, false) }
        for (i in 0 until e.size) {
            if (!e[i].alive) continue; if (i != by && e[i].team == e[by].team) continue
            val ex = e[i].b.x - gx; val ey = e[i].b.y + 0.9f - gy; val ez = e[i].b.z - gz; val dd = sqrtf(ex * ex + ey * ey + ez * ez); if (dd < R) hurt(i, by, pd * powf(1f - dd / R, 1.2f), false)
        }
        for (v in vh) { if (v.dead) continue; val ex = v.x - gx; val ez = v.z - gz; val dd = fmax(0f, sqrtf(ex * ex + ez * ez) - 1.6f); if (dd < R) hurtVeh(v, vd * powf(1f - dd / R, 1.1f), by) }
        puffsBoom(gx, gz); puffExplosion(gx, gy, gz, 10, 5)
    }

    fun fire(i: Int) {
        val p = e[i]; if (role == 2) { p.cd = 60f / WEAPONS[p.wpn].rpm; return }
        val d = if (p.mnt) GameData.mount!! else WEAPONS[p.wpn]; val ex = !p.mnt && isExpl(p.wpn)
        if (!(i == meI && !p.mnt && wAtt[p.wpn and 15].muz == 1)) p.revealT = 5f
        var aDm = 1f; var aHip = 1f; var aRv = 1f; var aRh = 1f; var aVol = 1f
        if (i == meI && !p.mnt) {
            val at = wAtt[p.wpn and 15]
            if (at.muz == 1) { aDm = 0.95f; aRv *= 0.92f; aVol = 0.4f } else if (at.muz == 2) { aRh *= 0.55f; aRv *= 0.88f }
            if (at.under == 1) { aRv *= 0.82f; aRh *= 0.85f; aHip = 0.9f } else if (at.under == 2) aHip = 0.6f
        }
        run { val ddx = p.b.x - e[meI].b.x; val ddz = p.b.z - e[meI].b.z; val vv = 1f - sqrtf(ddx * ddx + ddz * ddz) / 70f
            Sfx.shot(if (p.mnt) 4 else GameData.wkind[p.wpn % WEAPON_COUNT], aVol < 1f, (if (i == meI) 1f else vv * 0.8f) * (if (aVol < 1f) 1f else 1f), 0f) }
        if (i == meI && drive < 0 && shells.size < 40) {
            val cy = cosf(p.yaw); val sy = sinf(p.yaw); val rx = cy; val rz = -sy
            val a = p.b.x + rx * 0.2f - sy * 0.3f; val b = p.b.y + (if (p.crouch) PK.eyec else PK.eye) - 0.18f; val c = p.b.z + rz * 0.2f - cy * 0.3f
            val vx = rx * (1.8f + frand()) + (frand() - 0.5f); val vy = 1.8f + frand() * 0.8f; val vz = rz * (1.8f + frand()) + (frand() - 0.5f)
            shells.add(Shell(a, b, c, vx, vy, vz, 1.5f))
        }
        if (!p.mnt) p.ammo--; p.cd = 60f / d.rpm; p.flash = 0.06f; p.prot = 0f
        val moveF = if (p.moving) 1.6f else 1f; val ads = (i == meI) && adsHeld
        var sp = (if (ads) d.ads else d.hip * aHip) * moveF; if (p.crouch) sp *= 0.7f
        val eyeY = p.b.y + (if (p.crouch) PK.eyec else PK.eye)
        for (k in 0 until d.pellets) {
            var a = sp; if (p.bot) { a = (0.022f + 0.05f * 0.5f) * (if (diff == 0) 3.2f else if (diff == 1) 1.5f else 0.8f) }
            var yw = p.yaw + (frand() - 0.5f) * 2 * a; var pt = p.pitch + (frand() - 0.5f) * 2 * a
            if (d.pellets > 1) { yw += (frand() - 0.5f) * 0.09f; pt += (frand() - 0.5f) * 0.09f }
            val cp = cosf(pt); val dx = -sinf(yw) * cp; val dy = sinf(pt); val dz = -cosf(yw) * cp
            val tw = raycastMap(w, p.b.x, eyeY, p.b.z, dx, dy, dz, d.r1 * 1.4f); var best = fmin(tw, d.r1 * 1.4f); var hv = -1; var hh = false
            for (j in 0 until e.size) {
                if (j == i || !e[j].alive || e[j].team == p.team) continue
                val h = rayPlayer(e[j].b.x, e[j].b.y, e[j].b.z, e[j].crouch, p.b.x, eyeY, p.b.z, dx, dy, dz, best); if (h.ok && h.t < best) { best = h.t; hv = j; hh = h.head }
            }
            if (air) for (jj in jets.indices) { val T = jets[jj]; if (!T.alive || T.team == p.team) continue; val px = T.x - p.b.x; val py = T.y + 1 - eyeY; val pz = T.z - p.b.z; val tc = px * dx + py * dy + pz * dz; if (tc < 0 || tc >= best) continue
                val qx = px - dx * tc; val qy = py - dy * tc; val qz = pz - dz * tc; if (qx * qx + qy * qy + qz * qz < 45f) { best = tc; hv = -1 } }
            run {
                var vhit: Veh? = null; var vpart = 0
                for (v in vh) {
                    if (v.dead) continue; if (v.driver == i) continue; if (v.driver >= 0 && e[v.driver].team == p.team) continue
                    val tv = VH.rayVeh(v, p.b.x, eyeY, p.b.z, dx, dy, dz, best, pOut); if (tv < best) { best = tv; vhit = v; vpart = pOut[0]; hv = -1 }
                }
                if (vhit != null && !ex) damageVeh(vhit, vpart, dmgAt(d, best) * aDm, i)
            }
            if (hv >= 0 && !ex) hurt(hv, i, dmgAt(d, best) * aDm * (if (hh) d.head else 1f), hh)
            if (ex && rks.size < 8) { rks.add(Rk(p.b.x + dx * 0.9f, eyeY - 0.15f + dy * 0.9f, p.b.z + dz * 0.9f, dx, dy, dz, fmax(0.5f, best - 0.9f), 0f, i.toFloat())) }
            if (best < d.r1 * 1.39f) sparkAt(p.b.x + dx * best, eyeY + dy * best, p.b.z + dz * best, dx, dy, dz, if (hv >= 0) 0 else 5)
            if (tr.size < 64) tr.add(Tracer(p.b.x + dx * 0.5f, eyeY - 0.12f, p.b.z + dz * 0.5f, p.b.x + dx * best, eyeY + dy * best, p.b.z + dz * best, 0.09f))
            if (role == 1 && ns.trOut.size < 12) ns.trOut.add(NetTr(p.b.x + dx * 0.5f, eyeY - 0.12f, p.b.z + dz * 0.5f, p.b.x + dx * best, eyeY + dy * best, p.b.z + dz * best))
        }
        if (i == meI) { p.rkP += d.recoilV * aRv * (if (ads) 0.6f else 1f) * 1.6f; p.rkY += (frand() - 0.5f) * d.recoilH * aRh * 3f }
    }
    fun reloadStart(p: Ent) {
        val d = WEAPONS[p.wpn]; if (p.mnt || p.reloadT >= 0 || p.ammo >= cap(p) || p.mags <= 0) return
        p.reloadT = 0f; if (p === e[meI]) Sfx.reload(GameData.wkind[p.wpn % WEAPON_COUNT], d.reload * p.relM, 0.8f)
    }
    fun updateWeapon(i: Int, dt: Float, trig: Boolean) {
        val p = e[i]; val d = if (p.mnt) GameData.mount!! else WEAPONS[p.wpn]; if (p.cd > 0) p.cd -= dt; if (p.flash > 0) p.flash -= dt
        if (p.mnt) { if (trig && p.cd <= 0) fire(i); return }
        if (p.reloadT >= 0) { p.reloadT += dt; if (p.reloadT >= d.reload * p.relM) { p.reloadT = -1f; p.mags--; p.ammo = cap(p) }; return }
        if (trig && p.cd <= 0) { if (p.ammo > 0) fire(i) else reloadStart(p) }
        if (p.ammo == 0 && p.mags <= 0 && i != 0) { p.mags = d.mags }
    }

    fun botThink(i: Int, dt: Float) {
        val p = e[i]; p.think -= dt
        if (p.think <= 0) {
            p.think = 0.25f + frand() * 0.25f; var best = -1; var bd = 1e9f; var vis = false
            for (j in 0 until e.size) {
                if (!e[j].alive || e[j].team == p.team) continue
                val dx = e[j].b.x - p.b.x; val dz = e[j].b.z - p.b.z; val dd = dx * dx + dz * dz
                if (dd < bd) { if (visible(p, e[j]) || best < 0) { best = j; bd = dd; vis = visible(p, e[j]) } }
            }
            if (best != p.target) { p.react = (if (diff == 0) 1.1f else if (diff == 1) 0.6f else 0.3f) }; p.target = best; p.sx = if (vis) 1f else 0f; p.sz = sqrtf(bd)
        }
    }
    fun updateBot(i: Int, dt: Float) {
        val p = e[i]; botThink(i, dt); p.react -= dt; p.strafeT -= dt; if (p.strafeT <= 0) { p.strafeT = 0.8f + frand() * 1.5f; p.strafe = if (frand() < 0.5f) -1f else 1f }
        var mx = 0f; var mz = 0f; var trig = false; p.sprint = false
        if (p.target >= 0 && e[p.target].alive) {
            val t2 = e[p.target]
            val dx = t2.b.x - p.b.x; val dz = t2.b.z - p.b.z; val d = sqrtf(dx * dx + dz * dz); val want = atan2f(-dx, -dz)
            val er = if (WEAPONS[p.wpn].r0 > 1) (if (p.wpn == 2) 9f else if (p.wpn == 1) 14f else 22f) else 22f; val vis = p.sx > 0.5f
            val dy = wrapPi(want - p.yaw); p.yaw += dy * fmin(1f, dt * (if (vis) (if (diff == 0) 3.5f else if (diff == 1) 6f else 10f) else 3f))
            val pitchW = atan2f((t2.b.y + 1.3f) - (p.b.y + PK.eye), fmax(1f, d)); p.pitch += (pitchW - p.pitch) * fmin(1f, dt * 8)
            if (vis && d < er) {
                mx = p.strafe * 0.6f; mz = if (d < er * 0.4f) 0.4f else (if (d > er * 0.8f) -0.4f else 0f)
                if (p.react <= 0 && fabsf(dy) < 0.25f && d < WEAPONS[p.wpn].r1 * 1.1f) { p.burst -= dt; if (p.burst <= -0.35f) p.burst = 0.35f + frand() * 0.5f; trig = p.burst > 0 }
            } else { mz = -1f; p.sprint = !vis && d > 20 }
        } else mz = -0.5f
        val fxw = -sinf(p.yaw); val fzw = -cosf(p.yaw); val rxw = cosf(p.yaw); val rzw = -sinf(p.yaw)
        var vx = (-mz) * fxw + mx * rxw; var vz = (-mz) * fzw + mx * rzw
        val sp = (if (p.sprint) PK.sprint else PK.walk) * (if (trig) 0.6f else 1f) * WEAPONS[p.wpn].move; val vl = sqrtf(vx * vx + vz * vz)
        if (vl > 0.01f) { vx = vx / vl * sp; vz = vz / vl * sp }
        if (p.turnT > 0) { p.turnT -= dt; val c = cosf(p.turnDir * 1.2f); val s = sinf(p.turnDir * 1.2f); val nx = vx * c - vz * s; val nz = vx * s + vz * c; vx = nx; vz = nz }
        val la = 0.9f; var jump = false
        if (vl > 0.01f && blocked(w, p.b.x + vx / sp * la, p.b.y, p.b.z + vz / sp * la, PK.h)) {
            if (p.turnT <= 0) { p.turnT = 0.5f + frand() * 0.6f; p.turnDir = if (frand() < 0.5f) -1f else 1f }; jump = frand() < 0.03f
        }
        val ox = p.b.x; val oz = p.b.z; moveBody(w, p.b, vx, vz, dt, jump, false)
        val mv = hypotf(p.b.x - ox, p.b.z - oz); p.moving = mv > 0.001f; p.phase += mv * 3.4f
        if (vl > 0.01f && mv < sp * dt * 0.3f) { p.stuck += dt; if (p.stuck > 0.6f) { p.turnT = 0.8f + frand(); p.turnDir = if (frand() < 0.5f) -1f else 1f; p.stuck = 0f } } else p.stuck = 0f
        updateWeapon(i, dt, trig); if (p.ammo <= cap(p) / 6 && !trig) reloadStart(p)
    }

    fun updatePlayer(dt: Float, inp: Input) {
        val p = e[meI]; adsHeld = inp.ads
        p.mnt = drive >= 0 && drive < vh.size && vh[drive].type == 0 && gunner; p.mvi = if (p.mnt) drive else -1
        val zoom = if (inp.ads) zoomOf(p) else 1f; val sensL = 1f / (if (inp.ads) fmax(1f, zoom * 0.8f) else 1f)
        p.yaw -= inp.lx * dt * sensL * this.sens; p.pitch += inp.ly * dt * sensL * this.sens; p.yaw += p.rkY * fmin(1f, dt * 30); p.pitch += p.rkP * fmin(1f, dt * 30); p.rkP *= 0.8f; p.rkY *= 0.8f
        p.pitch = fmax(-1.45f, fmin(1.45f, p.pitch))
        if (drive >= 0) {
            if (fabsf(inp.lx) + fabsf(inp.ly) < 0.05f) { lookIdle += dt; if (lookIdle > 1.0f) { val dy = wrapPi(vh[drive].h - p.yaw); p.yaw += dy * fmin(1f, dt * 1.6f) } } else lookIdle = 0f
            if (passenger()) {
                p.pitch = fmax(-1.2f, fmin(1.2f, p.pitch)); p.crouch = true; p.sprint = false; p.moving = false; lookIdle = 0f
                if (inp.reload) reloadStart(p)
                if ((inp.swapP && !prev.swapP) || (inp.swapN && !prev.swapN)) { prim = (prim + (if (inp.swapN && !prev.swapN) 1 else WEAPON_COUNT - 1)) % WEAPON_COUNT; giveWeapon(p, prim) }
            else if (inp.pick in 0 until WEAPON_COUNT && inp.pick != prim) { prim = inp.pick; giveWeapon(p, prim) }
                var trg = inp.fire; if (!p.mnt && !WEAPONS[p.wpn].autof && prev.fire) trg = false; updateWeapon(meI, dt, trg); return
            }
            p.pitch = fmax(-0.7f, fmin(0.5f, p.pitch)); p.crouch = false; p.sprint = false; p.moving = false; adsHeld = false; return
        }
        var wantCrouch = inp.crouch; if (!wantCrouch && p.crouch && blocked(w, p.b.x, p.b.y, p.b.z, PK.h)) wantCrouch = true; p.crouch = wantCrouch
        var mx = inp.mx; var mz = inp.my; val ml = hypotf(mx, mz); if (ml > 1) { mx /= ml; mz /= ml }
        p.sprint = inp.sprint && mz > 0.3f && !inp.ads && !p.crouch
        val sp = (if (p.crouch) PK.crouch else (if (p.sprint) PK.sprint else PK.walk)) * WEAPONS[p.wpn].move * (if (inp.ads) 0.65f else 1f)
        val fxw = -sinf(p.yaw); val fzw = -cosf(p.yaw); val rxw = cosf(p.yaw); val rzw = -sinf(p.yaw)
        val vx = (mz * fxw + mx * rxw) * sp; val vz = (mz * fzw + mx * rzw) * sp; val ox = p.b.x; val oz = p.b.z
        moveBody(w, p.b, vx, vz, dt, inp.jump, p.crouch); val mv = hypotf(p.b.x - ox, p.b.z - oz); p.moving = mv > 0.001f && p.b.onGround; p.phase += mv * 3.4f
        if (inp.reload) reloadStart(p)
        if ((inp.swapP && !prev.swapP) || (inp.swapN && !prev.swapN)) { prim = (prim + (if (inp.swapN && !prev.swapN) 1 else WEAPON_COUNT - 1)) % WEAPON_COUNT; giveWeapon(p, prim) }
            else if (inp.pick in 0 until WEAPON_COUNT && inp.pick != prim) { prim = inp.pick; giveWeapon(p, prim) }
        var trig = inp.fire && !p.sprint; if (!WEAPONS[p.wpn].autof && prev.fire) trig = false
        updateWeapon(meI, dt, trig)
        run {
            val pl = inp.c4 && !pc4; val dtn = inp.det && !pdet; pc4 = inp.c4; pdet = inp.det
            if (pl && p.alive) { val cpt = fmin(p.pitch - 0.45f, -0.45f); val cp = cosf(cpt); val dx = -sinf(p.yaw) * cp; val dy = sinf(cpt); val dz = -cosf(p.yaw) * cp; c4Place(meI, dx, dy, dz) }
            if (dtn && p.alive) c4Det(meI)
        }
    }

    fun updateDrone(dt: Float, inp: Input, useIn: Input) {
        val me = e[meI]; if (dnCd > 0) dnCd -= dt; for (o in e) if (o.reveal > 0) o.reveal -= dt
        if (inp.drone && !pdn && me.alive && drive < 0) {
            if (dn.on) landDrone(10f) else if (dnCd > 0) { say("المسيّرة تُشحن ${ceil(dnCd).toInt()} ث") }
            else { dn = Drn(); dn.on = true; dn.x = me.b.x; dn.y = me.b.y + 2f; dn.z = me.b.z; dn.yaw = me.yaw; dn.t = 25f; dnSpot = 0f; Sfx.play(Sfx.EMPTY, 0.7f) }
        }
        pdn = inp.drone; val fe = inp.fire && !pfireD; pfireD = inp.fire
        if (!dn.on) return
        if (!me.alive || drive >= 0) { landDrone(40f); return }
        dn.t -= dt; if (dn.t <= 0) { landDrone(20f); say("نفدت بطارية المسيّرة"); return }
        dn.yaw -= inp.lx * dt * sens; dn.pitch = fmax(-1.4f, fmin(1.4f, dn.pitch + inp.ly * dt * sens))
        val cp = cosf(dn.pitch); val sp = sinf(dn.pitch); val fx = -sinf(dn.yaw) * cp; val fy = sp; val fz = -cosf(dn.yaw) * cp; val rx = cosf(dn.yaw); val rz = -sinf(dn.yaw)
        val up = (if (inp.jump) 1f else 0f) - (if (inp.crouch) 1f else 0f)
        val tx = (fx * inp.my + rx * inp.mx) * 12f; val ty = fy * inp.my * 12f + up * 6f; val tz = (fz * inp.my + rz * inp.mx) * 12f; val k = fmin(1f, 6f * dt)
        dn.vx += (tx - dn.vx) * k; dn.vy += (ty - dn.vy) * k; dn.vz += (tz - dn.vz) * k
        val dx = dn.vx * dt; val dy = dn.vy * dt; val dz = dn.vz * dt; val L = sqrtf(dx * dx + dy * dy + dz * dz)
        if (L > 1e-5f) {
            val h = raycastMap(w, dn.x, dn.y, dn.z, dx / L, dy / L, dz / L, L + 0.5f)
            if (h < L + 0.5f) { dn.vx = 0f; dn.vy = 0f; dn.vz = 0f } else {
                val nx = dn.x + dx; val ny = dn.y + dy; val nz = dn.z + dz
                val hw = w.m.W * 0.5f - 0.5f; val hd = w.m.D * 0.5f - 0.5f
                if (fabsf(nx) < hw && fabsf(nz) < hd && ny > 0.5f && ny < 30f && hypotf(nx - me.b.x, nz - me.b.z) <= 70f) { dn.x = nx; dn.y = ny; dn.z = nz } else { dn.vx = 0f; dn.vy = 0f; dn.vz = 0f }
            }
        }
        dnSpot -= dt
        if (dnSpot <= 0) {
            dnSpot = 0.6f
            for (o in e) {
                if (!o.alive || o.team == me.team) continue; val ex = o.b.x - dn.x; val ey = o.b.y + 1.2f - dn.y; val ez = o.b.z - dn.z; val d = sqrtf(ex * ex + ey * ey + ez * ez); if (d > 45f || d < 0.1f) continue
                if (raycastMap(w, dn.x, dn.y, dn.z, ex / d, ey / d, ez / d, d) >= d - 0.3f) o.reveal = 2.5f
            }
        }
        if (fe) {
            val gx = dn.x; val gy = dn.y; val gz = dn.z; landDrone(40f)
            for (i in 0 until e.size) {
                if (!e[i].alive || e[i].team == me.team) continue; val ex = e[i].b.x - gx; val ey = e[i].b.y + 1f - gy; val ez = e[i].b.z - gz; val d = sqrtf(ex * ex + ey * ey + ez * ez); if (d > 7f) continue
                val h = raycastMap(w, gx, gy + 0.2f, gz, ex / (d + 1e-4f), (ey - 0.2f) / (d + 1e-4f), ez / (d + 1e-4f), d); if (h < d - 0.3f) continue; hurt(i, meI, 140f * powf(1f - d / 7f, 1.2f), false)
            }
            for (v in vh) { if (v.dead) continue; val ex = v.x - gx; val ez = v.z - gz; val d = sqrtf(ex * ex + ez * ez); if (d < 7.5f) hurtVeh(v, 220f * (1f - d / 7.5f), meI) }
            puffExplosion(gx, gy, gz, 10, 5)
            sparkAt(gx, gy, gz, 0f, -1f, 0f, 30); Sfx.play(Sfx.EXPLODE, 0.8f)
        }
        useIn.mx = 0f; useIn.my = 0f; useIn.lx = 0f; useIn.ly = 0f
        useIn.fire = false; useIn.ads = false; useIn.jump = false; useIn.crouch = false; useIn.reload = false; useIn.swapP = false; useIn.swapN = false; useIn.grenade = false; useIn.sprint = false; useIn.tri = false
    }
    fun landDrone(cd: Float) { dn.on = false; dnCd = cd }

    fun updateGren(dt: Float, inp: Input) {
        val me = e[meI]
        if (inp.grenade && !pg && gren > 0 && me.alive && drive < 0) {
            gren--; val cp = cosf(me.pitch + 0.25f)
            grs.add(Gr(me.b.x, me.b.y + 1.5f, me.b.z, -sinf(me.yaw) * cp * 17f, sinf(me.pitch + 0.25f) * 17f + 2f, -cosf(me.yaw) * cp * 17f, 2.2f)); Sfx.play(Sfx.EMPTY, 0.6f)
        }
        pg = inp.grenade
        run { var k = 0; while (k < shells.size) { val c = shells[k]; c.t -= dt; if (c.y > 0.03f) { c.vy -= 18f * dt; c.x += c.vx * dt; c.y += c.vy * dt; c.z += c.vz * dt; if (c.y < 0.03f) { c.y = 0.03f; c.vx = 0f; c.vz = 0f } }; if (c.t <= 0) shells.removeAt(k) else k++ } }
        run {
            var k = 0
            while (k < rks.size) {
                val q = rks[k]; q.t += dt * 115f
                if (q.t >= q.len) { val gx = q.ox + q.dx * q.len; val gy = q.oy + q.dy * q.len; val gz = q.oz + q.dz * q.len; explosionAt(gx, gy, gz, q.by.toInt(), 6.5f, WEAPONS[8].dmg, 340f); rks.removeAt(k); continue }
                if (frand() < 0.7f) { val a = q.ox + q.dx * (q.t - 0.6f); val b = q.oy + q.dy * (q.t - 0.6f); val c = q.oz + q.dz * (q.t - 0.6f); val d = 0.5f + frand() * 0.5f; val f = 0.5f + frand() * 0.3f; val g = 0.5f + frand() * 0.4f; puffs.add(Puff(a, b, c, d, f, g, false)) }
                k++
            }
        }
        var k = 0
        while (k < grs.size) {
            val g = grs[k]; g.t -= dt; g.vy -= 22f * dt
            val dx = g.vx * dt; val dy = g.vy * dt; val dz = g.vz * dt; val L = sqrtf(dx * dx + dy * dy + dz * dz)
            if (L > 1e-5f) {
                val h = raycastMap(w, g.x, g.y, g.z, dx / L, dy / L, dz / L, L + 0.15f)
                if (h < L + 0.15f) {
                    val tx = g.x + dx / L * h; val ty = g.y + dy / L * h; val tz = g.z + dz / L * h; g.x = tx - dx / L * 0.12f; g.y = ty - dy / L * 0.12f; g.z = tz - dz / L * 0.12f
                    if (fabsf(dy) >= fabsf(dx) && fabsf(dy) >= fabsf(dz)) { g.vy = -g.vy * 0.4f; g.vx *= 0.7f; g.vz *= 0.7f; if (fabsf(g.vy) < 1.5f) g.vy = 0f }
                    else if (fabsf(dx) >= fabsf(dz)) g.vx = -g.vx * 0.5f else g.vz = -g.vz * 0.5f
                } else { g.x += dx; g.y += dy; g.z += dz }
            }
            if (g.y < 0.15f) { g.y = 0.15f; if (g.vy < 0) g.vy = -g.vy * 0.35f; g.vx *= 0.9f; g.vz *= 0.9f }
            if (g.t <= 0) {
                val gx = g.x; val gy = g.y; val gz = g.z
                for (i in 0 until e.size) { if (!e[i].alive) continue; val ex = e[i].b.x - gx; val ey = e[i].b.y + 0.9f - gy; val ez = e[i].b.z - gz; val dd = sqrtf(ex * ex + ey * ey + ez * ez); if (dd < 6.5f) hurt(i, meI, 150f * (1f - dd / 6.5f) + 10f, false) }
                for (v in vh) { if (v.dead) continue; val ex = v.x - gx; val ez = v.z - gz; val dd = sqrtf(ex * ex + ez * ez); if (dd < 7f) hurtVeh(v, 220f * (1f - dd / 7f), meI) }
                puffExplosion(gx, gy, gz, 10, 5)
                sparkAt(gx, gy, gz, 0f, -1f, 0f, 30)
                run { val ex = gx - me.b.x; val ez = gz - me.b.z; Sfx.play(Sfx.EXPLODE, fmax(0.25f, 1f - sqrtf(ex * ex + ez * ez) / 60f)) }
                grs.removeAt(k); continue
            }
            k++
        }
    }

    // ---- attachments ----
    fun optList(wi0: Int, o: IntArray): Int {
        val L = arrayOf(intArrayOf(0, 1, 2, 3), intArrayOf(0, 1, 2, -1), intArrayOf(0, 1, -1, -1), intArrayOf(3, 4, -1, -1), intArrayOf(0, 1, 2, 3), intArrayOf(1, 2, 3, 4), intArrayOf(0, 1, -1, -1), intArrayOf(0, 1, -1, -1), intArrayOf(0, -1, -1, -1))
        var wi = wi0; if (wi < 0 || wi > 8) wi = 0; var n = 0; for (i in 0 until 4) if (L[wi][i] >= 0) o[n++] = L[wi][i]; return n
    }
    fun applyWAtt(p: Ent) {
        if (p.wpn < 0 || p.wpn >= 16) return; val a = wAtt[p.wpn]
        if (p === e[meI]) { val Z = floatArrayOf(0f, 1.5f, 2.2f, 3.6f, 6f); optZ = Z[a.optic] }
        p.magM = if (a.mag == 1) 1.5f else 1f; p.relM = if (a.mag == 1) 1.12f else if (a.mag == 2) 0.72f else 1f; if (p.ammo > cap(p)) p.ammo = cap(p)
    }
    fun cycleAtt(what: Int) {
        val p = e[meI]; val a = wAtt[p.wpn and 15]
        if (what == 0) { val o = IntArray(4); val n = optList(p.wpn, o); var k = 0; for (i in 0 until n) if (o[i] == a.optic) k = i; a.optic = o[(k + 1) % n] }
        else if (what == 1) a.muz = (a.muz + 1) % 3 else if (what == 2) a.under = (a.under + 1) % 3 else a.mag = (a.mag + 1) % 3
        applyWAtt(p)
    }

    // ---- main update ----
    fun update(dt: Float, inp: Input) {
        frameCount++
        if (role != 0 || menu == 2 || ns.joining) netPump(dt)
        if (menu == 1) {
            val up = inp.my > 0.5f && prev.my <= 0.5f; val dnn = inp.my < -0.5f && prev.my >= -0.5f; val rt = inp.mx > 0.5f && prev.mx <= 0.5f; val lf = inp.mx < -0.5f && prev.mx >= -0.5f
            if (up) menuRow = (menuRow + 2) % 3; if (dnn) menuRow = (menuRow + 1) % 3; val d = if (rt) 1 else if (lf) -1 else 0
            if (d != 0) { if (menuRow == 0) menuSel = (menuSel + d + MAP_COUNT) % MAP_COUNT else if (menuRow == 1) diff = (diff + d + 3) % 3 else mode = (mode + d + modeCount) % modeCount }
            if (menuSel == 8) mode = 0
            if (msgT > 0) msgT -= dt
            if ((inp.confirm && !prev.confirm) || (inp.start && !prev.start)) { fovMul = 1f; hudK = 1f; if (mode == 1) hostStart() else if (mode == 2) lobbyStart() else { menu = 0; startMatch(menuSel) } }
            prev.set(inp); return
        }
        if (menu == 2) {
            val up = inp.my > 0.5f && prev.my <= 0.5f; val dnn = inp.my < -0.5f && prev.my >= -0.5f; if (msgT > 0) msgT -= dt
            if (ns.hosts.isNotEmpty()) { if (up) ns.hostSel = (ns.hostSel + ns.hosts.size - 1) % ns.hosts.size; if (dnn) ns.hostSel = (ns.hostSel + 1) % ns.hosts.size }
            if (inp.confirm && !prev.confirm && !ns.joining && ns.hosts.isNotEmpty()) joinAddr(ns.hosts[ns.hostSel].addr)
            if (inp.back && !prev.back) { netClose(); menu = 1 }
            prev.set(inp); return
        }
        if (inp.start && !prev.start) paused = !paused
        use.set(inp)
        if (paused) {
            if (inp.tri && !prev.tri) cycleAtt(0); if (inp.reload && !prev.reload) cycleAtt(1); if (inp.swapP && !prev.swapP) cycleAtt(2); if (inp.fire && !prev.fire) cycleAtt(3)
            if (inp.mx > 0.5f && prev.mx <= 0.5f) sens = fmin(2.5f, sens + 0.2f); if (inp.mx < -0.5f && prev.mx >= -0.5f) sens = fmax(0.3f, sens - 0.2f)
            if (inp.back && !prev.back) { if (role != 0) netClose(); menu = 1; paused = false; prev.set(inp); return }
            if (role == 0) { prev.set(inp); return }
            use.clear()
        }
        if (over && role != 2) { overT += dt; if ((inp.confirm && !prev.confirm && !paused) || (role == 1 && overT > 8f)) { overT = 0f; startMatch(mapIdx) }; prev.set(inp); return }
        if (over) { prev.set(inp); for (q in tr) q.t -= dt; tr.removeAll { it.t <= 0 }; return }
        run {
            val m0 = e[meI]
            if (m0.alive && m0.moving && drive < 0 && !dn.on && (m0.phase / (if (m0.sprint) 2.2f else 2.8f)).toInt() != (lastPh / (if (m0.sprint) 2.2f else 2.8f)).toInt()) Sfx.play(Sfx.STEP, if (m0.crouch) 0.12f else 0.3f, 0f)
            lastPh = m0.phase
            Sfx.engSpeed = if (drive >= 0 && drive < vh.size) fabsf(vh[drive].speed) else -1f; Sfx.droneVol = if (dn.on) 1f else 0f
        }
        t += dt; updateGren(dt, inp); if (msgT > 0) msgT -= dt; if (hitMark > 0) hitMark -= dt; if (dmgFlash > 0) dmgFlash -= dt; if (hitAngT > 0) hitAngT -= dt
        for (q in tr) q.t -= dt; tr.removeAll { it.t <= 0 }
        for (k in 0 until 4) if (feedT[k] > 0) feedT[k] -= dt
        for (k in sparks) { k.t -= dt; k.vy -= 9f * dt; k.x += k.vx * dt; k.y = fmax(0.02f, k.y + k.vy * dt); k.z += k.vz * dt }; sparks.removeAll { it.t <= 0 }
        updateDrone(dt, inp, use)
        if (role == 2) {
            val li = ns.lastIn; li.set(use)
            if (use.c4 && !netPc4) ns.c4pCnt = (ns.c4pCnt + 1) and 255; if (use.det && !netPdet) ns.c4dCnt = (ns.c4dCnt + 1) and 255; netPc4 = use.c4; netPdet = use.det
            for (p in e) if (p.prot > 0) p.prot -= dt
            clientUpdate(dt, use)
            for (o in e) if (o.revealT > 0) o.revealT -= dt
            for (q in puffs) { q.t -= dt; q.y += q.vy * dt; q.sz += dt * 0.8f }; puffs.removeAll { it.t <= 0 }
            prev.set(inp); return
        }
        if (air) airUpdate(dt, use) else for (i in 0 until e.size) {
            val p = e[i]
            if (p.prot > 0) p.prot -= dt
            if (!p.alive) { p.respawn -= dt; if (p.respawn <= 0) spawn(i); continue }
            if (i == meI) updatePlayer(dt, use) else if (p.remote) updateRemote(i, dt) else updateBot(i, dt)
        }
        if (!air) for (o in e) if (o.revealT > 0) o.revealT -= dt
        c4Tick(dt)
        if (!air) updateVeh(dt, use)
        prev.set(inp)
    }
}
