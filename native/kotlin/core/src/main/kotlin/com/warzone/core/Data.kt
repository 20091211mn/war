package com.warzone.core

import java.io.ByteArrayOutputStream
import java.nio.ByteBuffer
import java.nio.ByteOrder

/** Minimal JSON parser (objects -> Map, arrays -> List, numbers -> Double, strings, booleans, null). */
object Json {
    fun parse(s: String): Any? { val p = P(s); val v = p.value(); return v }
    private class P(val s: String) {
        var i = 0
        fun ws() { while (i < s.length && s[i] <= ' ') i++ }
        fun value(): Any? {
            ws()
            val c = s[i]
            return when {
                c == '{' -> obj()
                c == '[' -> arr()
                c == '"' -> str()
                s.startsWith("true", i) -> { i += 4; true }
                s.startsWith("false", i) -> { i += 5; false }
                s.startsWith("null", i) -> { i += 4; null }
                else -> num()
            }
        }
        fun obj(): Map<String, Any?> {
            val m = LinkedHashMap<String, Any?>(); i++; ws()
            if (s[i] == '}') { i++; return m }
            while (true) {
                ws(); val k = str(); ws(); i++ // ':'
                m[k] = value(); ws()
                if (s[i] == ',') { i++; continue }
                i++; return m
            }
        }
        fun arr(): List<Any?> {
            val l = ArrayList<Any?>(); i++; ws()
            if (s[i] == ']') { i++; return l }
            while (true) {
                l.add(value()); ws()
                if (s[i] == ',') { i++; continue }
                i++; return l
            }
        }
        fun str(): String {
            val sb = StringBuilder(); i++
            while (s[i] != '"') {
                if (s[i] == '\\') {
                    i++
                    when (val e = s[i]) {
                        'n' -> sb.append('\n'); 't' -> sb.append('\t'); 'r' -> sb.append('\r')
                        'u' -> { sb.append(s.substring(i + 1, i + 5).toInt(16).toChar()); i += 4 }
                        else -> sb.append(e)
                    }
                } else sb.append(s[i])
                i++
            }
            i++; return sb.toString()
        }
        fun num(): Double {
            val st = i
            while (i < s.length && (s[i].isDigit() || s[i] == '-' || s[i] == '+' || s[i] == '.' || s[i] == 'e' || s[i] == 'E')) i++
            return s.substring(st, i).toDouble()
        }
    }
}

@Suppress("UNCHECKED_CAST")
private fun List<Any?>.fl(): FloatArray = FloatArray(size) { (this[it] as Double).toFloat() }

class Box(@JvmField val x0: Float, @JvmField val y0: Float, @JvmField val z0: Float, @JvmField val x1: Float, @JvmField val y1: Float, @JvmField val z1: Float, @JvmField val k: Int)
class P2(@JvmField val x: Float, @JvmField val z: Float)
class V3(@JvmField val x: Float, @JvmField val z: Float, @JvmField val h: Float, @JvmField val t: Int)

class MapDef(
    @JvmField val W: Float, @JvmField val D: Float,
    @JvmField val boxes: Array<Box>, @JvmField val deco: Array<Box>,
    @JvmField val red: Array<P2>, @JvmField val blue: Array<P2>, @JvmField val veh: Array<V3>,
    @JvmField val cols: Array<FloatArray>,
    @JvmField val sky: FloatArray, @JvmField val hor: FloatArray, @JvmField val ground: FloatArray, @JvmField val fog: FloatArray,
    @JvmField val fogN: Float, @JvmField val fogF: Float, @JvmField val sun: FloatArray, @JvmField val amb: Float, @JvmField val sunI: Float
) {
    val nv get() = veh.size
    val ncol get() = cols.size
}

class WeaponDef(
    @JvmField val id: String, @JvmField val dmg: Float, @JvmField val head: Float, @JvmField val rpm: Float, @JvmField val autof: Boolean,
    @JvmField val pellets: Int, @JvmField val mag: Int, @JvmField val mags: Int, @JvmField val reload: Float, @JvmField val hip: Float, @JvmField val ads: Float,
    @JvmField val recoilV: Float, @JvmField val recoilH: Float, @JvmField val r0: Float, @JvmField val r1: Float, @JvmField val minMul: Float,
    @JvmField val move: Float, @JvmField val zoom: Float, @JvmField val type: Int, @JvmField val name: String
)

class GunDef(@JvmField val mz: Float, @JvmField val sy: Float, @JvmField val np: Int, @JvmField val p: FloatArray, @JvmField val pc: CharArray,
             @JvmField val hasMag: Boolean, @JvmField val mag: FloatArray) {
    fun px(i: Int) = p[i * 6]; fun py(i: Int) = p[i * 6 + 1]; fun pz(i: Int) = p[i * 6 + 2]
    fun psx(i: Int) = p[i * 6 + 3]; fun psy(i: Int) = p[i * 6 + 4]; fun psz(i: Int) = p[i * 6 + 5]
}

/** 3D gun mesh (from GLB conversion): groups 0 front, 1 magazine, 2 moving part, 3 stock. */
class GunMesh(val nv: Int, val grp: IntArray, val lo: FloatArray, val rng: FloatArray, val pos: ShortArray, val nor: ByteArray, val col: ByteArray, val idx: ShortArray) {
    // decoded vertices
    val vx = FloatArray(nv); val vy = FloatArray(nv); val vz = FloatArray(nv)
    val nx = FloatArray(nv); val ny = FloatArray(nv); val nz = FloatArray(nv)
    val cr = FloatArray(nv); val cg = FloatArray(nv); val cb = FloatArray(nv)
    init {
        for (i in 0 until nv) {
            vx[i] = lo[0] + (pos[i * 3] + 32768) / 65535f * rng[0]
            vy[i] = lo[1] + (pos[i * 3 + 1] + 32768) / 65535f * rng[1]
            vz[i] = lo[2] + (pos[i * 3 + 2] + 32768) / 65535f * rng[2]
            nx[i] = nor[i * 3] / 127f; ny[i] = nor[i * 3 + 1] / 127f; nz[i] = nor[i * 3 + 2] / 127f
            cr[i] = (col[i * 3].toInt() and 255) / 255f; cg[i] = (col[i * 3 + 1].toInt() and 255) / 255f; cb[i] = (col[i * 3 + 2].toInt() and 255) / 255f
        }
    }
    fun index(i: Int): Int = idx[i].toInt() and 0xFFFF
}

object GameData {
    @JvmField var maps: Array<MapDef> = emptyArray()
    @JvmField var weapons: Array<WeaponDef> = emptyArray()
    @JvmField var mount: WeaponDef? = null
    @JvmField var wkind: IntArray = IntArray(0)
    @JvmField var gundefs: Array<GunDef> = emptyArray()
    private val meshes = HashMap<String, GunMesh?>()
    val MAP_COUNT get() = maps.size
    val WEAPON_COUNT get() = weapons.size
    private var loaded = false

    fun readRes(name: String): ByteArray {
        val st = GameData::class.java.getResourceAsStream("/warzone/$name") ?: throw IllegalStateException("missing resource $name")
        st.use { val bo = ByteArrayOutputStream(); val buf = ByteArray(65536); while (true) { val n = it.read(buf); if (n < 0) break; bo.write(buf, 0, n) }; return bo.toByteArray() }
    }

    @Synchronized fun load() {
        if (loaded) return
        @Suppress("UNCHECKED_CAST")
        val ml = Json.parse(String(readRes("maps.json"), Charsets.UTF_8)) as List<Map<String, Any?>>
        maps = Array(ml.size) { k ->
            val m = ml[k]
            fun f(n: String) = (m[n] as List<Any?>).fl()
            fun fv(n: String) = (m[n] as Double).toFloat()
            fun boxes(n: String): Array<Box> { val a = f(n); return Array(a.size / 7) { Box(a[it * 7], a[it * 7 + 1], a[it * 7 + 2], a[it * 7 + 3], a[it * 7 + 4], a[it * 7 + 5], a[it * 7 + 6].toInt()) } }
            fun p2(n: String): Array<P2> { val a = f(n); return Array(a.size / 2) { P2(a[it * 2], a[it * 2 + 1]) } }
            val vv = f("veh"); val cc = f("cols")
            MapDef(fv("W"), fv("D"), boxes("boxes"), boxes("deco"), p2("red"), p2("blue"),
                Array(vv.size / 4) { V3(vv[it * 4], vv[it * 4 + 1], vv[it * 4 + 2], vv[it * 4 + 3].toInt()) },
                Array(cc.size / 3) { floatArrayOf(cc[it * 3], cc[it * 3 + 1], cc[it * 3 + 2]) },
                f("sky"), f("hor"), f("ground"), f("fog"), fv("fogN"), fv("fogF"), f("sun"), fv("amb"), fv("sunI"))
        }
        @Suppress("UNCHECKED_CAST")
        val wj = Json.parse(String(readRes("weapons.json"), Charsets.UTF_8)) as Map<String, Any?>
        fun wd(m: Map<String, Any?>): WeaponDef {
            fun n(k: String) = (m[k] as Double).toFloat()
            return WeaponDef(m["id"] as String, n("dmg"), n("head"), n("rpm"), n("autof") != 0f, n("pellets").toInt(), n("mag").toInt(), n("mags").toInt(), n("reload"),
                n("hip"), n("ads"), n("recoilV"), n("recoilH"), n("r0"), n("r1"), n("minMul"), n("move"), n("zoom"), n("type").toInt(), m["name"] as String)
        }
        @Suppress("UNCHECKED_CAST")
        weapons = (wj["weapons"] as List<Map<String, Any?>>).map { wd(it) }.toTypedArray()
        @Suppress("UNCHECKED_CAST")
        mount = wd(wj["mount"] as Map<String, Any?>)
        @Suppress("UNCHECKED_CAST")
        wkind = (wj["wkind"] as List<Any?>).map { (it as Double).toInt() }.toIntArray()
        @Suppress("UNCHECKED_CAST")
        gundefs = (wj["gundefs"] as List<Map<String, Any?>>).map {
            val raw = it["p"] as List<Any?>; val np = (it["np"] as Double).toInt()
            val p = FloatArray(np * 6); val pc = CharArray(np)
            for (i in 0 until np) { for (j in 0 until 6) p[i * 6 + j] = (raw[i * 7 + j] as Double).toFloat(); pc[i] = (raw[i * 7 + 6] as String)[0] }
            GunDef((it["mz"] as Double).toFloat(), (it["sy"] as Double).toFloat(), np, p, pc, (it["hasMag"] as Double).toInt() != 0, (it["mag"] as List<Any?>).fl())
        }.toTypedArray()
        loaded = true
    }

    @Synchronized fun mesh(name: String): GunMesh? = meshes.getOrPut(name) {
        try {
            val bb = ByteBuffer.wrap(readRes("gm_$name.bin")).order(ByteOrder.LITTLE_ENDIAN)
            val nv = bb.int; val grp = IntArray(4) { bb.int }; val lo = FloatArray(3) { bb.float }; val rng = FloatArray(3) { bb.float }
            val pos = ShortArray(nv * 3); bb.asShortBuffer().get(pos); bb.position(bb.position() + pos.size * 2)
            val nor = ByteArray(nv * 3); bb.get(nor); val col = ByteArray(nv * 3); bb.get(col)
            val ni = bb.int; val idx = ShortArray(ni); bb.asShortBuffer().get(idx)
            GunMesh(nv, grp, lo, rng, pos, nor, col, idx)
        } catch (e: Exception) { null }
    }

    /** Which weapon index uses which 3D mesh (null = box model). */
    fun meshOf(wi: Int): GunMesh? = when (wi) { 0 -> mesh("rifle"); 3 -> mesh("sniper"); 5 -> mesh("dmr"); 8 -> mesh("launcher"); else -> null }
}
