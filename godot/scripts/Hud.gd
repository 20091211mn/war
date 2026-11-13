class_name Hud
extends Control
# واجهة اللعب: عصا حركة + سحب للنظر + أزرار لمس (إطلاق/قفز/تعبئة/تصويب/جري/تبديل/إيقاف) + شريط صحة وذخيرة + مؤشرات
# يعمل باللمس (موبايل) وبلوحة المفاتيح والفأرة (حاسوب)

var game: Node
var touches := {}            # index -> {role, pos, origin}
var stick := Vector2.ZERO
var stick_c := Vector2.ZERO
var dmg_flash := 0.0
var muzzle := 0.0
var hit_mark := 0.0
var feed: Array = []         # [نص, وقت]
var msg := ""
var msg_t := 0.0
var mouse_look := false

func _ready() -> void:
	set_anchors_preset(Control.PRESET_FULL_RECT)
	mouse_filter = Control.MOUSE_FILTER_IGNORE
	process_mode = Node.PROCESS_MODE_ALWAYS

func reset_touch() -> void:
	touches.clear(); stick = Vector2.ZERO; stick_c = Vector2.ZERO

var hits: Array = []
func flash_damage() -> void: dmg_flash = 1.0
func hit_from(p: Vector3) -> void:
	hits.append([p, 1.6]); if hits.size() > 4: hits.pop_front()
func fire_flash() -> void: muzzle = 0.06
func hitmark() -> void: hit_mark = 0.25
func say(s: String) -> void: msg = s; msg_t = 2.5
func add_feed(s: String) -> void:
	feed.append([s, 4.0]); if feed.size() > 4: feed.pop_front()

# ---- مناطق الأزرار (نسبة إلى حجم الشاشة) ----
func _btn(name: String) -> Dictionary:
	var s := size
	match name:
		"fire": return {"c": Vector2(s.x - 120, s.y - 120), "r": 68.0}
		"jump": return {"c": Vector2(s.x - 250, s.y - 80), "r": 44.0}
		"ads": return {"c": Vector2(s.x - 250, s.y - 190), "r": 44.0}
		"reload": return {"c": Vector2(s.x - 120, s.y - 250), "r": 40.0}
		"sprint": return {"c": Vector2(130, s.y - 300), "r": 38.0}
		"next": return {"c": Vector2(s.x - 360, s.y - 150), "r": 36.0}
		"pause": return {"c": Vector2(s.x - 50, 50), "r": 30.0}
		"use": return {"c": Vector2(s.x - 480, s.y - 90), "r": 44.0}
		"seat": return {"c": Vector2(s.x - 480, s.y - 200), "r": 38.0}
		"c4": return {"c": Vector2(s.x - 480, s.y - 310), "r": 34.0}
		"det": return {"c": Vector2(s.x - 360, s.y - 370), "r": 34.0}
		"gren": return {"c": Vector2(s.x - 360, s.y - 260), "r": 36.0}
		"crouch": return {"c": Vector2(130, s.y - 420), "r": 36.0}
	return {"c": Vector2.ZERO, "r": 0.0}
const BTNS := ["fire", "jump", "ads", "reload", "sprint", "next", "pause", "use", "seat", "gren", "crouch", "c4", "det"]

func _btn_on(n: String) -> bool:
	if game == null or game.player == null: return n != "use" and n != "seat"
	if n == "gren" or n == "crouch" or n == "c4": return game.player.veh == null
	if n == "det": return game.player.veh == null and (game.player as Player).c4s.size() > 0
	if n == "use": return game.player.veh != null or game.near_vehicle(game.player) != null
	if n == "seat": return game.player.veh != null and (game.player.veh as Vehicle).occupant_count() < 2 and (game.player.veh as Vehicle).vtype >= 0
	return true

func _hit(p: Vector2) -> String:
	for n in BTNS:
		if not _btn_on(n): continue
		var b := _btn(n)
		if p.distance_to(b["c"]) <= float(b["r"]) * 1.25: return n
	return ""

func _input(ev: InputEvent) -> void:
	if game == null or game.state != "play" or game.overlay: return
	var inp: Dictionary = game.inp
	if ev is InputEventScreenTouch:
		var t := ev as InputEventScreenTouch
		if t.pressed:
			var n := _hit(t.position)
			if n == "":
				if t.position.x < size.x * 0.42: touches[t.index] = {"role": "stick", "origin": t.position}; stick_c = t.position; stick = Vector2.ZERO
				else: touches[t.index] = {"role": "look", "last": t.position}
			else:
				touches[t.index] = {"role": n, "last": t.position}
				match n:
					"fire": inp["fire"] = true
					"jump": inp["jump"] = true
					"ads": inp["ads"] = not bool(inp["ads"])
					"reload": inp["reload"] = true
					"sprint": inp["sprint"] = not bool(inp["sprint"])
					"next": inp["next"] = true
					"pause": game.open_pause()
					"use": inp["use"] = true
					"seat": inp["seat"] = true
					"gren": inp["gren"] = true
					"c4": inp["c4"] = true
					"det": inp["det"] = true
					"crouch": inp["crouch"] = not bool(inp["crouch"])
		else:
			if touches.has(t.index):
				var r: String = touches[t.index]["role"]
				if r == "stick": stick = Vector2.ZERO; inp["mv"] = Vector2.ZERO
				if r == "fire": inp["fire"] = false
				touches.erase(t.index)
	elif ev is InputEventScreenDrag:
		var d := ev as InputEventScreenDrag
		if touches.has(d.index):
			var r: String = touches[d.index]["role"]
			if r == "stick":
				var v: Vector2 = (d.position - touches[d.index]["origin"]) / 90.0
				if v.length() > 1.0: v = v.normalized()
				stick = v; inp["mv"] = Vector2(v.x, v.y)
			elif r == "look" or r == "fire":
				inp["look"] = inp["look"] + d.relative
	# ---- حاسوب ----
	elif ev is InputEventMouse and (ev as InputEventMouse).device == InputEvent.DEVICE_ID_EMULATION:
		return  # فأرة مُحاكاة من اللمس (لتعمل أزرار القوائم) — اللعب يستخدم أحداث اللمس مباشرة
	elif ev is InputEventMouseMotion and mouse_look:
		inp["look"] = inp["look"] + (ev as InputEventMouseMotion).relative
	elif ev is InputEventMouseButton:
		var mb := ev as InputEventMouseButton
		if mb.button_index == MOUSE_BUTTON_LEFT:
			if not mouse_look and mb.pressed: mouse_look = true; Input.mouse_mode = Input.MOUSE_MODE_CAPTURED
			inp["fire"] = mb.pressed
		elif mb.button_index == MOUSE_BUTTON_RIGHT: inp["ads"] = mb.pressed
	elif ev is InputEventKey:
		var k := ev as InputEventKey
		if k.echo: return
		match k.keycode:
			KEY_SPACE: if k.pressed: inp["jump"] = true
			KEY_R: if k.pressed: inp["reload"] = true
			KEY_Q: if k.pressed: inp["next"] = true
			KEY_E: if k.pressed: inp["use"] = true
			KEY_F: if k.pressed: inp["seat"] = true
			KEY_G: if k.pressed: inp["gren"] = true
			KEY_X: if k.pressed: inp["c4"] = true
			KEY_Z: if k.pressed: inp["det"] = true
			KEY_C: inp["crouch"] = k.pressed
			KEY_SHIFT: inp["sprint"] = k.pressed
			KEY_ESCAPE: if k.pressed: game.open_pause()

func _process(dt: float) -> void:
	if game == null: return
	var inp: Dictionary = game.inp
	if game.state == "play":
		# لوحة المفاتيح للحركة
		if not touches.values().any(func(v): return v["role"] == "stick"):
			var kx := int(Input.is_key_pressed(KEY_D)) - int(Input.is_key_pressed(KEY_A))
			var ky := int(Input.is_key_pressed(KEY_S)) - int(Input.is_key_pressed(KEY_W))
			if kx != 0 or ky != 0 or inp["mv"] != Vector2.ZERO:
				if not DisplayServer.is_touchscreen_available() or kx != 0 or ky != 0: inp["mv"] = Vector2(kx, ky).normalized() if (kx != 0 or ky != 0) else Vector2.ZERO
	dmg_flash = maxf(0.0, dmg_flash - dt * 1.5); muzzle = maxf(0.0, muzzle - dt); hit_mark = maxf(0.0, hit_mark - dt)
	msg_t = maxf(0.0, msg_t - dt)
	for h in hits: h[1] -= dt
	hits = hits.filter(func(h): return h[1] > 0.0)
	for f in feed: f[1] -= dt
	feed = feed.filter(func(f): return f[1] > 0.0)
	queue_redraw()

func _txt(p: Vector2, s: String, sz: int, col: Color, align := HORIZONTAL_ALIGNMENT_LEFT, w := -1.0) -> void:
	var font := get_theme_default_font()
	draw_string(font, p + Vector2(1.5, 1.5), s, align, w, sz, Color(0, 0, 0, 0.7))
	draw_string(font, p, s, align, w, sz, col)

func _hud_weapon(p: Fighter, s: Vector2) -> void:
	var w := p.weapon()
	_txt(Vector2(s.x - 340, s.y - 330), Settings.t(w["name"]), 20, Color(0.7, 0.85, 1.0), HORIZONTAL_ALIGNMENT_RIGHT, 320.0)
	var am := ("%d / %d" % [p.ammo, p.reserve]) if p.reload_t <= 0.0 else Settings.t("جارٍ التعبئة…")
	_txt(Vector2(s.x - 340, s.y - 304), am, 30, Color.WHITE, HORIZONTAL_ALIGNMENT_RIGHT, 320.0)
	if p is Player: _txt(Vector2(s.x - 340, s.y - 345), "💣 %d" % (p as Player).gren, 20, Color(1, 0.85, 0.5), HORIZONTAL_ALIGNMENT_RIGHT, 320.0)

func _draw() -> void:
	if game == null or game.state != "play" or game.player == null: return
	var p: Fighter = game.player
	var s := size
	var cx := s.x * 0.5; var cy := s.y * 0.5
	var z := float(p.weapon()["zoom"])
	var scoped: bool = p.alive and game.player.ads and z >= 3.4
	# ---- عدسة المنظار (بلا مربع يغطي العدو): حلقة سوداء + شعرة رفيعة ----
	if scoped:
		var rr := minf(s.x, s.y) * 0.34
		draw_arc(Vector2(cx, cy), rr + 600.0, 0, TAU, 8, Color.BLACK, 1200.0)
		draw_arc(Vector2(cx, cy), rr, 0, TAU, 96, Color(0, 0, 0, 1), 3.0)
		draw_line(Vector2(cx - rr, cy), Vector2(cx - 8, cy), Color(0, 0, 0, 0.9), 1.5); draw_line(Vector2(cx + 8, cy), Vector2(cx + rr, cy), Color(0, 0, 0, 0.9), 1.5)
		draw_line(Vector2(cx, cy - rr), Vector2(cx, cy - 8), Color(0, 0, 0, 0.9), 1.5); draw_line(Vector2(cx, cy + 8), Vector2(cx, cy + rr), Color(0, 0, 0, 0.9), 1.5)
		draw_circle(Vector2(cx, cy), 2.0, Color(1, 0.15, 0.15))
	elif p.alive:
		var g := 6.0 + p.recoil * 10.0
		var cc := Color(1, 1, 1, 0.9)
		draw_line(Vector2(cx - g - 9, cy), Vector2(cx - g, cy), cc, 2.0); draw_line(Vector2(cx + g, cy), Vector2(cx + g + 9, cy), cc, 2.0)
		draw_line(Vector2(cx, cy - g - 9), Vector2(cx, cy - g), cc, 2.0); draw_line(Vector2(cx, cy + g), Vector2(cx, cy + g + 9), cc, 2.0)
	if hit_mark > 0.0:
		var hc := Color(1, 1, 1, hit_mark * 4.0)
		for sg in [Vector2(1, 1), Vector2(-1, 1), Vector2(1, -1), Vector2(-1, -1)]:
			draw_line(Vector2(cx, cy) + sg * 6.0, Vector2(cx, cy) + sg * 14.0, hc, 2.0)
	if dmg_flash > 0.0: draw_rect(Rect2(Vector2.ZERO, s), Color(0.8, 0, 0, dmg_flash * 0.28))
	# ---- صحة وذخيرة ----
	var hp := clampf(p.hp / 100.0, 0.0, 1.0)
	draw_rect(Rect2(24, s.y - 56, 260, 20), Color(0, 0, 0, 0.5)); draw_rect(Rect2(26, s.y - 54, 256.0 * hp, 16), Color(0.3, 0.85, 0.35) if hp > 0.35 else Color(0.9, 0.25, 0.2))
	_txt(Vector2(292, s.y - 40), str(int(maxf(p.hp, 0.0))), 22, Color.WHITE)
	if p.veh != null:
		var vv: Vehicle = p.veh as Vehicle
		var vp := clampf(vv.hp / float(vv.P["hp"]), 0.0, 1.0)
		draw_rect(Rect2(24, s.y - 90, 260, 14), Color(0, 0, 0, 0.5)); draw_rect(Rect2(26, s.y - 88, 256.0 * vp, 10), Color(0.95, 0.75, 0.2) if vp > 0.35 else Color(0.9, 0.25, 0.2))
		_txt(Vector2(292, s.y - 76), "%d km/h" % int(absf(vv.speed) * 3.6), 20, Color.WHITE)
		if vv.vtype == 1 and vv.cannon_t > 0.0: draw_rect(Rect2(cx - 40, cy + 40, 80.0 * (1.0 - vv.cannon_t / 2.5), 5), Color(1, 0.8, 0.3, 0.9))
		_txt(Vector2(s.x - 340, s.y - 330), Settings.t("مركبة") if vv.vtype == 0 else Settings.t("دبابة"), 20, Color(1, 0.85, 0.4), HORIZONTAL_ALIGNMENT_RIGHT, 320.0)
		_txt(Vector2(s.x - 340, s.y - 304), ("MG" if (vv.vtype == 0 and p.veh_seat == 1) else ("CANNON" if vv.vtype == 1 and (vv.gunner == p or (vv.gunner == null and vv.driver == p)) else "DRIVE")), 28, Color.WHITE, HORIZONTAL_ALIGNMENT_RIGHT, 320.0)
	else: _hud_weapon(p, s)
	var w := p.weapon()
	# ---- النتيجة والوقت ----
	_txt(Vector2(cx - 170, 36), "%d" % game.score[0], 30, Color(1, 0.35, 0.35), HORIZONTAL_ALIGNMENT_RIGHT, 100.0)
	_txt(Vector2(cx - 60, 36), "%d:%02d" % [int(game.time_left if game.time_cap > 0.0 else game.match_t) / 60, int(game.time_left if game.time_cap > 0.0 else game.match_t) % 60], 26, Color.WHITE, HORIZONTAL_ALIGNMENT_CENTER, 120.0)
	_txt(Vector2(cx + 70, 36), "%d" % game.score[1], 30, Color(0.4, 0.65, 1), HORIZONTAL_ALIGNMENT_LEFT, 100.0)
	# ---- سجل القتل والرسالة ----
	var fy := 90.0
	for f in feed: _txt(Vector2(s.x - 360, fy), f[0], 18, Color(1, 1, 1, clampf(f[1], 0, 1)), HORIZONTAL_ALIGNMENT_RIGHT, 300.0); fy += 24
	for h in hits:
		var rel: Vector3 = (p as Node3D).global_transform.affine_inverse() * (h[0] as Vector3)
		var ang := atan2(rel.x, -rel.z) - PI * 0.5
		draw_arc(Vector2(cx, cy), 150.0, ang - 0.28, ang + 0.28, 12, Color(1, 0.1, 0.1, clampf(float(h[1]), 0.0, 1.0) * 0.85), 10.0)
	if msg_t > 0.0: _txt(Vector2(cx - 300, s.y * 0.3), msg, 30, Color(1, 0.9, 0.4, clampf(msg_t, 0, 1)), HORIZONTAL_ALIGNMENT_CENTER, 600.0)
	if not p.alive: _txt(Vector2(cx - 300, s.y * 0.5), Settings.t("أُصبت! عودة بعد ") + str(ceili(game.resp_t)), 34, Color(1, 0.4, 0.4), HORIZONTAL_ALIGNMENT_CENTER, 600.0)
	if p.alive and p.veh == null and game.near_vehicle(p) != null: _txt(Vector2(cx - 200, s.y * 0.66), Settings.t("اضغط للركوب") + "  [E]", 24, Color(1, 0.9, 0.5), HORIZONTAL_ALIGNMENT_CENTER, 400.0)
	# ---- علامات الحلفاء ----
	for f in game.fighters:
		if f == p or not f.alive or f.team != p.team: continue
		var wp: Vector3 = f.global_position + Vector3(0, 2.1, 0)
		if game.player.cam.is_position_behind(wp): continue
		var sp: Vector2 = game.player.cam.unproject_position(wp)
		var tc := Color(0.4, 0.8, 1.0, 0.9); draw_line(sp + Vector2(-6, -8), sp, tc, 2.0); draw_line(sp, sp + Vector2(6, -8), tc, 2.0); draw_line(sp + Vector2(-6, -8), sp + Vector2(6, -8), tc, 2.0)
		_txt(sp + Vector2(-40, -12), f.fname, 14, Color(0.6, 0.85, 1.0), HORIZONTAL_ALIGNMENT_CENTER, 80.0)
	# ---- أزرار اللمس ----
	if DisplayServer.is_touchscreen_available() or game.show_touch:
		var names := {"fire": "🔥", "jump": "⤒", "ads": "◎", "reload": "↻", "sprint": "»", "next": "⇄", "pause": "II", "use": "🚪", "seat": "⇅", "gren": "💣", "c4": "C4", "det": "⚡", "crouch": "⤓"}
		for n in BTNS:
			if not _btn_on(n): continue
			var b := _btn(n)
			var on: bool = (n == "ads" and bool(game.inp["ads"])) or (n == "sprint" and bool(game.inp["sprint"])) or (n == "crouch" and bool(game.inp["crouch"]))
			draw_circle(b["c"], b["r"], Color(1, 1, 1, 0.32 if on else 0.14)); draw_arc(b["c"], b["r"], 0, TAU, 40, Color(1, 1, 1, 0.5), 2.0)
			_txt(b["c"] + Vector2(-b["r"], 8), "FIRE" if n == "fire" else names[n], 22 if n != "fire" else 20, Color.WHITE, HORIZONTAL_ALIGNMENT_CENTER, b["r"] * 2.0)
		if stick_c != Vector2.ZERO and touches.values().any(func(v): return v["role"] == "stick"):
			draw_circle(stick_c, 70, Color(1, 1, 1, 0.1)); draw_circle(stick_c + stick * 60.0, 30, Color(1, 1, 1, 0.3))
