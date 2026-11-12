class_name UiTheme
extends RefCounted
# مظهر واجهة حديث: بطاقات زجاجية مدوّرة، لون مميّز أزرق، أزرار بيضاوية مع ظلال وتأثير ضغط

const BG := Color("0b1020")
const CARD := Color(0.09, 0.12, 0.2, 0.86)
const BORDER := Color(0.28, 0.35, 0.55, 0.55)
const ACCENT := Color("4f8cff")
const ACCENT2 := Color("ff4d5e")
const TEXT := Color("e8ecf8")
const MUTED := Color("8b95b5")

static func flat(bg: Color, radius := 18, border := Color(0, 0, 0, 0), bw := 0, shadow := 0) -> StyleBoxFlat:
	var s := StyleBoxFlat.new(); s.bg_color = bg
	s.set_corner_radius_all(radius)
	s.set_border_width_all(bw); s.border_color = border
	if shadow > 0: s.shadow_size = shadow; s.shadow_color = Color(0, 0, 0, 0.45); s.shadow_offset = Vector2(0, 4)
	s.content_margin_left = 18; s.content_margin_right = 18; s.content_margin_top = 10; s.content_margin_bottom = 10
	return s

static func make() -> Theme:
	var t := Theme.new()
	t.set_default_font_size(24)
	t.set_color("font_color", "Label", TEXT)
	t.set_color("font_color", "Button", TEXT); t.set_color("font_hover_color", "Button", Color.WHITE); t.set_color("font_pressed_color", "Button", Color.WHITE)
	t.set_stylebox("normal", "Button", flat(Color(0.16, 0.2, 0.32, 0.95), 16, BORDER, 1))
	t.set_stylebox("hover", "Button", flat(Color(0.2, 0.26, 0.42, 0.98), 16, ACCENT, 1))
	t.set_stylebox("pressed", "Button", flat(ACCENT, 16))
	t.set_stylebox("focus", "Button", StyleBoxEmpty.new())
	return t

static func accent_button() -> Array:
	return [flat(ACCENT, 28, Color(1, 1, 1, 0.25), 1, 14), flat(Color("6a9dff"), 28, Color(1, 1, 1, 0.4), 1, 18), flat(Color("3868cc"), 28, Color(1, 1, 1, 0.25), 1, 6)]

static func style_accent(b: Button) -> void:
	var st := accent_button()
	b.add_theme_stylebox_override("normal", st[0]); b.add_theme_stylebox_override("hover", st[1]); b.add_theme_stylebox_override("pressed", st[2])
	b.add_theme_color_override("font_color", Color.WHITE)

static func style_ghost(b: Button) -> void:
	b.add_theme_stylebox_override("normal", flat(Color(1, 1, 1, 0.06), 28, BORDER, 1))
	b.add_theme_stylebox_override("hover", flat(Color(1, 1, 1, 0.12), 28, ACCENT, 1))
	b.add_theme_stylebox_override("pressed", flat(Color(1, 1, 1, 0.2), 28))

# ضغط فوري + تأثير تصغير بسيط
static func juicy(b: Button) -> void:
	b.focus_mode = Control.FOCUS_NONE
	b.action_mode = BaseButton.ACTION_MODE_BUTTON_PRESS
	b.resized.connect(func(): b.pivot_offset = b.size * 0.5)
	b.button_down.connect(func():
		var tw := b.create_tween(); tw.tween_property(b, "scale", Vector2(0.95, 0.95), 0.05))
	b.button_up.connect(func():
		var tw := b.create_tween(); tw.tween_property(b, "scale", Vector2.ONE, 0.08))

const BG_SHADER := "shader_type canvas_item;\nvoid fragment(){ vec2 uv = UV; float t = TIME*0.15; vec3 col = vec3(0.04,0.06,0.13); col += vec3(0.9,0.2,0.25)*0.20*smoothstep(0.65,0.0,distance(uv,vec2(0.12+0.05*sin(t),0.18+0.1*cos(t*1.3)))); col += vec3(0.2,0.45,1.0)*0.24*smoothstep(0.75,0.0,distance(uv,vec2(0.88+0.05*cos(t*0.9),0.82+0.08*sin(t)))); col += 0.012*step(0.97, fract(uv.x*48.0+uv.y*6.0)); COLOR = vec4(col,1.0); }"
