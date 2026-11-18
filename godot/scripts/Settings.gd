extends Node
# إعدادات اللعبة: الجودة (0..4) واللغة (0 عربي، 1 English، 2 Español) — تُحفظ في user://settings.cfg
signal changed
var quality := (2 if OS.has_feature("mobile") else 3)
var lang := 0
var auto_update := true
var sens := 1.0          # حساسية النظر (0.3..2.5)
var ads_sens := 0.8      # مضاعف الحساسية أثناء التصويب
var fov := 75.0
var invert_y := false
var gyro := false        # التصويب بالجيروسكوب (جوال فقط)
var gyro_sens := 1.0
var gyro_mode := 0       # 0 دائماً، 1 فقط أثناء التصويب (ADS)
var gyro_flip := false   # عكس محور الجيرو إن كان اتجاهه معكوساً على الجهاز
var volume := 1.0
var fps_cap := 1         # 0:30  1:60  2:90  3:120
var res_pct := 0         # 0 تلقائي حسب الجودة، 1:50% 2:75% 3:100%
var show_fps := false
var servers: Array = []   # آخر خوادم الأونلاين المستخدمة (نص host[:port])
var FPS_CAPS := [30, 60, 90, 120]
var LANGS := ["العربية", "English", "Español"]
var QUALS := ["ضعيفة جداً", "ضعيفة", "متوسطة", "عالية", "فائقة"]
var _tab := {}
const CFG := "user://settings.cfg"

func _ready() -> void:
	var f := FileAccess.open("res://data/lang.json", FileAccess.READ)
	if f: _tab = JSON.parse_string(f.get_as_text())
	var c := ConfigFile.new()
	if c.load(CFG) == OK:
		quality = clampi(int(c.get_value("g", "quality", quality)), 0, 4)
		lang = clampi(int(c.get_value("g", "lang", 0)), 0, 2)
		auto_update = bool(c.get_value("g", "auto_update", true))
		sens = clampf(float(c.get_value("g", "sens", 1.0)), 0.3, 2.5); ads_sens = clampf(float(c.get_value("g", "ads_sens", 0.8)), 0.3, 1.5)
		fov = clampf(float(c.get_value("g", "fov", 75.0)), 60.0, 100.0); invert_y = bool(c.get_value("g", "invert_y", false))
		gyro = bool(c.get_value("g", "gyro", false)); gyro_sens = clampf(float(c.get_value("g", "gyro_sens", 1.0)), 0.3, 3.0); gyro_flip = bool(c.get_value("g", "gyro_flip", false)); gyro_mode = clampi(int(c.get_value("g", "gyro_mode", 0)), 0, 1)
		volume = clampf(float(c.get_value("g", "volume", 1.0)), 0.0, 1.0); fps_cap = clampi(int(c.get_value("g", "fps_cap", 1)), 0, 3)
		var sv = c.get_value("g", "servers", [])
		servers = (sv as Array).filter(func(x): return x is String) if sv is Array else []
		res_pct = clampi(int(c.get_value("g", "res_pct", 0)), 0, 3); show_fps = bool(c.get_value("g", "show_fps", false))
	_apply_locale(); apply_runtime()

# إعدادات تسري فوراً: سقف الإطارات وصوت الماستر
func apply_runtime() -> void:
	Engine.max_fps = FPS_CAPS[fps_cap]
	AudioServer.set_bus_volume_db(0, linear_to_db(maxf(volume, 0.0001)))

func save() -> void:
	var c := ConfigFile.new()
	c.set_value("g", "quality", quality); c.set_value("g", "lang", lang); c.set_value("g", "auto_update", auto_update)
	for k in ["sens", "ads_sens", "fov", "invert_y", "gyro", "gyro_sens", "gyro_flip", "gyro_mode", "volume", "fps_cap", "res_pct", "show_fps", "servers"]: c.set_value("g", k, get(k))
	c.save(CFG); apply_runtime()
	_apply_locale(); changed.emit()

func _apply_locale() -> void:
	TranslationServer.set_locale(["ar", "en", "es"][lang])

# الترجمة: النص العربي هو المفتاح
func t(ar: String) -> String:
	if lang == 0 or not _tab.has(ar): return ar
	return _tab[ar][lang - 1]

# إعدادات الرسم حسب الجودة
func res_scale() -> float: return [0.6, 0.75, 0.9, 1.0, 1.0][quality] if res_pct == 0 else [0.5, 0.75, 1.0][res_pct - 1]
func shadows() -> bool: return quality >= 2
func msaa() -> int: return 0 if quality < 2 else (2 if quality >= 4 else 1)
func fog_k() -> float: return [0.45, 0.7, 1.0, 1.5, 2.2][quality]
func trees() -> int: return [0, 0, 10, 28, 60][quality]
func windows() -> int: return [0, 0, 0, 150, 300][quality]
func lod_dist() -> float: return [14.0, 24.0, 38.0, 60.0, 95.0][quality]
