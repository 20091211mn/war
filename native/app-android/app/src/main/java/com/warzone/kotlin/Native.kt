package com.warzone.kotlin

import java.nio.ByteBuffer

/** جسر JNI إلى محرك اللعبة (C++). كل شيء آخر في التطبيق Kotlin. */
object Native {
    init { System.loadLibrary("warzone") }
    external fun init(screenW: Int, screenH: Int, cores: Int)
    external fun setScreen(screenW: Int, screenH: Int)
    /** action: 0 ضغط، 1 حركة، 2 رفع، 3 إلغاء الكل */
    external fun touch(action: Int, id: Int, x: Float, y: Float)
    external fun frame(dt: Float)
    external fun width(): Int
    external fun height(): Int
    external fun resizeSeq(): Int
    /** ينسخ صورة RGBA من المحرك إلى مخزن مباشر تحت القفل؛ wh[0..1] = العرض والارتفاع. يرجع 1 عند النجاح */
    external fun copyPixels(buf: ByteBuffer, wh: IntArray): Int
    external fun mix(buf: ShortArray, frames: Int)
    external fun pause()
}
