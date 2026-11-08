package com.warzone.core

fun main(args: Array<String>) {
    var bad = 0
    fun run(name: String, f: () -> String?) { try { val r = f(); println("PASS $name" + (if (r != null) "\n$r" else "")) } catch (e: Throwable) { bad++; println("FAIL $name: ${e.message}"); e.printStackTrace() } }
    val dir = args.getOrNull(0) ?: "core/src/test/resources/golden"
    for (m in listOf(0, 4, 5)) run("golden map$m") { Checks.golden(m, java.io.File("$dir/map${m}_seed7.txt").readText()) }
    run("rng") { Checks.rngSanity() }; run("data") { Checks.dataSanity() }; run("menu") { Checks.menuFlow() }; run("match") { Checks.match() }; run("allmaps") { Checks.allMapsRun() }
    run("vehicle") { Checks.vehicleDrive() }; run("grenade/c4/rpg") { Checks.grenadeC4Rpg() }; run("sfx") { Checks.sfxSanity() }
    System.exit(if (bad > 0) 1 else 0)
}
