class_name Updater
extends Node
# فحص/تنزيل التحديث من إصدار GitHub «latest». رقم النسخة يُختم وقت البناء في config/version (dev محلياً).
signal checked(state: int, remote: String)   # 0 محدّث، 1 يوجد تحديث، 2 فشل
static var state := -1   # آخر نتيجة فحص: -1 لم يُفحص
static var remote := ""
const BASE := "https://github.com/20091211mn/war/releases/download/latest/"
const PAGE := "https://github.com/20091211mn/war/releases/tag/latest"

static func version() -> String:
	var v := str(ProjectSettings.get_setting("application/config/version", ""))
	return v if v != "" else "dev"

static func apk_url() -> String: return BASE + "Warzone-godot.apk"

func check() -> void:
	state = -2
	var h := HTTPRequest.new(); h.timeout = 10.0; add_child(h)
	h.request_completed.connect(func(res: int, code: int, _hd: PackedStringArray, body: PackedByteArray):
		h.queue_free()
		var r := body.get_string_from_utf8().strip_edges()
		if res != HTTPRequest.RESULT_SUCCESS or code != 200 or not r.is_valid_int(): state = 2; checked.emit(2, ""); return
		var cur := version()
		state = 1 if (not cur.is_valid_int() or int(r) > int(cur)) and r != cur else 0; remote = r; checked.emit(state, r))
	if h.request(BASE + "version-android.txt") != OK: h.queue_free(); state = 2; checked.emit(2, "")

# أندرويد: يفتح تنزيل الـAPK (يظهر طلب التثبيت فوق النسخة الحالية). غيره: صفحة الإصدار.
static func install() -> void:
	OS.shell_open(apk_url() if OS.get_name() == "Android" else PAGE)
