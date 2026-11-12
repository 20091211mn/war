extends Node
# إعدادات اللعبة: الجودة (0..4) واللغة (0 عربي، 1 English، 2 Español) — تُحفظ في user://settings.cfg
signal changed
var quality := 3
var lang := 0
var LANGS := ["العربية", "English", "Español"]
var QUALS := ["ضعيفة جداً", "ضعيفة", "متوسطة", "عالية", "فائقة"]
var _tab := {}
const CFG := "user://settings.cfg"

func _ready() -> void:
	var f := FileAccess.open("res://data/lang.json", FileAccess.READ)
	if f: _tab = JSON.parse_string(f.get_as_text())
	var c := ConfigFile.new()
	if c.load(CFG) == OK:
		quality = clampi(int(c.get_value("g", "quality", 3)), 0, 4)
		lang = clampi(int(c.get_value("g", "lang", 0)), 0, 2)
	_apply_locale()

func save() -> void:
	var c := ConfigFile.new()
	c.set_value("g", "quality", quality); c.set_value("g", "lang", lang); c.save(CFG)
	_apply_locale(); changed.emit()

func _apply_locale() -> void:
	TranslationServer.set_locale(["ar", "en", "es"][lang])

# الترجمة: النص العربي هو المفتاح
func t(ar: String) -> String:
	if lang == 0 or not _tab.has(ar): return ar
	return _tab[ar][lang - 1]

# إعدادات الرسم حسب الجودة
func res_scale() -> float: return [0.5, 0.65, 0.8, 1.0, 1.0][quality]
func shadows() -> bool: return quality >= 2
func msaa() -> int: return 2 if quality >= 3 else 0
func fog_k() -> float: return [0.45, 0.7, 1.0, 1.5, 2.2][quality]
func trees() -> int: return [0, 0, 10, 28, 60][quality]
func windows() -> int: return [0, 0, 0, 150, 300][quality]
func lod_dist() -> float: return [14.0, 24.0, 38.0, 60.0, 95.0][quality]
