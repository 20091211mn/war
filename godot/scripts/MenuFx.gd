class_name MenuFx
extends RefCounted
# مؤثرات واجهة البداية: أيقونات متجهة (بدون ايموجي)، شيدرات العنوان والتظليل، شرر متصاعد، ومقطع افتتاحي سينمائي

const SHINE := "shader_type canvas_item;\nvoid fragment(){ vec4 c = texture(TEXTURE, UV) * COLOR; float x = SCREEN_UV.x - SCREEN_UV.y * 0.3; float p = fract(x * 0.7 - TIME * 0.2); float band = exp(-pow((p - 0.5) * 16.0, 2.0)); c.rgb += band * vec3(0.9, 0.95, 1.0) * 0.85 * c.a; COLOR = c; }"
const VIG := "shader_type canvas_item;\nuniform float flip = 0.0;\nvoid fragment(){ vec2 uv = UV; float x = mix(uv.x, 1.0 - uv.x, flip); float side = smoothstep(0.9, 0.05, x); float vig = smoothstep(0.35, 1.05, distance(uv, vec2(0.5))); float low = smoothstep(0.55, 1.0, uv.y) * 0.4; float a = clamp(side * 0.74 + vig * 0.5 + low, 0.0, 0.93); vec3 col = mix(vec3(0.02, 0.03, 0.07), vec3(0.04, 0.07, 0.17), uv.y); COLOR = vec4(col, a); }"

static func _shader_mat(code: String) -> ShaderMaterial:
	var sh := Shader.new(); sh.code = code; var m := ShaderMaterial.new(); m.shader = sh; return m

# تدرّج لوني سينمائي فوق المشهد ثلاثي الأبعاد: تباين أعلى، دفء في الإضاءات، تبريد في الظلال، وإزالة غشاوة الضباب
const GRADE := "shader_type canvas_item;\nuniform sampler2D scr : hint_screen_texture, filter_linear;\nvoid fragment(){ vec3 c = texture(scr, SCREEN_UV).rgb; float l = dot(c, vec3(0.299, 0.587, 0.114)); c = mix(vec3(l), c, 1.35); c = (c - 0.5) * 1.28 + 0.46; c *= mix(vec3(0.78, 0.9, 1.1), vec3(1.14, 1.02, 0.86), smoothstep(0.25, 0.75, l)); vec2 q = SCREEN_UV - 0.5; c *= 1.0 - dot(q, q) * 1.1; COLOR = vec4(clamp(c, 0.0, 1.0), 1.0); }"

static func grade(parent: Control) -> ColorRect:
	var r := ColorRect.new(); r.set_anchors_preset(Control.PRESET_FULL_RECT); r.mouse_filter = Control.MOUSE_FILTER_IGNORE; r.material = _shader_mat(GRADE); r.color = Color.WHITE
	parent.add_child(r); return r

static func shine_material() -> ShaderMaterial: return _shader_mat(SHINE)

static func vignette(parent: Control) -> ColorRect:
	var r := ColorRect.new(); r.set_anchors_preset(Control.PRESET_FULL_RECT); r.mouse_filter = Control.MOUSE_FILTER_IGNORE; r.material = _shader_mat(VIG); r.color = Color.WHITE
	parent.add_child(r); return r

# شرر/غبار متصاعد خفيف فوق الخلفية
static func embers(parent: Control) -> Control:
	var h := Control.new(); h.set_anchors_preset(Control.PRESET_FULL_RECT); h.mouse_filter = Control.MOUSE_FILTER_IGNORE; parent.add_child(h)
	var p := CPUParticles2D.new(); p.amount = 46; p.lifetime = 8.0; p.preprocess = 8.0; p.local_coords = false
	p.emission_shape = CPUParticles2D.EMISSION_SHAPE_RECTANGLE; p.direction = Vector2(0.25, -1.0); p.spread = 18.0; p.gravity = Vector2.ZERO
	p.initial_velocity_min = 18.0; p.initial_velocity_max = 55.0; p.scale_amount_min = 1.2; p.scale_amount_max = 3.2
	var g := Gradient.new(); g.set_color(0, Color(1.0, 0.62, 0.35, 0.0)); g.set_color(1, Color(1.0, 0.45, 0.3, 0.0)); g.add_point(0.25, Color(1.0, 0.7, 0.4, 0.75)); g.add_point(0.7, Color(1.0, 0.5, 0.3, 0.45))
	p.color_ramp = g; h.add_child(p)
	h.resized.connect(func(): p.position = Vector2(h.size.x * 0.5, h.size.y + 10.0); p.emission_rect_extents = Vector2(h.size.x * 0.5, 4.0))
	return h

# ---------------------------------------------------------------- أيقونات متجهة
static func draw_icon(c: Control, kind: String, col: Color) -> void:
	if kind == "chev_a" or kind == "chev_b": kind = chev_for(Settings.lang == 0, kind == "chev_a")
	var s := c.size; var m := minf(s.x, s.y); var ct := s * 0.5; var r := m * 0.5
	match kind:
		"play":
			var pts := PackedVector2Array([ct + Vector2(-0.32, -0.42) * m, ct + Vector2(0.44, 0.0) * m, ct + Vector2(-0.32, 0.42) * m])
			c.draw_colored_polygon(pts, col)
		"chev_l", "chev_r":
			var d := -1.0 if kind == "chev_l" else 1.0
			c.draw_polyline(PackedVector2Array([ct + Vector2(-0.14 * d, -0.3) * m, ct + Vector2(0.16 * d, 0.0), ct + Vector2(-0.14 * d, 0.3) * m]), col, maxf(3.0, m * 0.1), true)
		"close":
			var k := m * 0.26; c.draw_line(ct + Vector2(-k, -k), ct + Vector2(k, k), col, maxf(3.0, m * 0.1), true); c.draw_line(ct + Vector2(k, -k), ct + Vector2(-k, k), col, maxf(3.0, m * 0.1), true)
		"grid":
			var q := m * 0.2; var gap := m * 0.06
			for i in 2:
				for j in 2: c.draw_rect(Rect2(ct + Vector2((i - 1) * (q + gap) + (gap * 0.5 if i == 1 else -gap * 0.5), (j - 1) * (q + gap) + (gap * 0.5 if j == 1 else -gap * 0.5)) + Vector2(0, 0), Vector2(q, q)), col)
		"gear":
			var rr := m * 0.3
			c.draw_arc(ct, rr, 0.0, TAU, 32, col, maxf(3.0, m * 0.09), true)
			for i in 8:
				var a := i * TAU / 8.0; var dv := Vector2(cos(a), sin(a))
				c.draw_line(ct + dv * rr, ct + dv * (rr + m * 0.13), col, maxf(4.0, m * 0.12), true)
			c.draw_circle(ct, m * 0.07, col)

static func icon(kind: String, col := Color.WHITE) -> Control:
	var c := Control.new(); c.set_anchors_preset(Control.PRESET_FULL_RECT); c.mouse_filter = Control.MOUSE_FILTER_IGNORE
	c.draw.connect(func(): draw_icon(c, kind, col)); return c

static func attach_icon(b: Button, kind: String, col := Color.WHITE, pad := 0.0) -> void:
	b.text = ""; var ic := icon(kind, col)
	if pad > 0.0: ic.offset_left = pad; ic.offset_top = pad; ic.offset_right = -pad; ic.offset_bottom = -pad
	b.add_child(ic)

# اتجاه الأسهم يتبع اتجاه اللغة: السهم الأول يشير إلى طرف الصف الخارجي
static func chev_for(rtl: bool, first: bool) -> String:
	return "chev_l" if (first != rtl) else "chev_r"

# ---------------------------------------------------------------- مقطع افتتاحي
# يُعرض مرة عند الفتح: ظهور من الأسود فوق طيران كاميرا سينمائي، RED تصطدم بـ BLUE، ثم عنوان اللعبة. أي لمسة تتخطى.
static func intro(game: Node, ui: Control, on_done: Callable) -> Control:
	var vs := ui.get_viewport_rect().size
	var root := Control.new(); root.set_anchors_preset(Control.PRESET_FULL_RECT); root.mouse_filter = Control.MOUSE_FILTER_STOP; ui.add_child(root)
	root.layout_direction = Control.LAYOUT_DIRECTION_LTR  # الافتتاحية مرسومة بإحداثيات مطلقة: لا تنعكس مع RTL
	var done := [false]
	var stage := Control.new(); stage.set_anchors_preset(Control.PRESET_FULL_RECT); stage.mouse_filter = Control.MOUSE_FILTER_IGNORE; root.add_child(stage)
	var black := ColorRect.new(); black.color = Color(0, 0, 0, 1); black.set_anchors_preset(Control.PRESET_FULL_RECT); black.mouse_filter = Control.MOUSE_FILTER_IGNORE; stage.add_child(black)
	var vig := vignette(stage); vig.material.set_shader_parameter("flip", 0.5)
	var bars: Array = []
	for top in [true, false]:
		var b := ColorRect.new(); b.color = Color.BLACK; b.mouse_filter = Control.MOUSE_FILTER_IGNORE; stage.add_child(b)
		b.set_anchors_preset(Control.PRESET_TOP_WIDE if top else Control.PRESET_BOTTOM_WIDE); b.custom_minimum_size = Vector2(0, vs.y * 0.13); b.offset_bottom = vs.y * 0.13 if top else 0.0; b.offset_top = 0.0 if top else -vs.y * 0.13
		bars.append(b)
	var mk := func(txt: String, size: int, col: Color) -> Label:
		var l := Label.new(); l.text = txt; l.add_theme_font_size_override("font_size", size); l.add_theme_color_override("font_color", col)
		l.add_theme_color_override("font_outline_color", Color(0, 0, 0, 0.7)); l.add_theme_constant_override("outline_size", 10)
		l.horizontal_alignment = HORIZONTAL_ALIGNMENT_CENTER; l.vertical_alignment = VERTICAL_ALIGNMENT_CENTER; l.size = Vector2(560, 200); l.pivot_offset = l.size * 0.5; l.modulate.a = 0.0; stage.add_child(l); return l
	var cy := vs.y * 0.5 - 100.0
	var red: Label = mk.call("RED", 150, Color(1.0, 0.3, 0.3)); var blue: Label = mk.call("BLUE", 150, Color(0.35, 0.6, 1.0)); var vsl: Label = mk.call("VS", 64, Color.WHITE)
	red.position = Vector2(-600.0, cy); blue.position = Vector2(vs.x + 40.0, cy); vsl.position = Vector2(vs.x * 0.5 - 280.0, cy + 10.0); vsl.scale = Vector2(3, 3)
	var title: Label = mk.call("WARZONE", 150, Color.WHITE); title.size = Vector2(vs.x, 200); title.position = Vector2(0, cy - 10.0); title.pivot_offset = title.size * 0.5; title.material = shine_material()
	var sub := Label.new(); sub.text = "RED  VS  BLUE"; sub.add_theme_font_size_override("font_size", 28); sub.add_theme_color_override("font_color", Color(0.6, 0.7, 0.95)); sub.size = Vector2(vs.x, 40); sub.position = Vector2(0, cy + 190.0); sub.horizontal_alignment = HORIZONTAL_ALIGNMENT_CENTER; sub.modulate.a = 0.0; stage.add_child(sub)
	var skip := Label.new(); skip.text = Settings.t("المس للتخطي"); skip.add_theme_font_size_override("font_size", 20); skip.add_theme_color_override("font_color", Color(1, 1, 1, 0.55)); skip.size = Vector2(vs.x, 30); skip.position = Vector2(0, vs.y * 0.93 - 4.0); skip.horizontal_alignment = HORIZONTAL_ALIGNMENT_CENTER; stage.add_child(skip)
	var flash := ColorRect.new(); flash.color = Color(1, 1, 1, 0); flash.set_anchors_preset(Control.PRESET_FULL_RECT); flash.mouse_filter = Control.MOUSE_FILTER_IGNORE; stage.add_child(flash)
	var finish := func():
		if done[0]: return
		done[0] = true
		var tw := root.create_tween(); tw.tween_property(root, "modulate:a", 0.0, 0.35); tw.tween_callback(func(): root.queue_free(); on_done.call())
	root.gui_input.connect(func(e: InputEvent):
		if (e is InputEventMouseButton and e.pressed) or (e is InputEventScreenTouch and e.pressed) or (e is InputEventKey and e.pressed): finish.call())
	var tw := root.create_tween()
	tw.tween_property(black, "color:a", 0.0, 0.9)
	tw.parallel().tween_property(red, "modulate:a", 1.0, 0.25).set_delay(0.5)
	tw.parallel().tween_property(red, "position:x", vs.x * 0.27 - 280.0, 0.5).set_delay(0.5).set_trans(Tween.TRANS_BACK).set_ease(Tween.EASE_OUT)
	tw.parallel().tween_property(blue, "modulate:a", 1.0, 0.25).set_delay(0.85)
	tw.parallel().tween_property(blue, "position:x", vs.x * 0.73 - 280.0, 0.5).set_delay(0.85).set_trans(Tween.TRANS_BACK).set_ease(Tween.EASE_OUT)
	tw.tween_callback(func():
		if "sfx" in game and not game.selftest: game.sfx_play("boom", game.prev_cam.global_position if game.prev_cam != null else Vector3.ZERO))
	tw.tween_property(vsl, "modulate:a", 1.0, 0.05)
	tw.parallel().tween_property(vsl, "scale", Vector2.ONE, 0.18).set_trans(Tween.TRANS_QUAD).set_ease(Tween.EASE_OUT)
	tw.parallel().tween_property(flash, "color:a", 0.75, 0.05)
	tw.tween_property(flash, "color:a", 0.0, 0.4)
	tw.parallel().tween_property(stage, "position", Vector2(6, -4), 0.04)
	tw.tween_property(stage, "position", Vector2.ZERO, 0.12)
	tw.tween_interval(0.35)
	tw.tween_property(red, "modulate:a", 0.0, 0.25); tw.parallel().tween_property(blue, "modulate:a", 0.0, 0.25); tw.parallel().tween_property(vsl, "modulate:a", 0.0, 0.25)
	tw.parallel().tween_property(red, "position:x", red.position.x - 120.0, 0.25); tw.parallel().tween_property(blue, "position:x", blue.position.x + 120.0, 0.25)
	title.scale = Vector2(1.5, 1.5)
	tw.tween_property(title, "modulate:a", 1.0, 0.3)
	tw.parallel().tween_property(title, "scale", Vector2.ONE, 0.55).set_trans(Tween.TRANS_EXPO).set_ease(Tween.EASE_OUT)
	tw.parallel().tween_property(sub, "modulate:a", 1.0, 0.5).set_delay(0.25)
	tw.tween_interval(1.1)
	tw.tween_callback(finish)
	return root
