extends Node
# تحديث داخل اللعبة بلا متصفح ولا تثبيت APK: حزمة موارد (patch.pck) تُنزَّل إلى user:// وتُحمَّل عند الإقلاع
# *قبل* باقي السكربتات (هذا أول autoload)، فتحلّ محل سكربتات اللعبة وبياناتها. لا تغيّر المحرك ولا الصلاحيات ولا class_name جديدة.
# أمان: إن أقلعت اللعبة مرتين دون الوصول إلى القائمة (Main يستدعي boot_ok) تُحذف الحزمة وتعود النسخة الأصلية.
const PCK := "user://patch.pck"
const META := "user://patch.json"
const TRY := "user://patch_try"
static var patch_v := 0
static var loaded := false

static func base_v() -> int:
	var v := str(ProjectSettings.get_setting("application/config/version", ""))
	return int(v) if v.is_valid_int() else 0

func _init() -> void:
	if not FileAccess.file_exists(PCK): return
	var m = JSON.parse_string(FileAccess.get_file_as_string(META)) if FileAccess.file_exists(META) else null
	patch_v = int(m.get("v", 0)) if m is Dictionary else 0
	if patch_v <= base_v(): drop(); return              # ثُبّتت نسخة APK أحدث من الحزمة
	var tries := int(FileAccess.get_file_as_string(TRY)) if FileAccess.file_exists(TRY) else 0
	if tries >= 2: drop(); return                       # إقلاعان فاشلان: تراجع
	var f := FileAccess.open(TRY, FileAccess.WRITE); if f: f.store_string(str(tries + 1)); f.close()
	loaded = ProjectSettings.load_resource_pack(PCK, true)
	if not loaded: drop()

static func drop() -> void:
	patch_v = 0; loaded = false
	for p in [PCK, META, TRY]: if FileAccess.file_exists(p): DirAccess.remove_absolute(p)

static func boot_ok() -> void:
	if FileAccess.file_exists(TRY): DirAccess.remove_absolute(TRY)

static func current() -> int: return maxi(base_v(), patch_v)
