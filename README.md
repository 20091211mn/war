# Warzone — حرب فرق 3D (أحمر ضد أزرق)

لعبة أصلية بالكامل (لا توجد نسخة ويب) بثلاث واجهات:

| النسخة | المسار | البناء |
|---|---|---|
| PS4 (C++ / OpenOrbis) | `native/src` | `.github/workflows/ps4-native.yml` |
| iOS (Swift + محرك C++) | `native/app-ios` | `.github/workflows/ios-swift.yml` |
| أندرويد Kotlin صِرف | `native/kotlin`, `native/app-android-pure` | `.github/workflows/android-kotlin-pure.yml` |

## المزايا
- 9 خرائط منها **قاعدة جوية F-35** (حرب طائرات: قيادة بفيزياء حقيقية، ذكاء طيارين، مدفع HEI، النزول بـ RPG بخمس طلقات، الفوز بتدمير 30 طائرة — لاعب واحد).
- مركبات، مسيّرة، C4، قنابل، ملحقات أسلحة، بوتات بثلاث مستويات.
- لعب جماعي على الشبكة المحلية (UDP): استضافة وانضمام من PS4 وiOS وأندرويد.

## بيانات اللعبة
مصدر الخرائط والأسلحة في `tools/data/`، ويُصدَّر إلى C++ وKotlin:
```sh
node tools/export-native.js   # -> native/gen/data.h
node tools/export-kotlin.js   # -> native/kotlin/core/src/main/resources/warzone
```
