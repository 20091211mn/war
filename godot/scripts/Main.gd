extends Node3D
# المشهد الرئيسي: القائمة والإعدادات، بناء المباراة، الأسلحة (Hitscan)، الصوت، المشاهد حسب الجودة (أشجار/نوافذ)

var data: Dictionary
var weapons: Array = []
var maps: Array = []
var state := "menu"        # menu | play | pause | over
var inp := {"mv": Vector2.ZERO, "look": Vector2.ZERO, "fire": false, "jump": false, "reload": false, "ads": false, "sprint": false, "next": false, "use": false, "seat": false, "gren": false, "crouch": false, "c4": false, "det": false, "drone": false, "tup": false, "tdn": false}
var map_idx := 2
var diff := 1
var bots_per_team := 5
var player: Player = null
var fighters: Array = []
var vehicles: Array = []
var jets: Array = []
var air := false
var foot_t := 0.0
var nav_map := RID()
var nav_ready := false
var hud: Hud
var world: Node3D
var map_w := 100.0
var map_d := 100.0
var map_info := {}
var score := [0, 0]
var score_lim := 30
var time_left := 600.0
var resp_t := 0.0
var time := 0.0
var show_touch := false
var ui: CanvasLayer
var ui_root: Control
var lan: Lan
var time_cap := 600.0   # 0 = بلا حد (يُعرض الوقت المنقضي)
var match_t := 0.0
var overlay := false   # في LAN: قائمة الإيقاف تُعرض فوق اللعب دون إيقاف المحاكاة
var menu_root: Control
var pause_root: Control
var over_label: Label
var sfx := {}
var scenery: Node3D
var selftest := false
var gren_test := false
var air_test := false
var at_j: Jet = null
var at_f := 0
var gt_f := 0
var lan_test := ""
var test_frames := 0
var shot_path := ""
var menu_shot := ""
var dbg_picker := false
var thumb_path := ""
var _mf := 0
var dbg_wpn := -1
var dbg_rl := -1.0
var dbg_ads := false
var dbg_botcam := false
var dbg_vehcam := -1
var veh_test := -1
var veh_shot := ""

func _ready() -> void:
	var f := FileAccess.open("res://data/game.json", FileAccess.READ)
	data = JSON.parse_string(f.get_as_text())
	maps = data["maps"]; weapons = data["weapons"]
	_make_sfx()
	ui = CanvasLayer.new(); ui.layer = 10; ui.process_mode = Node.PROCESS_MODE_ALWAYS; add_child(ui)
	ui_root = Control.new(); ui_root.theme = UiTheme.make(); ui_root.mouse_filter = Control.MOUSE_FILTER_IGNORE; ui.add_child(ui_root)
	hud = Hud.new(); hud.game = self; ui.add_child(hud)
	Settings.changed.connect(_on_settings)
	lan = Lan.new(); lan.game = self; lan.process_mode = Node.PROCESS_MODE_ALWAYS; lan.name = "Lan"; add_child(lan)
	_build_menu(); _build_pause()
	lan.build_ui()
	_show_menu()
	var args := OS.get_cmdline_user_args()
	selftest = "--selftest" in args
	for a in args:
		if a.begins_with("--shot="): shot_path = a.substr(7)
		if a.begins_with("--menushot="): menu_shot = a.substr(11)
		if a == "--picker": dbg_picker = true
		if a.begins_with("--thumb="): thumb_path = a.substr(8)
		if a.begins_with("--quality="): Settings.quality = int(a.substr(10))
		if a.begins_with("--lang="): Settings.lang = int(a.substr(7))
		if a.begins_with("--map="): map_idx = int(a.substr(6))
		if a.begins_with("--wpn="): dbg_wpn = int(a.substr(6))
		if a.begins_with("--rl="): dbg_rl = float(a.substr(5))
		if a == "--ads": dbg_ads = true
		if a == "--botcam": dbg_botcam = true
		if a.begins_with("--vshot="): veh_shot = a.substr(8)
		if a.begins_with("--vehtest="): veh_test = int(a.substr(10))
		if a == "--grentest": gren_test = true
		if a == "--airtest": air_test = true
		if a.begins_with("--vehcam="): dbg_vehcam = int(a.substr(9))
		if a == "--lanhost": lan_test = "host"
		if a.begins_with("--lanjoin="): lan_test = "join:" + a.substr(10)
	if not (selftest or shot_path != ""): _refresh(); _build_preview()
	if lan_test == "host": selftest = true; lan.host_start()
	elif lan_test.begins_with("join:"): selftest = true; lan.join(lan_test.substr(5))
	elif selftest or shot_path != "":
		start_match()
		if dbg_wpn >= 0 and player != null: player.setup_weapon(dbg_wpn); player.rebuild_viewmodel()
		if dbg_rl >= 0.0 and player != null: player.dbg_rp = dbg_rl
		if dbg_ads: inp["ads"] = true

func cam_pos() -> Vector3:
	if player != null and player.cam != null: return player.cam.global_position
	return Vector3.ZERO

# ---------------------------------------------------------------- الصوت (يُولَّد بالكود)
func _make_sfx() -> void:
	sfx["shot"] = _noise(0.16, 0.9, 5.0, 900.0)
	sfx["reload"] = _noise(0.08, 0.5, 14.0, 2600.0)
	sfx["boom"] = _noise(0.5, 1.0, 3.0, 220.0)

func _noise(dur: float, vol: float, decay: float, tone: float) -> AudioStreamWAV:
	var rate := 22050; var n := int(dur * rate); var b := PackedByteArray(); b.resize(n * 2)
	var lp := 0.0
	for i in n:
		var tt := float(i) / rate
		var raw := randf_range(-1.0, 1.0)
		lp += (raw - lp) * clampf(tone / rate * 6.28, 0.02, 1.0)
		var v := lp * exp(-tt * decay * 4.0) * vol
		b.encode_s16(i * 2, int(clampf(v, -1.0, 1.0) * 30000.0))
	var w := AudioStreamWAV.new(); w.format = AudioStreamWAV.FORMAT_16_BITS; w.mix_rate = rate; w.data = b
	return w

func sfx_play(name: String, pos: Vector3) -> void:
	if selftest: return
	var p := AudioStreamPlayer3D.new(); p.stream = sfx[name]; p.unit_size = 14.0; p.max_distance = 140.0; add_child(p); p.global_position = pos; p.play()
	p.finished.connect(p.queue_free)

# ---------------------------------------------------------------- القوائم (واجهة حديثة)
var _rows: Array = []
var _title: Label
var _sub: Label
var _start_btn: Button
var _res_btn: Button
var _menu_btn: Button
var _pause_title: Label

func _card(parent: Control, min_w := 0.0) -> VBoxContainer:
	var pc := PanelContainer.new(); pc.add_theme_stylebox_override("panel", UiTheme.flat(UiTheme.CARD, 28, UiTheme.BORDER, 1, 24)); pc.custom_minimum_size = Vector2(min_w, 0)
	parent.add_child(pc)
	var m := MarginContainer.new(); m.add_theme_constant_override("margin_left", 26); m.add_theme_constant_override("margin_right", 26); m.add_theme_constant_override("margin_top", 22); m.add_theme_constant_override("margin_bottom", 22); pc.add_child(m)
	var vb := VBoxContainer.new(); vb.add_theme_constant_override("separation", 14); m.add_child(vb); return vb

func _glass_label(t: String, size: int, col := UiTheme.TEXT) -> Label:
	var l := Label.new(); l.text = t; l.add_theme_font_size_override("font_size", size); l.add_theme_color_override("font_color", col)
	l.add_theme_color_override("font_outline_color", Color(0, 0, 0, 0.55)); l.add_theme_constant_override("outline_size", 6); return l

# صف خيارات: شرائح (Segmented) بدل الأسهم — لمسة واحدة لاختيار القيمة
func _seg(parent: Control, label_ar: String, opts: Callable, get_i: Callable, set_i: Callable, fs := 22) -> void:
	var box := VBoxContainer.new(); box.add_theme_constant_override("separation", 8); parent.add_child(box)
	var lab := Label.new(); lab.add_theme_font_size_override("font_size", 19); lab.add_theme_color_override("font_color", UiTheme.MUTED); box.add_child(lab)
	var row := HBoxContainer.new(); row.add_theme_constant_override("separation", 8); box.add_child(row)
	var chips: Array = []
	var n: int = (opts.call() as Array).size()
	for i in n:
		var b := Button.new(); b.custom_minimum_size = Vector2(0, 64); b.size_flags_stretch_ratio = 1.0; b.size_flags_horizontal = Control.SIZE_EXPAND_FILL; b.clip_text = false
		b.add_theme_font_size_override("font_size", fs); UiTheme.juicy(b)
		var idx := i
		b.pressed.connect(func(): set_i.call(idx); _refresh())
		row.add_child(b); chips.append(b)
	_rows.append({"lab": lab, "chips": chips, "ar": label_ar, "opts": opts, "get": get_i})

func _style_chip(b: Button, on: bool) -> void:
	var sel := UiTheme.flat(UiTheme.ACCENT, 18, Color(1, 1, 1, 0.3), 1, 10) if on else UiTheme.flat(Color(1, 1, 1, 0.06), 18, UiTheme.BORDER, 1)
	b.add_theme_stylebox_override("normal", sel)
	b.add_theme_stylebox_override("hover", sel if on else UiTheme.flat(Color(1, 1, 1, 0.12), 18, UiTheme.ACCENT, 1))
	b.add_theme_stylebox_override("pressed", UiTheme.flat(Color("3868cc") if on else Color(1, 1, 1, 0.2), 18))
	b.add_theme_color_override("font_color", Color.WHITE if on else UiTheme.MUTED)

func _bg_layer(root: Control, dim: bool) -> void:
	var bg := ColorRect.new(); bg.set_anchors_preset(Control.PRESET_FULL_RECT); bg.color = Color(0.02, 0.03, 0.07, 0.8 if dim else 0.0); bg.mouse_filter = Control.MOUSE_FILTER_STOP; root.add_child(bg)

func _settings_rows(vb: Control, with_map: bool) -> void:
	if with_map:
		_seg(vb, "الصعوبة", func(): return ["سهل", "عادي", "صعب"].map(func(x): return Settings.t(x)), func(): return diff, func(i): diff = i)
		return
	_seg(vb, "الجودة", func(): return Settings.QUALS.map(func(x): return Settings.t(x)), func(): return Settings.quality, func(i): Settings.quality = i; Settings.save(); Settings.changed.emit(), 16)
	_seg(vb, "اللغة", func(): return Settings.LANGS, func(): return Settings.lang, func(i): Settings.lang = i; Settings.save(); Settings.changed.emit())

var _map_cards: Array = []
var _gear: Button
var _map_lab: Label
var _map_main: PanelContainer
var _map_strip: HBoxContainer
var _map_picker: Control
var _map_title: Label
var _map_meta: Label

func _thumb(i: int, h: float) -> Control:
	var path := "res://assets/maps/map%d.png" % i
	var tr := TextureRect.new(); tr.mouse_filter = Control.MOUSE_FILTER_IGNORE; tr.expand_mode = TextureRect.EXPAND_IGNORE_SIZE; tr.stretch_mode = TextureRect.STRETCH_KEEP_ASPECT_COVERED; tr.custom_minimum_size = Vector2(0, h)
	if ResourceLoader.exists(path): tr.texture = load(path)
	return tr

func _map_card(i: int) -> Control:
	var m: Dictionary = maps[i]; var cols: Array = m["theme"]["cols"]
	var pc := PanelContainer.new(); pc.custom_minimum_size = Vector2(230, 160); pc.clip_contents = true
	var vb := VBoxContainer.new(); vb.add_theme_constant_override("separation", 0); pc.add_child(vb)
	vb.add_child(_thumb(i, 96))
	var l := Label.new(); l.name = "nm"; l.add_theme_font_size_override("font_size", 20); l.horizontal_alignment = HORIZONTAL_ALIGNMENT_CENTER; l.vertical_alignment = VERTICAL_ALIGNMENT_CENTER
	l.autowrap_mode = TextServer.AUTOWRAP_WORD_SMART; l.size_flags_vertical = Control.SIZE_EXPAND_FILL; l.mouse_filter = Control.MOUSE_FILTER_IGNORE; vb.add_child(l)
	var btn := Button.new(); btn.set_anchors_preset(Control.PRESET_FULL_RECT); btn.focus_mode = Control.FOCUS_NONE; btn.flat = true
	btn.add_theme_stylebox_override("normal", StyleBoxEmpty.new()); btn.add_theme_stylebox_override("hover", StyleBoxEmpty.new()); btn.add_theme_stylebox_override("pressed", UiTheme.flat(Color(1, 1, 1, 0.12), 20))
	btn.pressed.connect(_pick_map.bind(i))
	pc.add_child(btn); return pc

var _overlays: Array = []
func _overlay(title_ar: String, min_w: float) -> VBoxContainer:
	var root := Control.new(); root.set_anchors_preset(Control.PRESET_FULL_RECT); root.visible = false; menu_root.add_child(root)
	var dim := ColorRect.new(); dim.color = Color(0.03, 0.05, 0.09, 0.86); dim.set_anchors_preset(Control.PRESET_FULL_RECT); dim.mouse_filter = Control.MOUSE_FILTER_STOP; root.add_child(dim)
	var cc := CenterContainer.new(); cc.set_anchors_preset(Control.PRESET_FULL_RECT); root.add_child(cc)
	var vb := _card(cc, min_w)
	var hb := HBoxContainer.new(); vb.add_child(hb)
	var t := _glass_label("", 38); t.size_flags_horizontal = Control.SIZE_EXPAND_FILL; t.set_meta("ar", title_ar); hb.add_child(t)
	var x := Button.new(); x.text = "✕"; x.custom_minimum_size = Vector2(64, 64); x.add_theme_font_size_override("font_size", 26); UiTheme.style_ghost(x); UiTheme.juicy(x); x.pressed.connect(func(): root.visible = false); hb.add_child(x)
	vb.set_meta("root", root); vb.set_meta("title", t); _overlays.append(vb); return vb

func _open(vb: Control) -> void:
	_refresh(); (vb.get_meta("root") as Control).visible = true

func _pick_map(i: int) -> void:
	map_idx = i
	if _map_picker != null: (_map_picker.get_meta("root") as Control).visible = false
	_refresh(); _build_preview()

func _build_menu() -> void:
	menu_root = Control.new(); menu_root.name = "Menu"; menu_root.set_anchors_preset(Control.PRESET_FULL_RECT); menu_root.mouse_filter = Control.MOUSE_FILTER_IGNORE; ui_root.add_child(menu_root)
	var dim := ColorRect.new(); dim.set_anchors_preset(Control.PRESET_FULL_RECT); dim.color = Color(0.03, 0.05, 0.1, 0.4); dim.mouse_filter = Control.MOUSE_FILTER_IGNORE; menu_root.add_child(dim)
	var mg := MarginContainer.new(); mg.set_anchors_preset(Control.PRESET_FULL_RECT)
	for k in ["left", "right"]: mg.add_theme_constant_override("margin_" + k, 44)
	for k in ["top", "bottom"]: mg.add_theme_constant_override("margin_" + k, 24)
	menu_root.add_child(mg)
	var hb := HBoxContainer.new(); mg.add_child(hb)
	# ---- لوحة رئيسية مدمجة: العنوان + بطاقة الخريطة + الصعوبة + LAN + ابدأ ----
	var panel := _card(hb, 560)
	panel.add_theme_constant_override("separation", 10)
	panel.get_parent().get_parent().size_flags_vertical = Control.SIZE_EXPAND_FILL
	var top := HBoxContainer.new(); panel.add_child(top)
	var tag := Label.new(); tag.text = "RED  VS  BLUE"; tag.add_theme_font_size_override("font_size", 20); tag.add_theme_color_override("font_color", UiTheme.ACCENT2); tag.size_flags_horizontal = Control.SIZE_EXPAND_FILL; top.add_child(tag)
	_gear = Button.new(); _gear.text = "⚙"; _gear.custom_minimum_size = Vector2(60, 56); _gear.add_theme_font_size_override("font_size", 28); UiTheme.style_ghost(_gear); UiTheme.juicy(_gear); top.add_child(_gear)
	_title = Label.new(); _title.text = "WARZONE"; _title.add_theme_font_size_override("font_size", 58); panel.add_child(_title)
	_sub = Label.new(); _sub.add_theme_font_size_override("font_size", 18); _sub.add_theme_color_override("font_color", UiTheme.MUTED); _sub.autowrap_mode = TextServer.AUTOWRAP_WORD_SMART; panel.add_child(_sub)
	var sp := Control.new(); sp.size_flags_vertical = Control.SIZE_EXPAND_FILL; panel.add_child(sp)
	# بطاقة الخريطة الحالية: لمسة واحدة تفتح شاشة اختيار الخرائط
	_map_lab = Label.new(); _map_lab.add_theme_font_size_override("font_size", 19); _map_lab.add_theme_color_override("font_color", UiTheme.MUTED); panel.add_child(_map_lab)
	var mc := PanelContainer.new(); mc.custom_minimum_size = Vector2(0, 96); panel.add_child(mc); _map_main = mc
	var mh := HBoxContainer.new(); mh.add_theme_constant_override("separation", 14); mh.mouse_filter = Control.MOUSE_FILTER_IGNORE; mc.add_child(mh)
	_map_strip = HBoxContainer.new(); _map_strip.add_theme_constant_override("separation", 0); _map_strip.custom_minimum_size = Vector2(160, 0); _map_strip.clip_contents = true; _map_strip.mouse_filter = Control.MOUSE_FILTER_IGNORE; mh.add_child(_map_strip)
	var tv := VBoxContainer.new(); tv.alignment = BoxContainer.ALIGNMENT_CENTER; tv.size_flags_horizontal = Control.SIZE_EXPAND_FILL; tv.mouse_filter = Control.MOUSE_FILTER_IGNORE; mh.add_child(tv)
	_map_title = Label.new(); _map_title.add_theme_font_size_override("font_size", 28); _map_title.mouse_filter = Control.MOUSE_FILTER_IGNORE; tv.add_child(_map_title)
	_map_meta = Label.new(); _map_meta.add_theme_font_size_override("font_size", 17); _map_meta.add_theme_color_override("font_color", UiTheme.MUTED); _map_meta.mouse_filter = Control.MOUSE_FILTER_IGNORE; tv.add_child(_map_meta)
	var ch := Label.new(); ch.text = "▦"; ch.add_theme_font_size_override("font_size", 34); ch.add_theme_color_override("font_color", UiTheme.ACCENT); ch.mouse_filter = Control.MOUSE_FILTER_IGNORE; mh.add_child(ch)
	var mbtn := Button.new(); mbtn.set_anchors_preset(Control.PRESET_FULL_RECT); mbtn.flat = true; mbtn.focus_mode = Control.FOCUS_NONE; mbtn.action_mode = BaseButton.ACTION_MODE_BUTTON_PRESS
	for st in ["normal", "hover"]: mbtn.add_theme_stylebox_override(st, StyleBoxEmpty.new())
	mbtn.add_theme_stylebox_override("pressed", UiTheme.flat(Color(1, 1, 1, 0.1), 20)); mc.add_child(mbtn)
	_settings_rows(panel, true)
	lan.add_menu_buttons(panel)
	_start_btn = Button.new(); _start_btn.custom_minimum_size = Vector2(0, 88); _start_btn.add_theme_font_size_override("font_size", 34); UiTheme.style_accent(_start_btn); UiTheme.juicy(_start_btn); _start_btn.pressed.connect(start_match); panel.add_child(_start_btn)
	# ---- شاشة اختيار الخرائط (شبكة بطاقات) + الإعدادات ----
	var pk := _overlay("اختر الخريطة", 1060)
	var sc := ScrollContainer.new(); sc.horizontal_scroll_mode = ScrollContainer.SCROLL_MODE_DISABLED; sc.custom_minimum_size = Vector2(0, 350); pk.add_child(sc)
	var gr := GridContainer.new(); gr.columns = 4; gr.add_theme_constant_override("h_separation", 14); gr.add_theme_constant_override("v_separation", 14); gr.size_flags_horizontal = Control.SIZE_EXPAND_FILL; sc.add_child(gr)
	for i in maps.size():
		var c := _map_card(i); c.size_flags_horizontal = Control.SIZE_EXPAND_FILL; gr.add_child(c); _map_cards.append(c)
	_map_picker = pk
	mbtn.pressed.connect(func(): _open(pk))
	var st := _overlay("الإعدادات", 640)
	_settings_rows(st, false)
	_gear.pressed.connect(func(): _open(st))
	_refresh()

func _build_pause() -> void:
	pause_root = Control.new(); pause_root.name = "Pause"; pause_root.set_anchors_preset(Control.PRESET_FULL_RECT); pause_root.mouse_filter = Control.MOUSE_FILTER_IGNORE; ui_root.add_child(pause_root); _bg_layer(pause_root, true)
	var cc := CenterContainer.new(); cc.set_anchors_preset(Control.PRESET_FULL_RECT); pause_root.add_child(cc)
	var vb := _card(cc, 620)
	_pause_title = Label.new(); _pause_title.horizontal_alignment = HORIZONTAL_ALIGNMENT_CENTER; _pause_title.add_theme_font_size_override("font_size", 44); vb.add_child(_pause_title)
	over_label = Label.new(); over_label.horizontal_alignment = HORIZONTAL_ALIGNMENT_CENTER; over_label.add_theme_font_size_override("font_size", 34); over_label.add_theme_color_override("font_color", UiTheme.ACCENT2); vb.add_child(over_label)
	_settings_rows(vb, false)
	_res_btn = Button.new(); _res_btn.custom_minimum_size = Vector2(0, 80); _res_btn.add_theme_font_size_override("font_size", 30); UiTheme.style_accent(_res_btn); UiTheme.juicy(_res_btn); _res_btn.pressed.connect(resume); vb.add_child(_res_btn)
	_menu_btn = Button.new(); _menu_btn.custom_minimum_size = Vector2(0, 72); _menu_btn.add_theme_font_size_override("font_size", 26); UiTheme.style_ghost(_menu_btn); UiTheme.juicy(_menu_btn); _menu_btn.pressed.connect(_show_menu); vb.add_child(_menu_btn)

func _refresh() -> void:
	if _title == null: return
	_sub.text = Settings.t("حرب فرق ثلاثية الأبعاد — الأحمر ضد الأزرق")
	_start_btn.text = Settings.t("ابدأ اللعب")
	for r in _rows:
		r["lab"].text = Settings.t(r["ar"])
		var o: Array = r["opts"].call(); var g: int = r["get"].call(); var chips: Array = r["chips"]
		for i in chips.size():
			(chips[i] as Button).text = str(o[i]); _style_chip(chips[i], i == g)
	if _pause_title != null:
		var ov := state == "over"
		_pause_title.text = Settings.t("انتهت المباراة" if ov else "إيقاف مؤقت"); _res_btn.text = Settings.t("مباراة جديدة" if ov else "متابعة"); _menu_btn.text = Settings.t("القائمة الرئيسية")
	for i in _map_cards.size():
		var pc: PanelContainer = _map_cards[i]; var on := i == map_idx
		pc.add_theme_stylebox_override("panel", UiTheme.flat(Color(0.2, 0.35, 0.75, 0.92) if on else Color(0.08, 0.1, 0.18, 0.82), 20, UiTheme.ACCENT if on else UiTheme.BORDER, 3 if on else 1, 12 if on else 0))
		(pc.find_child("nm", true, false) as Label).text = Settings.t(maps[i]["name"])
	for o in _overlays:
		var tl := o.get_meta("title") as Label; tl.text = Settings.t(tl.get_meta("ar"))
	if _map_title != null:
		_map_lab.text = Settings.t("الخريطة"); _map_title.text = Settings.t(maps[map_idx]["name"])
		_map_meta.text = "%d × %d m" % [int(maps[map_idx]["W"]), int(maps[map_idx]["D"])]
		_map_main.add_theme_stylebox_override("panel", UiTheme.flat(Color(1, 1, 1, 0.07), 18, UiTheme.BORDER, 1, 10))
		for c in _map_strip.get_children(): c.queue_free()
		var th := _thumb(map_idx, 0); th.size_flags_horizontal = Control.SIZE_EXPAND_FILL; _map_strip.add_child(th)
	var rtl := Settings.lang == 0
	ui_root.layout_direction = Control.LAYOUT_DIRECTION_RTL if rtl else Control.LAYOUT_DIRECTION_LTR

# معاينة ثلاثية الأبعاد حيّة خلف القائمة (كاميرا تدور ببطء)
var prev_world: Node3D
var prev_cam: Camera3D
func _build_preview() -> void:
	if prev_world != null and is_instance_valid(prev_world): prev_world.queue_free()
	prev_world = Node3D.new(); prev_world.name = "Preview"; add_child(prev_world)
	var m: Dictionary = maps[map_idx]
	MapBuilder.build(m, prev_world)
	prev_cam = Camera3D.new(); prev_cam.fov = 62.0; prev_cam.far = 600.0; prev_world.add_child(prev_cam); prev_cam.current = true
	_orbit(0.0)

func _orbit(t: float) -> void:
	if prev_cam == null or not is_instance_valid(prev_cam): return
	var m: Dictionary = maps[map_idx]; var r := minf(float(m["W"]), float(m["D"])) * 0.32
	var a := t * 0.12
	prev_cam.position = Vector3(cos(a) * r, 11.0 + r * 0.18, sin(a) * r)
	prev_cam.look_at(Vector3(0, 1.5, 0))

func _on_settings() -> void:
	_refresh(); apply_quality()

func _fade_in(c: Control) -> void:
	c.modulate.a = 0.0; var tw := c.create_tween(); tw.tween_property(c, "modulate:a", 1.0, 0.18)

func _show_menu() -> void:
	if lan != null: lan.leave()
	overlay = false
	state = "menu"; menu_root.visible = true; _fade_in(menu_root); pause_root.visible = false; hud.visible = false; map_info.clear()
	Input.mouse_mode = Input.MOUSE_MODE_VISIBLE; get_tree().paused = false
	if world != null: world.queue_free(); world = null
	for f in fighters: if is_instance_valid(f): f.queue_free()
	fighters.clear(); player = null; _refresh(); _build_preview()

func _set_paused(b: bool) -> void:
	get_tree().paused = b and lan.role == 0

func open_pause() -> void:
	if state != "play": return
	overlay = lan.role != 0
	state = "pause" if lan.role == 0 else "play"; pause_root.visible = true; over_label.text = ""; _fade_in(pause_root); _set_paused(true); Input.mouse_mode = Input.MOUSE_MODE_VISIBLE; hud.mouse_look = false
	_release_inputs(); _refresh()

# تفريغ كل المدخلات واللمسات العالقة (عند الإيقاف/نهاية المباراة) كي لا يبقى جويستيك أو إطلاق وهمي
func _release_inputs() -> void:
	inp["fire"] = false; inp["mv"] = Vector2.ZERO; inp["look"] = Vector2.ZERO; inp["ads"] = false; inp["sprint"] = false; inp["jump"] = false; inp["use"] = false; inp["seat"] = false; inp["gren"] = false; inp["crouch"] = false; inp["c4"] = false; inp["det"] = false; inp["drone"] = false; inp["tup"] = false; inp["tdn"] = false
	hud.reset_touch()

func resume() -> void:
	overlay = false
	if state == "over":  # بعد نهاية المباراة: الزر "مباراة جديدة" وليس متابعة المباراة المنتهية
		get_tree().paused = false
		if lan.role == 1: lan.host_restart()
		elif lan.role == 0: start_match()
		return
	state = "play"; pause_root.visible = false; get_tree().paused = false

# ---------------------------------------------------------------- بناء المباراة
func start_match() -> void:
	if prev_world != null and is_instance_valid(prev_world): prev_world.queue_free()
	prev_world = null; prev_cam = null
	if world != null: world.queue_free()
	for f in fighters: if is_instance_valid(f): f.queue_free()
	fighters.clear(); vehicles.clear()
	world = Node3D.new(); world.name = "World"; add_child(world)
	var m: Dictionary = maps[map_idx]
	map_info = MapBuilder.build(m, world); map_w = m["W"]; map_d = m["D"]
	score = [0, 0]; time_left = 600.0; time_cap = 600.0; match_t = 0.0; resp_t = 0.0; score_lim = 30
	player = Player.new(); player.game = self; player.team = 0; player.fname = Settings.t("اللاعب"); world.add_child(player)
	player.setup_weapon(0); fighters.append(player); player.respawn(spawn_point(0))
	for t in 2:
		for i in bots_per_team - (1 if t == 0 else 0):
			var b := Bot.new(); b.game = self; b.team = t; b.fname = "BOT%d" % (fighters.size()); b.acc = [0.09, 0.055, 0.03][diff]
			world.add_child(b); fighters.append(b); b.respawn(spawn_point(t))
	scenery = Node3D.new(); world.add_child(scenery); build_scenery(); apply_quality()
	air = map_idx == 8; jets.clear()
	if air:
		nav_ready = false; air_init()
	else:
		spawn_vehicles(m)
		nav_ready = false; nav_map = RID(); Nav.bake(self, m, world)
	menu_root.visible = false; pause_root.visible = false; hud.visible = true; state = "play"
	hud.say(Settings.t("اختر سلاحك") if false else m["name"])

# مباراة LAN: role 1 مضيف (slot 0 = لاعب المضيف) أو 2 عميل (slot = خانتي). كل الخانات n؛ الفريق = رقم الخانة & 1
func start_lan_match(role: int, slot: int, cfg: Dictionary) -> void:
	if prev_world != null and is_instance_valid(prev_world): prev_world.queue_free()
	prev_world = null; prev_cam = null
	if world != null: world.queue_free()
	for f in fighters: if is_instance_valid(f): f.queue_free()
	fighters.clear()
	map_idx = int(cfg["map"]); if map_idx == 8: map_idx = 0   # حرب الجو لاعب واحد فقط حالياً
	air = false; jets.clear(); world = Node3D.new(); world.name = "World"; add_child(world)
	var m: Dictionary = maps[map_idx]
	map_info = MapBuilder.build(m, world); map_w = m["W"]; map_d = m["D"]
	score = [0, 0]; score_lim = int(cfg["lim"]); var mins := int(cfg["mins"]); time_cap = mins * 60.0; time_left = time_cap if mins > 0 else 1.0e9; match_t = 0.0; resp_t = 0.0
	var n := int(cfg["n"]); var nbots := int(cfg["bots"])
	for i in n:
		if i == slot:
			player = Player.new(); player.game = self; player.team = i & 1; player.fname = Settings.t("اللاعب"); world.add_child(player)
			player.setup_weapon(0); fighters.append(player); player.respawn(spawn_point(player.team))
		else:
			var b := Bot.new(); b.game = self; b.team = i & 1; b.fname = "P%d" % i if role == 2 else "BOT%d" % i; b.acc = [0.09, 0.055, 0.03][diff]
			world.add_child(b); fighters.append(b)
			if role == 2: b.net_ctrl = true; b.net_pos = spawn_point(b.team); b.visible = false; b.alive = false   # دمية: الحالة تأتي من المضيف
			elif i > nbots: b.off = true; b.alive = false; b.visible = false; b.fname = "-"; b.global_position = Vector3(0, -50, 0)
			else: b.respawn(spawn_point(b.team))
	scenery = Node3D.new(); world.add_child(scenery); build_scenery(); apply_quality()
	menu_root.visible = false; pause_root.visible = false; for r in lan.screens(): r.visible = false
	hud.visible = true; state = "play"; overlay = false; get_tree().paused = false
	hud.say(m["name"])

func spawn_point(team: int) -> Vector3:
	var arr: Array = maps[map_idx]["red"] if team == 0 else maps[map_idx]["blue"]
	var p: Array = arr[randi() % arr.size()]
	return Vector3(p[0], 0.3, p[1])

func random_point() -> Vector3:
	return Vector3(randf_range(-map_w * 0.45, map_w * 0.45), 0.3, randf_range(-map_d * 0.45, map_d * 0.45))

func random_weapon() -> int:
	return [0, 0, 1, 2, 4, 5, 6][randi() % 7]

func switch_weapon(f: Fighter, d: int) -> void:
	var order := [0, 1, 2, 5, 3, 4, 6, 7, 8]
	var i := order.find(f.wi); if i < 0: i = 0
	f.setup_weapon(order[(i + d + order.size()) % order.size()])
	if f is Player: (f as Player).rebuild_viewmodel(); hud.say(Settings.t(f.weapon()["name"]))

# ---------------------------------------------------------------- الإطلاق والضرر
func hitscan(shooter: Fighter, from: Vector3, dir: Vector3, w: Dictionary, muz := Vector3.INF) -> void:
	if lan.role == 2 and shooter == player: lan.send_fire(from, dir, shooter.wi)   # العميل: المضيف يحسم الضرر
	if lan.role == 1: lan.send_fx(fighters.find(shooter), from, dir)
	var to := from + dir * 220.0
	var ex: Array = [shooter.get_rid()]
	if shooter.veh != null: ex.append((shooter.veh as Vehicle).hit_body.get_rid()); ex.append(shooter.veh.get_rid())
	var q := PhysicsRayQueryParameters3D.create(from, to, 7, ex)
	var r := get_world_3d().direct_space_state.intersect_ray(q)
	var endp := to
	if w["kind"] == "launcher" and not jets.is_empty():
		var bd := (r["position"] as Vector3).distance_to(from) if not r.is_empty() else 220.0
		for j in jets:
			var J := j as Jet
			if not J.alive or J.team == shooter.team: continue
			var pv := J.position - from; var tc := pv.dot(dir)
			if tc > 0.0 and tc < bd and (pv - dir * tc).length() < 6.5:
				_explode(shooter, from + dir * tc); _tracer(from + dir * 0.8, from + dir * tc); return
	if not r.is_empty():
		endp = r["position"]
		var c: Object = r["collider"]
		if w["kind"] == "launcher":
			_explode(shooter, endp)
		elif c is Fighter and (c as Fighter).team != shooter.team:
			var f := c as Fighter
			var d := from.distance_to(endp)
			var r0: float = w["range"][0]; var r1: float = w["range"][1]
			var k := lerpf(1.0, float(w["minMul"]), clampf((d - r0) / maxf(r1 - r0, 1.0), 0.0, 1.0))
			var head: bool = endp.y > f.global_position.y + 1.5
			f.take_damage(float(w["dmg"]) * k * (float(w["head"]) if head else 1.0), shooter, head)
			if shooter == player: hud.hitmark()
			elif lan.role == 1 and shooter is Bot and (shooter as Bot).human: lan.send_hit(fighters.find(shooter), head)
		elif c is StaticBody3D and (c as StaticBody3D).has_meta("veh"):
			var v: Vehicle = (c as StaticBody3D).get_meta("veh")
			if v.team < 0 or v.team != shooter.team: v.take_hit(float(w["dmg"]), int(r["shape"]), shooter)
			_spark(endp)
		else:
			_spark(endp)
	if shooter == player or from.distance_to(cam_pos()) < 60.0:
		var st: Vector3 = muz
		if st == Vector3.INF: st = (shooter as Fighter).muzzle_pos() if shooter.has_method("muzzle_pos") else from + dir * 0.8
		_tracer(st, endp); _bullet(st, endp)

# مؤثرات طلقة لاعب آخر على العميل (رسم فقط بلا ضرر)
func fx_shot(slot: int, from: Vector3, dir: Vector3) -> void:
	if slot < 0 or slot >= fighters.size(): return
	var f: Fighter = fighters[slot]
	if not is_instance_valid(f): return
	var to := from + dir * 220.0
	var r := get_world_3d().direct_space_state.intersect_ray(PhysicsRayQueryParameters3D.create(from, to, 3, [f.get_rid()]))
	var endp: Vector3 = r["position"] if not r.is_empty() else to
	if f is Bot: (f as Bot).kick = 1.0
	sfx_play("shot", from)
	if from.distance_to(cam_pos()) < 90.0:
		var st: Vector3 = (f as Bot).muzzle_pos() if f is Bot else from
		_tracer(st, endp); _bullet(st, endp)

func _explode(src: Fighter, p: Vector3) -> void:
	blast(src, p, 5.0, float(weapons[8]["dmg"]), 420.0)
	fx_blast(p, 0.7)

# انفجار عام: أذى للمقاتلين (بلا أذى للفريق إلا الذات) وللمركبات
func blast(src: Fighter, p: Vector3, R: float, pd: float, vd: float) -> void:
	sfx_play("boom", p)
	for f in fighters:
		if not is_instance_valid(f) or not f.alive or f.jet != null: continue
		var d: float = f.global_position.distance_to(p)
		if d < R and (f.team != src.team or f == src):
			f.take_damage(pd * pow(1.0 - d / R, 0.9), src, false)
	for v in vehicles:
		if not is_instance_valid(v) or v.dead: continue
		if v.team >= 0 and v.team == src.team and not (v.driver == src or v.gunner == src): continue
		var d := maxf(0.0, Vector2(v.global_position.x - p.x, v.global_position.z - p.z).length() - 1.6)
		if d < R + 1.5: v.take_damage(vd * pow(1.0 - d / (R + 1.5), 1.1), src)
	for j in jets.duplicate():
		var J := j as Jet
		if not is_instance_valid(J) or not J.alive or J.team == src.team: continue
		var dj := J.position.distance_to(p)
		if dj < R + 5.0: J.hurt(vd * 0.5 * (1.0 - dj / (R + 5.0)), src, true)
	_spark(p, 14)

# قذيفة مدفع الدبابة
func cannon_hit(src: Fighter, p: Vector3, from_v: Vehicle) -> void:
	blast(src, p, 6.5, 230.0, 420.0)
	fx_blast(p, 0.9)

# كرة نار + دخان + شظايا + وميض ضوئي
func fx_blast(p: Vector3, s: float) -> void:
	if world == null or Settings.quality < 1 and s < 0.9: return
	var ball := MeshInstance3D.new(); var sm := SphereMesh.new(); sm.radius = 1.0; sm.height = 2.0; sm.radial_segments = 12; sm.rings = 6; ball.mesh = sm
	var m := StandardMaterial3D.new(); m.shading_mode = BaseMaterial3D.SHADING_MODE_UNSHADED; m.albedo_color = Color(1, 0.6, 0.2, 0.9); m.transparency = BaseMaterial3D.TRANSPARENCY_ALPHA; m.blend_mode = BaseMaterial3D.BLEND_MODE_ADD; ball.material_override = m
	ball.cast_shadow = GeometryInstance3D.SHADOW_CASTING_SETTING_OFF; world.add_child(ball); ball.global_position = p; ball.scale = Vector3.ONE * 0.3 * s
	var tw := create_tween(); tw.set_parallel(true)
	tw.tween_property(ball, "scale", Vector3.ONE * 3.2 * s, 0.35).set_ease(Tween.EASE_OUT)
	tw.tween_property(m, "albedo_color:a", 0.0, 0.5)
	tw.chain().tween_callback(ball.queue_free)
	if Settings.quality >= 2:
		var l := OmniLight3D.new(); l.light_color = Color(1, 0.65, 0.3); l.light_energy = 6.0 * s; l.omni_range = 18.0 * s; l.shadow_enabled = false; world.add_child(l); l.global_position = p + Vector3(0, 1.0, 0)
		var tl := create_tween(); tl.tween_property(l, "light_energy", 0.0, 0.5); tl.tween_callback(l.queue_free)
	var sp := CPUParticles3D.new(); sp.one_shot = true; sp.emitting = true; sp.amount = int(20 * s * [0.3, 0.6, 1.0, 1.2, 1.5][Settings.quality]) + 4; sp.lifetime = 2.4; sp.explosiveness = 0.9; sp.direction = Vector3.UP; sp.spread = 50.0
	sp.initial_velocity_min = 2.0 * s; sp.initial_velocity_max = 6.0 * s; sp.gravity = Vector3(0, 0.6, 0); sp.scale_amount_min = 1.2 * s; sp.scale_amount_max = 2.6 * s
	var qm := QuadMesh.new(); var pm := StandardMaterial3D.new(); pm.shading_mode = BaseMaterial3D.SHADING_MODE_UNSHADED; pm.vertex_color_use_as_albedo = true; pm.billboard_mode = BaseMaterial3D.BILLBOARD_PARTICLES; pm.transparency = BaseMaterial3D.TRANSPARENCY_ALPHA; pm.cull_mode = BaseMaterial3D.CULL_DISABLED; qm.material = pm; sp.mesh = qm
	var g := Gradient.new(); g.set_color(0, Color(0.25, 0.22, 0.2, 0.85)); g.set_color(1, Color(0.1, 0.1, 0.1, 0.0)); sp.color_ramp = g
	sp.cast_shadow = GeometryInstance3D.SHADOW_CASTING_SETTING_OFF; world.add_child(sp); sp.global_position = p
	get_tree().create_timer(3.0).timeout.connect(sp.queue_free)
	if s >= 0.9: _spark(p, 30)

func spawn_vehicles(m: Dictionary) -> void:
	if lan != null and lan.role != 0: return   # المركبات في اللعب الفردي (LAN لاحقاً)
	for vd in m["vehicles"]:
		var v := Vehicle.new(); world.add_child(v); v.setup(self, int(vd[3]), float(vd[0]), float(vd[1]), float(vd[2])); vehicles.append(v)

# ركوب/نزول: أقرب مركبة ضمن 2.8م من الجسم
func near_vehicle(f: Fighter) -> Vehicle:
	var best: Vehicle = null; var bd := 2.8
	for v in vehicles:
		if not is_instance_valid(v) or v.seat_for(f) < 0: continue
		var l: Vector3 = v.to_local(f.global_position)
		var dd := Vector2(maxf(0.0, absf(l.x) - float(v.P["hw"])), maxf(0.0, absf(l.z) - float(v.P["hl"]))).length()
		if dd < bd and l.y < 2.3: bd = dd; best = v
	return best

func try_use(f: Fighter) -> void:
	if air:
		if f.jet != null: air_eject(f as Player); return
		var J := near_jet(f)
		if J != null: air_board(f as Player, J); return
	if f.veh != null: (f.veh as Vehicle).leave(f); return
	var v := near_vehicle(f)
	if v != null: v.enter(f, v.seat_for(f))

func _spark(p: Vector3, n := 3) -> void:
	if Settings.quality < 1: return
	n = int(n * [0.3, 0.6, 1.0, 1.2, 1.6][Settings.quality])
	for i in n:
		var mi := MeshInstance3D.new(); var bm := BoxMesh.new(); bm.size = Vector3.ONE * 0.07; mi.mesh = bm
		var m := StandardMaterial3D.new(); m.albedo_color = Color(1, 0.7, 0.2); m.emission_enabled = true; m.emission = Color(1, 0.6, 0.1); mi.material_override = m
		mi.cast_shadow = GeometryInstance3D.SHADOW_CASTING_SETTING_OFF; world.add_child(mi); mi.global_position = p
		var tw := create_tween(); tw.tween_property(mi, "global_position", p + Vector3(randf_range(-1, 1), randf_range(0.2, 1.2), randf_range(-1, 1)) * 0.6, 0.25); tw.tween_callback(mi.queue_free)

# رصاصة مرئية (كبسولة نحاسية متوهجة تنطلق من الفوهة إلى نقطة الإصابة)
func _bullet(a: Vector3, b: Vector3) -> void:
	if Settings.quality < 1: return
	var l := a.distance_to(b)
	if l < 2.0: return
	var mi := MeshInstance3D.new(); var cm := CapsuleMesh.new(); cm.radius = 0.014; cm.height = 0.16; cm.radial_segments = 6; cm.rings = 2; mi.mesh = cm
	var m := StandardMaterial3D.new(); m.albedo_color = Color(1.0, 0.75, 0.3); m.shading_mode = BaseMaterial3D.SHADING_MODE_UNSHADED; mi.material_override = m
	mi.cast_shadow = GeometryInstance3D.SHADOW_CASTING_SETTING_OFF; world.add_child(mi)
	mi.look_at_from_position(a, b, Vector3.UP); mi.rotate_object_local(Vector3.RIGHT, PI * 0.5)  # محور الكبسولة Y -> اتجاه الإطلاق
	var tw := create_tween(); tw.tween_property(mi, "global_position", b, clampf(l / 380.0, 0.02, 0.4)); tw.tween_callback(mi.queue_free)

# ظرف فارغ يُطرد من فتحة الإخراج ويسقط بتأثير الجاذبية
func eject_shell(p: Vector3, bs: Basis) -> void:
	if Settings.quality < 2 or world == null: return
	var mi := MeshInstance3D.new(); var cm := CylinderMesh.new(); cm.top_radius = 0.006; cm.bottom_radius = 0.007; cm.height = 0.045; cm.radial_segments = 6; mi.mesh = cm
	var m := StandardMaterial3D.new(); m.albedo_color = Color(0.85, 0.65, 0.2); m.metallic = 0.9; m.roughness = 0.35; mi.material_override = m
	mi.cast_shadow = GeometryInstance3D.SHADOW_CASTING_SETTING_OFF; world.add_child(mi); mi.global_position = p
	var v := bs.x.normalized() * randf_range(1.6, 2.4) + bs.y.normalized() * randf_range(1.0, 1.8) + bs.z.normalized() * randf_range(0.2, 0.6)
	var spin := Vector3(randf_range(-9, 9), randf_range(-9, 9), randf_range(-9, 9))
	var tw := mi.create_tween(); tw.tween_method(func(t: float):
		if is_instance_valid(mi): mi.global_position = p + v * t + Vector3(0, -4.9 * t * t, 0); mi.rotation = spin * t, 0.0, 0.7, 0.7)
	tw.tween_callback(mi.queue_free)

func _tracer(a: Vector3, b: Vector3) -> void:
	var l := a.distance_to(b)
	if l < 1.0: return
	var mi := MeshInstance3D.new(); var bm := BoxMesh.new(); bm.size = Vector3(0.025, 0.025, l); mi.mesh = bm
	var m := StandardMaterial3D.new(); m.albedo_color = Color(1, 0.85, 0.4, 0.8); m.shading_mode = BaseMaterial3D.SHADING_MODE_UNSHADED; m.transparency = BaseMaterial3D.TRANSPARENCY_ALPHA; mi.material_override = m
	mi.cast_shadow = GeometryInstance3D.SHADOW_CASTING_SETTING_OFF; world.add_child(mi); mi.look_at_from_position((a + b) * 0.5, b, Vector3.UP)
	var tw := create_tween(); tw.tween_property(m, "albedo_color:a", 0.0, 0.08); tw.tween_callback(mi.queue_free)

func on_kill(src: Fighter, victim: Fighter, head: bool) -> void:
	if src != null and src != victim: score[src.team] += 1
	var kn := (src.fname if src != null else "-")
	hud.add_feed("%s  ✕  %s%s" % [kn, victim.fname, "  ★" if head else ""])
	if src != null and src != victim:
		var ms: int = int(src.get_meta("streak", 0)) + 1; src.set_meta("streak", ms)
		var lt := Time.get_ticks_msec() / 1000.0; var multi: int = (int(src.get_meta("multi", 0)) + 1) if lt - float(src.get_meta("lastk", -9.0)) < 4.0 else 1
		src.set_meta("multi", multi); src.set_meta("lastk", lt)
		if src == player:
			var sm := {3: "ثلاث قتلات متتالية!", 5: "خمس قتلات - لا يُوقف!", 7: "سلسلة مرعبة x7", 10: "أسطورة! عشر قتلات"}
			var mm := {2: "قتل مزدوج", 3: "قتل ثلاثي", 4: "قتل رباعي"}
			if sm.has(ms): hud.say(Settings.t(sm[ms]))
			elif mm.has(multi): hud.say(Settings.t(mm[multi]))
			else: hud.say(Settings.t("تصويبة رأس!") if head else victim.fname)
	victim.set_meta("streak", 0); victim.set_meta("multi", 0)
	if false: pass
	if lan.role == 1: lan.send_feed("%s  ✕  %s%s" % [kn, victim.fname, "  ★" if head else ""])
	if victim == player: resp_t = 4.0
	else:
		if victim is Bot and not air: (victim as Bot).play_death(src.global_position if src != null else victim.global_position)
		get_tree().create_timer(2.2).timeout.connect(func(): if is_instance_valid(victim) and not victim.alive: victim.visible = false)
		var hum := victim is Bot and (victim as Bot).net_ctrl
		get_tree().create_timer(4.0 if hum else 3.0).timeout.connect(func():
			if is_instance_valid(victim) and state != "menu" and not (victim as Bot).off:
				if air: air_spawn(victim)
				elif hum: lan.respawn_human(victim as Bot)
				else: (victim as Bot).respawn(spawn_point(victim.team)))
	if score[0] >= score_lim or score[1] >= score_lim: _end_match()

func _end_match() -> void:
	if state == "over": return
	state = "over"; overlay = false; pause_root.visible = true; _set_paused(true); Input.mouse_mode = Input.MOUSE_MODE_VISIBLE; hud.mouse_look = false; _release_inputs(); _refresh()
	over_label.text = Settings.t("فاز الفريق الأحمر") if score[0] >= score[1] else Settings.t("فاز الفريق الأزرق")

# ---------------------------------------------------------------- الجودة والمشاهد
func apply_quality() -> void:
	var vp := get_viewport()
	vp.scaling_3d_scale = Settings.res_scale()
	vp.msaa_3d = Viewport.MSAA_2X if Settings.msaa() > 0 else Viewport.MSAA_DISABLED
	if map_info.has("sun") and is_instance_valid(map_info["sun"]) and world != null:
		(map_info["sun"] as DirectionalLight3D).shadow_enabled = Settings.shadows()
		var th: Dictionary = map_info["theme"]; var env: Environment = map_info["env"]
		var k := Settings.fog_k()
		env.fog_depth_begin = float(th["fogN"]) * k; env.fog_depth_end = float(th["fogF"]) * k
		env.fog_mode = Environment.FOG_MODE_DEPTH; env.fog_depth_curve = 1.0; env.fog_density = 1.0
	if world != null and scenery != null and is_instance_valid(scenery): build_scenery()

var _tree_shader: Shader
func _get_tree_shader() -> Shader:
	if _tree_shader == null:
		_tree_shader = Shader.new()
		_tree_shader.code = "shader_type spatial;\nuniform vec3 col : source_color = vec3(0.15,0.4,0.15);\nuniform float y0 = 0.0;\nvoid vertex(){ float ph = INSTANCE_CUSTOM.x; float h = max(VERTEX.y + y0, 0.0); float s = sin(TIME*1.25+ph)*0.09 + sin(TIME*3.1+ph*2.0)*0.025; VERTEX.x += s*h*0.35; VERTEX.z += s*h*0.2; }\nvoid fragment(){ ALBEDO = col; ROUGHNESS = 1.0; }\n"
	return _tree_shader

func build_scenery() -> void:
	for c in scenery.get_children(): c.queue_free()
	var m: Dictionary = maps[map_idx]
	var th: Dictionary = m["theme"]
	var snow := map_idx == 3; var desert := map_idx in [1, 4, 5, 7]
	var nt := Settings.trees()
	if nt > 0 and map_idx < 8:
		var pts := _tree_points(m, nt)
		if pts.size() > 0:
			var trunk := MultiMesh.new(); trunk.transform_format = MultiMesh.TRANSFORM_3D; trunk.use_custom_data = true
			var cm := CylinderMesh.new(); cm.top_radius = 0.1; cm.bottom_radius = 0.16; cm.height = 3.6; trunk.mesh = cm; trunk.instance_count = pts.size()
			var fol := MultiMesh.new(); fol.transform_format = MultiMesh.TRANSFORM_3D; fol.use_custom_data = true
			var sph := SphereMesh.new(); sph.radius = 1.4; sph.height = 2.4 if not desert else 0.8; fol.mesh = sph; fol.instance_count = pts.size()
			for i in pts.size():
				var p: Vector3 = pts[i]; var s := p.y; var ph := randf() * 6.28
				trunk.set_instance_transform(i, Transform3D(Basis.from_scale(Vector3(s, s, s)), Vector3(p.x, 1.8 * s, p.z))); trunk.set_instance_custom_data(i, Color(ph, 0, 0, 0))
				fol.set_instance_transform(i, Transform3D(Basis.from_scale(Vector3(s, s, s)), Vector3(p.x, 3.9 * s, p.z))); fol.set_instance_custom_data(i, Color(ph, 0, 0, 0))
			var tm := ShaderMaterial.new(); tm.shader = _get_tree_shader(); tm.set_shader_parameter("col", Vector3(0.33, 0.22, 0.12)); tm.set_shader_parameter("y0", 1.8)
			var fm := ShaderMaterial.new(); fm.shader = _get_tree_shader(); fm.set_shader_parameter("col", Vector3(0.16, 0.34, 0.2) if snow else (Vector3(0.3, 0.5, 0.16) if desert else Vector3(0.14, 0.42, 0.16))); fm.set_shader_parameter("y0", 3.9)
			var a := MultiMeshInstance3D.new(); a.multimesh = trunk; a.material_override = tm; scenery.add_child(a)
			var b := MultiMeshInstance3D.new(); b.multimesh = fol; b.material_override = fm; scenery.add_child(b)
	var nw := Settings.windows()
	if nw > 0 and map_idx < 8:
		var dark: Array = []; var lit: Array = []
		var used := 0
		for bx in m["boxes"]:
			if used >= nw: break
			var bw: float = bx[3] - bx[0]; var bd: float = bx[5] - bx[2]; var bh: float = bx[4] - bx[1]
			if bh < 4.0 or bx[1] > 2.0 or maxf(bw, bd) > 50.0 or minf(bw, bd) < 2.5: continue
			var cx: float = (bx[0] + bx[3]) * 0.5; var cz: float = (bx[2] + bx[5]) * 0.5
			for f in 4:
				if used >= nw: break
				var zf := f < 2; var face: float = bx[2] if f == 0 else (bx[5] if f == 1 else (bx[0] if f == 2 else bx[3]))
				var ln := bw if zf else bd; var cnt := maxi(1, int(ln / 2.6)); var step := ln / cnt
				for i2 in cnt:
					var wy: float = bx[1] + 1.6
					while wy < bx[4] - 1.2 and used < nw:
						var o := (i2 + 0.5) * step - ln * 0.5
						var pos := Vector3(cx + o, wy + 0.7, face + (-0.03 if f == 0 else 0.03)) if zf else Vector3(face + (-0.03 if f == 2 else 0.03), wy + 0.7, cz + o)
						var t := Transform3D(Basis.from_scale(Vector3(1.1, 1.4, 0.08) if zf else Vector3(0.08, 1.4, 1.1)), pos)
						if ((i2 * 7 + int(wy * 3) + map_idx) % 5) == 0: lit.append(t)
						else: dark.append(t)
						used += 1; wy += 3.2
		_win_mm(dark, Color(0.1, 0.2, 0.32), false); _win_mm(lit, Color(0.95, 0.85, 0.5), true)

func _win_mm(arr: Array, c: Color, emit: bool) -> void:
	if arr.is_empty(): return
	var mm := MultiMesh.new(); mm.transform_format = MultiMesh.TRANSFORM_3D; mm.mesh = BoxMesh.new(); mm.instance_count = arr.size()
	for i in arr.size(): mm.set_instance_transform(i, arr[i])
	var mi := MultiMeshInstance3D.new(); mi.multimesh = mm
	var mat := StandardMaterial3D.new(); mat.albedo_color = c; mat.roughness = 0.2; mat.metallic = 0.3
	if emit: mat.emission_enabled = true; mat.emission = c; mat.emission_energy_multiplier = 0.9
	mi.material_override = mat; mi.cast_shadow = GeometryInstance3D.SHADOW_CASTING_SETTING_OFF; scenery.add_child(mi)

func _tree_points(m: Dictionary, nt: int) -> Array:
	var rng := RandomNumberGenerator.new(); rng.seed = 7919 * (map_idx + 7)
	var out: Array = []
	var W: float = m["W"]; var D: float = m["D"]
	var tries := 0
	while out.size() < nt and tries < 900:
		tries += 1
		var x := (rng.randf() - 0.5) * W * 0.96; var z := (rng.randf() - 0.5) * D * 0.96; var bad := false
		for bx in m["boxes"]:
			if x > bx[0] - 2.2 and x < bx[3] + 2.2 and z > bx[2] - 2.2 and z < bx[5] + 2.2 and bx[4] > 0.3: bad = true; break
		if not bad:
			for sp in m["red"] + m["blue"]:
				if (x - sp[0]) * (x - sp[0]) + (z - sp[1]) * (z - sp[1]) < 100.0: bad = true; break
		if bad: continue
		out.append(Vector3(x, 0.8 + rng.randf() * 0.6, z))
	return out

# ---------------------------------------------------------------- الحلقة
func _process(dt: float) -> void:
	if state == "menu" and prev_cam != null and thumb_path == "": _orbit(Time.get_ticks_msec() / 1000.0)
	if thumb_path != "":
		_mf += 1
		if state == "menu" and _mf == 5:
			_orbit(14.0 + map_idx * 3.0); ui_root.visible = false
			for n in prev_world.get_children():
				if n is WorldEnvironment: (n as WorldEnvironment).environment.fog_enabled = false
			if map_idx == 8:
				prev_cam.position = Vector3(-75, 45, 40); prev_cam.fov = 68.0; prev_cam.look_at(Vector3(-15, 55, -30))
				for k in 4:
					var pj := Jet.make(self, k & 1, prev_world); pj.alive = false; pj.yaw = -0.6 + k * 0.35; pj.pitch = 0.08; pj.roll = 0.5 - k * 0.3
					pj.position = Vector3(-50 + k * 22, 52 + (k % 2) * 8, -10 - k * 14); pj._apply_tf()
			if map_idx == 0:
				var mw: float = maps[0]["W"]; var md: float = maps[0]["D"]
				prev_cam.position = Vector3(-mw * 0.42, 5.2, md * 0.42); prev_cam.fov = 80.0; prev_cam.look_at(Vector3(mw * 0.05, 0.6, -md * 0.05))
		if _mf == 25:
			var im := get_viewport().get_texture().get_image(); im.resize(512, 288, Image.INTERPOLATE_LANCZOS); im.save_png(thumb_path); get_tree().quit(0)
	if menu_shot != "":
		if dbg_picker and _map_picker != null: _open(_map_picker)
		_mf += 1
		if _mf == 30:
			get_viewport().get_texture().get_image().save_png(menu_shot); get_tree().quit(0)
	var vs := get_viewport().get_visible_rect().size
	for c in [hud, ui_root]:
		if c != null and c.size != vs: c.position = Vector2.ZERO; c.size = vs

func _physics_process(dt: float) -> void:
	if state != "play": return
	time += dt; match_t += dt
	if lan.role != 2:
		time_left -= dt
		if time_left <= 0.0: _end_match()
		if player != null and not player.alive:
			resp_t -= dt
			if resp_t <= 0.0:
				if air: air_spawn(player)
				else: player.respawn(spawn_point(player.team))
		if air and player != null and player.alive and player.jet == null:
			foot_t += dt; _air_foot()
	if shot_path != "":
		test_frames += 1
		inp["mv"] = Vector2(0, -0.3)
		if dbg_botcam and fighters.size() > 3:  # كاميرا ثالثة تتابع بوتاً من الجانب (لفحص حركة الجري والإطلاق)
			var bt: Fighter = fighters[3]; player.global_position = Vector3(0, 80, 0); player.velocity = Vector3.ZERO
			var side := bt.global_transform.basis.x * 2.4 + bt.global_transform.basis.z * -1.6 + Vector3(0, 1.3, 0)
			player.cam.global_position = bt.global_position + side; player.cam.look_at(bt.global_position + Vector3(0, 1.05, 0)); player.gun_parts.visible = false; player.arm_r.visible = false; player.arm_l.visible = false
		if dbg_vehcam >= 0 and dbg_vehcam < vehicles.size():
			var vv: Vehicle = vehicles[dbg_vehcam]; player.global_position = Vector3(0, 80, 0); player.velocity = Vector3.ZERO
			var sd := vv.global_transform.basis.x * 6.5 + vv.global_transform.basis.z * -4.5 + Vector3(0, 2.8, 0)
			player.cam.global_position = vv.global_position + sd; player.cam.look_at(vv.global_position + Vector3(0, 1.0, 0)); player.gun_parts.visible = false; player.arm_r.visible = false; player.arm_l.visible = false
		if dbg_botcam and test_frames == 120 and fighters.size() > 3:
			(fighters[3] as Bot).alive = false; (fighters[3] as Bot).play_death(fighters[3].global_position + Vector3(0, 0, -3))
		if test_frames == 150:
			get_viewport().get_texture().get_image().save_png(shot_path); print("SHOT saved"); get_tree().quit(0)
		return
	if lan_test != "":
		test_frames += 1
		if state == "play" and lan_test.begins_with("join:"): inp["mv"] = Vector2(0, -1); inp["look"] = Vector2(1, 0)
		if test_frames % 150 == 0 or test_frames == (500 if lan_test.begins_with("join:") else 1300):
			var hum := 0; var info := []
			for i in fighters.size():
				var b := fighters[i] as Bot
				if b != null and b.human: hum += 1; info.append("%d@%s" % [i, str(b.global_position.snapped(Vector3(1, 1, 1)))])
			print("LANTEST ", lan_test.get_slice(":", 0), " role=", lan.role, " state=", state, " slot=", lan.my_slot, " humans=", hum, " ", info, " me=", (str(player.global_position.snapped(Vector3(1, 1, 1))) if player != null else "-"), " fighters=", fighters.size(), " score=", score)
			if test_frames == (500 if lan_test.begins_with("join:") else 1300): get_tree().quit(0)
		return
	if air_test and air:
		at_f += 1
		if at_f == 1: print("AT start")
		if at_f == 100:
			for j in jets.duplicate(): if j.pilot != null and j.pilot != player: (j as Jet).pilot.jet = null; jet_die(j, null)
			at_j = Jet.make(self, 0); at_j.pilot = fighters[3]; at_j.position = Vector3(600, 150, 0); at_j.yaw = -1.5708; at_j.vel = Vector3(105, 0, 0); at_j.th = 0.7; jets.append(at_j)
		if at_f > 100 and at_f % 120 == 0 and is_instance_valid(at_j): print("AT x=", int(at_j.position.x), " y=", int(at_j.position.y), " yaw=", snappedf(at_j.yaw, 0.01), " roll=", snappedf(at_j.roll, 0.01), " v=", int(at_j.speed()))
		if at_f == 190 and not player.alive: air_spawn(player)
		if at_f == 200: player.spawn_prot = 20.0; inp["use"] = true
		if at_f == 230: print("AT eject: jet=", player.jet != null, " wi=", player.wi, " pos=", player.global_position.snapped(Vector3.ONE), " alive=", player.alive)
		if at_f == 900: print("AT foot_t=", snappedf(foot_t, 0.1), " near=", near_jet(player) != null)
		if at_f == 910 and near_jet(player) != null: inp["use"] = true
		if at_f == 960: print("AT board: jet=", player.jet != null, " th=", (player.jet.th if player.jet != null else -1.0))
		if at_f == 1200: get_tree().quit(0)
	if gren_test:
		gt_f += 1
		var pl := player as Player
		pl.spawn_prot = 5.0; pl.drone_cd = 0.0 if gt_f < 466 else pl.drone_cd
		if gt_f >= 400 and gt_f < 460: inp["crouch"] = true
		if gt_f == 30: inp["gren"] = true
		if gt_f == 460: print("GT after gren=", pl.gren, " crouch_t=", snappedf(pl.crouch_t, 0.01), " camy=", snappedf(pl.cam.position.y, 0.01), " nodes=", get_tree().get_nodes_in_group("x").size())
		if gt_f == 465: inp["drone"] = true
		if gt_f == 470 or gt_f == 500: print("GT d? ", pl.drone != null, " cd=", pl.drone_cd, " veh=", pl.veh, " t=", (pl.drone.t if pl.drone != null else -1.0), " alive=", pl.alive)
		if gt_f > 466 and gt_f < 560: inp["mv"] = Vector2(0, -1); inp["fire"] = false
		if gt_f == 560: print("GT drone=", pl.drone != null, " dpos=", pl.drone.global_position.snapped(Vector3(0.1, 0.1, 0.1)) if pl.drone != null else "-", " cur=", get_viewport().get_camera_3d() == (pl.drone.cam if pl.drone != null else null)); inp["fire"] = true
		if gt_f == 575: print("GT after boom drone=", pl.drone != null, " cd=", snappedf(pl.drone_cd, 0.1), " cam=", get_viewport().get_camera_3d() == pl.cam)
		if gt_f == 580: get_tree().quit(0)
	if veh_test >= 0:
		test_frames += 1
		var vv: Vehicle = vehicles[veh_test]
		if test_frames == 5: player.global_position = vv.to_global(Vector3(-3.5 if vv.vtype == 0 else -3.9, 0.3, 0)); inp["use"] = true
		if test_frames == 12: print("VT entered=", player.veh != null, " seat=", player.veh_seat if player.veh != null else -1, " team=", vv.team)
		if test_frames > 20 and test_frames < 220: inp["mv"] = Vector2(0.0, -1.0); inp["look"] = Vector2(0.4, 0) if test_frames > 100 else Vector2.ZERO
		if test_frames > 150 and test_frames < 215 and vv.vtype == 1: inp["fire"] = true
		if test_frames == 100: print("VT t=100 speed=", snappedf(vv.speed, 0.1), " pos=", vv.global_position.snapped(Vector3(0.1, 0.1, 0.1)), " cam=", player.cam.global_position.snapped(Vector3(0.1, 0.1, 0.1)))
		if test_frames == 220: print("VT t=220 speed=", snappedf(vv.speed, 0.1), " pos=", vv.global_position.snapped(Vector3(0.1, 0.1, 0.1)), " hp=", int(vv.hp), " turret=", snappedf(vv.ty, 0.01), " h=", snappedf(vv.h, 0.01)); inp["mv"] = Vector2.ZERO; inp["fire"] = false; inp["use"] = true
		if test_frames == 240: print("VT exited=", player.veh == null, " pos=", player.global_position.snapped(Vector3(0.1, 0.1, 0.1)), " camY=", player.cam.position.y); get_tree().quit(0)
		if test_frames == 200 and veh_shot != "": get_viewport().get_texture().get_image().save_png(veh_shot)
		return
	if selftest:
		test_frames += 1
		if not (gren_test and gt_f > 460): inp["fire"] = true; inp["mv"] = Vector2(0, -1); inp["look"] = Vector2(3, 0)
		if test_frames == 20:  # نهاية المباراة ثم "مباراة جديدة": يجب أن تبدأ مباراة نظيفة
			hud.touches[0] = {"role": "stick", "origin": Vector2.ZERO}; score[0] = score_lim; _end_match()
			var ok1 := state == "over" and get_tree().paused and hud.touches.is_empty() and _res_btn.text == Settings.t("مباراة جديدة")
			resume()
			var ok2 := state == "play" and not get_tree().paused and score == [0, 0] and not pause_root.visible and fighters.size() == 10
			print("OVERTEST ", "ok" if (ok1 and ok2) else "FAIL %s %s" % [ok1, ok2])
		if air and test_frames % 300 == 0:
			var a := []
			for j in jets: a.append("t%d%s p%.0f y%.0f v%.0f r%.2f" % [(j as Jet).team, "P" if (j as Jet).pilot != null else "-", (j as Jet).position.x, (j as Jet).position.y, (j as Jet).speed(), (j as Jet).roll])
			print("AIR f=", test_frames, " n=", jets.size(), " score=", score, " ", a)
		if test_frames == 1800:
			for v in vehicles: print("VEH type=", v.vtype, " team=", v.team, " driver=", (v.driver.fname if v.driver != null else "-"), " gunner=", (v.gunner.fname if v.gunner != null else "-"), " hp=", int(v.hp), " moved=", snappedf(v.global_position.distance_to(v.home), 0.1), " dead=", v.dead)
			if nav_ready:
				var pa := NavigationServer3D.map_get_path(nav_map, Vector3(-60, 0, 0), Vector3(60, 0, 0), true)
				print("NAV ready path_pts=", pa.size(), " straight=", 120, " len=", snappedf(Vector3(-60, 0, 0).distance_to(pa[0]) if pa.size() > 0 else 0.0, 0.1))
			else: print("NAV not ready")
			print("SELFTEST ok map=", maps[map_idx]["name"], " fighters=", fighters.size(), " score=", score, " player_hp=", player.hp, " ammo=", player.ammo)
			get_tree().quit(0)

# ---------------------------------------------------------------- حرب الجو (خريطة 8): مقاتلات F-35
func air_init() -> void:
	foot_t = 0.0
	for f in fighters: air_spawn(f)
	for k in 4: _jet_ground(k / 2, 6 + k % 2)
	hud.say(Settings.t("دمّر 30 طائرة معادية بالمدفع"))

func _jet_ground(tm: int, slot: int) -> void:
	var arr: Array = maps[map_idx]["blue"] if tm != 0 else maps[map_idx]["red"]
	var sp: Array = arr[slot % arr.size()]
	var J := Jet.make(self, tm); J.yaw = 1.5708 if tm != 0 else -1.5708; J.position = Vector3(sp[0], 0.0, sp[1]); J.th = 0.0; J.spare = true; J.gnd = true; J.pilot = null
	J._apply_tf(); jets.append(J)

func air_spawn(f: Fighter) -> void:
	var tm := f.team; var J := Jet.make(self, tm)
	J.yaw = 1.5708 if tm != 0 else -1.5708
	J.position = Vector3((1.0 if tm != 0 else -1.0) * (map_w * 0.5 - 150.0 - randf() * 70.0), 110.0 + randf() * 60.0, (randf() - 0.5) * map_d * 0.5)
	J.vel = Vector3(-sin(J.yaw), 0, -cos(J.yaw)) * 105.0; J.th = 0.7; J.pilot = f; J._apply_tf(); jets.append(J)
	f.hp = 100.0; f.alive = true; f.spawn_prot = 2.0; f.jet = J; f.collision_layer = 0
	if f is Player: (f as Player).enter_jet(J)
	else: f.visible = false; f.global_position = J.position

func near_jet(f: Fighter) -> Jet:
	var best: Jet = null; var bd := 18.0
	for j in jets:
		var J := j as Jet
		if not J.alive or J.pilot != null or J.team != f.team or not J.gnd or J.speed() > 8.0: continue
		var d := Vector2(J.position.x - f.global_position.x, J.position.z - f.global_position.z).length()
		if d < bd: bd = d; best = J
	return best

func air_board(pl: Player, J: Jet) -> void:
	J.pilot = pl; J.th = 0.0; J.vel = Vector3.ZERO; foot_t = 0.0; pl.enter_jet(J)
	hud.say(Settings.t("ارفع الدفع ثم اسحب الذراع للإقلاع"))

func air_eject(pl: Player) -> void:
	var J := pl.jet as Jet; if J == null: return
	var on_gnd := J.gnd and J.speed() < 10.0
	J.pilot = null; J.th = 0.0
	var pos := Vector3(J.position.x, 0.1, J.position.z)
	if on_gnd: pos += J.basis.x * 9.0; J.vel = Vector3.ZERO; hud.say(Settings.t("نزلت من الطائرة — لديك RPG"))
	else: hud.say(Settings.t("قفزت بالمظلة — الطائرة تحلّق بلا طيار"))
	pl.exit_jet(pos, J.yaw); pl.setup_weapon(8); pl.rebuild_viewmodel(); foot_t = 0.0

func _air_foot() -> void:
	if foot_t > 10.0 and near_jet(player) == null:
		var J := Jet.make(self, player.team); J.yaw = player.yaw; J.position = player.global_position + player.global_transform.basis.x * 12.0; J.position.y = 0.0
		J.th = 0.0; J.spare = true; J.gnd = true; J._apply_tf(); jets.append(J); foot_t = 0.0
		hud.say(Settings.t("وصلت طائرة بديلة بجانبك"))

func jet_tracer(a: Vector3, b: Vector3) -> void:
	var l := a.distance_to(b)
	if l < 2.0: return
	var mi := MeshInstance3D.new(); var bm := BoxMesh.new(); bm.size = Vector3(0.18, 0.18, l); mi.mesh = bm
	var m := StandardMaterial3D.new(); m.albedo_color = Color(1, 0.9, 0.4, 0.9); m.shading_mode = BaseMaterial3D.SHADING_MODE_UNSHADED; m.transparency = BaseMaterial3D.TRANSPARENCY_ALPHA; mi.material_override = m
	mi.cast_shadow = GeometryInstance3D.SHADOW_CASTING_SETTING_OFF; world.add_child(mi); mi.look_at_from_position((a + b) * 0.5, b, Vector3.UP)
	var tw := create_tween(); tw.tween_property(m, "albedo_color:a", 0.0, 0.1); tw.tween_callback(mi.queue_free)

func jet_die(J: Jet, by: Fighter) -> void:
	if not J.alive: return
	J.alive = false; jets.erase(J)
	var pos := J.position; var p := J.pilot; J.pilot = null
	fx_blast(pos, 2.4); sfx_play("boom", pos)
	for k in 6: fx_blast(pos + Vector3(randf_range(-5, 5), randf_range(-2, 3), randf_range(-5, 5)), 0.9)
	var jteam := J.team
	if J.spare: get_tree().create_timer(25.0).timeout.connect(func(): if air and state == "play": _jet_ground(jteam, 6 + randi() % 2))
	if p != null:
		p.alive = false; p.deaths += 1; p.jet = null; p.collision_layer = 2
		if p == player:
			player.exit_jet(Vector3(pos.x, maxf(pos.y, 0.5), pos.z), J.yaw, true); hud.say(Settings.t("أُسقطت طائرتك!")); resp_t = 5.0
		else:
			p.visible = false
			get_tree().create_timer(5.0).timeout.connect(func(): if is_instance_valid(p) and state == "play" and air and not p.alive: air_spawn(p))
	if by != null and by.team != J.team:
		by.kills += 1; score[by.team] += 1
		hud.add_feed("%s  ✕  F-35 (%s)" % [by.fname, p.fname if p != null else Settings.t("طائرة")])
		if by == player: hud.say("%s %d / %d" % [Settings.t("أسقطت طائرة!"), score[by.team], score_lim])
		if score[0] >= score_lim or score[1] >= score_lim: _end_match()
	J.queue_free()
