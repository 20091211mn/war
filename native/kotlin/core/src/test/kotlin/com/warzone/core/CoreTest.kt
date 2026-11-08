package com.warzone.core

import kotlin.test.Test

class CoreTest {
    private fun golden(map: Int) = javaClass.getResource("/golden/map${map}_seed7.txt")!!.readText()
    @Test fun goldenMap0() { println(Checks.golden(0, golden(0))) }
    @Test fun goldenMap4Vehicles() { println(Checks.golden(4, golden(4))) }
    @Test fun goldenMap5() { println(Checks.golden(5, golden(5))) }
    @Test fun rng() { println(Checks.rngSanity()) }
    @Test fun data() { println(Checks.dataSanity()) }
    @Test fun menuFlow() { println(Checks.menuFlow()) }
    @Test fun fullMatch() { println(Checks.match()) }
    @Test fun allMaps() { println(Checks.allMapsRun()) }
    @Test fun vehicles() { println(Checks.vehicleDrive()) }
    @Test fun grenadeC4RpgDrone() { println(Checks.grenadeC4Rpg()) }
    @Test fun sfx() { println(Checks.sfxSanity()) }
}
