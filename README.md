# Warzone — حرب فرق 3D (أحمر ضد أزرق)

| النسخة | المسار | البناء |
|---|---|---|
| **Godot 4.7** (أندرويد APK + iOS IPA + سطح المكتب) — النسخة الرئيسية | `godot/` | `.github/workflows/godot.yml`, `godot-ios.yml` |
| PS4 (C++ / OpenOrbis) | `native/src` | `.github/workflows/ps4-native.yml` |

(نسختا Kotlin وSwift القديمتان حُذفتا؛ كل مزاياهما انتقلت إلى Godot.)

## المزايا (Godot)
- 9 خرائط، منها **قاعدة جوية F-35**: طيران بفيزياء حقيقية، طيارون بذكاء اصطناعي، مدفع، النزول بـ RPG والعودة للطائرة، الفوز بتدمير 30 طائرة (لاعب واحد).
- مركبات (جيب ودبابة)، مسيّرة استطلاع انتحارية، قنابل يدوية، C4، انحناء.
- بوتات ذكية بالملاحة (NavMesh) والتغطية وركوب المركبات.
- رسائل سلاسل القتل، مؤشر اتجاه الإصابة، حركات تعبئة وسقوط واقعية.
- لعب جماعي على الشبكة المحلية (UDP) بين Godot وPS4 (المركبات والجو غير متزامنة عبر LAN بعد).

## بيانات اللعبة والترجمة
مصدر الخرائط والأسلحة في `tools/data/`:
```sh
node tools/export-native.js   # -> native/gen/data.h (PS4)
node tools/export-godot.js    # -> godot/data/game.json
python3 tools/make-lang.py    # -> native/src/lang_gen.h + godot/data/lang.json
```
