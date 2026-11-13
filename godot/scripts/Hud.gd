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
		"tup": return {"c": Vector2(130, s.y - 300), "r": 40.0}
		"tdn": return {"c": Vector2(130, s.y - 420), "r": 40.0}
		"drone": return {"c": Vector2(130, s.y - 500), "r": 34.0}
		"gren": return {"c": Vector2(s.x - 360, s.y - 260), "r": 36.0}
		"crouch": return {"c": Vector2(130, s.y - 420), "r": 36.0}
	return {"c": Vector2.ZERO, "r": 0.0}
const BTNS := ["fire", "jump", "ads", "reload", "sprint", "next", "pause", "use", "seat", "gren", "crouch", "c4", "det", "drone", "tup", "tdn"]

func _btn_on(n: String) -> bool:
	if game == null or game.player == null: return n != "use" and n != "seat" and n != "tup" and n != "tdn"
	if game.player.jet != null: return n in ["fire", "ads", "pause", "use", "tup", "tdn"]
	if n == "tup" or n == "tdn": return false
	if n == "use" and game.air: return game.near_jet(game.player) != null
	if n == "drone": return game.player.veh == null
	if n == "gren" or n == "crouch" or n == "c4": return game.player.veh == null and (game.player as Player).drone == null
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
					"tup": inp["tup"] = true
					"tdn": inp["tdn"] = true
					"gren": inp["gren"] = true
					"drone": inp["drone"] = true
					"c4": inp["c4"] = true
					"det": inp["det"] = true
					"crouch": inp["crouch"] = not bool(inp["crouch"])
		else:
			if touches.has(t.index):
				var r: String = touches[t.index]["role"]
				if r == "stick": stick = Vector2.ZERO; inp["mv"] = Vector2.ZERO
				if r == "fire": inp["fire"] = false
				if r == "tup": inp["tup"] = false
				if r == "tdn": inp["tdn"] = false
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
			KEY_SPACE:
				inp["tup"] = k.pressed
				if k.pressed: inp["jump"] = true
			KEY_R: if k.pressed: inp["reload"] = true
			KEY_Q: if k.pressed: inp["next"] = true
			KEY_E: if k.pressed: inp["use"] = true
			KEY_F: if k.pressed: inp["seat"] = true
			KEY_G: if k.pressed: inp["gren"] = true
			KEY_V: if k.pressed: inp["drone"] = true
			KEY_X: if k.pressed: inp["c4"] = true
			KEY_Z: if k.pressed: inp["det"] = true
			KEY_C:
				inp["crouch"] = k.pressed; inp["tdn"] = k.pressed
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
	if p is Player:
		var gx := s.x - 28.0; var gy := s.y - 378.0; var gc := Color(1, 0.8, 0.4)
		_txt(Vector2(gx - 320, gy - 12), "x%d" % (p as Player).gren, 20, gc, HORIZONTAL_ALIGNMENT_RIGHT, 296.0)
		draw_circle(Vector2(gx - 12, gy + 2), 7.0, gc); draw_rect(Rect2(gx - 15, gy - 9, 6, 4), gc)

const BTN_COL := {"fire": Color(1.0, 0.32, 0.25), "gren": Color(1.0, 0.7, 0.25), "c4": Color(1.0, 0.55, 0.2), "det": Color(1.0, 0.85, 0.25), "drone": Color(0.4, 0.9, 1.0),
	"use": Color(0.5, 1.0, 0.6), "seat": Color(0.5, 1.0, 0.6), "reload": Color(0.55, 0.8, 1.0), "tup": Color(1.0, 0.75, 0.3), "tdn": Color(0.6, 0.8, 1.0)}

func _pl(pts: Array, col: Color, w := 3.0, closed := false) -> void:
	var a := PackedVector2Array(pts)
	if closed: a.append(a[0])
	draw_polyline(a, col, w, true)

func _btn_draw(n: String, c: Vector2, r: float, on: bool) -> void:
	var ac: Color = BTN_COL.get(n, Color(0.85, 0.92, 1.0))
	draw_circle(c + Vector2(0, 3), r + 2.0, Color(0, 0, 0, 0.28))
	draw_circle(c, r, Color(0.06, 0.09, 0.14, 0.62))
	draw_circle(c, r * 0.88, Color(ac.r, ac.g, ac.b, 0.34 if on else 0.10))
	draw_arc(c, r, 0, TAU, 48, Color(ac.r, ac.g, ac.b, 0.95 if on else 0.6), 3.0 if on else 2.2, true)
	draw_arc(c, r - 4.0, -2.4, -0.7, 16, Color(1, 1, 1, 0.35), 2.0, true)
	var k := r * 0.42; var col := Color(1, 1, 1, 0.96) if on else Color(ac.lerp(Color.WHITE, 0.55), 0.95); var w := 3.2
	match n:
		"fire":
			draw_arc(c, k * 0.95, 0, TAU, 24, col, w, true); draw_circle(c, k * 0.28, col)
			for d in [Vector2(1, 0), Vector2(-1, 0), Vector2(0, 1), Vector2(0, -1)]: draw_line(c + d * k * 1.15, c + d * k * 1.7, col, w)
		"jump":
			_pl([c + Vector2(-k, k * 0.3), c + Vector2(0, -k * 0.5), c + Vector2(k, k * 0.3)], col, w + 0.6); _pl([c + Vector2(-k, k * 1.0), c + Vector2(0, k * 0.2), c + Vector2(k, k * 1.0)], Color(col, 0.55), w)
		"ads":
			draw_arc(c, k, 0, TAU, 28, col, w, true); draw_circle(c, 2.5, col)
			for d in [Vector2(1, 0), Vector2(-1, 0), Vector2(0, 1), Vector2(0, -1)]: draw_line(c + d * k * 0.45, c + d * k * 1.5, col, w * 0.8)
		"reload":
			draw_arc(c, k, -0.4, 4.6, 28, col, w, true); var e := c + Vector2(cos(-0.4), sin(-0.4)) * k
			_pl([e + Vector2(-k * 0.1, -k * 0.65), e, e + Vector2(k * 0.7, -k * 0.1)], col, w)
		"sprint":
			for o in [-k * 0.55, k * 0.35]: _pl([c + Vector2(o - k * 0.25, -k * 0.9), c + Vector2(o + k * 0.55, 0), c + Vector2(o - k * 0.25, k * 0.9)], col, w + 0.4)
		"next":
			_pl([c + Vector2(-k, -k * 0.45), c + Vector2(k, -k * 0.45)], col, w); _pl([c + Vector2(k * 0.4, -k * 0.95), c + Vector2(k, -k * 0.45), c + Vector2(k * 0.4, k * 0.05)], col, w)
			_pl([c + Vector2(k, k * 0.45), c + Vector2(-k, k * 0.45)], col, w); _pl([c + Vector2(-k * 0.4, -k * 0.05), c + Vector2(-k, k * 0.45), c + Vector2(-k * 0.4, k * 0.95)], col, w)
		"pause":
			draw_rect(Rect2(c + Vector2(-k * 0.65, -k * 0.8), Vector2(k * 0.45, k * 1.6)), col); draw_rect(Rect2(c + Vector2(k * 0.2, -k * 0.8), Vector2(k * 0.45, k * 1.6)), col)
		"use":
			_pl([c + Vector2(k * 0.2, -k), c + Vector2(k, -k), c + Vector2(k, k), c + Vector2(k * 0.2, k)], col, w)
			_pl([c + Vector2(-k, 0), c + Vector2(k * 0.5, 0)], col, w); _pl([c + Vector2(-k * 0.1, -k * 0.6), c + Vector2(k * 0.5, 0), c + Vector2(-k * 0.1, k * 0.6)], col, w)
		"seat":
			_pl([c + Vector2(-k * 0.6, -k * 0.2), c + Vector2(-k * 0.6, -k), c + Vector2(-k * 1.0, -k * 0.55)], col, w); _pl([c + Vector2(-k * 0.6, -k), c + Vector2(-k * 0.2, -k * 0.55)], col, w)
			_pl([c + Vector2(k * 0.6, k * 0.2), c + Vector2(k * 0.6, k), c + Vector2(k * 1.0, k * 0.55)], col, w); _pl([c + Vector2(k * 0.6, k), c + Vector2(k * 0.2, k * 0.55)], col, w)
			_pl([c + Vector2(-k * 0.6, -k * 0.2), c + Vector2(k * 0.6, k * 0.2)], Color(col, 0.5), w)
		"gren":
			draw_circle(c + Vector2(0, k * 0.25), k * 0.85, col); draw_rect(Rect2(c + Vector2(-k * 0.28, -k * 0.95), Vector2(k * 0.56, k * 0.4)), col)
			_pl([c + Vector2(k * 0.28, -k * 0.8), c + Vector2(k * 1.0, -k * 0.5), c + Vector2(k * 1.0, k * 0.1)], col, w)
			draw_arc(c + Vector2(0, k * 0.25), k * 0.45, 0.4, 1.6, 8, Color(0.06, 0.09, 0.14, 0.8), 2.0)
		"c4":
			draw_rect(Rect2(c + Vector2(-k * 1.05, -k * 0.6), Vector2(k * 2.1, k * 1.2)), col, false, w)
			for i in 3: draw_circle(c + Vector2(-k * 0.55 + i * k * 0.55, 0), k * 0.16, col)
			draw_circle(c + Vector2(k * 0.85, -k * 0.95), k * 0.14, Color(1, 0.25, 0.2)); _pl([c + Vector2(k * 0.85, -k * 0.8), c + Vector2(k * 0.85, -k * 0.6)], col, 2.0)
		"det":
			draw_colored_polygon(PackedVector2Array([c + Vector2(k * 0.25, -k * 1.1), c + Vector2(-k * 0.7, k * 0.15), c + Vector2(-k * 0.05, k * 0.15), c + Vector2(-k * 0.3, k * 1.1), c + Vector2(k * 0.75, -k * 0.2), c + Vector2(k * 0.1, -k * 0.2)]), col)
		"tup", "tdn":
			var sg := -1.0 if n == "tup" else 1.0
			_pl([c + Vector2(-k * 0.85, sg * k * 0.25), c + Vector2(0, -sg * k * 0.55), c + Vector2(k * 0.85, sg * k * 0.25)], col, w + 0.6)
			for i in 3: draw_line(c + Vector2(-k * 0.55 + i * k * 0.55, sg * k * 0.75), c + Vector2(-k * 0.55 + i * k * 0.55, sg * k * (0.75 + 0.25 * (i + 1))), Color(col, 0.7), w)
		"drone":
			draw_line(c + Vector2(-k, -k), c + Vector2(k, k), col, w); draw_line(c + Vector2(k, -k), c + Vector2(-k, k), col, w)
			for d in [Vector2(-1, -1), Vector2(1, -1), Vector2(-1, 1), Vector2(1, 1)]: draw_arc(c + d * k, k * 0.42, 0, TAU, 14, col, 2.4, true)
			draw_circle(c, k * 0.3, col)
		"crouch":
			_pl([c + Vector2(-k, -k * 0.3), c + Vector2(0, k * 0.5), c + Vector2(k, -k * 0.3)], col, w + 0.6); draw_line(c + Vector2(-k, k * 1.0), c + Vector2(k, k * 1.0), Color(col, 0.7), w)
		_:
			_txt(c + Vector2(-r, 8), n, 18, col, HORIZONTAL_ALIGNMENT_CENTER, r * 2.0)

func _hud_jet(p: Player, s: Vector2) -> void:
	var J := p.jet as Jet
	if J == null or not is_instance_valid(J): return
	var cam := get_viewport().get_camera_3d(); if cam == null: return
	var g := Color(0.35, 1.0, 0.5, 0.9)
	# بيبر المدفع (نقطة على بعد 350 م أمام الأنف)
	var aim := J.position + J.fwd() * 350.0
	if not cam.is_position_behind(aim):
		var a := cam.unproject_position(aim)
		draw_arc(a, 22.0, 0, TAU, 28, g, 2.0); draw_circle(a, 2.5, g)
		for d in [Vector2(1, 0), Vector2(-1, 0), Vector2(0, 1), Vector2(0, -1)]: draw_line(a + d * 22.0, a + d * 32.0, g, 2.0)
	# أعداء: أقواس + مسافة
	var near_d := 1.0e9
	for j in game.jets:
		var T := j as Jet
		if not T.alive or T.team == J.team: continue
		var dv := T.position - J.position; var dist := dv.length(); near_d = minf(near_d, dist)
		var col := Color(1, 0.25, 0.2, 0.95)
		if not cam.is_position_behind(T.position):
			var sp := cam.unproject_position(T.position); var r := clampf(900.0 / maxf(dist, 50.0) + 10.0, 10.0, 34.0)
			for sg in [Vector2(1, 1), Vector2(-1, 1), Vector2(1, -1), Vector2(-1, -1)]:
				draw_polyline(PackedVector2Array([sp + sg * Vector2(r, r * 0.4), sp + sg * Vector2(r, r), sp + sg * Vector2(r * 0.4, r)]), col, 2.0)
			_txt(sp + Vector2(-40, r + 22), "%d m" % int(dist), 16, col, HORIZONTAL_ALIGNMENT_CENTER, 80.0)
		else:  # خلفك: سهم على الحافة
			var loc := cam.global_transform.affine_inverse() * T.position; var ang := atan2(loc.x, -loc.y * 0.0 + 0.0001) if false else atan2(loc.x, maxf(0.01, absf(loc.y) + 0.01))
			var ctr := s * 0.5; var dir2 := Vector2(loc.x, -loc.y).normalized() if Vector2(loc.x, -loc.y).length() > 0.01 else Vector2(0, 1)
			var ep := ctr + dir2 * minf(s.x, s.y) * 0.42
			draw_colored_polygon(PackedVector2Array([ep + dir2 * 14.0, ep + dir2.rotated(2.4) * 12.0, ep + dir2.rotated(-2.4) * 12.0]), col)
	# أدوات الطيران: سرعة، ارتفاع، دفع، صحة، ذخيرة
	var spd := J.speed() * 3.6
	_txt(Vector2(24, s.y - 160), "%d km/h" % int(spd), 24, g)
	_txt(Vector2(24, s.y - 132), "ALT %d m" % int(J.position.y), 22, g)
	draw_rect(Rect2(24, s.y - 104, 160, 12), Color(0, 0, 0, 0.5)); draw_rect(Rect2(26, s.y - 102, 156.0 * J.th, 8), Color(1, 0.7, 0.2) if J.ab < 0.3 else Color(1, 0.35, 0.15))
	_txt(Vector2(192, s.y - 108), "THR %d%%" % int(J.th * 100.0), 16, Color.WHITE)
	var hpk := clampf(J.hp / 300.0, 0.0, 1.0)
	draw_rect(Rect2(24, s.y - 86, 160, 12), Color(0, 0, 0, 0.5)); draw_rect(Rect2(26, s.y - 84, 156.0 * hpk, 8), Color(0.3, 0.85, 0.35) if hpk > 0.35 else Color(0.9, 0.25, 0.2))
	_txt(Vector2(192, s.y - 90), "HP", 16, Color.WHITE)
	_txt(Vector2(s.x - 340, s.y - 330), "F-35", 20, Color(0.7, 0.85, 1.0), HORIZONTAL_ALIGNMENT_RIGHT, 320.0)
	_txt(Vector2(s.x - 340, s.y - 304), "GUN %d" % J.gun, 28, Color.WHITE, HORIZONTAL_ALIGNMENT_RIGHT, 320.0)
	if J.gnd: _txt(Vector2(s.x * 0.5 - 300, s.y * 0.72), Settings.t("على المدرج — ارفع الدفع THR+ ثم اسحب للأعلى"), 22, Color(1, 0.9, 0.4), HORIZONTAL_ALIGNMENT_CENTER, 600.0)
	elif J.stall and J.speed() < 60.0: _txt(Vector2(s.x * 0.5 - 200, s.y * 0.2), "STALL", 34, Color(1, 0.3, 0.2), HORIZONTAL_ALIGNMENT_CENTER, 400.0)
	if not J.gnd and J.vel.y < 0.0 and J.position.y < -J.vel.y * 6.0 + 40.0: _txt(Vector2(s.x * 0.5 - 200, s.y * 0.26), "PULL UP", 34, Color(1, 0.2, 0.15, 0.6 + 0.4 * sin(Time.get_ticks_msec() * 0.02)), HORIZONTAL_ALIGNMENT_CENTER, 400.0)

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
	elif p.alive and p.jet == null:
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
	if p.jet != null: _hud_jet(p as Player, s)
	elif p.veh != null:
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
	# كشف الأعداء (المسيّرة) + شاشة المسيّرة
	var ac := get_viewport().get_camera_3d()
	if ac != null:
		for f in game.fighters:
			if is_instance_valid(f) and f.alive and f.reveal > 0.0 and f.team != p.team:
				var wp: Vector3 = f.global_position + Vector3(0, 2.0, 0)
				if not ac.is_position_behind(wp):
					var sp := ac.unproject_position(wp); var dpx := 14.0
					draw_colored_polygon(PackedVector2Array([sp + Vector2(0, dpx), sp + Vector2(-dpx * 0.7, -dpx * 0.4), sp + Vector2(dpx * 0.7, -dpx * 0.4)]), Color(1, 0.15, 0.1, clampf(f.reveal, 0.0, 1.0)))
	if p is Player and (p as Player).drone != null:
		var dr: Drone = (p as Player).drone
		draw_rect(Rect2(Vector2.ZERO, s), Color(0, 0.1, 0.05, 0.12))
		draw_arc(Vector2(cx, cy), 26.0, 0, TAU, 32, Color(0.3, 1, 0.5, 0.8), 2.0)
		_txt(Vector2(cx - 300, 40), Settings.t("مسيّرة") + "  %d s   [FIRE] = " % ceili(dr.t) + Settings.t("تفجير") + "   [V] = " + Settings.t("هبوط"), 20, Color(0.4, 1, 0.6), HORIZONTAL_ALIGNMENT_CENTER, 600.0)
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
		for n in BTNS:
			if not _btn_on(n): continue
			var b := _btn(n)
			var on: bool = (n == "ads" and bool(game.inp["ads"])) or (n == "sprint" and bool(game.inp["sprint"])) or (n == "crouch" and bool(game.inp["crouch"]))
			_btn_draw(n, b["c"], b["r"], on)
		if stick_c != Vector2.ZERO and touches.values().any(func(v): return v["role"] == "stick"):
			draw_circle(stick_c, 70, Color(1, 1, 1, 0.1)); draw_circle(stick_c + stick * 60.0, 30, Color(1, 1, 1, 0.3))
