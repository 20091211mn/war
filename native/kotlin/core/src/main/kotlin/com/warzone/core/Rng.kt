package com.warzone.core

/** Bit-exact port of glibc rand()/srand() (TYPE_3 additive feedback generator) so that results are comparable with the C++ engine. */
class GlibcRand(seed: Int = 1) {
    private val r = IntArray(34)
    private var f = 0
    private var b = 0
    private val tbl = IntArray(31)
    private var fptr = 3
    private var rptr = 0

    init { srand(seed) }

    fun srand(seedIn: Int) {
        var seed = if (seedIn == 0) 1 else seedIn
        tbl[0] = seed
        var word = seed
        for (i in 1 until 31) {
            // word = 16807 * word % 2147483647 using Schrage's method (as glibc does)
            val hi = word / 127773
            val lo = word % 127773
            word = 16807 * lo - 2836 * hi
            if (word < 0) word += 2147483647
            tbl[i] = word
        }
        fptr = 3
        rptr = 0
        for (i in 0 until 310) next()
    }

    /** Returns a value in [0, 2147483647]. */
    fun next(): Int {
        tbl[fptr] += tbl[rptr]
        val result = (tbl[fptr].toLong() and 0xFFFFFFFFL) ushr 1
        fptr++
        if (fptr >= 31) { fptr = 0; rptr++ } else { rptr++; if (rptr >= 31) rptr = 0 }
        return result.toInt()
    }

    fun frand(): Float = next().toFloat() / 2147483647f
}

/** Process-wide RNG used by the game (C++ used libc rand()). */
object Rnd {
    @JvmField var g = GlibcRand(1)
    fun seed(s: Int) { g = GlibcRand(s) }
    @JvmStatic fun rand(): Int = g.next()
    @JvmStatic fun frand(): Float = g.next().toFloat() / 2147483648f // (float)rand()/(float)RAND_MAX; (float)RAND_MAX == 2^31
}
