extends Node3D
# المشهد الرئيسي: القائمة والإعدادات، بناء المباراة، الأسلحة (Hitscan)، الصوت، المشاهد حسب الجودة (أشجار/نوافذ)

var data: Dictionary
var weapons: Array = []
var maps: Array = []
var state := "menu"        # menu | play | pause | over
var inp := {"mv": Vector2.ZERO, "look": Vector2.ZERO, "fire": false, "jump": false, "reload": false, "ads": false, "sprint": false, "next": false}
var map_idx := 2
var diff := 1
var bots_per_team := 5
var player: Player = null
var fighters: Array = []
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
var menu_root: Control
var pause_root: Control
var over_label: Label
var sfx := {}
var scenery: Node3D
var selftest := false
var test_frames := 0
var shot_path := ""
var menu_shot := ""
var _mf := 0

func _ready() -> void:
	var f := FileAccess.open("res://data/game.json", FileAccess.READ)
	data = JSON.parse_string(f.get_as_text())
	maps = data["maps"]; weapons = data["weapons"]
	_make_sfx()
	ui = CanvasLayer.new(); ui.layer = 10; ui.process_mode = Node.PROCESS_MODE_ALWAYS; add_child(ui)
	ui_root = Control.new(); ui_root.theme = UiTheme.make(); ui_root.mouse_filter = Control.MOUSE_FILTER_IGNORE; ui.add_child(ui_root)
	hud = Hud.new(); hud.game = self; ui.add_child(hud)
	Settings.changed.connect(_on_settings)
	_build_menu(); _build_pause()
	_show_menu()
	var args := OS.get_cmdline_user_args()
	selftest = "--selftest" in args
	for a in args:
		if a.begins_with("--shot="): shot_path = a.substr(7)
		if a.begins_with("--menushot="): menu_shot = a.substr(11)
		if a.begins_with("--quality="): Settings.quality = int(a.substr(10))
		if a.begins_with("--lang="): Settings.lang = int(a.substr(7))
		if a.begins_with("--map="): map_idx = int(a.substr(6))
	if selftest or shot_path != "":
		start_match()

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

func _chev(txt: String) -> Button:
	var b := Button.new(); b.text = txt; b.custom_minimum_size = Vector2(72, 72); UiTheme.juicy(b)
	b.add_theme_stylebox_override("normal", UiTheme.flat(Color(1, 1, 1, 0.08), 36, UiTheme.BORDER, 1)); b.add_theme_stylebox_override("hover", UiTheme.flat(Color(1, 1, 1, 0.16), 36, UiTheme.ACCENT, 1)); b.add_theme_stylebox_override("pressed", UiTheme.flat(UiTheme.ACCENT, 36))
	return b

func _cycler(parent: Control, label_ar: String, get_text: Callable, step: Callable) -> void:
	var box := VBoxContainer.new(); box.add_theme_constant_override("separation", 4); parent.add_child(box)
	var lab := Label.new(); lab.add_theme_font_size_override("font_size", 19); lab.add_theme_color_override("font_color", UiTheme.MUTED); box.add_child(lab)
	var row := HBoxContainer.new(); row.add_theme_constant_override("separation", 10); box.add_child(row)
	var lb := _chev("‹"); row.add_child(lb)
	var pc := PanelContainer.new(); pc.size_flags_horizontal = Control.SIZE_EXPAND_FILL; pc.add_theme_stylebox_override("panel", UiTheme.flat(Color(1, 1, 1, 0.05), 36, UiTheme.BORDER, 1)); row.add_child(pc)
	var val := Label.new(); val.horizontal_alignment = HORIZONTAL_ALIGNMENT_CENTER; val.vertical_alignment = VERTICAL_ALIGNMENT_CENTER; val.add_theme_font_size_override("font_size", 28); val.custom_minimum_size = Vector2(0, 60); pc.add_child(val)
	var rb := _chev("›"); row.add_child(rb)
	lb.pressed.connect(func(): step.call(-1); _refresh())
	rb.pressed.connect(func(): step.call(1); _refresh())
	_rows.append({"lab": lab, "val": val, "ar": label_ar, "get": get_text})

func _bg_layer(root: Control, dim: bool) -> void:
	var bg := ColorRect.new(); bg.set_anchors_preset(Control.PRESET_FULL_RECT); root.add_child(bg)
	if dim: bg.color = Color(0.02, 0.03, 0.07, 0.78)
	else:
		var sh := Shader.new(); sh.code = UiTheme.BG_SHADER; var mat := ShaderMaterial.new(); mat.shader = sh; bg.material = mat; bg.color = Color.WHITE

func _settings_rows(vb: Control, with_map: bool) -> void:
	if with_map:
		_cycler(vb, "الخريطة", func(): return Settings.t(maps[map_idx]["name"]), func(d): map_idx = (map_idx + d + maps.size() - 1) % (maps.size() - 1))
		_cycler(vb, "الصعوبة", func(): return Settings.t(["سهل", "عادي", "صعب"][diff]), func(d): diff = (diff + d + 3) % 3)
	_cycler(vb, "الجودة", func(): return Settings.t(Settings.QUALS[Settings.quality]), func(d): Settings.quality = (Settings.quality + d + 5) % 5; Settings.save())
	_cycler(vb, "اللغة", func(): return Settings.LANGS[Settings.lang], func(d): Settings.lang = (Settings.lang + d + 3) % 3; Settings.save())

func _build_menu() -> void:
	menu_root = Control.new(); menu_root.name = "Menu"; ui_root.add_child(menu_root); _bg_layer(menu_root, false)
	var mg := MarginContainer.new(); mg.set_anchors_preset(Control.PRESET_FULL_RECT)
	for k in ["left", "right"]: mg.add_theme_constant_override("margin_" + k, 70)
	for k in ["top", "bottom"]: mg.add_theme_constant_override("margin_" + k, 40)
	menu_root.add_child(mg)
	var hb := HBoxContainer.new(); hb.add_theme_constant_override("separation", 60); mg.add_child(hb)
	# ---- اليسار: الشعار + زر البدء ----
	var left := VBoxContainer.new(); left.size_flags_horizontal = Control.SIZE_EXPAND_FILL; left.alignment = BoxContainer.ALIGNMENT_CENTER; left.add_theme_constant_override("separation", 18); hb.add_child(left)
	var tag := Label.new(); tag.text = "RED  VS  BLUE"; tag.add_theme_font_size_override("font_size", 22); tag.add_theme_color_override("font_color", UiTheme.ACCENT2); left.add_child(tag)
	_title = Label.new(); _title.text = "WARZONE"; _title.add_theme_font_size_override("font_size", 84); left.add_child(_title)
	_sub = Label.new(); _sub.add_theme_font_size_override("font_size", 24); _sub.add_theme_color_override("font_color", UiTheme.MUTED); _sub.autowrap_mode = TextServer.AUTOWRAP_WORD_SMART; left.add_child(_sub)
	var sp := Control.new(); sp.custom_minimum_size = Vector2(0, 18); left.add_child(sp)
	_start_btn = Button.new(); _start_btn.custom_minimum_size = Vector2(0, 96); _start_btn.add_theme_font_size_override("font_size", 34); UiTheme.style_accent(_start_btn); UiTheme.juicy(_start_btn); _start_btn.pressed.connect(start_match); left.add_child(_start_btn)
	# ---- اليمين: بطاقة الإعدادات ----
	var right := _card(hb, 640); right.get_parent().get_parent().size_flags_vertical = Control.SIZE_SHRINK_CENTER
	_settings_rows(right, true)
	_refresh()

func _build_pause() -> void:
	pause_root = Control.new(); pause_root.name = "Pause"; ui_root.add_child(pause_root); _bg_layer(pause_root, true)
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
		r["lab"].text = Settings.t(r["ar"]); r["val"].text = r["get"].call()
	if _pause_title != null:
		_pause_title.text = Settings.t("إيقاف مؤقت"); _res_btn.text = Settings.t("متابعة"); _menu_btn.text = Settings.t("القائمة الرئيسية")
	var rtl := Settings.lang == 0
	ui_root.layout_direction = Control.LAYOUT_DIRECTION_RTL if rtl else Control.LAYOUT_DIRECTION_LTR

func _on_settings() -> void:
	_refresh(); apply_quality()

func _fade_in(c: Control) -> void:
	c.modulate.a = 0.0; var tw := c.create_tween(); tw.tween_property(c, "modulate:a", 1.0, 0.18)

func _show_menu() -> void:
	state = "menu"; menu_root.visible = true; _fade_in(menu_root); pause_root.visible = false; hud.visible = false; map_info.clear()
	Input.mouse_mode = Input.MOUSE_MODE_VISIBLE; get_tree().paused = false
	if world != null: world.queue_free(); world = null
	for f in fighters: if is_instance_valid(f): f.queue_free()
	fighters.clear(); player = null; _refresh()

func open_pause() -> void:
	if state != "play": return
	state = "pause"; pause_root.visible = true; over_label.text = ""; _fade_in(pause_root); get_tree().paused = true; Input.mouse_mode = Input.MOUSE_MODE_VISIBLE; hud.mouse_look = false
	inp["fire"] = false; inp["mv"] = Vector2.ZERO; _refresh()

func resume() -> void:
	state = "play"; pause_root.visible = false; get_tree().paused = false

# ---------------------------------------------------------------- بناء المباراة
func start_match() -> void:
	if world != null: world.queue_free()
	for f in fighters: if is_instance_valid(f): f.queue_free()
	fighters.clear()
	world = Node3D.new(); world.name = "World"; add_child(world)
	var m: Dictionary = maps[map_idx]
	map_info = MapBuilder.build(m, world); map_w = m["W"]; map_d = m["D"]
	score = [0, 0]; time_left = 600.0; resp_t = 0.0
	player = Player.new(); player.game = self; player.team = 0; player.fname = Settings.t("اللاعب"); world.add_child(player)
	player.setup_weapon(0); fighters.append(player); player.respawn(spawn_point(0))
	for t in 2:
		for i in bots_per_team - (1 if t == 0 else 0):
			var b := Bot.new(); b.game = self; b.team = t; b.fname = "BOT%d" % (fighters.size()); b.acc = [0.09, 0.055, 0.03][diff]
			world.add_child(b); fighters.append(b); b.respawn(spawn_point(t))
	scenery = Node3D.new(); world.add_child(scenery); build_scenery(); apply_quality()
	menu_root.visible = false; pause_root.visible = false; hud.visible = true; state = "play"
	hud.say(Settings.t("اختر سلاحك") if false else m["name"])

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
func hitscan(shooter: Fighter, from: Vector3, dir: Vector3, w: Dictionary) -> void:
	var to := from + dir * 220.0
	var q := PhysicsRayQueryParameters3D.create(from, to, 3, [shooter.get_rid()])
	var r := get_world_3d().direct_space_state.intersect_ray(q)
	var endp := to
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
		else:
			_spark(endp)
	if shooter == player or from.distance_to(cam_pos()) < 60.0: _tracer(from + dir * 0.8, endp)

func _explode(src: Fighter, p: Vector3) -> void:
	sfx_play("boom", p)
	for f in fighters:
		var d: float = f.global_position.distance_to(p)
		if d < 5.0 and f.team != src.team or f == src and d < 5.0:
			f.take_damage(float(weapons[8]["dmg"]) * (1.0 - d / 5.0), src, false)
	_spark(p, 14)

func _spark(p: Vector3, n := 3) -> void:
	if Settings.quality < 1: return
	n = int(n * [0.3, 0.6, 1.0, 1.2, 1.6][Settings.quality])
	for i in n:
		var mi := MeshInstance3D.new(); var bm := BoxMesh.new(); bm.size = Vector3.ONE * 0.07; mi.mesh = bm
		var m := StandardMaterial3D.new(); m.albedo_color = Color(1, 0.7, 0.2); m.emission_enabled = true; m.emission = Color(1, 0.6, 0.1); mi.material_override = m
		mi.cast_shadow = GeometryInstance3D.SHADOW_CASTING_SETTING_OFF; world.add_child(mi); mi.global_position = p
		var tw := create_tween(); tw.tween_property(mi, "global_position", p + Vector3(randf_range(-1, 1), randf_range(0.2, 1.2), randf_range(-1, 1)) * 0.6, 0.25); tw.tween_callback(mi.queue_free)

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
	if src == player: hud.say(Settings.t("تصويبة رأس!") if head else victim.fname)
	if victim == player: resp_t = 4.0
	else:
		victim.visible = false
		get_tree().create_timer(3.0).timeout.connect(func(): if is_instance_valid(victim) and state != "menu": (victim as Bot).respawn(spawn_point(victim.team)))
	if score[0] >= score_lim or score[1] >= score_lim: _end_match()

func _end_match() -> void:
	if state == "over": return
	state = "over"; pause_root.visible = true; get_tree().paused = true; Input.mouse_mode = Input.MOUSE_MODE_VISIBLE
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
func _process(_dt: float) -> void:
	if menu_shot != "":
		_mf += 1
		if _mf == 30:
			get_viewport().get_texture().get_image().save_png(menu_shot); get_tree().quit(0)
	var vs := get_viewport().get_visible_rect().size
	for c in [hud, ui_root]:
		if c != null and c.size != vs: c.position = Vector2.ZERO; c.size = vs

func _physics_process(dt: float) -> void:
	if state != "play": return
	time += dt
	time_left -= dt
	if time_left <= 0.0: _end_match()
	if player != null and not player.alive:
		resp_t -= dt
		if resp_t <= 0.0: player.respawn(spawn_point(0))
	if shot_path != "":
		test_frames += 1
		inp["mv"] = Vector2(0, -0.3)
		if test_frames == 150:
			get_viewport().get_texture().get_image().save_png(shot_path); print("SHOT saved"); get_tree().quit(0)
		return
	if selftest:
		test_frames += 1
		inp["fire"] = true; inp["mv"] = Vector2(0, -1); inp["look"] = Vector2(3, 0)
		if test_frames == 1800:
			print("SELFTEST ok map=", maps[map_idx]["name"], " fighters=", fighters.size(), " score=", score, " player_hp=", player.hp, " ammo=", player.ammo)
			get_tree().quit(0)
