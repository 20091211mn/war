package com.warzone.kotlin

import android.graphics.*
import com.warzone.core.Game
import kotlin.math.*

object HudDiag { @Volatile var line = ""; @Volatile var err: String? = null }

/** واجهة اللعبة: تُرسم بـ Canvas فوق صورة OpenGL (قائمة، شريط الحياة، الذخيرة، النتيجة، الأزرار). */
class Hud(private val g: Game, private val c: Controls) {
    private val p = Paint(Paint.ANTI_ALIAS_FLAG)
    private val tp = Paint(Paint.ANTI_ALIAS_FLAG).apply { typeface = Typeface.DEFAULT_BOLD; color = Color.WHITE }
    private val rect = RectF()
    private val MAPS = arrayOf("المستودع", "أطلال الصحراء", "شارع المدينة", "القاعدة القطبية", "صحراء الشاص", "أهرامات الجيزة", "المدينة القديمة", "ميناء الإسكندرية")
    private val DIFF = arrayOf("سهل", "عادي", "صعب")
    private val RED = 0xFFE23B3B.toInt(); private val BLUE = 0xFF3B82F6.toInt(); private val ACC = 0xFFFFB02E.toInt()

    private fun text(cv: Canvas, s: String, x: Float, y: Float, size: Float, col: Int = Color.WHITE, align: Paint.Align = Paint.Align.CENTER) {
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
        val me = g.e[g.meI]
        // وميض الضرر
        if (g.dmgFlash > 0) { p.style = Paint.Style.FILL; p.color = Color.argb((min(0.5f, g.dmgFlash) * 160).toInt(), 220, 0, 0); cv.drawRect(0f, 0f, w, h, p) }
        if (!g.paused) hud(cv, w, h, u, me)
        if (g.over) {
            panel(cv, w * 0.25f, h * 0.18f, w * 0.75f, h * 0.45f)
            val win = g.winner; text(cv, if (win == 0) "فاز الفريق الأحمر" else "فاز الفريق الأزرق", w / 2, h * 0.30f, 34 * u, if (win == 0) RED else BLUE)
            text(cv, "${g.score[0]} - ${g.score[1]}", w / 2, h * 0.38f, 28 * u)
        }
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
        if (me.alive && g.drive < 0 || g.passenger()) {
            p.style = Paint.Style.STROKE; p.strokeWidth = 2.2f * u; p.color = if (g.hitMark > 0) 0xFFFF4040.toInt() else 0xCCFFFFFF.toInt()
            val s = 9f * u; val gp = 4f * u
            cv.drawLine(cx - s - gp, cy, cx - gp, cy, p); cv.drawLine(cx + gp, cy, cx + s + gp, cy, p); cv.drawLine(cx, cy - s - gp, cx, cy - gp, p); cv.drawLine(cx, cy + gp, cx, cy + s + gp, p)
            if (g.hitMark > 0) { p.strokeWidth = 3f * u; val k = 12f * u; cv.drawLine(cx - k, cy - k, cx - 4 * u, cy - 4 * u, p); cv.drawLine(cx + k, cy - k, cx + 4 * u, cy - 4 * u, p); cv.drawLine(cx - k, cy + k, cx - 4 * u, cy + 4 * u, p); cv.drawLine(cx + k, cy + k, cx + 4 * u, cy + 4 * u, p) }
        }
        // النتيجة والوقت
        panel(cv, w / 2 - 110 * u, 6 * u, w / 2 + 110 * u, 50 * u)
        text(cv, "${g.score[0]}", w / 2 - 70 * u, 40 * u, 30 * u, RED); text(cv, "${g.score[1]}", w / 2 + 70 * u, 40 * u, 30 * u, BLUE)
        val left = max(0, (g.t).toInt()); text(cv, String.format("%d:%02d", left / 60, left % 60), w / 2, 40 * u, 22 * u)
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
        for (i in 0 until 4) if (g.feedT[i] > 0 && g.feed[i].isNotEmpty()) text(cv, g.feed[i], 12 * u, 66 * u + i * 20 * u, 15 * u, Color.WHITE, Paint.Align.LEFT)
        // رادار صغير
        radar(cv, w, u, me)
    }

    private fun radar(cv: Canvas, w: Float, u: Float, me: com.warzone.core.Ent) {
        val R = 52f * u; val cx = w - R - 70 * u; val cy = R + 8 * u
        p.style = Paint.Style.FILL; p.color = 0x990B0F14.toInt(); cv.drawCircle(cx, cy, R, p); p.style = Paint.Style.STROKE; p.color = 0x66FFFFFF; p.strokeWidth = 1.5f; cv.drawCircle(cx, cy, R, p)
        val sc = R / 50f; val cs = cos(me.yaw); val sn = sin(me.yaw)
        p.style = Paint.Style.FILL
        for (i in g.e.indices) { val o = g.e[i]; if (i == g.meI || !o.alive || o.gone) continue
            val show = o.team == me.team || o.revealT > 0 || o.reveal > 0; if (!show) continue
            val dx = o.b.x - me.b.x; val dz = o.b.z - me.b.z; val rx = dx * cs - dz * sn; val rz = dx * sn + dz * cs
            val px = cx + rx * sc; val py = cy + rz * sc; if ((px - cx) * (px - cx) + (py - cy) * (py - cy) > R * R) continue
            p.color = if (o.team == 0) RED else BLUE; cv.drawCircle(px, py, 3f * u, p) }
        p.color = Color.WHITE; val tri = Path(); tri.moveTo(cx, cy - 6 * u); tri.lineTo(cx - 4 * u, cy + 4 * u); tri.lineTo(cx + 4 * u, cy + 4 * u); tri.close(); cv.drawPath(tri, p)
    }

    private val MODES = arrayOf("لاعب واحد", "استضافة (LAN)", "انضمام (LAN)")
    private fun lobby(cv: Canvas, w: Float, h: Float, u: Float) {
        p.style = Paint.Style.FILL; p.color = 0xCC0B0F14.toInt(); cv.drawRect(0f, 0f, w, h, p)
        text(cv, "الانضمام إلى لعبة LAN", w / 2, h * 0.16f, 28 * u)
        val n = g.ns
        if (n.hosts.isEmpty()) text(cv, if (n.status.isNotEmpty()) n.status else "جارٍ البحث عن مضيف على نفس الشبكة…", w / 2, h * 0.40f, 18 * u, 0xFFB8C4D4.toInt())
        else if (n.status.isNotEmpty()) text(cv, n.status, w / 2, h * 0.23f, 16 * u, 0xFFFFC85A.toInt())
        for (b in c.btns) { val l = b.x - b.w / 2; val t = b.y - b.h / 2
            if (b.id >= B.H0 && b.id < B.H0 + 5) { val hs = n.hosts.getOrNull(b.id - B.H0) ?: continue
                panel(cv, l, t, l + b.w, t + b.h, if (n.hostSel == b.id - B.H0) 0xFF3B82F6.toInt() else 0xDD18202C.toInt())
                text(cv, "${hs.name} — ${MAPS[hs.map % MAPS.size]} — ${hs.players}/${hs.maxp}", b.x, b.y + 7 * u, 17 * u) }
            else if (b.id == B.BACK) { panel(cv, l, t, l + b.w, t + b.h, if (c.held[B.BACK]) 0xFF3B82F6.toInt() else 0xDD18202C.toInt()); text(cv, "رجوع", b.x, b.y + 7 * u, 18 * u) } }
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
                B.START -> { panel(cv, l, t, l + b.w, t + b.h, if (c.held[B.START]) 0xFFFFC85A.toInt() else ACC, 0xFFFFFFFF.toInt()); text(cv, "ابدأ اللعب", b.x, b.y + 9 * u, 26 * u, 0xFF1A1200.toInt()) }
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
        if (g.menu == 1 || g.paused) return
        for (b in c.btns) {
            if (b.w > 0f) { if (b.id == B.OK) { val l = b.x - b.w / 2; val t = b.y - b.h / 2; panel(cv, l, t, l + b.w, t + b.h, if (c.held[b.id]) 0xFFFFC85A.toInt() else ACC); text(cv, "OK", b.x, b.y + 8 * u, 22 * u, 0xFF1A1200.toInt()) }; continue }
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
