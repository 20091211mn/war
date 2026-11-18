class_name Updater
extends Node
# فحص وتنزيل تحديث اللعبة من إصدار GitHub «latest» داخل اللعبة نفسها (HTTPRequest) بلا متصفح.
# التحديث = حزمة موارد (Patcher.gd) تُطبَّق عند الإقلاع التالي.
signal checked(state: int, remote: String)   # يُطلق عند كل تغيّر في الحالة/التقدّم
# الحالات: -1 لم يُفحص، -2 جارٍ الفحص، 0 محدّث، 1 يوجد تحديث، 2 فشل، 3 جارٍ التنزيل، 4 جاهز (أعد التشغيل)
static var state := -1
static var remote := ""
static var progress := 0.0
static var err := ""
static var BASES := ["https://github.com/20091211mn/war/releases/download/latest/"]   # مرايا بديلة تُضاف هنا
const DL := "user://patch_dl.tmp"
var _h: HTTPRequest
var _mi := 0

static func version() -> String:
	var c := Patcher.current()
	if c > 0: return str(c)
	var v := str(ProjectSettings.get_setting("application/config/version", ""))
	return v if v != "" else "dev"

func check() -> void:
	if state == -2 or state == 3: return
	state = -2; err = ""; checked.emit(state, remote)
	_get_manifest(0)

func _get_manifest(i: int) -> void:
	_mi = i
	var h := HTTPRequest.new(); h.timeout = 12.0; h.max_redirects = 8; add_child(h)
	h.request_completed.connect(func(res: int, code: int, _hd: PackedStringArray, body: PackedByteArray):
		h.queue_free()
		var d = JSON.parse_string(body.get_string_from_utf8()) if res == HTTPRequest.RESULT_SUCCESS and code == 200 else null
		if res != HTTPRequest.RESULT_SUCCESS or code != 200 or not (d is Dictionary) or not d.has("v"):
			if i + 1 < BASES.size(): _get_manifest(i + 1); return
			state = 2; err = "check %d/%d" % [res, code]; checked.emit(state, ""); return
		remote = str(int(d["v"]))
		state = 1 if int(d["v"]) > Patcher.current() else 0
		checked.emit(state, remote))
	if h.request(BASES[i] + "patch.json") != OK: h.queue_free(); state = 2; checked.emit(state, "")

func download() -> void:
	if state == 3 or state == 4 or remote == "": return
	state = 3; progress = 0.0; err = ""; checked.emit(state, remote)
	_get_pck(0)

func _get_pck(i: int) -> void:
	_h = HTTPRequest.new(); _h.download_file = DL; _h.use_threads = true; _h.max_redirects = 8; _h.timeout = 60.0; add_child(_h)
	var h := _h
	h.request_completed.connect(func(res: int, code: int, _hd: PackedStringArray, _b: PackedByteArray):
		h.queue_free(); _h = null
		if res == HTTPRequest.RESULT_SUCCESS and code == 200 and _install():
			state = 4; progress = 1.0; checked.emit(state, remote); return
		if i + 1 < BASES.size(): _get_pck(i + 1); return
		state = 2; err = "download %d/%d" % [res, code]; checked.emit(state, remote))
	if h.request(BASES[i] + "warzone-patch.pck") != OK: h.queue_free(); _h = null; state = 2; checked.emit(state, remote)

func _process(_dt: float) -> void:
	if state == 3 and is_instance_valid(_h):
		var tot := _h.get_body_size()
		progress = float(_h.get_downloaded_bytes()) / float(tot) if tot > 0 else 0.0
		checked.emit(state, remote)

# تحقّق من الملف (ترويسة GDPC وحجم معقول) ثم ثبّته كحزمة جاهزة للإقلاع التالي
func _install() -> bool:
	var f := FileAccess.open(DL, FileAccess.READ)
	if f == null: return false
	var ok := f.get_length() > 500000 and f.get_buffer(4).get_string_from_ascii() == "GDPC"
	f.close()
	if not ok: DirAccess.remove_absolute(DL); return false
	if FileAccess.file_exists(Patcher.PCK): DirAccess.remove_absolute(Patcher.PCK)
	if DirAccess.rename_absolute(DL, Patcher.PCK) != OK: return false
	var m := FileAccess.open(Patcher.META, FileAccess.WRITE); if m == null: return false
	m.store_string(JSON.stringify({"v": int(remote)})); m.close()
	var t := Patcher.TRY
	if FileAccess.file_exists(t): DirAccess.remove_absolute(t)
	return true
