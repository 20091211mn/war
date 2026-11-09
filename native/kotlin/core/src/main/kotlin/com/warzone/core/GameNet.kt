package com.warzone.core

import java.net.InetAddress

/** اللعب الجماعي LAN (مضيف/عميل) — ترجمة Kotlin لبروتوكول game.h في إصدار C++ (PS4/آيفون). */

fun Game.netClose() {
    val n = ns
    if (role == 2 && n.sock.ready()) { val b = NetC.buf(2); b.put(NetC.T_BYE.toByte()); b.put(NetC.NET_VER.toByte()); n.hostAddr?.let { n.sock.send(it, NetC.PORT_GAME, b.array()) } }
    n.sock.close(); n.beaconRx.close(); n.beaconTx.close(); role = 0; meI = 0; n.joining = false
    for (c in n.cli) { c.on = false; c.ent = -1 }
}

fun Game.hostStart() {
    val n = ns; n.myIp = NetC.localIp(); n.sock.open(NetC.PORT_GAME, false); n.beaconTx.open(0, true)
    role = 1; meI = 0; for (c in n.cli) { c.on = false; c.ent = -1 }
    menu = 0; fovMul = 1f; hudK = 1f; startMatch(menuSel); say("استضافة لعبة LAN — بانتظار اللاعبين"); msgT = 8f; n.snapT = 0f; n.beaconT = 0f
}
fun Game.lobbyStart() { val n = ns; n.beaconRx.open(NetC.PORT_BEACON, true); menu = 2; n.hosts.clear(); n.hostSel = 0; n.joining = false; n.status = "" }
fun Game.joinAddr(a: InetAddress) { val n = ns; n.sock.open(0, false); n.hostAddr = a; n.joining = true; n.joinT = 0f; n.retryT = 1f; n.status = "جارٍ الاتصال…" }
fun Game.joinIp(ip: String): Boolean = try { joinAddr(InetAddress.getByName(ip)); true } catch (e: Exception) { false }

private fun Game.assignSlot(): Int {
    val hc = IntArray(2); for (p in e) if (p.remote) hc[p.team]++; hc[0] += 1
    val pref = if (hc[0] <= hc[1]) 0 else 1
    for (pass in 0 until 2) for (i in 1 until e.size) { val p = e[i]; if (!p.remote && p.bot && p.team == (if (pass == 1) 1 - pref else pref)) return i }
    return -1
}

fun Game.packSnap() {
    val n = ns; val ne = e.size; val nVeh = minOf(8, vh.size); val nC4 = minOf(6, c4s.size)
    val b = NetC.buf(1400)
    b.put(NetC.T_SNAP.toByte()); b.put(mapIdx.toByte()); b.put(n.matchId.toByte()); b.put(ne.toByte()); b.put((if (over) 1 else 0).toByte()); b.put(winner.toByte())
    b.putShort(score[0].toShort()); b.putShort(score[1].toShort()); b.put(n.evq.size.toByte()); b.put(n.trOut.size.toByte()); n.netSeq = (n.netSeq + 1) and 0xFFFF; b.putShort(n.netSeq.toShort()); b.put(nVeh.toByte()); b.put(nC4.toByte())
    for (i in 0 until ne) { val p = e[i]
        b.putFloat(p.b.x); b.putFloat(p.b.y); b.putFloat(p.b.z); b.putFloat(p.yaw); b.putFloat(p.pitch); b.putFloat(p.flash); b.putFloat(p.phase); b.putFloat(p.reloadT); b.putShort(p.hp.toInt().toShort())
        val fl = (if (p.alive) 1 else 0) or (if (p.crouch) 2 else 0) or (if (p.moving) 4 else 0) or (if (p.sprint) 8 else 0) or (if (p.team != 0) 16 else 0) or (if (p.remote || i == 0) 32 else 0) or (if (p.prot > 0) 64 else 0) or (if (p.revealT > 0) 128 else 0)
        b.put(fl.toByte()); b.put(p.wpn.toByte()); b.put(p.ammo.toByte()); b.put(p.mags.toByte()); b.put(p.spawnSeq.toByte()); b.putShort(p.kills.toShort()); b.putShort(p.deaths.toShort()); b.put(p.c4Left.toByte()) }
    for (v in n.evq) { b.put(v.id.toByte()); b.put(v.killer.toByte()); b.put(v.victim.toByte()); b.put(v.head.toByte()) }
    for (q in n.trOut) { b.putFloat(q.x0); b.putFloat(q.y0); b.putFloat(q.z0); b.putFloat(q.x1); b.putFloat(q.y1); b.putFloat(q.z1) }; n.trOut.clear()
    for (k in 0 until nVeh) { val v = vh[k]; b.putFloat(v.x); b.putFloat(v.z); b.putFloat(v.h); b.putFloat(v.speed); b.putFloat(v.steer); b.putShort(v.hp.toInt().toShort()); b.put(v.driver.toByte()); b.put(((if (v.dead) 1 else 0) or (if (v.burn) 2 else 0)).toByte()) }
    for (k in 0 until nC4) { val c = c4s[k]; b.putFloat(c.x); b.putFloat(c.y); b.putFloat(c.z); b.put(c.team.toByte()); b.put(c.owner.toByte()) }
    val len = b.position(); val arr = b.array()
    for (c in n.cli) if (c.on) c.addr?.let { n.sock.send(it, c.port, arr, len) }
}

private fun Game.applySnap(pk: Pkt) {
    val n = ns; if (pk.n < NetC.SZ_HEAD) return
    val b = NetC.wrap(pk.d); b.position(1)
    val map = NetC.u8(b); val mid = NetC.u8(b); val cnt = NetC.u8(b); val ov = NetC.u8(b); val win = NetC.u8(b); val s0 = b.short.toInt(); val s1 = b.short.toInt(); val nEvt = NetC.u8(b); val nTr = NetC.u8(b); b.short; val nVeh = NetC.u8(b); val nC4 = NetC.u8(b)
    val need = NetC.SZ_HEAD + cnt * NetC.SZ_ENT + nEvt * NetC.SZ_EVT + nTr * NetC.SZ_TR + nVeh * NetC.SZ_VEH + nC4 * NetC.SZ_C4
    if (pk.n < need || cnt > 16 || cnt < 2) return
    n.srvSilence = 0f
    if (map != mapIdx || e.size != cnt) { mapIdx = map % MAP_COUNT; w.load(mapIdx); e = ArrayList(cnt); for (i in 0 until cnt) { val p = Ent(); p.bot = false; p.alive = false; e.add(p) } }
    if (mid != n.matchId) { n.matchId = mid; tr.clear(); n.evq.clear() }
    over = ov != 0; winner = win; score[0] = s0; score[1] = s1
    for (i in 0 until cnt) {
        val x = b.float; val y = b.float; val z = b.float; val yaw = b.float; val pitch = b.float; val flash = b.float; val phase = b.float; val reloadT = b.float; val hp = b.short.toInt()
        val fl = NetC.u8(b); val wpn = NetC.u8(b); val ammo = NetC.u8(b); val mags = NetC.u8(b); val sseq = NetC.u8(b); val kills = NetC.u16(b); val deaths = NetC.u16(b); val c4 = NetC.u8(b)
        val p = e[i]; p.team = if (fl and 16 != 0) 1 else 0; if (fl and 128 != 0) p.revealT = 5f; p.remote = (fl and 32 != 0) && i != 0; val al = fl and 1 != 0; p.kills = kills; p.deaths = deaths; p.name = nm(i)
        if (i == meI) {
            p.c4Left = c4; if (hp < p.hp && al) dmgFlash = 0.5f; p.hp = hp.toFloat()
            if (sseq != p.spawnSeq || (al && !p.alive)) { p.spawnSeq = sseq; p.b.x = x; p.b.y = y; p.b.z = z; p.b.vy = 0f; p.yaw = yaw; p.pitch = 0f; p.crouch = false; prim = wpn; p.wpn = wpn }
            p.alive = al; p.prot = if (fl and 64 != 0) 1f else 0f
            if (wpn == p.wpn) { if (ammo < p.ammo && p.alive) { p.rkP += WEAPONS[p.wpn].recoilV * 1.6f; Sfx.shot(GameData.wkind[p.wpn % WEAPON_COUNT], false, 1f, 0f) }; p.ammo = ammo; p.mags = mags; p.reloadT = reloadT; p.flash = flash }
        } else {
            val first = !p.alive && al; p.alive = al; p.hp = hp.toFloat(); p.tx = x; p.ty = y; p.tz = z; p.tyaw = yaw; p.pitch = pitch; p.crouch = fl and 2 != 0; p.moving = fl and 4 != 0; p.sprint = fl and 8 != 0; p.wpn = wpn; p.flash = flash; p.phase = phase; p.ammo = ammo
            if (first || sseq != p.spawnSeq) { p.spawnSeq = sseq; p.b.x = x; p.b.y = y; p.b.z = z; p.yaw = yaw }
        }
    }
    for (k in 0 until nEvt) { val id = NetC.u8(b); val killer = NetC.u8(b); val victim = NetC.u8(b); val head = NetC.u8(b)
        if ((id - n.lastEvt).toByte() > 0) { n.lastEvt = id; if (killer == meI) hitMark = 0.25f; addFeed("${nm(killer)} > ${nm(victim)}${if (head != 0) " HS" else ""}") } }
    for (k in 0 until nTr) { val q = FloatArray(6) { b.float }; tr.add(Tracer(q[0], q[1], q[2], q[3], q[4], q[5], 0.09f)) }
    if (nVeh > 0 && vh.size != nVeh) { vh = ArrayList(); for (k in 0 until nVeh) vh.add(makeVeh(k)); drive = -1 }
    for (k in 0 until nVeh) { val x = b.float; val z = b.float; val h = b.float; val sp = b.float; val st = b.float; val hp = b.short.toInt(); val dr = b.get().toInt(); val fl = NetC.u8(b)
        if (k >= vh.size) continue; val v = vh[k]; val wasDead = v.dead; v.dead = fl and 1 != 0; v.burn = fl and 2 != 0; v.hp = hp.toFloat(); v.tx = x; v.tz = z; v.th = h
        v.speed = sp; v.steer = st; v.driver = dr; if (!wasDead && v.dead) puffsBoom(v.x, v.z); if (wasDead && !v.dead) { v.x = x; v.z = z; v.h = h }; if (v.dead) v.driver = -1 }
    run { val nc = ArrayList<C4s>(); for (k in 0 until nC4) { val x = b.float; val y = b.float; val z = b.float; val tm = NetC.u8(b); val ow = NetC.u8(b); nc.add(C4s(x, y, z, tm, ow)) }
        for (o in c4s) { var f = false; for (q in nc) if (fabsf(q.x - o.x) < 0.02f && fabsf(q.z - o.z) < 0.02f) f = true; if (!f && o.owner >= 0 && o.owner < e.size && e[o.owner].alive) blastAt(o.x, o.y, o.z, o.owner, o.team, 8f, 0f, 0f) }
        c4s.clear(); c4s.addAll(nc) }
}

fun Game.updateRemote(i: Int, dt: Float) {
    val p = e[i]; val n = p.rin ?: return; if (!n.valid) return
    if (!p.rc4Init) { p.rc4Init = true; p.rc4p = n.c4p; p.rc4d = n.c4d }
    else {
        if (n.c4p != p.rc4p) { p.rc4p = n.c4p; val cpt = fmin(n.pitch - 0.45f, -0.45f); val cp = cosf(cpt); c4Place(i, -sinf(n.yaw) * cp, sinf(cpt), -cosf(n.yaw) * cp) }
        if (n.c4d != p.rc4d) { p.rc4d = n.c4d; c4Det(i) }
    }
    p.b.x = fmax(-w.m.W / 2f, fmin(w.m.W / 2f, n.x)); p.b.y = n.y; p.b.z = fmax(-w.m.D / 2f, fmin(w.m.D / 2f, n.z)); p.yaw = n.yaw; p.pitch = n.pitch
    p.crouch = n.flags and 2 != 0; p.sprint = n.flags and 8 != 0; p.moving = n.flags and 16 != 0; if (p.moving) p.phase += dt * 8
    if (n.wpn < WEAPON_COUNT && n.wpn != p.wpn) giveWeapon(p, n.wpn)
    if (n.flags and 4 != 0) reloadStart(p); updateWeapon(i, dt, (n.flags and 1 != 0) && !p.sprint && n.veh == 0)
}

/** استقبال/إرسال الشبكة كل إطار. */
fun Game.netPump(dt: Float) {
    val n = ns
    if (role == 1) {
        if (n.sock.state < 0) { say("خطأ شبكة: ${n.sock.err}"); netClose(); menu = 1; return }
        while (true) { val pk = n.sock.recv() ?: break; if (pk.n < 2) continue; val ty = pk.d[0].toInt()
            var ci = -1; for (k in 0 until 8) if (n.cli[k].on && n.cli[k].addr == pk.addr && n.cli[k].port == pk.port) ci = k
            if (ty == NetC.T_HELLO) {
                if (ci < 0) { for (k in 0 until 8) { if (ci >= 0) break; if (!n.cli[k].on) { val en = assignSlot(); if (en < 0) break; ci = k; val c = n.cli[k]; c.on = true; c.addr = pk.addr; c.port = pk.port; c.ent = en
                    val p = e[en]; p.remote = true; p.bot = false; p.rin = null; p.rc4Init = false; spawn(en); p.name = nm(en); addFeed("${nm(en)} انضم") } } }
                val wl = NetC.buf(4); wl.put(NetC.T_WELCOME.toByte())
                if (ci >= 0) { n.cli[ci].last = 0f; wl.put(n.cli[ci].ent.toByte()); wl.put(mapIdx.toByte()); wl.put(e.size.toByte()) } else { wl.put((-1).toByte()); wl.put(0); wl.put(0) }
                n.sock.send(pk.addr, pk.port, wl.array())
            } else if (ty == NetC.T_BYE && ci >= 0) { val p = e[n.cli[ci].ent]; p.remote = false; p.bot = true; addFeed("${nm(n.cli[ci].ent)} غادر"); n.cli[ci].on = false; n.cli[ci].ent = -1 }
            else if (ty == NetC.T_INPUT && ci >= 0 && pk.n >= NetC.SZ_INPUT) {
                n.cli[ci].last = 0f; val b = NetC.wrap(pk.d); b.position(2); val seq = NetC.u16(b); val p = e[n.cli[ci].ent]; val cur = p.rin
                if (cur == null || !cur.valid || (seq - cur.seq).toShort() > 0) { val r = cur ?: NetIn(); r.valid = true; r.seq = seq; r.x = b.float; r.y = b.float; r.z = b.float; r.yaw = b.float; r.pitch = b.float
                    r.flags = NetC.u8(b); r.wpn = NetC.u8(b); r.vx = b.float; r.vz = b.float; r.vh = b.float; r.vsp = b.float; r.veh = NetC.u8(b); r.c4p = NetC.u8(b); r.c4d = NetC.u8(b); p.rin = r }
            }
        }
        for (c in n.cli) if (c.on) { c.last += dt; if (c.last > 5f) { val p = e[c.ent]; p.remote = false; p.bot = true; c.on = false; c.ent = -1 } }
        n.snapT += dt; if (n.snapT >= 0.05f) { n.snapT = 0f; packSnap() }
        n.beaconT += dt; if (n.beaconT >= 1f) { n.beaconT = 0f; val bc = NetC.buf(NetC.SZ_BEACON); bc.put(NetC.T_BEACON.toByte()); bc.put(mapIdx.toByte()); bc.put(humans().toByte()); bc.put(8)
            val nm0 = "KOTLIN HOST".toByteArray(); bc.put(nm0, 0, minOf(15, nm0.size))
            try { n.beaconTx.send(InetAddress.getByName("255.255.255.255"), NetC.PORT_BEACON, bc.array()); val ip = n.myIp; if (ip != null) n.beaconTx.send(InetAddress.getByAddress(byteArrayOf(ip[0], ip[1], ip[2], (-1).toByte())), NetC.PORT_BEACON, bc.array()) } catch (_: Exception) {} }
    } else if (role == 2 || n.joining) {
        if (n.sock.state < 0) { say("خطأ شبكة: ${n.sock.err}"); n.status = "خطأ شبكة"; netClose(); if (menu != 2) menu = 1; return }
        while (true) { val pk = n.sock.recv() ?: break; if (pk.n < 1) continue; val ty = pk.d[0].toInt()
            if (ty == NetC.T_WELCOME && n.joining && pk.n >= 4) { val slot = pk.d[1].toInt() and 255; val map = pk.d[2].toInt() and 255; val total = pk.d[3].toInt() and 255; n.joining = false
                if (slot == 255) { n.status = "المضيف ممتلئ"; n.sock.close() }
                else { role = 2; meI = slot; mapIdx = map % MAP_COUNT; w.load(mapIdx); e = ArrayList(total); for (i in 0 until total) { val p = Ent(); p.bot = false; p.alive = false; e.add(p) }
                    c4s.clear(); tr.clear(); vh = ArrayList(); drive = -1; score[0] = 0; score[1] = 0; menu = 0; n.srvSilence = 0f; n.inT = 0f; over = false; n.lastEvt = 0; n.evq.clear(); n.beaconRx.close(); say("تم الاتصال") } }
            else if (ty == NetC.T_SNAP && role == 2) applySnap(pk) }
        if (n.joining) { n.joinT += dt; n.retryT += dt
            if (n.sock.ready() && n.retryT > 0.4f && n.joinT < 8f) { n.retryT = 0f; val h = NetC.buf(2); h.put(NetC.T_HELLO.toByte()); h.put(NetC.NET_VER.toByte()); n.hostAddr?.let { n.sock.send(it, NetC.PORT_GAME, h.array()) } }
            if (n.joinT >= 8f) { n.joining = false; n.sock.close(); n.status = "لا رد من المضيف" } }
        if (role == 2) { n.srvSilence += dt; if (n.srvSilence > 4f) { netClose(); menu = 1; say("انقطع الاتصال"); return }
            n.inT += dt; if (n.inT >= 1f / 30) { n.inT = 0f; val p = e[meI]; val b = NetC.buf(NetC.SZ_INPUT); n.netSeq = (n.netSeq + 1) and 0xFFFF; val li = n.lastIn
                b.put(NetC.T_INPUT.toByte()); b.put(meI.toByte()); b.putShort(n.netSeq.toShort()); b.putFloat(p.b.x); b.putFloat(p.b.y); b.putFloat(p.b.z); b.putFloat(p.yaw); b.putFloat(p.pitch)
                b.put(((if (gunner) 128 else 0) or (if (li.fire) 1 else 0) or (if (p.crouch) 2 else 0) or (if (li.reload) 4 else 0) or (if (p.sprint) 8 else 0) or (if (p.moving) 16 else 0) or (if (li.ads) 32 else 0)).toByte()); b.put(prim.toByte())
                b.putFloat(0f); b.putFloat(0f); b.putFloat(0f); b.putFloat(0f); b.put(0); b.put(n.c4pCnt.toByte()); b.put(n.c4dCnt.toByte()); n.hostAddr?.let { n.sock.send(it, NetC.PORT_GAME, b.array()) } } }
    }
    if (menu == 2 && n.beaconRx.ready()) {
        while (true) { val pk = n.beaconRx.recv() ?: break; if (pk.n < NetC.SZ_BEACON || pk.d[0].toInt() != NetC.T_BEACON) continue
            val map = pk.d[1].toInt() and 255; val pl = pk.d[2].toInt() and 255; val mx = pk.d[3].toInt() and 255; var nl = 4; while (nl < 20 && pk.d[nl].toInt() != 0) nl++; val name = String(pk.d, 4, nl - 4)
            var f = false; for (h in n.hosts) if (h.addr == pk.addr) { h.map = map; h.players = pl; h.seen = 0f; f = true }
            if (!f) n.hosts.add(HostEnt(pk.addr, map, pl, mx, name)) }
        for (h in n.hosts) h.seen += dt; n.hosts.removeAll { it.seen > 4f }; if (n.hostSel >= n.hosts.size) n.hostSel = maxOf(0, n.hosts.size - 1)
    }
}

fun Game.humans(): Int { var c = 0; for (p in e) if (p.remote) c++; return c + (if (role == 1) 1 else 0) }

/** حركة العميل: لاعبه المحلي فقط + تنعيم الباقين والمركبات. */
fun Game.clientUpdate(dt: Float, use: Input) {
    for (i in 0 until e.size) { val p = e[i]; if (i == meI) { if (p.alive) updatePlayer(dt, use); continue }; if (p.gone) continue
        val k = fmin(1f, dt * 16f); p.b.x += (p.tx - p.b.x) * k; p.b.y += (p.ty - p.b.y) * k; p.b.z += (p.tz - p.b.z) * k
        var dy = p.tyaw - p.yaw; while (dy > 3.14159f) dy -= 6.28318f; while (dy < -3.14159f) dy += 6.28318f; p.yaw += dy * k; if (p.flash > 0) p.flash -= dt }
    val a = fmin(1f, dt * 14f)
    for (v in vh) { v.x += (v.tx - v.x) * a; v.z += (v.tz - v.z) * a; var dh = v.th - v.h; while (dh > 3.14159f) dh -= 6.28318f; while (dh < -3.14159f) dh += 6.28318f; v.h += dh * a }
    nearVeh = false
}
