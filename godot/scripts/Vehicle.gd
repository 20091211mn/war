class_name Vehicle
extends CharacterBody3D
# مركبات: الشاص (جيب بيك أب بـ5 مقاعد ورشاش مثبّت) والدبابة (مدفع بمدى 160م). منقولة من فيزياء المحرك الأصلي (veh.h)
# فيزياء قيادة (جر جانبي، تعثر الإطارات)، أضرار بالأجزاء (إطارات/خزان)، حرق وانفجار، سائق بوت بتفادي عوائق، مدفعي بوت

const JEEP := [
	[0,0.5,0,1.7,0.2,4.9,.1,.1,.11,0],
	[0,0.86,-1.7,1.96,0.46,1.6,0.78,0.72,0.58,0],
	[0,1.12,-1.55,1.2,0.05,0.9,.13,.13,.14,0],
	[-0.97,0.98,-1.55,0.14,0.34,1.1,0.78,0.72,0.58,0],
	[0.97,0.98,-1.55,0.14,0.34,1.1,0.78,0.72,0.58,0],
	[0,0.74,-2.52,1.9,0.38,0.1,.13,.13,.14,0],
	[0,0.5,-2.58,2.0,0.16,0.14,.45,.46,.48,0],
	[-0.68,0.94,-2.53,0.26,0.2,0.06,1,.95,.75,1],
	[0.68,0.94,-2.53,0.26,0.2,0.06,1,.95,.75,1],
	[0,0.72,-0.15,2.0,0.66,1.5,0.78,0.72,0.58,0],
	[0,1.86,-0.15,1.96,0.1,1.2,0.78,0.72,0.58,0],
	[-0.93,1.45,-0.74,0.08,0.8,0.08,0.78,0.72,0.58,0],
	[0.93,1.45,-0.74,0.08,0.8,0.08,0.78,0.72,0.58,0],
	[-0.93,1.45,0.4,0.08,0.8,0.1,0.78,0.72,0.58,0],
	[0.93,1.45,0.4,0.08,0.8,0.1,0.78,0.72,0.58,0],
	[0,1.46,-0.76,1.8,0.78,0.04,.2,.3,.38,0],
	[0,1.46,0.45,1.8,0.6,0.04,.2,.3,.38,0],
	[-0.45,0.62,-0.25,0.7,0.14,0.6,.2,.2,.19,0],
	[0.45,0.62,-0.25,0.7,0.14,0.6,.2,.2,.19,0],
	[-0.45,0.95,0.12,0.7,0.55,0.12,.22,.22,.21,0],
	[0.45,0.95,0.12,0.7,0.55,0.12,.22,.22,.21,0],
	[0,0.98,-0.72,1.7,0.12,0.2,.13,.13,.14,0],
	[0,0.88,1.55,1.9,0.08,1.9,.13,.13,.14,0],
	[-0.96,1.08,1.55,0.08,0.42,1.9,0.78,0.72,0.58,0],
	[0.96,1.08,1.55,0.08,0.42,1.9,0.78,0.72,0.58,0],
	[0,1.08,2.46,1.9,0.42,0.08,0.78,0.72,0.58,0],
	[0,1.08,0.66,1.9,0.42,0.08,0.78,0.72,0.58,0],
	[0,0.62,1.6,1.6,0.28,1.6,.1,.1,.1,0],
	[0,0.7,-2.575,1.1,0.03,0.03,.62,.64,.68,0],
	[0,0.77,-2.575,1.1,0.03,0.03,.62,.64,.68,0],
	[0,0.84,-2.575,1.1,0.03,0.03,.62,.64,.68,0],
	[0,0.86,-2.58,0.2,0.1,0.03,.7,.1,.1,0],
	[0,0.37,2.56,2.0,0.14,0.12,.4,.41,.43,0],
	[-0.55,1.1,-1.4,0.6,0.012,0.45,.62,.56,.42,0],
	[0.35,1.1,-1.9,0.5,0.012,0.3,.62,.56,.42,0],
	[0.1,1.45,-0.3,0.7,0.012,0.5,.62,.56,.42,0],
	[0.55,1.35,0.9,0.8,0.8,0.3,.06,.06,.07,0],
	[0,1.28,1.7,0.1,0.8,0.1,.07,.07,.07,0],
	[-0.55,1.0,2.0,0.3,0.2,0.2,.3,.3,.24,0],
	[-0.7,0.62,2.52,0.3,0.12,0.05,.9,.1,.1,1],
	[0.7,0.62,2.52,0.3,0.12,0.05,.9,.1,.1,1],
	[-1.005,0.96,-0.62,0.012,0.72,0.025,.05,.05,.05,0],
	[1.005,0.96,-0.62,0.012,0.72,0.025,.05,.05,.05,0],
	[-1.005,0.96,0.32,0.012,0.72,0.025,.05,.05,.05,0],
	[1.005,0.96,0.32,0.012,0.72,0.025,.05,.05,.05,0],
	[-1.12,1.36,-0.82,0.09,0.15,0.1,.06,.06,.06,0],
	[1.12,1.36,-0.82,0.09,0.15,0.1,.06,.06,.06,0],
	[-1.05,0.5,-0.15,0.12,0.05,1.5,.08,.08,.08,0],
	[1.05,0.5,-0.15,0.12,0.05,1.5,.08,.08,.08,0]
]

const VPS := [
	{"hp": 450.0, "L": 3.0, "hw": 1.0, "hl": 2.5, "max_fwd": 26.0, "max_rev": 7.0, "accel": 7.5, "brake": 16.0, "max_steer": 0.52, "crash_min": 4.0, "armor": 1.0, "tracked": false, "min_h": 0.55},
	{"hp": 1100.0, "L": 3.6, "hw": 1.5, "hl": 3.1, "max_fwd": 11.0, "max_rev": 5.0, "accel": 3.8, "brake": 12.0, "max_steer": 1.0, "crash_min": 6.0, "armor": 0.14, "tracked": true, "min_h": 1.15}]
# صناديق الإصابة: [x0,y0,z0,x1,y1,z1,جزء]  (0 جسم، 1 خزان، 10+i عجلة)
const BODY := [[-1,0.3,-2.5,1,1.1,-0.9,0],[-1,0.3,-0.9,1,1.05,0.6,0],[-0.95,1.8,-0.7,0.95,1.9,0.4,0],[-1,0.3,0.6,1,0.85,2.5,1],
	[-1,0.85,0.6,-0.9,1.25,2.5,0],[0.9,0.85,0.6,1,1.25,2.5,0],[-1,0.85,2.4,1,1.25,2.5,0],[-1,0.85,0.6,1,1.25,0.7,0]]
const BODY_T := [[-1.45,0.35,-3.1,1.45,1.3,3.1,0],[-1.0,1.3,-1.5,1.0,1.95,1.3,0],[-0.12,1.5,-4.8,0.12,1.8,-1.5,0],[-1.2,1.0,1.4,1.2,1.35,3.0,1],[-1.62,0.1,-3.0,-1.2,0.9,3.0,0],[1.2,0.1,-3.0,1.62,0.9,3.0,0]]
const WH := [[-0.95, -1.55], [0.95, -1.55], [-0.95, 1.5], [0.95, 1.5]]
const MG := {"kind": "lmg", "dmg": 26.0, "range": [45.0, 130.0], "minMul": 0.45, "head": 1.4, "pellets": 1}
const SEATS_JEEP := [Vector3(-0.45, 0.4, -0.25), Vector3(0, 0.7, 1.9), Vector3(0.45, 0.4, -0.25), Vector3(-0.45, 0.4, 0.95), Vector3(0.45, 0.4, 0.95)]

var game: Node
var vtype := 0
var P: Dictionary
var hp := 450.0
var team := -1
var driver: Fighter = null
var gunner: Fighter = null
var riders: Array = []
var dead := false
var burn := false
var respawn_t := 0.0
var h := 0.0
var vx := 0.0
var vz := 0.0
var speed := 0.0
var steer := 0.0
var tires := 0
var tire_hp := [30.0, 30.0, 30.0, 30.0]
var crash_t := 0.0
var cannon_t := 0.0
var kick := 0.0
var ty := 0.0            # اتجاه البرج/الرشاش (عالمي)
var tp := 0.0
var want_ty := 0.0
var want_tp := 0.0
var mg_cd := 0.0
var home := Vector3.ZERO
var home_h := 0.0
var in_thr := 0.0
var in_st := 0.0
var in_hb := false
var in_fire := false
# مرئيات
var vis: Node3D
var turret: Node3D
var gun_pivot: Node3D
var gun_slide: Node3D
var muzzle: Node3D
var flash_m: MeshInstance3D
var mg_yaw: Node3D
var mg_pitch: Node3D
var mg_muzzle: Node3D
var wheels: Array = []     # [pivot, mesh] لكل عجلة
var ribs: Array = []
var rib_phase := 0.0
var mats: Array = []       # [material, اللون الأصلي]
var team_mats: Array = []
var hit_body: StaticBody3D
var hit_parts: Array = []
var smoke: CPUParticles3D
var fire: CPUParticles3D
var dust: CPUParticles3D
var engine: AudioStreamPlayer3D
var wheel_spin := 0.0
# ذكاء السائق
var ai_t := 0.0
var ai_goal := Vector3.ZERO
var ai_target: Node3D = null
var ai_stuck := 0.0
var ai_rev := 0.0
var ai_rev_dir := 1.0
var ai_burst := 0.0
var ai_rest := 0.0
static var engine_stream: AudioStreamWAV = null

func setup(g: Node, t: int, x: float, z: float, hd: float) -> void:
	game = g; vtype = t; P = VPS[t]; hp = P["hp"]; home = Vector3(x, 0, z); home_h = hd
	h = hd; ty = hd; want_ty = hd
	global_position = home; rotation.y = h
	collision_layer = 1; collision_mask = 1
	var hw: float = P["hw"]; var hl: float = P["hl"]; var mh: float = P["min_h"]
	var cs := CollisionShape3D.new(); var sh := BoxShape3D.new(); sh.size = Vector3(hw * 2.0, 1.6 - mh, hl * 2.0); cs.shape = sh; cs.position = Vector3(0, (mh + 1.6) * 0.5, 0); add_child(cs)
	_build_hit()
	vis = Node3D.new(); add_child(vis)
	if t == 0: _build_jeep()
	else: _build_tank()
	_build_fx()
	_set_team_color()

# ---------------------------------------------------------------- مواد ونماذج
func _mat(c: Color, unlit: bool) -> StandardMaterial3D:
	var m := StandardMaterial3D.new(); m.albedo_color = c; m.roughness = 0.75; m.metallic = 0.12
	if unlit: m.emission_enabled = true; m.emission = c; m.emission_energy_multiplier = 1.6
	mats.append([m, c]); return m

func _bx(parent: Node3D, sz: Vector3, pos: Vector3, c: Color, unlit := false) -> MeshInstance3D:
	var mi := MeshInstance3D.new(); var bm := BoxMesh.new(); bm.size = sz; mi.mesh = bm; mi.position = pos; mi.material_override = _mat(c, unlit)
	if Settings.quality < 2: mi.cast_shadow = GeometryInstance3D.SHADOW_CASTING_SETTING_OFF
	parent.add_child(mi); return mi

func _cyl(parent: Node3D, r: float, w: float, pos: Vector3, c: Color) -> MeshInstance3D:
	var mi := MeshInstance3D.new(); var cm := CylinderMesh.new(); cm.top_radius = r; cm.bottom_radius = r; cm.height = w; cm.radial_segments = 14 if Settings.quality >= 2 else 8; cm.rings = 1; mi.mesh = cm
	mi.rotation.z = PI * 0.5; mi.position = pos; mi.material_override = _mat(c, false)
	if Settings.quality < 2: mi.cast_shadow = GeometryInstance3D.SHADOW_CASTING_SETTING_OFF
	parent.add_child(mi); return mi

func _build_hit() -> void:
	hit_body = StaticBody3D.new(); hit_body.collision_layer = 4; hit_body.collision_mask = 0; hit_body.set_meta("veh", self); add_child(hit_body)
	var boxes: Array = BODY if vtype == 0 else BODY_T
	for b in boxes:
		var cs := CollisionShape3D.new(); var sh := BoxShape3D.new(); sh.size = Vector3(b[3] - b[0], b[4] - b[1], b[5] - b[2]); cs.shape = sh
		cs.position = Vector3((b[0] + b[3]) * 0.5, (b[1] + b[4]) * 0.5, (b[2] + b[5]) * 0.5); hit_body.add_child(cs); hit_parts.append(int(b[6]))
	if vtype == 0:
		for i in 4:
			var cs := CollisionShape3D.new(); var sh := BoxShape3D.new(); sh.size = Vector3(0.4, 0.76, 0.8); cs.shape = sh; cs.position = Vector3(WH[i][0], 0.38, WH[i][1]); hit_body.add_child(cs); hit_parts.append(10 + i)

func _build_jeep() -> void:
	for p in JEEP:
		var mi := _bx(vis, Vector3(p[3], p[4], p[5]), Vector3(p[0], p[1], p[2]), Color(p[6], p[7], p[8]), p[9] == 1)
		if p[0] == 0 and abs(p[1] - 1.86) < 0.01: team_mats.append(mi.material_override)
	# شريط لون الفريق على السقف
	var tm := _bx(vis, Vector3(1.5, 0.03, 0.5), Vector3(0, 1.93, -0.15), Color(1, 0, 0)); team_mats.append(tm.material_override)
	for i in 4:
		var pv := Node3D.new(); pv.position = Vector3(WH[i][0], 0.38, WH[i][1]); vis.add_child(pv)
		var sp := Node3D.new(); pv.add_child(sp)
		var wm := _cyl(sp, 0.38, 0.34, Vector3.ZERO, Color(0.06, 0.06, 0.07))
		_bx(wm, Vector3(0.12, 0.04, 0.12), Vector3(0.26, 0.18, 0.0), Color(0.55, 0.55, 0.58))   # صامولة لرؤية الدوران
		wheels.append([pv, sp])
	# الرشاش المثبّت
	mg_yaw = Node3D.new(); mg_yaw.position = Vector3(0, 1.72, 1.6); vis.add_child(mg_yaw)
	mg_pitch = Node3D.new(); mg_yaw.add_child(mg_pitch)
	_bx(mg_pitch, Vector3(0.16, 0.16, 0.7), Vector3(0, 0, -0.15), Color(0.1, 0.1, 0.11))
	_bx(mg_pitch, Vector3(0.8, 0.45, 0.04), Vector3(0, 0.13, -0.2), Color(0.78, 0.72, 0.58))
	_bx(mg_pitch, Vector3(0.05, 0.05, 0.55), Vector3(0, -0.04, -0.75), Color(0.06, 0.06, 0.06))
	mg_muzzle = Node3D.new(); mg_muzzle.position = Vector3(0, -0.04, -1.05); mg_pitch.add_child(mg_muzzle)
	flash_m = _flash(mg_muzzle, 0.45)

func _flash(parent: Node3D, sz: float) -> MeshInstance3D:
	var q := QuadMesh.new(); q.size = Vector2(sz, sz)
	var m := StandardMaterial3D.new(); m.shading_mode = BaseMaterial3D.SHADING_MODE_UNSHADED; m.albedo_color = Color(1, 0.8, 0.4, 0.95); m.transparency = BaseMaterial3D.TRANSPARENCY_ALPHA; m.blend_mode = BaseMaterial3D.BLEND_MODE_ADD; m.cull_mode = BaseMaterial3D.CULL_DISABLED; m.billboard_mode = BaseMaterial3D.BILLBOARD_ENABLED
	var mi := MeshInstance3D.new(); mi.mesh = q; mi.material_override = m; mi.cast_shadow = GeometryInstance3D.SHADOW_CASTING_SETTING_OFF; mi.visible = false; parent.add_child(mi); return mi

func _build_tank() -> void:
	var B := Color(0.34, 0.39, 0.25); var D := Color(0.11, 0.12, 0.1); var tint := B
	_bx(vis, Vector3(2.55, 0.62, 6.0), Vector3(0, 0.78, 0), B)
	for sx in [-1, 1]:
		_bx(vis, Vector3(0.62, 0.85, 6.5), Vector3(sx * 1.45, 0.5, 0), Color(0.07, 0.07, 0.07))
		_bx(vis, Vector3(0.7, 0.06, 6.4), Vector3(sx * 1.45, 0.96, 0), D)
		_bx(vis, Vector3(0.1, 0.2, 6.4), Vector3(sx * 1.78, 0.9, 0), B * 0.8)   # رفرف
		for i in 6:
			var z := -2.5 + i * 1.0
			var sp := Node3D.new(); sp.position = Vector3(sx * 1.5, 0.4, z); vis.add_child(sp)
			var rw := _cyl(sp, 0.37, 0.42, Vector3.ZERO, Color(0.2, 0.21, 0.19)); wheels.append([sp, sp])
			_bx(rw, Vector3(0.12, 0.05, 0.12), Vector3(0.25, 0.22, 0.0), Color(0.5, 0.5, 0.48))
		for i in 16:   # حلقات جنزير متحركة على السطح العلوي
			var rb := _bx(vis, Vector3(0.74, 0.05, 0.22), Vector3(sx * 1.45, 1.0, 0), Color(0.05, 0.05, 0.05)); ribs.append([rb, i / 16.0 * 6.4])
	var gl := _bx(vis, Vector3(2.5, 0.3, 1.5), Vector3(0, 1.0, -2.55), B); gl.rotation.x = -0.5
	_bx(vis, Vector3(2.4, 0.26, 1.2), Vector3(0, 1.12, 2.7), B)
	for i in 4: _bx(vis, Vector3(0.3, 0.03, 0.9), Vector3(-0.7 + i * 0.47, 1.28, 2.7), D)
	_bx(vis, Vector3(0.55, 0.24, 0.55), Vector3(0.75, 1.2, -2.2), D); _bx(vis, Vector3(2.4, 0.4, 0.08), Vector3(0, 0.7, -3.08), D)
	_bx(vis, Vector3(0.32, 0.18, 0.06), Vector3(-0.95, 0.88, -3.1), Color(1, 0.95, 0.75), true); _bx(vis, Vector3(0.32, 0.18, 0.06), Vector3(0.95, 0.88, -3.1), Color(1, 0.95, 0.75), true)
	_bx(vis, Vector3(0.3, 0.14, 0.05), Vector3(-1.0, 0.72, 3.04), Color(0.9, 0.1, 0.1), true); _bx(vis, Vector3(0.3, 0.14, 0.05), Vector3(1.0, 0.72, 3.04), Color(0.9, 0.1, 0.1), true)
	var tm := _bx(vis, Vector3(0.7, 0.04, 0.7), Vector3(-0.7, 1.3, 2.7), Color(1, 0, 0)); team_mats.append(tm.material_override)
	# البرج
	turret = Node3D.new(); turret.position = Vector3(0, 1.3, -0.1); vis.add_child(turret)
	_bx(turret, Vector3(2.3, 0.62, 2.9), Vector3(0, 0.32, 0.25), B)
	var fr := _bx(turret, Vector3(1.7, 0.54, 0.7), Vector3(0, 0.3, -1.3), B); fr.rotation.x = -0.3
	_bx(turret, Vector3(1.8, 0.4, 0.5), Vector3(0, 0.22, 1.7), B)
	_bx(turret, Vector3(0.75, 0.12, 0.75), Vector3(0.55, 0.72, 0.6), D)
	for sx in [-1, 1]: _bx(turret, Vector3(0.08, 0.3, 1.4), Vector3(sx * 1.18, 0.35, 0.3), Color(0.45, 0.47, 0.44))
	_bx(turret, Vector3(0.8, 0.7, 0.3), Vector3(0, 0.35, -1.75), D)   # قناع المدفع
	gun_pivot = Node3D.new(); gun_pivot.position = Vector3(0, 0.35, -1.7); turret.add_child(gun_pivot)
	gun_slide = Node3D.new(); gun_pivot.add_child(gun_slide)
	_bx(gun_slide, Vector3(0.24, 0.24, 3.2), Vector3(0, 0, -1.4), D)
	_bx(gun_slide, Vector3(0.36, 0.36, 0.34), Vector3(0, 0, -3.15), Color(0.07, 0.07, 0.07))
	muzzle = Node3D.new(); muzzle.position = Vector3(0, 0, -3.5); gun_slide.add_child(muzzle)
	flash_m = _flash(muzzle, 1.6)

func _set_team_color() -> void:
	var c := Color(0.45, 0.45, 0.45) if team < 0 else (Color(0.9, 0.2, 0.2) if team == 0 else Color(0.2, 0.45, 0.95))
	for m in team_mats: (m as StandardMaterial3D).albedo_color = c

func _build_fx() -> void:
	smoke = _particles(Color(0.08, 0.08, 0.08, 0.7), 1.8, 3.0, 2.0, 14)
	fire = _particles(Color(1.0, 0.55, 0.15, 0.9), 0.7, 2.0, 1.0, 10, true)
	dust = _particles(Color(0.72, 0.62, 0.45, 0.35), 1.0, 1.0, 0.6, 12)
	smoke.position = Vector3(0, 1.4, 0); fire.position = Vector3(0, 1.0, 0.3); dust.position = Vector3(0, 0.2, 2.2 if vtype == 0 else 3.0)
	for p in [smoke, fire, dust]: p.emitting = false; add_child(p)
	if engine_stream == null: engine_stream = _make_engine()
	engine = AudioStreamPlayer3D.new(); engine.stream = engine_stream; engine.unit_size = 16.0; engine.max_distance = 110.0; engine.volume_db = -4.0; add_child(engine)

func _particles(c: Color, size: float, life: float, vel: float, n: int, additive := false) -> CPUParticles3D:
	var p := CPUParticles3D.new(); p.amount = maxi(4, int(n * [0.3, 0.6, 1.0, 1.2, 1.5][Settings.quality])); p.lifetime = life; p.direction = Vector3.UP; p.spread = 22.0
	p.initial_velocity_min = vel * 0.6; p.initial_velocity_max = vel; p.gravity = Vector3(0, 0.3, 0); p.scale_amount_min = size * 0.6; p.scale_amount_max = size * 1.3; p.local_coords = false
	var qm := QuadMesh.new(); var m := StandardMaterial3D.new(); m.shading_mode = BaseMaterial3D.SHADING_MODE_UNSHADED; m.vertex_color_use_as_albedo = true; m.billboard_mode = BaseMaterial3D.BILLBOARD_PARTICLES
	m.transparency = BaseMaterial3D.TRANSPARENCY_ALPHA; m.cull_mode = BaseMaterial3D.CULL_DISABLED
	if additive: m.blend_mode = BaseMaterial3D.BLEND_MODE_ADD
	qm.material = m; p.mesh = qm
	var g := Gradient.new(); g.set_color(0, c); g.set_color(1, Color(c.r, c.g, c.b, 0.0)); p.color_ramp = g
	var sc := Curve.new(); sc.add_point(Vector2(0, 0.4)); sc.add_point(Vector2(1, 1.6)); p.scale_amount_curve = sc
	p.cast_shadow = GeometryInstance3D.SHADOW_CASTING_SETTING_OFF; return p

static func _make_engine() -> AudioStreamWAV:
	var rate := 22050; var n := int(0.4 * rate); var b := PackedByteArray(); b.resize(n * 2)
	for i in n:
		var t := float(i) / rate
		var v := 0.0
		for k in 1:
			v += (fmod(t * 50.0, 1.0) * 2.0 - 1.0) * 0.55 + sin(t * TAU * 100.0) * 0.25 + sin(t * TAU * 25.0) * 0.3 + (fmod(t * 150.0, 1.0) * 2.0 - 1.0) * 0.12
		b.encode_s16(i * 2, int(clampf(v * 0.5, -1.0, 1.0) * 20000.0))
	var w := AudioStreamWAV.new(); w.format = AudioStreamWAV.FORMAT_16_BITS; w.mix_rate = rate; w.data = b; w.loop_mode = AudioStreamWAV.LOOP_FORWARD; w.loop_begin = 0; w.loop_end = n
	return w

# ---------------------------------------------------------------- مقاعد
func seat_for(f: Fighter) -> int:
	if dead: return -1
	if team >= 0 and team != f.team: return -1
	if driver == null: return 0
	if gunner == null: return 1
	if vtype == 0:
		for i in 3:
			if i >= riders.size() or riders[i] == null: return 2 + i
	return -1

func occupant_count() -> int:
	var n := (1 if driver != null else 0) + (1 if gunner != null else 0)
	for r in riders: if r != null: n += 1
	return n

func enter(f: Fighter, seat: int) -> void:
	if f.veh != null: return
	if seat == 0: driver = f
	elif seat == 1: gunner = f
	else:
		while riders.size() < seat - 1: riders.append(null)
		riders[seat - 2] = f
	if team < 0 or occupant_count() == 1: team = f.team; _set_team_color()
	f.veh = self; f.veh_seat = seat; f.collision_layer = 0; f.collision_mask = 0; f.velocity = Vector3.ZERO
	if f is Bot: (f as Bot).set_seated(true, vtype, seat)
	if f == game.player: game.player.on_enter_vehicle(self)
	_place(f, 0.0)

func leave(f: Fighter) -> void:
	if f.veh != self: return
	if driver == f: driver = null
	if gunner == f: gunner = null
	for i in riders.size(): if riders[i] == f: riders[i] = null
	var spots := [Vector3(-1.9, 0, -0.25), Vector3(1.9, 0, -0.25), Vector3(0, 0, 3.6), Vector3(0, 0, -3.6)] if vtype == 0 else [Vector3(2.6, 0, 0), Vector3(-2.6, 0, 0), Vector3(0, 0, 4.2), Vector3(0, 0, -4.2)]
	f.veh = null; f.veh_seat = 0; f.collision_layer = 2; f.collision_mask = 1; f.visible = f.alive
	var done := false
	for s in spots:
		f.global_position = to_global(s) + Vector3(0, 0.15, 0)
		if not f.test_move(f.global_transform, Vector3.ZERO, null, 0.001, true): done = true; break
	if not done: f.global_position = to_global(Vector3(0, 2.6, 0))
	f.velocity = Vector3.ZERO
	if f is Bot: (f as Bot).set_seated(false, vtype, 0)
	if f == game.player: game.player.on_exit_vehicle()

func switch_seat(f: Fighter) -> void:
	var to := 1 if f.veh_seat == 0 else 0
	var occ: Fighter = gunner if to == 1 else driver
	if occ != null and occ != f: return
	if driver == f: driver = null
	if gunner == f: gunner = null
	if to == 0: driver = f
	else: gunner = f
	f.veh_seat = to
	if f is Bot: (f as Bot).set_seated(true, vtype, to)

func _seat_xf(seat: int) -> Transform3D:
	if vtype == 0: return global_transform * Transform3D(Basis.IDENTITY, SEATS_JEEP[mini(seat, 4)])
	if seat == 0: return global_transform * Transform3D(Basis.IDENTITY, Vector3(0, 0.6, -0.5))
	return turret.global_transform * Transform3D(Basis.IDENTITY, Vector3(0.55, 0.35, 0.5))

func _place(f: Fighter, dt: float) -> void:
	var xf := _seat_xf(f.veh_seat)
	f.global_position = xf.origin; f.velocity = Vector3.ZERO
	if f is Bot:
		var yaw_f: float = h
		if f.veh_seat == 1: yaw_f = ty
		(f as Bot).yaw = yaw_f; f.rotation.y = yaw_f

# ---------------------------------------------------------------- فيزياء
func tires_blown() -> int:
	return (tires & 1) + ((tires >> 1) & 1) + ((tires >> 2) & 1) + ((tires >> 3) & 1)

func _drive(dt: float, thr: float, st: float, hb: bool) -> float:
	var T: bool = P["tracked"]
	var fx := -sin(h); var fz := -cos(h); var rx := cos(h); var rz := -sin(h)
	var vf := vx * fx + vz * fz; var vl := vx * rx + vz * rz
	var nb := 0 if T else tires_blown()
	var max_fwd := maxf(5.0, float(P["max_fwd"]) * (1.0 - 0.17 * nb)); var accel: float = float(P["accel"]) * (1.0 - 0.1 * nb); var brake: float = P["brake"]
	if thr > 0.0:
		if vf < -0.5: vf += brake * dt
		else: vf += accel * thr * dt * maxf(0.0, 1.0 - vf / max_fwd)
	elif thr < 0.0:
		if vf > 0.5: vf -= brake * dt
		else: vf -= accel * 0.6 * -thr * dt * maxf(0.0, 1.0 - (-vf) / float(P["max_rev"]))
	else:
		var roll := (1.5 + 0.012 * vf * vf) * dt; vf -= signf(vf) * minf(absf(vf), roll)
	if vf > max_fwd: vf = maxf(max_fwd, vf - 12.0 * dt)
	if hb: vf -= signf(vf) * minf(absf(vf), 9.0 * dt)
	var grip := 14.0 if T else (1.4 if hb else 7.0) * (1.0 - 0.12 * nb)
	vl *= exp(-grip * dt)
	var yaw := 0.0
	if T:
		steer += (st * 0.5 - steer) * minf(1.0, dt * 8.0); yaw = st * 0.85 * (1.0 - minf(0.5, absf(vf) / 24.0))
	else:
		var ms: float = float(P["max_steer"]) * (1.0 - minf(0.72, absf(vf) / 38.0)); steer += (st * ms - steer) * minf(1.0, dt * 6.0)
		yaw = vf / float(P["L"]) * tan(steer) * (1.35 if hb else 1.0)
		var pull := float(((tires >> 1 & 1) + (tires >> 3 & 1)) - ((tires & 1) + (tires >> 2 & 1)))
		yaw += pull * 0.09 * minf(1.0, absf(vf) / 10.0) * (-1.0 if vf < 0.0 else 1.0)
	var nh := h - yaw * dt; var crash := 0.0
	if absf(nh - h) > 0.00001:
		if not test_move(Transform3D(Basis(Vector3.UP, nh), global_position), Vector3.ZERO, null, 0.002, true): h = nh; rotation.y = h
		elif absf(vf) > 1.0: crash = maxf(crash, absf(vf) * 0.4)
	var nfx := -sin(h); var nfz := -cos(h); var nrx := cos(h); var nrz := -sin(h)
	var nvx := nfx * vf + nrx * vl; var nvz := nfz * vf + nrz * vl; var hit := false
	if move_and_collide(Vector3(nvx * dt, 0, 0)) != null: hit = true; nvx *= -0.2
	if move_and_collide(Vector3(0, 0, nvz * dt)) != null: hit = true; nvz *= -0.2
	if hit:
		crash = maxf(crash, absf(vf)); nvx *= 0.6; nvz *= 0.6
	vx = nvx; vz = nvz; speed = nvx * nfx + nvz * nfz
	global_position.y = 0.0
	var lx: float = game.map_w * 0.5 - float(P["hl"]); var lz: float = game.map_d * 0.5 - float(P["hl"])
	global_position.x = clampf(global_position.x, -lx, lx); global_position.z = clampf(global_position.z, -lz, lz)
	return crash

# ---------------------------------------------------------------- الحلقة الرئيسية
func _physics_process(dt: float) -> void:
	if game == null or game.state == "menu": return
	if dead:
		respawn_t -= dt
		smoke.emitting = respawn_t > 1.0; fire.emitting = respawn_t > 12.0 and Settings.quality >= 1; engine.stop()
		if respawn_t <= 0.0: _respawn()
		return
	var aimer: Fighter = _aimer()
	# ---- مدخلات ----
	in_thr = 0.0; in_st = 0.0; in_hb = false; in_fire = false
	if driver != null:
		if driver == game.player and game.player.veh == self:
			var inp: Dictionary = game.inp
			var mv: Vector2 = inp["mv"]
			in_thr = clampf(-mv.y, -1.0, 1.0); in_st = clampf(mv.x, -1.0, 1.0); in_hb = bool(inp["jump"])
			if vtype == 1 and aimer == driver: in_fire = bool(inp["fire"])
		else: _ai_drive(dt)
	else: in_hb = absf(speed) < 2.0
	if vtype == 0 and gunner != null and gunner == game.player: in_hb = true
	# ---- تصويب البرج/الرشاش ----
	_aim(dt, aimer)
	# ---- فيزياء ----
	var crash := _drive(dt, in_thr, in_st, in_hb)
	if crash > float(P["crash_min"]) and crash_t <= 0.0:
		crash_t = 0.45; take_damage((crash - float(P["crash_min"])) * 9.0, null)
		if driver == game.player: game.hud.flash_damage()
	crash_t = maxf(0.0, crash_t - dt)
	if dead: return
	if burn:
		hp -= 6.0 * dt
		if hp <= 0.0: explode(driver if driver != null else null); return
	# ---- نيران ----
	cannon_t = maxf(0.0, cannon_t - dt); kick = maxf(0.0, kick - dt * 3.0); mg_cd = maxf(0.0, mg_cd - dt)
	if vtype == 1 and in_fire and cannon_t <= 0.0: _cannon(aimer)
	if vtype == 0 and gunner != null and _wants_mg(gunner): _mg(gunner)
	# ---- ركاب: تحديث مواضعهم + سحق ----
	for f in _occupants(): _place(f, dt)
	_crush()
	_animate(dt)
	smoke.emitting = burn and Settings.quality >= 1; fire.emitting = burn and Settings.quality >= 1
	dust.emitting = absf(speed) > 3.5 and Settings.quality >= 1
	_engine(dt)

func _occupants() -> Array:
	var a: Array = []
	if driver != null: a.append(driver)
	if gunner != null: a.append(gunner)
	for r in riders: if r != null: a.append(r)
	return a

func _aimer() -> Fighter:
	if vtype == 0: return gunner
	return gunner if gunner != null else driver

func _wants_mg(f: Fighter) -> bool:
	if f == game.player: return bool(game.inp["fire"])
	if f is Bot: return ai_burst > 0.0 and (f as Bot).target != null
	return false

func _animate(dt: float) -> void:
	wheel_spin += speed * dt / 0.38
	for i in wheels.size():
		var w: Array = wheels[i]; var sp: Node3D = w[1]
		sp.rotation.x = -wheel_spin
		if vtype == 0:
			var bl := ((tires >> i) & 1) == 1
			(w[0] as Node3D).rotation.y = -steer if i < 2 else 0.0
			(w[0] as Node3D).position.y = 0.30 if bl else 0.38
			sp.scale = Vector3(1, 0.72, 0.72) if bl else Vector3.ONE
	rib_phase = fposmod(rib_phase + speed * dt, 6.4)
	for r in ribs:
		var rb: MeshInstance3D = r[0]; rb.position.z = fposmod(float(r[1]) + rib_phase + 3.2, 6.4) - 3.2
	if vtype == 1:
		turret.rotation.y = ty - h
		gun_pivot.rotation.x = tp
		gun_slide.position.z = kick * 0.45
		flash_m.visible = kick > 0.82
	else:
		mg_yaw.rotation.y = ty - h; mg_pitch.rotation.x = tp
		flash_m.visible = mg_cd > 0.07 and gunner != null

func _engine(dt: float) -> void:
	if game.selftest or driver == null:
		if engine.playing: engine.stop()
		return
	if not engine.playing: engine.play()
	var k := clampf(absf(speed) / float(P["max_fwd"]), 0.0, 1.0)
	engine.pitch_scale = lerpf(engine.pitch_scale, 0.7 + k * 1.4 + absf(in_thr) * 0.15, minf(1.0, dt * 4.0))
	engine.volume_db = lerpf(-8.0, -1.0, k) + (2.0 if driver == game.player else 0.0)

# سحق المشاة أثناء السرعة + دفعهم خارج الجسم
func _crush() -> void:
	if absf(speed) < 5.0: return
	for f in game.fighters:
		if not is_instance_valid(f) or not f.alive or f.team == team or f.veh != null: continue
		var l := to_local(f.global_position)
		if absf(l.x) < float(P["hw"]) + 0.45 and absf(l.z) < float(P["hl"]) + 0.45 and l.y < 1.8 and l.y > -0.5 and f.spawn_prot <= 0.0:
			f.take_damage(absf(speed) * (24.0 if vtype == 1 else 14.0), driver, false)

# ---------------------------------------------------------------- تصويب وسلاح
func _aim(dt: float, aimer: Fighter) -> void:
	if aimer == null:
		if vtype == 1: want_ty = h; want_tp = 0.0
		else: want_ty = h; want_tp = 0.0
	elif aimer == game.player and game.player.veh == self:
		var pl: Player = game.player
		if vtype == 1:
			var o := pl.cam.global_position; var d := -pl.cam.global_transform.basis.z
			var q := PhysicsRayQueryParameters3D.create(o, o + d * 160.0, 7, [pl.get_rid(), hit_body.get_rid(), get_rid()])
			var r := get_world_3d().direct_space_state.intersect_ray(q)
			var pt: Vector3 = r["position"] if not r.is_empty() else o + d * 160.0
			var pv := gun_pivot.global_position; var dd := pt - pv
			want_ty = atan2(-dd.x, -dd.z); want_tp = clampf(asin(clampf(dd.y / maxf(dd.length(), 0.01), -1.0, 1.0)), -0.35, 0.5)
		else:
			want_ty = pl.yaw; want_tp = clampf(pl.pitch, -0.5, 0.9)
	elif aimer is Bot:
		_ai_aim(dt, aimer as Bot)
	var rate_y := 1.8 if vtype == 1 else 7.0; var rate_p := 0.9 if vtype == 1 else 4.0
	ty += clampf(wrapf(want_ty - ty, -PI, PI), -rate_y * dt, rate_y * dt)
	tp += clampf(want_tp - tp, -rate_p * dt, rate_p * dt)

func _muzzle_pos() -> Vector3:
	return muzzle.global_position if vtype == 1 else mg_muzzle.global_position

func _muzzle_dir() -> Vector3:
	var n: Node3D = muzzle if vtype == 1 else mg_muzzle
	return -n.global_transform.basis.z

func _cannon(shooter: Fighter) -> void:
	cannon_t = 2.5; kick = 1.0
	var o := _muzzle_pos(); var d := _muzzle_dir()
	var q := PhysicsRayQueryParameters3D.create(o, o + d * 160.0, 7, [hit_body.get_rid(), get_rid()] + _occ_rids())
	var r := get_world_3d().direct_space_state.intersect_ray(q)
	var ep: Vector3 = r["position"] if not r.is_empty() else o + d * 160.0
	game.sfx_play("boom", o); game._tracer(o, ep); game.cannon_hit(shooter, ep, self)
	game.fx_blast(o, 0.35)

func _occ_rids() -> Array:
	var a: Array = []
	for f in _occupants(): a.append(f.get_rid())
	return a

func _mg(shooter: Fighter) -> void:
	if mg_cd > 0.0: return
	mg_cd = 0.1
	var o := _muzzle_pos(); var d := _muzzle_dir()
	var sp := 0.012
	d = (d + Vector3(randf_range(-sp, sp), randf_range(-sp, sp), randf_range(-sp, sp))).normalized()
	game.hitscan(shooter, o, d, MG, o)
	game.sfx_play("shot", o)

# ---------------------------------------------------------------- أضرار
func take_damage(amt: float, src: Fighter) -> void:
	if dead or amt <= 0.0: return
	hp -= amt
	if hp <= 0.0: explode(src)
	elif hp < 150.0: burn = true

func take_hit(dmg: float, shape: int, src: Fighter) -> void:
	if dead: return
	var part: int = hit_parts[shape] if shape >= 0 and shape < hit_parts.size() else 0
	if part >= 10:
		var i := part - 10
		if (tires >> i) & 1: return
		tire_hp[i] -= dmg
		if tire_hp[i] <= 0.0: tires |= 1 << i; game.sfx_play("reload", global_position)
		return
	var k := 1.3 if part == 1 else 0.35
	take_damage(dmg * k * float(P["armor"]), src)
	if src == game.player: game.hud.hitmark()

func explode(src: Fighter) -> void:
	if dead: return
	dead = true; burn = false; respawn_t = 30.0; hp = 0.0
	for f in _occupants(): leave(f)
	driver = null; gunner = null; riders.clear()
	game.fx_blast(global_position + Vector3(0, 1.0, 0), 1.0)
	for f in game.fighters:
		if not is_instance_valid(f) or not f.alive: continue
		var d: float = f.global_position.distance_to(global_position)
		if d < 7.0: f.take_damage(260.0 * pow(1.0 - d / 7.0, 1.1), src, false)
	if src == game.player: game.hud.hitmark(); game.hud.say(Settings.t("دُمِّرت المركبة!"))
	for m in mats: (m[0] as StandardMaterial3D).albedo_color = (m[1] as Color) * 0.14; (m[0] as StandardMaterial3D).emission_enabled = false
	vx = 0.0; vz = 0.0; speed = 0.0; kick = 0.0; flash_m.visible = false
	for p in [dust]: p.emitting = false

func _respawn() -> void:
	dead = false; burn = false; hp = P["hp"]; tires = 0; tire_hp = [30.0, 30.0, 30.0, 30.0]; team = -1; _set_team_color()
	global_position = home; h = home_h; rotation.y = h; ty = h; tp = 0.0; vx = 0.0; vz = 0.0; speed = 0.0
	for m in mats: (m[0] as StandardMaterial3D).albedo_color = m[1]; if (m[1] as Color).get_luminance() > 0.85: (m[0] as StandardMaterial3D).emission_enabled = true
	smoke.emitting = false; fire.emitting = false

# ---------------------------------------------------------------- ذكاء
func _probe(o: Vector3, hd: float, ln: float) -> bool:
	var d := Vector3(-sin(hd), 0, -cos(hd))
	var q := PhysicsRayQueryParameters3D.create(o, o + d * ln, 1, [get_rid()])
	return get_world_3d().direct_space_state.intersect_ray(q).is_empty()

func _enemy_pos() -> Vector3:
	var arr: Array = game.maps[game.map_idx]["blue" if team == 0 else "red"]
	var s := Vector2.ZERO
	for p in arr: s += Vector2(p[0], p[1])
	s /= float(arr.size()); return Vector3(s.x, 0, s.y)

func _ai_find_target(pos: Vector3, rng: float, need_los: bool) -> Node3D:
	var best: Node3D = null; var bd := rng
	for f in game.fighters:
		if not is_instance_valid(f) or not f.alive or f.team == team or f.veh != null and f.veh == self: continue
		var d := pos.distance_to(f.global_position)
		if d < bd and (not need_los or _vis_to(pos, f.global_position + Vector3(0, 1.1, 0))): best = f; bd = d
	for v in game.vehicles:
		if v == self or v.dead or v.team < 0 or v.team == team or v.occupant_count() == 0: continue
		var d: float = pos.distance_to(v.global_position)
		if d < bd * 1.25 and (not need_los or _vis_to(pos, v.global_position + Vector3(0, 1.2, 0))): best = v; bd = d / 1.25
	return best

func _vis_to(a: Vector3, b: Vector3) -> bool:
	var q := PhysicsRayQueryParameters3D.create(a, b, 1, [get_rid()])
	return get_world_3d().direct_space_state.intersect_ray(q).is_empty()

func _ai_drive(dt: float) -> void:
	ai_t -= dt
	var pos := global_position
	if ai_t <= 0.0:
		ai_t = randf_range(0.5, 0.9)
		ai_target = _ai_find_target(pos + Vector3(0, 1.5, 0), 130.0, false)
		ai_goal = ai_target.global_position if ai_target != null else _enemy_pos()
	if ai_target != null and is_instance_valid(ai_target): ai_goal = ai_target.global_position
	var to := ai_goal - pos; to.y = 0.0
	var dist := to.length()
	var desired := atan2(-to.x, -to.z)
	var dh := wrapf(desired - h, -PI, PI)
	var o := pos + Vector3(0, float(P["min_h"]) + 0.25, 0)
	var ln := 8.0 if vtype == 0 else 6.5
	var fc := _probe(o, h, ln); var lc := _probe(o, h + 0.55, ln * 0.8); var rc := _probe(o, h - 0.55, ln * 0.8)
	var st := clampf(-dh * 1.6, -1.0, 1.0); var thr := 1.0 if absf(dh) < 0.6 else 0.45
	if not lc and rc: st = maxf(st, 0.8)
	elif not rc and lc: st = minf(st, -0.8)
	elif not lc and not rc: thr = -0.7; st = ai_rev_dir
	if not fc and lc and rc: st = 0.8 * ai_rev_dir; thr = 0.4
	if vtype == 1 and ai_target != null and dist < 45.0 and _vis_to(pos + Vector3(0, 1.8, 0), ai_target.global_position + Vector3(0, 1.1, 0)): thr = 0.0   # الدبابة تثبّت وتقصف
	if vtype == 0 and ai_target != null and dist < 9.0: thr = 0.3
	if dist < 4.0: thr = 0.0
	# تعثّر → رجوع
	if ai_rev > 0.0:
		ai_rev -= dt; thr = -1.0; st = -ai_rev_dir
	elif absf(speed) < 0.9 and thr > 0.1:
		ai_stuck += dt
		if ai_stuck > 1.1: ai_rev = 1.3; ai_stuck = 0.0; ai_rev_dir = -1.0 if randf() > 0.5 else 1.0
	else: ai_stuck = maxf(0.0, ai_stuck - dt)
	in_thr = thr; in_st = st; in_hb = thr == 0.0 and absf(speed) < 3.0

func _ai_aim(dt: float, b: Bot) -> void:
	var o := _muzzle_pos()
	var rng := 140.0 if vtype == 1 else 85.0
	ai_burst -= dt
	var t: Node3D = ai_target if (vtype == 1 and ai_target != null and is_instance_valid(ai_target)) else null
	if t == null or b != driver:
		t = _ai_find_target(o, rng, true) if int(Time.get_ticks_msec() / 400) % 2 == 0 or ai_target == null else ai_target
		ai_target = t
	if t == null or not is_instance_valid(t):
		want_ty = h; want_tp = 0.0; ai_burst = 0.0; return
	var tp_pos := t.global_position + Vector3(0, 1.1, 0)
	if t is Vehicle: tp_pos = t.global_position + Vector3(0, 1.0, 0)
	var dd := tp_pos - o
	want_ty = atan2(-dd.x, -dd.z); want_tp = clampf(asin(clampf(dd.y / maxf(dd.length(), 0.01), -1.0, 1.0)), -0.35 if vtype == 1 else -0.5, 0.5 if vtype == 1 else 0.9)
	var aligned := absf(wrapf(want_ty - ty, -PI, PI)) < 0.05 and absf(want_tp - tp) < 0.06
	if vtype == 1:
		if aligned and cannon_t <= 0.0 and dd.length() < rng and _vis_to(o, tp_pos): in_fire = true
	else:
		if ai_burst <= -ai_rest and aligned: ai_burst = randf_range(0.6, 1.4); ai_rest = randf_range(0.3, 0.9)
		if not aligned: ai_burst = minf(ai_burst, 0.0)
	b.target = t as Fighter if t is Fighter else b.target

