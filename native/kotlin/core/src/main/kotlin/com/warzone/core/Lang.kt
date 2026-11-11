package com.warzone.core

/** اللغة والجودة. lang: 0 عربي، 1 English، 2 Español — quality: 0..4. onSave يُضبط من التطبيق للحفظ الدائم. */
object Lang {
    @JvmField var lang = 0
    @JvmField var quality = 3
    @JvmField var onSave: (() -> Unit)? = null
    private val map: HashMap<String, Array<String>> by lazy { HashMap<String, Array<String>>().also { m -> for (e in LangGen.TAB) m[e[0]] = e } }
    private val prefixes: List<Array<String>> by lazy { LangGen.TAB.filter { it[0].length >= 4 }.sortedByDescending { it[0].length } }
    private val cache = HashMap<String, String>()

    fun save() { onSave?.invoke() }

    @JvmStatic fun t(s: String): String {
        if (lang <= 0 || s.isEmpty()) return s
        val key = lang.toString() + s
        cache[key]?.let { return it }
        val r = tr(s); if (cache.size > 600) cache.clear(); cache[key] = r; return r
    }
    private fun pick(e: Array<String>) = if (lang == 1) e[1] else e[2]
    private fun tr(s: String): String {
        map[s]?.let { return pick(it) }
        for (e in prefixes) if (s.startsWith(e[0])) return pick(e) + fixTail(s.substring(e[0].length))
        for (e in prefixes) if (s.endsWith(e[0])) return fixTail(s.substring(0, s.length - e[0].length)) + pick(e)
        return s
    }
    private fun fixTail(x: String) = if (x.endsWith(" ث")) x.dropLast(1) + "s" else x
}
