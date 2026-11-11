package com.warzone.kotlin

import android.graphics.*
import com.warzone.core.Lang
import com.warzone.core.Game
import com.warzone.core.myJet
import com.warzone.core.jetSpeed
import kotlin.math.*

object HudDiag { @Volatile var line = ""; @Volatile var err: String? = null }

/** واجهة اللعبة: تُرسم بـ Canvas فوق صورة OpenGL (قائمة، شريط الحياة، الذخيرة، النتيجة، الأزرار). */
class Hud(private val g: Game, private val c: Controls) {
    private val p = Paint(Paint.ANTI_ALIAS_FLAG)
    private val tp = Paint(Paint.ANTI_ALIAS_FLAG).apply { typeface = Typeface.DEFAULT_BOLD; color = Color.WHITE }
    private val rect = RectF()
    private val MAPS = arrayOf("المستودع", "أطلال الصحراء", "شارع المدينة", "القاعدة القطبية", "صحراء الشاص", "أهرامات الجيزة", "المدينة القديمة", "ميناء الإسكندرية", "قاعدة جوية F-35")
    private val DIFF = arrayOf("سهل", "عادي", "صعب")
    private val RED = 0xFFE23B3B.toInt(); private val BLUE = 0xFF3B82F6.toInt(); private val ACC = 0xFF35D6FF.toInt()

    private fun text(cv: Canvas, s0: String, x: Float, y: Float, size: Float, col: Int = Color.WHITE, align: Paint.Align = Paint.Align.CENTER) {
        val s = com.warzone.core.Lang.t(s0)
        tp.textSize = size; tp.textAlign = align; tp.color = 0xFF000000.toInt(); tp.alpha = 150; cv.drawText(s, x + 1.5f, y + 1.5f, tp)
        tp.color = col; tp.alpha = 255; cv.drawText(s, x, y, tp)
    }
    private fun panel(cv: Canvas, l: Float, t: Float, r: Float, b: Float, fill: Int = 0xCC0B0F14.toInt(), stroke: Int = 0x55FFFFFF) {
        rect.set(l, t, r, b); p.style = Paint.Style.FILL; p.color = fill; cv.drawRoundRect(rect, 12f, 12f, p)
        p.style = Paint.Style.STROKE; p.strokeWidth = 2f; p.color = stroke; cv.drawRoundRect(rect, 12f, 12f, p)
    }

    fun draw(cv: Canvas, w: Float, h: Float) {
        val u = h / 540f; c.layout()
        diag(cv, w, h, u)
        if (g.menu == 1) { menu(cv, w, h, u); return }
        if (g.menu == 2) { lobby(cv, w, h, u); return }
        if (g.menu == 4) { room(cv, w, h, u); return }
        val me = g.e[g.meI]
        // وميض الضرر
        if (g.dmgFlash > 0) { p.style = Paint.Style.FILL; p.color = Color.argb((min(0.5f, g.dmgFlash) * 160).toInt(), 220, 0, 0); cv.drawRect(0f, 0f, w, h, p) }
        if (!g.paused) hud(cv, w, h, u, me)
        if (g.over) scoreboard(cv, w, h, u)
        if (g.paused) pause(cv, w, h, u)
        controls(cv, w, h, u)
        if (c.wheelOpen && !g.paused && g.menu == 0) wheel(cv, w, h, u)
    }

    private fun diag(cv: Canvas, w: Float, h: Float, u: Float) {
        text(cv, HudDiag.line, 8 * u, h - 6 * u, 11 * u, 0xFFB8FFB8.toInt(), Paint.Align.LEFT)
        HudDiag.err?.let { e -> p.style = Paint.Style.FILL; p.color = 0xE0200000.toInt(); cv.drawRect(0f, 0f, w, h, p); var y = 30 * u; for (ln in ("ERROR\n" + e).split("\n")) { text(cv, ln, 12 * u, y, 13 * u, 0xFFFF8080.toInt(), Paint.Align.LEFT); y += 17 * u } }
    }

    private fun hud(cv: Canvas, w: Float, h: Float, u: Float, me: com.warzone.core.Ent) {
        // التصويب
        val cx = w / 2; val cy = h / 2
        if (g.air) airHud(cv, w, h, u, me)
        val zm = g.zoomOf(me); val scoped = g.adsHeld && me.alive && g.drive < 0 && !g.dn.on && !(g.air && g.myJet() >= 0) && zm >= 2f
        if (scoped) { // المنظار: شعرة رفيعة خاصة به (الشعرة الديناميكية كانت تغطي العدو البعيد)
            p.style = Paint.Style.FILL
            if (zm >= 3.4f) { val rad = h * 0.33f; val pa = Path(); pa.fillType = Path.FillType.EVEN_ODD; pa.addRect(0f, 0f, w, h, Path.Direction.CW); pa.addCircle(cx, cy, rad, Path.Direction.CW); p.color = Color.BLACK; cv.drawPath(pa, p)
                p.style = Paint.Style.STROKE; p.strokeWidth = 1.2f * u; p.color = 0xE6000000.toInt(); cv.drawLine(cx - rad, cy, cx + rad, cy, p); cv.drawLine(cx, cy - rad, cx, cy + rad, p) }
            else { p.color = 0xF2FF2828.toInt(); cv.drawCircle(cx, cy, 1.6f * u, p); p.style = Paint.Style.STROKE; p.strokeWidth = 1f * u; p.color = 0xD9000000.toInt(); val rd = h / 12f; val gp2 = 7f * u
                cv.drawLine(cx - rd, cy, cx - gp2, cy, p); cv.drawLine(cx + gp2, cy, cx + rd, cy, p); cv.drawLine(cx, cy + gp2, cx, cy + rd, p); cv.drawLine(cx, cy - rd, cx, cy - gp2, p) }
        } else
        if (me.alive && !(g.air && g.myJet() >= 0) && (g.drive < 0 || g.passenger())) {
            p.style = Paint.Style.STROKE; p.strokeWidth = 2.2f * u; p.color = if (g.hitMark > 0) 0xFFFF4040.toInt() else 0xCCFFFFFF.toInt()
            val s = 9f * u; val gp = (4f + (if (me.moving) 3f else 0f) + (if (me.flash > 0) 3f else 0f)) * u
            cv.drawLine(cx - s - gp, cy, cx - gp, cy, p); cv.drawLine(cx + gp, cy, cx + s + gp, cy, p); cv.drawLine(cx, cy - s - gp, cx, cy - gp, p); cv.drawLine(cx, cy + gp, cx, cy + s + gp, p)
            if (g.hitMark > 0) { p.strokeWidth = 3f * u; val k = 12f * u; cv.drawLine(cx - k, cy - k, cx - 4 * u, cy - 4 * u, p); cv.drawLine(cx + k, cy - k, cx + 4 * u, cy - 4 * u, p); cv.drawLine(cx - k, cy + k, cx - 4 * u, cy + 4 * u, p); cv.drawLine(cx + k, cy + k, cx + 4 * u, cy + 4 * u, p) }
        }
        // النتيجة والوقت
        panel(cv, w / 2 - 110 * u, 6 * u, w / 2 + 110 * u, 50 * u)
        text(cv, "${g.score[0]}", w / 2 - 70 * u, 40 * u, 30 * u, RED); text(cv, "${g.score[1]}", w / 2 + 70 * u, 40 * u, 30 * u, BLUE)
        val left = max(0, (if (g.timeLim > 0) g.timeLim else 600) - g.t.toInt()); text(cv, String.format("%d:%02d", left / 60, left % 60), w / 2, 40 * u, 22 * u)
        // الحياة
        val hp = me.hp.coerceIn(0f, 100f)
        panel(cv, 10 * u, 8 * u, 170 * u, 38 * u)
        p.style = Paint.Style.FILL; p.color = if (hp > 35) 0xFF42D66B.toInt() else 0xFFE23B3B.toInt(); rect.set(16 * u, 14 * u, (16 + 148 * hp / 100f) * u, 32 * u); cv.drawRoundRect(rect, 6f, 6f, p)
        text(cv, "${hp.toInt()}", 92 * u, 29 * u, 16 * u)
        // الذخيرة والسلاح: بطاقة أسفل الشاشة (اضغطها لفتح عجلة الأسلحة)
        if (me.alive && g.drive < 0) weaponCard(cv, w, h, u, me)
        if (g.drive >= 0 && g.drive < g.vh.size) { text(cv, "المركبة ${g.vh[g.drive].hp.toInt()}", w / 2, h - 30 * u, 24 * u, ACC) }
        else if (g.nearVeh && me.alive) text(cv, "اضغط CAR لركوب المركبة", cx, h - 150 * u, 18 * u, ACC)
        if (!me.alive) text(cv, "أُصبت! عودة بعد ${max(0, me.respawn.toInt() + 1)}", cx, h * 0.45f, 28 * u, 0xFFFF6060.toInt())
        // الرسالة وسجل القتل
        if (g.msgT > 0) text(cv, g.msg, cx, h * 0.22f, 24 * u, ACC)
        for (i in 0 until 4) if (g.feedT[i] > 0 && g.feed[i].isNotEmpty()) {
            val a = (min(1f, g.feedT[i]) * 255).toInt(); tp.textSize = 14 * u; val fw = tp.measureText(g.feed[i]) + 22 * u; val x0 = w - fw - 10 * u; val y0 = 66 * u + i * 26 * u + (if (true) 0f else 0f)
            val mineK = g.feed[i].startsWith("YOU >")
            p.style = Paint.Style.FILL; p.color = 0xFF0A0E14.toInt(); p.alpha = (a * 0.75f).toInt(); rect.set(x0, y0, x0 + fw, y0 + 22 * u); cv.drawRoundRect(rect, 11 * u, 11 * u, p)
            p.color = if (mineK) ACC else 0xFF607080.toInt(); p.alpha = a; rect.set(x0 + 6 * u, y0 + 5 * u, x0 + 8 * u, y0 + 17 * u); cv.drawRect(rect, p); p.alpha = 255
            text(cv, g.feed[i], x0 + 14 * u, y0 + 16 * u, 14 * u, if (mineK) ACC else Color.WHITE, Paint.Align.LEFT) }
        compass(cv, w, u, me)
        if (me.alive && me.hp < 35f) { val pa = 0.22f + 0.14f * sin(g.t * 6f); p.style = Paint.Style.FILL
            for (k in 0 until 7) { p.color = Color.argb(((pa * (1 - k / 7f)) * 255).toInt(), 176, 16, 16); val d = k * 5 * u; cv.drawRect(0f, d, w, d + 5 * u, p); cv.drawRect(0f, h - d - 5 * u, w, h - d, p); cv.drawRect(d, 0f, d + 5 * u, h, p); cv.drawRect(w - d - 5 * u, 0f, w - d, h, p) } }
        if (g.hitAngT > 0) { val a = (min(1f, g.hitAngT * 1.5f) * 255).toInt(); p.style = Paint.Style.FILL; p.color = Color.argb(a, 255, 48, 48)
            for (k in -3..3) { val an = g.hitAng + k * 0.09f; cv.drawCircle(cx - sin(an) * h * 0.22f, cy - cos(an) * h * 0.22f, (if (k == 0) 5f else 3.5f) * u, p) } }
        // رادار صغير
        radar(cv, w, u, me)
    }

    private fun compass(cv: Canvas, w: Float, u: Float, me: com.warzone.core.Ent) {
        val bd = -me.yaw * 57.29578f; val px = 1.9f * u; val cw = 120 * px; val cx = w / 2; val cy = 62 * u
        p.style = Paint.Style.FILL; p.color = 0x8C0A0E14.toInt(); rect.set(cx - cw / 2 - 8 * u, cy - 4 * u, cx + cw / 2 + 8 * u, cy + 22 * u); cv.drawRoundRect(rect, 13 * u, 13 * u, p)
        for (k in floor((bd - 60) / 15).toInt()..ceil((bd + 60) / 15).toInt()) {
            val a = k * 15f; val off = a - bd; val fa = 1f - abs(off) / 62f; if (fa <= 0f) continue; val m = ((k * 15) % 360 + 360) % 360; val big = m % 45 == 0; val x = cx + off * px
            p.color = Color.WHITE; p.alpha = (fa * 200).toInt(); cv.drawRect(x, cy + (if (big) 0f else 5 * u), x + 1.5f * u, cy + (if (big) 10 * u else 8 * u), p); p.alpha = 255
            if (m % 90 == 0) text(cv, when (m) { 0 -> "N"; 90 -> "E"; 180 -> "S"; else -> "W" }, x, cy + 21 * u, 12 * u, if (m == 0) RED else Color.WHITE) }
        p.color = ACC; cv.drawRect(cx - 1 * u, cy - 6 * u, cx + 1 * u, cy - 1 * u, p)
    }
    private fun scoreboard(cv: Canvas, w: Float, h: Float, u: Float) {
        p.style = Paint.Style.FILL; p.color = 0x9E000000.toInt(); cv.drawRect(0f, 0f, w, h, p)
        val l = w * 0.15f; val t = h * 0.10f; val r = w * 0.85f; val b = h * 0.92f; panel(cv, l, t, r, b, 0xF0121A24.toInt())
        val win = g.winner; text(cv, if (win == 0) "فاز الفريق الأحمر" else "فاز الفريق الأزرق", w / 2, t + 36 * u, 30 * u, if (win == 0) RED else BLUE)
        text(cv, "${g.score[0]}  -  ${g.score[1]}", w / 2, t + 68 * u, 26 * u)
        var best = -1; var bk = -1; for (i in g.e.indices) if (g.e[i].kills > bk) { bk = g.e[i].kills; best = i }
        for (tm in 0..1) { val cw = (r - l) / 2; val x0 = l + tm * cw + 18 * u; val x1 = l + (tm + 1) * cw - 18 * u; var y = t + 98 * u; val col = if (tm == 0) RED else BLUE
            text(cv, "اللاعب", x0, y, 12 * u, 0xFF8FA0B4.toInt(), Paint.Align.LEFT); text(cv, "K", x1 - 70 * u, y, 12 * u, 0xFF8FA0B4.toInt(), Paint.Align.RIGHT); text(cv, "D", x1, y, 12 * u, 0xFF8FA0B4.toInt(), Paint.Align.RIGHT); y += 10 * u
            val ix = g.e.indices.filter { g.e[it].team == tm && !g.e[it].off }.sortedByDescending { g.e[it].kills }
            for (i in ix) { if (y > b - 66 * u) break; val me2 = i == g.meI
                p.style = Paint.Style.FILL; p.color = if (me2) 0xFF1F3340.toInt() else 0xFF121A24.toInt(); rect.set(x0 - 6 * u, y, x1 + 6 * u, y + 22 * u); cv.drawRoundRect(rect, 8 * u, 8 * u, p)
                p.color = col; cv.drawRect(x0 - 6 * u, y + 3 * u, x0 - 3 * u, y + 19 * u, p)
                text(cv, g.nm(i) + (if (i == best) "  MVP" else ""), x0 + 2 * u, y + 16 * u, 14 * u, if (me2) ACC else Color.WHITE, Paint.Align.LEFT)
                text(cv, "${g.e[i].kills}", x1 - 70 * u, y + 16 * u, 14 * u, Color.WHITE, Paint.Align.RIGHT); text(cv, "${g.e[i].deaths}", x1, y + 16 * u, 14 * u, 0xFF8FA0B4.toInt(), Paint.Align.RIGHT); y += 25 * u } }
    }
    private fun airHud(cv: Canvas, w: Float, h: Float, u: Float, me: com.warzone.core.Ent) {
        val cx = w / 2
        if (g.jetMsgT > 0) text(cv, g.jetMsg, cx, h / 3 + 14 * u, 18 * u, ACC)
        if (!me.alive || g.over) return
        val j = g.myJet()
        if (j < 0) {
            for (J in g.jets) { if (!J.alive || J.pilot >= 0 || J.team != me.team || !J.gnd) continue
                val d = hypot(J.x - me.b.x, J.z - me.b.z); if (d < 18f && g.jetSpeed(J) < 8f) { text(cv, "زر EXIT: ركوب الطائرة", cx, h * 0.62f, 18 * u, ACC); break } }
            return
        }
        val J = g.jets[j]; val s = g.jetSpeed(J); val r = g.r
        fun proj(x: Float, y: Float, z: Float, o: FloatArray): Boolean {
            val ddx = x - r.cam.x; val ddy = y - r.cam.y; val ddz = z - r.cam.z; val zc = ddx * r.fx + ddy * r.fy + ddz * r.fz; if (zc < 1f) return false
            val xc = ddx * r.rx + ddz * r.rz; val yc = ddx * r.ux + ddy * r.uy + ddz * r.uz
            o[0] = w / 2 + xc / zc * r.focal * (w / r.W); o[1] = h / 2 - yc / zc * r.focal * (h / r.H); o[2] = zc; return true
        }
        val o = FloatArray(3)
        p.style = Paint.Style.STROKE; p.strokeWidth = 2f * u
        for (k in g.jets.indices) { val T = g.jets[k]; if (!T.alive || k == j) continue; if (!proj(T.x, T.y, T.z, o)) continue
            if (o[0] < 10 || o[0] > w - 10 || o[1] < 10 || o[1] > h - 10) continue
            val en = T.team != J.team; p.color = if (en) 0xFFFF4A4A.toInt() else 0xFF60E080.toInt()
            val h2 = min(14 * u, max(3 * u, 28f * r.focal / o[2] / 3f)); cv.drawRect(o[0] - h2, o[1] - h2, o[0] + h2, o[1] + h2, p)
            if (en) text(cv, "${o[2].toInt()}m", o[0], o[1] + h2 + 12 * u, 11 * u, p.color) }
        if (s > 5f && proj(J.x + J.vx * 3f, J.y + J.vy * 3f, J.z + J.vz * 3f, o)) { p.color = 0xFF80FF80.toInt(); cv.drawCircle(o[0], o[1], 6 * u, p); cv.drawLine(o[0] - 12 * u, o[1], o[0] - 6 * u, o[1], p); cv.drawLine(o[0] + 6 * u, o[1], o[0] + 12 * u, o[1], p); cv.drawLine(o[0], o[1] - 12 * u, o[0], o[1] - 6 * u, p) }
        val px = cx - 90 * u; val py = h - 74 * u; panel(cv, px, py, px + 180 * u, py + 44 * u)
        text(cv, "${(s * 3.6f).toInt()} km/h", px + 50 * u, py + 28 * u, 16 * u); text(cv, "${J.y.toInt()} m", px + 138 * u, py + 28 * u, 16 * u)
        bar(cv, px, py - 12 * u, 180 * u, 7 * u, J.th, if (J.ab > 0.5f) 0xFFFF8A20.toInt() else ACC)
        text(cv, "HEI ${J.gun}", px + 180 * u + 36 * u, py + 26 * u, 14 * u, if (J.gun > 60) Color.WHITE else 0xFFFF6060.toInt())
        if (J.stall) text(cv, "STALL", cx, h / 2 - 34 * u, 26 * u, 0xFFFF4040.toInt()) else if (J.y < 45f && J.vy < -4f && !J.gnd) text(cv, "PULL UP", cx, h / 2 - 34 * u, 26 * u, 0xFFFF4040.toInt())
        if (abs(J.x) > g.w.m.W * 0.5f || abs(J.z) > g.w.m.D * 0.5f) text(cv, "عد إلى منطقة القتال", cx, h / 2 - 60 * u, 16 * u, 0xFFFFC040.toInt())
        if (J.gnd && s < 10f) text(cv, "EXIT: النزول من الطائرة", cx, h * 0.62f, 14 * u, 0xFFAAB4C0.toInt())
    }

    private fun radar(cv: Canvas, w: Float, u: Float, me: com.warzone.core.Ent) {
        val R = 52f * u; val cx = w - R - 70 * u; val cy = R + 8 * u
        p.style = Paint.Style.FILL; p.color = 0x990B0F14.toInt(); cv.drawCircle(cx, cy, R, p); p.style = Paint.Style.STROKE; p.color = 0x66FFFFFF; p.strokeWidth = 1.5f; cv.drawCircle(cx, cy, R, p)
        val sc = R / (if (g.air) 700f else 50f); val cs = cos(me.yaw); val sn = sin(me.yaw)
        p.style = Paint.Style.FILL
        for (i in g.e.indices) { val o = g.e[i]; if (i == g.meI || !o.alive || o.gone) continue
            val show = o.team == me.team || o.revealT > 0 || o.reveal > 0 || g.air; if (!show) continue
            val dx = o.b.x - me.b.x; val dz = o.b.z - me.b.z; val rx = dx * cs - dz * sn; val rz = dx * sn + dz * cs
            val px = cx + rx * sc; val py = cy + rz * sc; if ((px - cx) * (px - cx) + (py - cy) * (py - cy) > R * R) continue
            p.color = if (o.team == 0) RED else BLUE; cv.drawCircle(px, py, 3f * u, p) }
        p.color = Color.WHITE; val tri = Path(); tri.moveTo(cx, cy - 6 * u); tri.lineTo(cx - 4 * u, cy + 4 * u); tri.lineTo(cx + 4 * u, cy + 4 * u); tri.close(); cv.drawPath(tri, p)
    }

    private val QUAL = arrayOf("ضعيفة جداً", "ضعيفة", "متوسطة", "عالية", "فائقة"); private val LANGN = arrayOf("العربية", "English", "Español")
    private val MODES = arrayOf("لاعب واحد", "استضافة (LAN)", "انضمام (LAN)")
    private fun lobby(cv: Canvas, w: Float, h: Float, u: Float) {
        p.style = Paint.Style.FILL; p.color = 0xCC0B0F14.toInt(); cv.drawRect(0f, 0f, w, h, p)
        text(cv, "الانضمام إلى لعبة LAN", w / 2, h * 0.16f, 28 * u)
        val n = g.ns
        if (n.hosts.isEmpty()) text(cv, if (n.status.isNotEmpty()) n.status else "جارٍ البحث عن مضيف على نفس الشبكة…", w / 2, h * 0.40f, 18 * u, 0xFFB8C4D4.toInt())
        else if (n.status.isNotEmpty()) text(cv, n.status, w / 2, h * 0.23f, 16 * u, 0xFF8CE9FF.toInt())
        for (b in c.btns) { val l = b.x - b.w / 2; val t = b.y - b.h / 2
            if (b.id >= B.H0 && b.id < B.H0 + 5) { val hs = n.hosts.getOrNull(b.id - B.H0) ?: continue
                panel(cv, l, t, l + b.w, t + b.h, if (n.hostSel == b.id - B.H0) 0xFF3B82F6.toInt() else 0xDD18202C.toInt())
                text(cv, "${hs.name} — ${MAPS[hs.map % MAPS.size]} — ${hs.players}/${hs.maxp} — ${hs.lim5 * 5}K${if (hs.mins > 0) " ${hs.mins}د" else ""}${if (hs.style == 1) " مبارزة" else if (hs.bots > 0) " بوتات" else ""}", b.x, b.y + 7 * u, 16 * u) }
            else if (b.id == B.BACK) { panel(cv, l, t, l + b.w, t + b.h, if (c.held[B.BACK]) 0xFF3B82F6.toInt() else 0xDD18202C.toInt()); text(cv, "رجوع", b.x, b.y + 7 * u, 18 * u) } }
    }
    private fun room(cv: Canvas, w: Float, h: Float, u: Float) {
        p.style = Paint.Style.FILL; p.color = 0xCC0B0F14.toInt(); cv.drawRect(0f, 0f, w, h, p)
        text(cv, "إنشاء غرفة LAN", w / 2, h * 0.09f, 26 * u)
        val L = arrayOf("اسم الغرفة", "أقصى لاعبين", "حد النقاط", "الوقت", "البوتات", "النمط")
        val V = arrayOf(g.rmNameOf(g.rmName), "${g.rmMaxE()}", "${g.rmLimV(g.rmLimI)}", if (g.rmMinV(g.rmMinI) > 0) "${g.rmMinV(g.rmMinI)} min" else "بلا حد", if (g.rmBotsE() == 0) "بدون بوتات" else "${g.rmBotsE()}", if (g.rmStyle == 1) "مبارزة 1 ضد 1" else "فرق")
        for (b in c.btns) { val l = b.x - b.w / 2; val t = b.y - b.h / 2
            if (b.id >= B.RM0 && b.id < B.RM0 + 6) { val k = b.id - B.RM0; val sel = g.rmRow == k; val col = if (k and 1 == 1) BLUE else RED
                panel(cv, l, t, l + b.w, t + b.h, if (sel) 0xFF1F2A3A.toInt() else 0xDD18202C.toInt(), if (sel) col else 0x55FFFFFF)
                text(cv, L[k], l + b.w - 14 * u, b.y + 7 * u, 15 * u, 0xFF8FA0B4.toInt(), Paint.Align.RIGHT); text(cv, V[k], l + b.w * 0.30f, b.y + 8 * u, 20 * u, if (sel) Color.WHITE else 0xFFB8C4D4.toInt())
                if (sel) { text(cv, "◀", l + 20 * u, b.y + 7 * u, 18 * u, col); text(cv, "▶", l + b.w * 0.55f, b.y + 7 * u, 18 * u, col) } }
            else if (b.id == B.RMGO) { panel(cv, l, t, l + b.w, t + b.h, if (c.held[B.RMGO]) 0xFF8CE9FF.toInt() else ACC, 0xFFFFFFFF.toInt()); text(cv, "أنشئ الغرفة وابدأ", b.x, b.y + 9 * u, 24 * u, 0xFF04141A.toInt()) }
            else if (b.id == B.BACK) { panel(cv, l, t, l + b.w, t + b.h, if (c.held[B.BACK]) BLUE else 0xDD18202C.toInt()); text(cv, "رجوع", b.x, b.y + 7 * u, 18 * u) } }
    }
    private fun menu(cv: Canvas, w: Float, h: Float, u: Float) {
        p.style = Paint.Style.FILL; p.color = 0xB30B0F14.toInt(); cv.drawRect(0f, 0f, w, h, p)
        val cx = w * 0.30f
        text(cv, "WARZONE", w * 0.72f, h * 0.35f, 56 * u, 0xFFE23B3B.toInt()); text(cv, "WARZONE", w * 0.72f + 3 * u, h * 0.35f, 56 * u, BLUE); text(cv, "WARZONE", w * 0.72f + 1.5f * u, h * 0.35f, 56 * u)
        text(cv, "حرب فرق ثلاثية الأبعاد — الأحمر ضد الأزرق", w * 0.72f, h * 0.35f + 36 * u, 18 * u, 0xFFB8C4D4.toInt())
        text(cv, "نسخة Kotlin (OpenGL ES)", w * 0.72f, h * 0.35f + 62 * u, 15 * u, 0xFF8090A0.toInt())
        for (b in c.btns) {
            val l = b.x - b.w / 2; val t = b.y - b.h / 2
            when (b.id) {
                B.ROW_MAP -> { panel(cv, l, t, l + b.w, t + b.h); text(cv, "◀  ${MAPS[g.menuSel % MAPS.size]}  ▶", b.x, b.y + 8 * u, 22 * u); text(cv, "الخريطة", b.x, t - 6 * u, 14 * u, 0xFF8090A0.toInt()) }
                B.ROW_DIFF -> { panel(cv, l, t, l + b.w, t + b.h); text(cv, "◀  ${DIFF[g.diff % 3]}  ▶", b.x, b.y + 8 * u, 22 * u); text(cv, "الصعوبة", b.x, t - 6 * u, 14 * u, 0xFF8090A0.toInt()) }
                B.ROW_MODE -> { panel(cv, l, t, l + b.w, t + b.h); text(cv, "◀  ${MODES[g.mode % 3]}  ▶", b.x, b.y + 8 * u, 22 * u); text(cv, "الوضع", b.x, t - 6 * u, 14 * u, 0xFF8090A0.toInt()) }
                B.ROW_QUAL -> { panel(cv, l, t, l + b.w, t + b.h); text(cv, "◀  ${QUAL[Lang.quality % 5]}  ▶", b.x, b.y + 8 * u, 22 * u); text(cv, "الجودة", b.x, t - 6 * u, 14 * u, 0xFF8090A0.toInt()) }
                B.ROW_LANG -> { panel(cv, l, t, l + b.w, t + b.h); text(cv, "◀  ${LANGN[Lang.lang % 3]}  ▶", b.x, b.y + 8 * u, 22 * u); text(cv, "اللغة", b.x, t - 6 * u, 14 * u, 0xFF8090A0.toInt()) }
                B.START -> { panel(cv, l, t, l + b.w, t + b.h, if (c.held[B.START]) 0xFF8CE9FF.toInt() else ACC, 0xFFFFFFFF.toInt()); text(cv, "ابدأ اللعب", b.x, b.y + 9 * u, 26 * u, 0xFF04141A.toInt()) }
            }
        }
    }

    private fun pause(cv: Canvas, w: Float, h: Float, u: Float) {
        p.style = Paint.Style.FILL; p.color = 0xA00B0F14.toInt(); cv.drawRect(0f, 0f, w, h, p)
        text(cv, "إيقاف مؤقت", w / 2, h * 0.20f, 30 * u)
        text(cv, "الحساسية ${String.format("%.1f", g.sens)}", w / 2, h * 0.30f + 148 * u, 15 * u, 0xFFB8C4D4.toInt())
        for (b in c.btns) { val l = b.x - b.w / 2; val t = b.y - b.h / 2
            val lab = when (b.id) { B.PAUSE -> "متابعة"; B.BACK -> "القائمة الرئيسية"; B.SENS_DN -> "−"; B.SENS_UP -> "+"; B.P_OPT -> "منظار"; B.P_MUZ -> "فوهة"; B.P_GRIP -> "مقبض"; B.P_MAG -> "مخزن"; else -> b.label }
            panel(cv, l, t, l + b.w, t + b.h, if (c.held[b.id]) 0xFF3B82F6.toInt() else 0xDD18202C.toInt()); text(cv, lab, b.x, b.y + 7 * u, 18 * u) }
    }

    private fun controls(cv: Canvas, w: Float, h: Float, u: Float) {
        if (g.menu == 1 || g.menu == 4 || g.paused) return
        for (b in c.btns) {
            if (b.w > 0f) { if (b.id == B.OK) { val l = b.x - b.w / 2; val t = b.y - b.h / 2; panel(cv, l, t, l + b.w, t + b.h, if (c.held[b.id]) 0xFF8CE9FF.toInt() else ACC); text(cv, "OK", b.x, b.y + 8 * u, 22 * u, 0xFF04141A.toInt()) }; continue }
            val on = c.held[b.id] || (b.id == B.ADS && c.adsL) || (b.id == B.CROUCH && c.crL)
            p.style = Paint.Style.FILL; p.color = (if (b.id == B.FIRE) 0xFFE23B3B else 0xFFDDE6F2).toInt(); p.alpha = if (on) 140 else 56; cv.drawCircle(b.x, b.y, b.r, p)
            p.style = Paint.Style.STROKE; p.strokeWidth = 2f * u; p.color = Color.WHITE; p.alpha = 140; cv.drawCircle(b.x, b.y, b.r, p); p.alpha = 255
            text(cv, b.label, b.x, b.y + (if (b.r > 36 * u) 7 else 5) * u, (if (b.r > 36 * u) 17 else 12) * u)
        }
        c.moveStick?.let { (ox, oy) -> p.style = Paint.Style.STROKE; p.strokeWidth = 2f * u; p.color = Color.WHITE; p.alpha = 70; cv.drawCircle(ox, oy, 75 * u, p)
            p.style = Paint.Style.FILL; p.alpha = 100; cv.drawCircle(ox + c.stX * 75 * u, oy - c.stY * 75 * u, 26 * u, p); p.alpha = 255 }
    }

    private val maxDmg by lazy { g.WEAPONS.maxOf { it.dmg * max(1, it.pellets.coerceAtMost(4)) } }
    private val maxRpm by lazy { g.WEAPONS.maxOf { it.rpm } }
    private val maxRng by lazy { g.WEAPONS.maxOf { it.r1 } }
    private fun bar(cv: Canvas, x: Float, y: Float, wd: Float, hh: Float, v: Float, col: Int) {
        p.style = Paint.Style.FILL; p.color = 0x55FFFFFF; rect.set(x, y, x + wd, y + hh); cv.drawRoundRect(rect, hh / 2, hh / 2, p)
        p.color = col; rect.set(x, y, x + wd * v.coerceIn(0.04f, 1f), y + hh); cv.drawRoundRect(rect, hh / 2, hh / 2, p)
    }
    private fun weaponCard(cv: Canvas, w: Float, h: Float, u: Float, me: com.warzone.core.Ent) {
        val b = c.btns.firstOrNull { it.id == B.WEAPON } ?: return
        val l = b.x - b.w / 2; val t = b.y - b.h / 2; val wd = g.WEAPONS[me.wpn]
        panel(cv, l, t, l + b.w, t + b.h, 0xD0101822.toInt(), if (c.wheelOpen) ACC else 0x66FFFFFF)
        text(cv, wd.id.uppercase(), l + 14 * u, t + 26 * u, 17 * u, ACC, Paint.Align.LEFT)
        val low = me.ammo <= g.cap(me) / 4
        text(cv, "${me.ammo}", l + b.w - 82 * u, t + 52 * u, 34 * u, if (low) 0xFFFF6060.toInt() else Color.WHITE, Paint.Align.RIGHT)
        text(cv, "/ ${me.mags * g.cap(me)}", l + b.w - 76 * u, t + 52 * u, 16 * u, 0xFFB8C4D4.toInt(), Paint.Align.LEFT)
        text(cv, "▲ تغيير", l + 14 * u, t + 52 * u, 12 * u, 0xFF8FA0B4.toInt(), Paint.Align.LEFT)
        if (me.reloadT >= 0) text(cv, "إعادة التعبئة…", w / 2, h / 2 + 62 * u, 20 * u, ACC)
    }
    /** عجلة الأسلحة: شبكة 3×3 ببطاقات فيها الاسم وأشرطة الضرر والمعدل والمدى. */
    private fun wheel(cv: Canvas, w: Float, h: Float, u: Float) {
        p.style = Paint.Style.FILL; p.color = 0x99070B10.toInt(); cv.drawRect(0f, 0f, w, h, p)
        text(cv, "اختر سلاحك", w / 2, h * 0.5f - 168 * u, 24 * u)
        val cur = g.e[g.meI].wpn
        for (bt in c.btns) { if (bt.id < B.W0 || bt.id >= B.W0 + g.WEAPON_COUNT) continue
            val i = bt.id - B.W0; val wd = g.WEAPONS[i]; val l = bt.x - bt.w / 2; val t = bt.y - bt.h / 2; val sel = i == cur
            panel(cv, l, t, l + bt.w, t + bt.h, if (sel) 0xE0463614.toInt() else 0xE0121A24.toInt(), if (sel) ACC else 0x55FFFFFF)
            text(cv, wd.id.uppercase(), l + 12 * u, t + 24 * u, 16 * u, if (sel) ACC else Color.WHITE, Paint.Align.LEFT)
            val bw = 96 * u; val bx = l + bt.w - bw - 10 * u
            text(cv, "ضرر", bx - 6 * u, t + 19 * u, 9 * u, 0xFF9FB0C4.toInt(), Paint.Align.RIGHT); bar(cv, bx, t + 12 * u, bw, 7 * u, wd.dmg * max(1, wd.pellets.coerceAtMost(4)) / maxDmg, 0xFFE2573B.toInt())
            text(cv, "معدل", bx - 6 * u, t + 35 * u, 9 * u, 0xFF9FB0C4.toInt(), Paint.Align.RIGHT); bar(cv, bx, t + 28 * u, bw, 7 * u, wd.rpm / maxRpm, 0xFFFFB02E.toInt())
            text(cv, "مدى", bx - 6 * u, t + 51 * u, 9 * u, 0xFF9FB0C4.toInt(), Paint.Align.RIGHT); bar(cv, bx, t + 44 * u, bw, 7 * u, wd.r1 / maxRng, 0xFF42D66B.toInt())
            text(cv, "${wd.mag}", l + 12 * u, t + 52 * u, 13 * u, 0xFFB8C4D4.toInt(), Paint.Align.LEFT)
        }
    }
}
