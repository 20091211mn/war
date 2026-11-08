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
    /** صورة RGBA مباشرة من المحرك (بدون نسخ) */
    external fun pixels(): ByteBuffer?
    external fun mix(buf: ShortArray, frames: Int)
    external fun pause()
}
