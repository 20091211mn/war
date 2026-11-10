package com.warzone.core

import kotlin.math.*

/** حرب طائرات F-35 (خريطة 8): فيزياء طيران، مدفع، ذكاء اصطناعي، نزول بـ RPG وعودة للطائرة، فوز بتدمير 30 طائرة. */
class Jet {
    @JvmField var x = 0f; @JvmField var y = 0f; @JvmField var z = 0f; @JvmField var vx = 0f; @JvmField var vy = 0f; @JvmField var vz = 0f
    @JvmField var yaw = 0f; @JvmField var pitch = 0f; @JvmField var roll = 0f; @JvmField var th = 0.7f; @JvmField var hp = 300f
    @JvmField var fire = 0f; @JvmField var cdG = 0f; @JvmField var respT = 0f; @JvmField var aiT = 0f; @JvmField var ab = 0f; @JvmField var wob = 0f; @JvmField var evade = 0f; @JvmField var gunF = 0f
    @JvmField var team = 0; @JvmField var pilot = -1; @JvmField var gun = 600; @JvmField var tgt = -1; @JvmField var lastBy = -1
    @JvmField var alive = false; @JvmField var spare = false; @JvmField var gnd = false; @JvmField var stall = false
}

private fun clampA(v: Float, lo: Float, hi: Float): Float = if (v < lo) lo else if (v > hi) hi else v
private fun wrapPiA(a0: Float): Float { var a = a0; while (a > 3.14159f) a -= 6.28318f; while (a < -3.14159f) a += 6.28318f; return a }
private fun fr(): Float = Rnd.frand()

fun Game.jetSpeed(J: Jet): Float = sqrtf(J.vx * J.vx + J.vy * J.vy + J.vz * J.vz)
fun Game.myJet(): Int = if (air && meI >= 0 && meI < entJet.size) entJet[meI] else -1
fun Game.jetSay(s: String) { jetMsg = s; jetMsgT = 2.5f }

fun Game.jetNew(team: Int, airborne: Boolean, slot: Int): Int {
    var j = -1; for (k in jets.indices) if (!jets[k].alive && !jets[k].spare && jets[k].respT <= 0f) { j = k; break }
    val J = Jet(); J.team = team; J.alive = true; J.hp = 300f; J.pilot = -1; J.yaw = if (team != 0) 1.5708f else -1.5708f
    if (airborne) { J.x = (if (team != 0) 1 else -1) * (w.m.W * 0.5f - 150f - fr() * 70f); J.z = (fr() - 0.5f) * w.m.D * 0.5f; J.y = 110f + fr() * 60f; J.vx = -sinf(J.yaw) * 105f; J.vz = -cosf(J.yaw) * 105f; J.th = 0.7f }
    else { val sp = if (team != 0) w.m.blue else w.m.red; val s = sp[slot % sp.size]; J.x = s.x; J.z = s.z; J.y = 0f; J.th = 0f; J.spare = true; J.gnd = true }
    if (j < 0) { jets.add(J); j = jets.size - 1 } else jets[j] = J
    return j
}
fun Game.airInit() {
    jets.clear(); entJet = IntArray(e.size) { -1 }; footT = 0f; jetMsgT = 0f; rpgIdx = 0
    for (k in 0 until WEAPON_COUNT) if (WEAPONS[k].id == "rpg") rpgIdx = k
    for (i in 0 until e.size) airSpawn(i)
    for (k in 0 until 4) jetNew(k / 2, false, 6 + k % 2)
    jetSay("دمّر 30 طائرة معادية بالمدفع")
}
fun Game.airSpawn(i: Int) {
    val p = e[i]; val j = jetNew(p.team, true, 0); val J = jets[j]; J.pilot = i; entJet[i] = j
    p.alive = true; p.hp = 100f; p.prot = 2f; p.b.x = J.x; p.b.y = J.y; p.b.z = J.z; p.b.vy = 0f; p.yaw = J.yaw; p.pitch = 0f; p.mnt = false; p.reloadT = -1f
    if (i == meI) { adsHeld = false; drive = -1 }
}
fun Game.jetHurt(j: Int, dmg: Float, by: Int, inc: Boolean) {
    val J = jets[j]; if (!J.alive || dmg <= 0f) return; J.hp -= dmg; J.lastBy = by; if (inc && fr() < 0.3f) J.fire = fmax(J.fire, 5f)
    if (J.pilot == meI) { dmgFlash = 0.35f; Sfx.play(Sfx.HURT, 0.8f) }; if (by == meI) { hitMark = 0.25f; Sfx.play(Sfx.HIT, 0.6f) }
    if (J.hp <= 0f) jetDie(j, by)
}
fun Game.jetDie(j: Int, by: Int) {
    val J = jets[j]; if (!J.alive) return; J.alive = false; J.respT = if (J.spare) 25f else 0f; val ex = J.x; val ey = J.y; val ez = J.z
    for (k in 0 until 18) puffs.add(Puff(ex + (fr() - 0.5f) * 8, ey + (fr() - 0.5f) * 4, ez + (fr() - 0.5f) * 8, 1f + fr() * 3f, 1.4f + fr() * 1.4f, 2.5f + fr() * 3f, k < 11))
    sparkAt(ex, ey, ez, 0f, -1f, 0f, 40); run { val dd = sqrtf((ex - e[meI].b.x) * (ex - e[meI].b.x) + (ez - e[meI].b.z) * (ez - e[meI].b.z)); Sfx.play(Sfx.EXPLODE, fmax(0.25f, 1f - dd / 900f)) }
    val p = J.pilot; J.pilot = -1
    if (p >= 0) { val pe = e[p]; pe.alive = false; pe.deaths++; pe.respawn = 5f; entJet[p] = -1; if (p == meI) { streak = 0; jetSay("أُسقطت طائرتك!") } }
    if (by >= 0 && by < e.size && e[by].team != J.team) {
        val k = e[by]; k.kills++; score[k.team]++; addFeed("${nm(by)} > F-35 (${if (p >= 0) nm(p) else "طائرة"})")
        if (by == meI) { Sfx.play(Sfx.KILL, 0.8f); streak++; say("أسقطت طائرة! ${score[k.team]} / $scoreLim") }
        if (score[0] >= scoreLim || score[1] >= scoreLim) { over = true; winner = if (score[0] >= scoreLim) 0 else 1 }
    }
}

/** فيزياء الطيران: دفع + سحب + رفع يعتمد زاوية الهجوم (مع انهيار) + جاذبية + استقرار الأنف؛ الدوران يأتي من انحدار قوة الرفع. */
fun Game.jetStep(J: Jet, dt: Float, rc: Float, pc0: Float, yc: Float, thd: Float, ab: Boolean): Boolean {
    var pc = pc0
    val G = 9.8f; val KL = 0.00165f; val CD0 = 0.00065f; val KI = 0.00015f
    J.th = clampA(J.th + thd * 0.6f * dt, 0f, 1f); J.ab += ((if (ab && J.th > 0.9f) 1f else 0f) - J.ab) * fmin(1f, 4f * dt)
    val cp = cosf(J.pitch); val sp = sinf(J.pitch); val cy = cosf(J.yaw); val sy = sinf(J.yaw)
    val f0 = -sy * cp; val f1 = sp; val f2 = -cy * cp; val r00 = cy; val r01 = 0f; val r02 = -sy
    val u0x = r01 * f2 - r02 * f1; val u0y = r02 * f0 - r00 * f2; val u0z = r00 * f1 - r01 * f0
    val s = jetSpeed(J); val thrust = 15f * J.th + 12f * J.ab
    if (J.gnd) {
        var hs = sqrtf(J.vx * J.vx + J.vz * J.vz); val ax = thrust * 0.8f - 0.7f - CD0 * hs * hs; hs = fmax(0f, hs + ax * dt); if (J.th < 0.02f) hs = fmax(0f, hs - 6f * dt)
        J.yaw -= rc * 0.6f * fmin(1f, hs / 18f) * dt; J.roll *= expf(-6f * dt)
        if (hs > 55f && pc > 0.1f) J.pitch += pc * 0.5f * dt else J.pitch = fmax(0f, J.pitch * expf(-3f * dt)); J.pitch = clampA(J.pitch, 0f, 0.45f)
        val hy = J.yaw; J.vx = -sinf(hy) * hs; J.vz = -cosf(hy) * hs; val L = KL * hs * hs * fmin(1f, J.pitch * 5.2f + 0.05f); if (L > G) J.vy += (L - G) * dt else J.vy = 0f
        J.stall = false
    } else {
        val auth = clampA((s - 22f) / 60f, 0.12f, 1f)
        J.roll += rc * 2.4f * dt * auth; if (abs(rc) < 0.05f) J.roll -= J.roll * 0.8f * dt; J.roll = clampA(J.roll, -1.5f, 1.5f)
        val cr = cosf(J.roll); val sr = sinf(J.roll)
        val upx = u0x * cr - r00 * sr; val upy = u0y * cr - r01 * sr; val upz = u0z * cr - r02 * sr
        val rtx = r00 * cr + u0x * sr; val rty = r01 * cr + u0y * sr; val rtz = r02 * cr + u0z * sr
        val vf = J.vx * f0 + J.vy * f1 + J.vz * f2; val vu = J.vx * upx + J.vy * upy + J.vz * upz; val vr = J.vx * rtx + J.vy * rty + J.vz * rtz
        val aoa = atan2f(-vu, fmax(1f, vf)); var CL = aoa * 5.2f; J.stall = abs(aoa) > 0.30f || s < 40f
        if (abs(aoa) > 0.30f) { val sg = if (aoa > 0) 1f else -1f; CL = sg * fmax(0.4f, 0.30f * 5.2f - (abs(aoa) - 0.30f) * 6f) }
        run { val al = abs(aoa); if (al > 0.2f && ((pc > 0 && aoa > 0) || (pc < 0 && aoa < 0))) pc *= clampA((0.30f - al) / 0.10f, 0f, 1f) }
        val q = pc * 0.9f * auth; val ry = yc * 0.25f * auth
        var n0 = f0 + upx * q * dt + rtx * ry * dt; var n1 = f1 + upy * q * dt + rty * ry * dt; var n2 = f2 + upz * q * dt + rtz * ry * dt
        if (s > 15f) { val k = 1.3f * dt; n0 += (J.vx / s - f0) * k; n1 += (J.vy / s - f1) * k; n2 += (J.vz / s - f2) * k }
        if (s < 40f) n1 -= 0.55f * dt
        val nl = sqrtf(n0 * n0 + n1 * n1 + n2 * n2) + 1e-6f; n0 /= nl; n1 /= nl; n2 /= nl; J.pitch = clampA(asinf(clampA(n1, -1f, 1f)), -1.35f, 1.35f); J.yaw = atan2f(-n0, -n2)
        val L = KL * s * s * CL; val D = (CD0 + KI * CL * CL) * s * s
        var ax = f0 * thrust + upx * L - rtx * vr * 2.5f; var ay = f1 * thrust + upy * L - rty * vr * 2.5f - G; var az = f2 * thrust + upz * L - rtz * vr * 2.5f
        if (s > 0.5f) { ax -= J.vx / s * D; ay -= J.vy / s * D; az -= J.vz / s * D }; J.vx += ax * dt; J.vy += ay * dt; J.vz += az * dt
    }
    run { val ex = abs(J.x) - w.m.W * 0.5f; val ez = abs(J.z) - w.m.D * 0.5f
        if (ex > 0) J.vx -= (if (J.x > 0) 1f else -1f) * fmin(15f, ex * 0.3f) * dt; if (ez > 0) J.vz -= (if (J.z > 0) 1f else -1f) * fmin(15f, ez * 0.3f) * dt; if (J.y > 300f) J.vy -= (J.y - 300f) * 0.2f * dt + 20f * dt }
    J.x += J.vx * dt; J.y += J.vy * dt; J.z += J.vz * dt; val hx = w.m.W * 0.5f + 80f; val hz = w.m.D * 0.5f + 80f; J.x = clampA(J.x, -hx, hx); J.z = clampA(J.z, -hz, hz)
    if (J.y <= 0f) { val vd = -J.vy; if (!J.gnd && (vd > 8.5f || abs(J.roll) > 0.5f || (J.pitch < -0.35f && s > 30f))) return true; J.y = 0f; if (J.vy < 0) J.vy = 0f; J.gnd = true }
    else if (J.y > 0.4f) J.gnd = false
    if (J.y > 260f && J.vy > 0) J.vy -= 20f * dt
    if (J.y < 30f) { for (k in w.cellAt(J.x, J.z)) { val b = w.boxes[k]; if (J.x > b.x0 - 2 && J.x < b.x1 + 2 && J.z > b.z0 - 2 && J.z < b.z1 + 2 && J.y < b.y1 + 1.2f && b.y1 > 2f) return true } }
    return false
}

fun Game.jetGun(j: Int, aim: Int = -1) {
    val J = jets[j]; if (J.cdG > 0 || J.gun <= 0) return; J.cdG = 0.045f; J.gun--; val cp = cosf(J.pitch)
    var dx = -sinf(J.yaw) * cp + (fr() - 0.5f) * 0.016f; var dy = sinf(J.pitch) + (fr() - 0.5f) * 0.016f; var dz = -cosf(J.yaw) * cp + (fr() - 0.5f) * 0.016f; var l = sqrtf(dx * dx + dy * dy + dz * dz); dx /= l; dy /= l; dz /= l
    if (aim >= 0 && aim < jets.size && jets[aim].alive) { val T = jets[aim]; val ax = T.x + T.vx * 0.25f - J.x; val ay = T.y + T.vy * 0.25f - J.y; val az = T.z + T.vz * 0.25f - J.z; val al = sqrtf(ax * ax + ay * ay + az * az) + 1e-4f
        val er = if (diff == 0) 0.03f else if (diff == 1) 0.02f else 0.012f; dx = ax / al + (fr() - 0.5f) * er; dy = ay / al + (fr() - 0.5f) * er; dz = az / al + (fr() - 0.5f) * er; l = sqrtf(dx * dx + dy * dy + dz * dz); dx /= l; dy /= l; dz /= l }
    val ox = J.x + dx * 9f; val oy = J.y + dy * 9f - 0.3f; val oz = J.z + dz * 9f; var best = 700f; var hj = -1
    for (k in jets.indices) { val T = jets[k]; if (k == j || !T.alive || T.team == J.team) continue; val px = T.x - ox; val py = T.y + 1 - oy; val pz = T.z - oz; val tc = px * dx + py * dy + pz * dz; if (tc < 0 || tc >= best) continue
        val qx = px - dx * tc; val qy = py - dy * tc; val qz = pz - dz * tc; if (qx * qx + qy * qy + qz * qz < 45f) { best = tc; hj = k } }
    val tw = raycastMap(w, ox, oy, oz, dx, dy, dz, best); if (tw < best) { best = tw; hj = -1 }; if (oy + dy * best < 0f && dy < 0) best = fmax(1f, -oy / dy)
    if (tr.size < 64) { val d2 = fmin(best, 260f); tr.add(Tracer(ox, oy, oz, ox + dx * d2, oy + dy * d2, oz + dz * d2, 0.09f)) }
    if (hj >= 0) { jetHurt(hj, 16f, J.pilot, true); sparkAt(ox + dx * best, oy + dy * best, oz + dz * best, dx, dy, dz, 3) }
    if (J.pilot == meI || fr() < 0.15f) { val dd = sqrtf((J.x - e[meI].b.x) * (J.x - e[meI].b.x) + (J.z - e[meI].b.z) * (J.z - e[meI].b.z)); Sfx.shot(4, false, if (J.pilot == meI) 0.8f else fmax(0f, 0.6f - dd / 1200f), 0f) }
}

/** الطيار الآلي: أقرب هدف، انحدار نحوه، تفادي الأرض والحدود، إطلاق المدفع عند الاصطفاف. */
fun Game.jetAI(j: Int, dt: Float) {
    val J = jets[j]; J.aiT -= dt; val skill = if (diff == 0) 0.7f else if (diff == 1) 0.85f else 1f
    if (J.aiT <= 0f) { J.aiT = 0.6f; val keep = J.tgt >= 0 && jets[J.tgt].alive && jets[J.tgt].pilot >= 0 && J.wob > 0; J.wob -= 0.6f
        if (!keep) { J.wob = 7f; J.tgt = -1; var bs = 1e9f; for (k in jets.indices) { val T = jets[k]; if (!T.alive || T.team == J.team || T.pilot < 0) continue; val dx = T.x - J.x; val dy = T.y - J.y; val dz = T.z - J.z; val d = sqrtf(dx * dx + dy * dy + dz * dz); if (d < bs) { bs = d; J.tgt = k } } } }
    var tx = 0f; var ty = 140f; var tz = 0f; val have = J.tgt >= 0 && jets[J.tgt].alive; var dist = 1e9f; var cosA = -1f
    val cp = cosf(J.pitch); val fx = -sinf(J.yaw) * cp; val fy = sinf(J.pitch); val fz = -cosf(J.yaw) * cp
    if (have) { val T = jets[J.tgt]; val lead = fmin(1.2f, sqrtf((T.x - J.x) * (T.x - J.x) + (T.z - J.z) * (T.z - J.z)) / 300f); tx = T.x + T.vx * lead; ty = T.y + T.vy * lead; tz = T.z + T.vz * lead
        val dx = tx - J.x; val dy = ty - J.y; val dz = tz - J.z; dist = sqrtf(dx * dx + dy * dy + dz * dz); cosA = (dx * fx + dy * fy + dz * fz) / (dist + 1e-4f) }
    val W2 = w.m.W * 0.5f - 260f; val D2 = w.m.D * 0.5f - 220f; val oob = abs(J.x) > W2 || abs(J.z) > D2
    if (oob || !have) { tx = if (oob) 0f else tx; tz = if (oob) 0f else tz; if (!have && !oob) { tx = (if (J.team != 0) -1 else 1) * 200f; tz = 0f }; ty = fmax(ty, 130f) }
    ty = fmax(ty, 95f); val dx = tx - J.x; val dy = ty - J.y; val dz = tz - J.z; val hd = sqrtf(dx * dx + dz * dz) + 1e-3f; var ey = wrapPiA(atan2f(-dx, -dz) - J.yaw); var des = clampA(atan2f(dy, hd), -0.3f, 0.35f)
    if (J.y > 220f) des = fmin(des, -0.15f); if (J.y < 90f) des = fmax(des, 0.1f)
    val turnM = abs(ey) > 0.25f && J.y > 100f; val rmax = if (J.y < 100f) 0.5f else clampA(1.3f + J.vy * 0.02f, 0.8f, 1.45f)
    if (abs(ey) > 2.4f) { if (J.evade == 0f) J.evade = if (ey > 0) 1f else -1f; ey = J.evade * 2.4f } else J.evade = 0f
    val roll = clampA(-ey * 2.2f, -rmax, rmax); var rc = clampA((roll - J.roll) * 2.5f, -1f, 1f) * skill
    val cr = fmax(0.4f, cosf(J.roll)); var pc = (if (turnM) (if (abs(J.roll) > 0.7f) 1f else 0.4f) else clampA((des - J.pitch) * 2.2f + (1f / cr - 1f) * 0.6f, -1f, 1f)) * skill
    if (J.y > 190f) pc = fmin(pc, 0.3f - (J.y - 190f) * 0.02f); if (J.pitch < -0.45f) pc = 1f; if (J.pitch > 0.5f) pc = fmin(pc, -0.1f)
    if (J.vy < 0 && J.y < J.vy * J.vy / 20f + 70f && !J.gnd) { pc = 1f; rc = clampA(-J.roll * 2f, -1f, 1f) }
    if (J.y < 22f) { pc = 1f; rc = 0f }
    val s = jetSpeed(J); var thd = if (s < 105f) 1f else if (s > 135f) -1f else 0f; val ab = have && dist > 700f && diff > 0
    if (J.stall && J.y > 120f && J.pitch > 0f) { pc = -0.4f; rc = 0f; thd = 1f }
    if (J.y < 60f && J.vy < -12f) J.vy *= 1f - 3f * dt
    if (jetStep(J, dt, rc, pc, 0f, thd, ab)) { jetDie(j, -1); return }
    if (J.y < 24f && !J.gnd) { J.y = 24f; if (J.vy < 0) J.vy = 0f; if (J.pitch < 0) J.pitch *= 0.5f }
    if (have && dist < 650f && cosA > 0.8f && fr() < (if (diff == 0) 0.5f else 0.9f)) jetGun(j, J.tgt)
}

fun Game.airUpdate(dt: Float, inp: Input) {
    if (jetMsgT > 0) jetMsgT -= dt; val triE = inp.tri && !airPrev.tri; airPrev.set(inp)
    for (i in 0 until e.size) {
        val p = e[i]; if (p.prot > 0) p.prot -= dt
        if (!p.alive) { p.respawn -= dt; if (p.respawn <= 0f) airSpawn(i); continue }
        val j = entJet[i]
        if (i == meI && j < 0) { // على الأرض: قاذفة RPG
            adsHeld = inp.ads; updatePlayer(dt, inp); footT += dt
            var best = -1; var bd = 18f; for (k in jets.indices) { val J = jets[k]; if (!J.alive || J.pilot >= 0 || J.team != p.team || !J.gnd || jetSpeed(J) > 8f) continue; val d = sqrtf((J.x - p.b.x) * (J.x - p.b.x) + (J.z - p.b.z) * (J.z - p.b.z)); if (d < bd) { bd = d; best = k } }
            if (best >= 0 && triE) { jets[best].pilot = i; entJet[i] = best; jets[best].th = 0f; jets[best].vx = 0f; jets[best].vz = 0f; footT = 0f; adsHeld = false; jetSay("ارفع الدفع (THR+) ثم اسحب الذراع للإقلاع"); continue }
            if (footT > 10f && best < 0) { val nj = jetNew(p.team, false, 0); val J = jets[nj]; J.x = p.b.x + cosf(p.yaw) * 10f; J.z = p.b.z - sinf(p.yaw) * 10f; J.y = 0f; J.yaw = p.yaw; footT = 0f; jetSay("وصلت طائرة بديلة بجانبك") }
            continue
        }
        if (j < 0) continue
        val J = jets[j]
        if (i == meI) {
            adsHeld = false
            if (triE) { val onGnd = J.gnd && jetSpeed(J) < 10f; val rx = cosf(J.yaw); val rz = -sinf(J.yaw); J.pilot = -1; entJet[i] = -1; J.th = 0f
                p.b.vy = 0f; if (onGnd) { p.b.x = J.x + rx * 9f; p.b.z = J.z + rz * 9f; p.b.y = 0.1f; J.vx = 0f; J.vz = 0f; jetSay("نزلت من الطائرة — لديك RPG بخمس طلقات") } else { p.b.x = J.x; p.b.z = J.z; p.b.y = 0.1f; jetSay("قفزت بالمظلة — الطائرة تحلّق بلا طيار") }
                p.yaw = J.yaw; p.pitch = 0f; giveWeapon(p, rpgIdx); p.ammo = WEAPONS[rpgIdx].mag; p.mags = 5; footT = 0f; continue }
            val rc = inp.mx; val pc = inp.my; val yc = clampA(inp.lx * 0.4f, -1f, 1f); val thd = (if (inp.jump) 1f else 0f) - (if (inp.crouch) 1f else 0f)
            if (jetStep(J, dt, rc, pc, yc, thd, inp.ads)) { jetDie(j, -1); continue }
            if (inp.fire) jetGun(j)
        } else jetAI(j, dt)
        if (!J.alive) continue
        p.b.x = J.x; p.b.y = J.y; p.b.z = J.z; p.yaw = J.yaw; p.pitch = J.pitch; p.hp = J.hp / 3f; p.moving = false
    }
    for (j in jets.indices) { val J = jets[j]
        if (!J.alive) { if (J.spare && J.respT > 0) { J.respT -= dt; if (J.respT <= 0) jetNew(J.team, false, 6 + (j and 1)) }; continue }
        if (J.cdG > 0) J.cdG -= dt else if (J.gun < 600) J.gunF += dt * (if (J.pilot == meI) 10f else 40f); if (J.gunF >= 1f) { val add = J.gunF.toInt(); J.gun = min(600, J.gun + add); J.gunF -= add }
        if (J.pilot < 0 && !(J.gnd && jetSpeed(J) < 1f)) { if (jetStep(J, dt, 0f, 0f, 0f, 0f, false)) { jetDie(j, -1); continue } }
        if (J.fire > 0) { J.fire -= dt; J.hp -= 12f * dt; if (fr() < dt * 14f) puffs.add(Puff(J.x + (fr() - 0.5f) * 2, J.y, J.z + (fr() - 0.5f) * 2, 1.4f, 1f, 1.2f, fr() < 0.4f)); if (J.hp <= 0) jetDie(j, J.lastBy) }
    }
    for (q in puffs) { q.t -= dt; q.y += q.vy * dt; q.sz += dt * 0.8f }; puffs.removeAll { it.t <= 0 }
    for (o in e) if (o.revealT > 0) o.revealT -= dt
}
