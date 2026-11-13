class_name Player
extends Fighter
# اللاعب: حركة (مشي/جري/قفز/انحناء) + كاميرا أول شخص + مجسّم سلاح + تصويب

var cam: Camera3D
var vm: Node3D
var pitch := 0.0
var yaw := 0.0
var ads := false
var bob := 0.0
var kick := 0.0
var vy := 0.0
var sprinting := false
var gun_parts: Node3D
var sway := Vector2.ZERO
var draw_t := 0.0
var flash: Node3D
var chase_idle := 0.0
var mag: MeshInstance3D
var bolt: MeshInstance3D
var lh: Node3D            # اليد اليسرى (تتحرك أثناء التعبئة)
var rh: Node3D            # اليد اليمنى على القبضة
var arm_r: MeshInstance3D
var arm_l: MeshInstance3D
var muzzle: Node3D
var mag_home := Vector3.ZERO
var mag_c := Vector3.ZERO    # مركز المخزن (نقطة إمساك اليد)
var lh_home := Vector3.ZERO
var rh_home := Vector3.ZERO
var bolt_pt := Vector3.ZERO  # مقبض/ترباس الشحن
var ej_pt := Vector3.ZERO    # فتحة طرد الظرف
var sight_y := 0.09
var ads_adj := 0.0
var fire_hold := 0.0
var dbg_rp := -1.0           # للّقطات: تجميد تقدّم التعبئة
const SHOULDER_R := Vector3(0.52, -0.95, 0.5)
const SHOULDER_L := Vector3(-0.55, -0.9, 0.45)

func _ready() -> void:
	collision_layer = 2; collision_mask = 1
	var cs := CollisionShape3D.new(); var sh := CapsuleShape3D.new(); sh.radius = 0.35; sh.height = 1.75; cs.shape = sh; cs.position = Vector3(0, 0.875, 0); add_child(cs)
	floor_snap_length = 0.4
	cam = Camera3D.new(); cam.position = Vector3(0, EYE, 0); cam.fov = 75.0; cam.near = 0.03; cam.far = 400.0; add_child(cam)
	vm = Node3D.new(); cam.add_child(vm)
	rebuild_viewmodel()

func _inst(parent: Node3D, mesh: Mesh, pos := Vector3.ZERO) -> MeshInstance3D:
	var mi := MeshInstance3D.new(); mi.mesh = mesh; mi.position = pos; mi.cast_shadow = GeometryInstance3D.SHADOW_CASTING_SETTING_OFF; parent.add_child(mi); return mi

func _mat(c: Color, rough := 0.5, metal := 0.4) -> StandardMaterial3D:
	var m := StandardMaterial3D.new(); m.albedo_color = c; m.roughness = rough; m.metallic = metal; return m

func _part(sz: Vector3, pos: Vector3, c: Color) -> MeshInstance3D:
	var bm := BoxMesh.new(); bm.size = sz
	var mi := _inst(gun_parts, bm, pos); mi.material_override = _mat(c); return mi

func _hand(parent: Node3D, pos: Vector3, left: bool) -> Node3D:
	var h := Node3D.new(); h.position = pos; parent.add_child(h)
	var glove := Color(0.24, 0.25, 0.23)
	var palm := BoxMesh.new(); palm.size = Vector3(0.085, 0.06, 0.11); var p := _inst(h, palm); p.material_override = _mat(glove, 0.9, 0.0)
	var fing := BoxMesh.new(); fing.size = Vector3(0.08, 0.035, 0.07); var f := _inst(h, fing, Vector3(0, 0.03, -0.075 if left else 0.0)); f.material_override = _mat(glove, 0.9, 0.0)
	var thumb := BoxMesh.new(); thumb.size = Vector3(0.03, 0.03, 0.07); var t := _inst(h, thumb, Vector3(0.05 if left else -0.05, 0.01, -0.02)); t.material_override = _mat(glove, 0.9, 0.0)
	return h

func _arm() -> MeshInstance3D:
	var bm := BoxMesh.new(); bm.size = Vector3(1, 1, 1)
	var mi := _inst(vm, bm); mi.material_override = _mat(Color(0.2, 0.26, 0.17), 0.95, 0.0); return mi

func rebuild_viewmodel() -> void:
	if gun_parts != null: gun_parts.queue_free()
	if arm_r != null: arm_r.queue_free()
	if arm_l != null: arm_l.queue_free()
	gun_parts = Node3D.new(); vm.add_child(gun_parts)
	var w := weapon()
	var kind: String = w["kind"]
	mag = null; bolt = null
	var mdl := GunModel.get_model(wi) if GunModel.has(wi) else {}
	var mz := -0.62; var rear := 0.0; var gh := 0.09
	if not mdl.is_empty():
		# ---- موديل حقيقي: أمامي + مخزن + ترباس + كعب ----
		var ms: Array = mdl["meshes"]; var lo: Vector3 = mdl["lo"]; var hi: Vector3 = mdl["hi"]
		mz = lo.z; rear = hi.z; gh = hi.y
		if ms[0] != null: _inst(gun_parts, ms[0])
		if ms[3] != null: _inst(gun_parts, ms[3])
		if ms[1] != null: mag = _inst(gun_parts, ms[1])
		if ms[2] != null: bolt = _inst(gun_parts, ms[2])
		mag_home = Vector3.ZERO; mag_c = mdl["mag_c"] if ms[1] != null else Vector3(0, -0.08, (mz + rear) * 0.4)
		rh_home = Vector3(0, mag_c.y + 0.09, mag_c.z + 0.13) if ms[1] != null else Vector3(0, -0.07, rear * 0.2)
		lh_home = Vector3(0, -0.02, mz * 0.55)
		bolt_pt = Vector3(0.03, gh * 0.8, lerpf(mz, rear, 0.62)) if ms[2] == null else Vector3(0.04, (mdl["bolt_c"] as Vector3).y, (mdl["bolt_c"] as Vector3).z)
		ej_pt = Vector3(0.04, gh * 0.6, lerpf(mz, rear, 0.62))
		sight_y = gh * 0.95; ads_adj = 0.045
	else:
		# ---- مجسّم يدوي لبقية الأسلحة ----
		var L := 0.62; var H := 0.09
		match kind:
			"pistol", "magnum": L = 0.3; H = 0.11
			"smg": L = 0.5
			"shotgun": L = 0.85
			"lmg": L = 0.85; H = 0.12
		var dark := Color(0.12, 0.12, 0.13)
		_part(Vector3(0.06, H, L), Vector3(0, 0, -L * 0.5), dark)
		_part(Vector3(0.025, 0.025, L * 0.7), Vector3(0, H * 0.4, -L * 0.85), Color(0.2, 0.2, 0.22))
		_part(Vector3(0.05, 0.13, 0.06), Vector3(0, -0.11, -L * 0.2), Color(0.18, 0.18, 0.2))
		if kind in ["lmg", "smg"]: _part(Vector3(0.045, 0.12, 0.07), Vector3(0, -0.11, -L * 0.5), Color(0.22, 0.2, 0.15))
		if kind == "shotgun": _part(Vector3(0.05, 0.05, 0.3), Vector3(0, -0.04, -L * 0.7), Color(0.25, 0.18, 0.1))
		mz = -L * 1.25; rear = 0.0; gh = H
		if kind != "launcher" and kind != "shotgun":
			var mh := 0.2 if kind in ["lmg", "smg"] else 0.15
			mag_home = Vector3(0, -0.13 - mh * 0.3, -L * 0.52 - (0.08 if kind in ["pistol", "magnum"] else 0.0))
			mag = _part(Vector3(0.045, mh, 0.07), mag_home, Color(0.1, 0.1, 0.11)); mag_c = mag_home
		rh_home = Vector3(0, -0.15, -L * 0.2); lh_home = Vector3(-0.01, -0.06, -L * 0.72)
		bolt_pt = Vector3(0.03, H * 0.6, -L * 0.28); ej_pt = Vector3(0.04, H * 0.5, -L * 0.35)
		sight_y = H * 0.8; ads_adj = 0.0
	muzzle = Node3D.new(); muzzle.position = Vector3(0, 0.0, mz); gun_parts.add_child(muzzle)
	# وميض الفوهة: مربّعان متقاطعان + كرة
	flash = Node3D.new(); flash.position = Vector3(0, 0, mz - 0.03); gun_parts.add_child(flash)
	var fm := StandardMaterial3D.new(); fm.shading_mode = BaseMaterial3D.SHADING_MODE_UNSHADED; fm.albedo_color = Color(1, 0.85, 0.45, 0.9); fm.transparency = BaseMaterial3D.TRANSPARENCY_ALPHA; fm.blend_mode = BaseMaterial3D.BLEND_MODE_ADD; fm.cull_mode = BaseMaterial3D.CULL_DISABLED
	for r in [0.0, PI * 0.5]:
		var q := QuadMesh.new(); q.size = Vector2(0.2, 0.2); var qi := _inst(flash, q); qi.material_override = fm; qi.rotation.z = r
	var sp := SphereMesh.new(); sp.radius = 0.05; sp.height = 0.1; var si := _inst(flash, sp); si.material_override = fm
	flash.visible = false
	rh = _hand(gun_parts, rh_home, false); lh = _hand(gun_parts, lh_home, true)
	arm_r = _arm(); arm_l = _arm()
	draw_t = 1.0
	gun_parts.position = Vector3(0.2, -0.22, -0.45); gun_parts.scale = Vector3.ONE * 0.62

func _arm_to(arm: MeshInstance3D, shoulder: Vector3, hand: Node3D) -> void:
	var hp := vm.to_local(hand.global_position); var d := hp - shoulder; var ln := d.length()
	if ln < 0.01: return
	arm.transform = Transform3D(Basis.looking_at(d / ln, Vector3.UP) * Basis.from_scale(Vector3(0.055, 0.055, ln)), shoulder + d * 0.5)

func _ease(x: float) -> float:
	x = clampf(x, 0.0, 1.0); return x * x * (3.0 - 2.0 * x)

# حركة التعبئة (rp 0..1): اليد اليسرى تذهب للمخزن، تسحبه، تجلب جديداً، تُدخله، تشدّ الترباس، وتعود
# يعيد {lh: موضع اليد، magoff: إزاحة المخزن، magvis: ظاهر؟، bolt: 0..1}
func reload_pose(rp: float) -> Dictionary:
	var H := lh_home; var out := {"lh": H, "magoff": Vector3.ZERO, "magvis": true, "bolt": 0.0}
	if mag != null:
		var Mg := mag_c + Vector3(0.0, -0.03, 0.0); var Dn := Mg + Vector3(-0.05, -0.24, 0.12); var Bh := bolt_pt; var Bb := Bh + Vector3(0, 0, 0.09)
		if rp < 0.12: out["lh"] = H.lerp(Mg, _ease(rp / 0.12))
		elif rp < 0.30: var k := _ease((rp - 0.12) / 0.18); out["lh"] = Mg.lerp(Dn, k); out["magoff"] = (Dn - Mg) * k
		elif rp < 0.45: out["lh"] = Dn; out["magvis"] = false
		elif rp < 0.65: var k := _ease((rp - 0.45) / 0.20); out["lh"] = Dn.lerp(Mg, k); out["magoff"] = (Dn - Mg) * (1.0 - k)
		elif rp < 0.75: out["lh"] = Mg; out["magoff"] = Vector3(0, 0.012 * sin((rp - 0.65) / 0.1 * PI), 0)
		elif rp < 0.82: out["lh"] = Mg.lerp(Bh, _ease((rp - 0.75) / 0.07))
		elif rp < 0.89: var k := _ease((rp - 0.82) / 0.07); out["lh"] = Bh.lerp(Bb, k); out["bolt"] = k
		elif rp < 0.94: var k := _ease((rp - 0.89) / 0.05); out["lh"] = Bb.lerp(Bh, k); out["bolt"] = 1.0 - k
		else: out["lh"] = Bh.lerp(H, _ease((rp - 0.94) / 0.06))
	else:  # شوتغن/قاذف: ضخّ بيد أمامية
		var Pb := H + Vector3(0, 0.01, 0.17)
		var ph := fmod(rp * 3.0, 1.0)
		out["lh"] = H.lerp(Pb, sin(ph * PI)); out["bolt"] = sin(ph * PI)
	return out

func _physics_process(dt: float) -> void:
	super._physics_process(dt)
	if not alive:
		cam.position.y = lerpf(cam.position.y, 0.3, dt * 4.0); return
	var inp: Dictionary = game.inp
	if inp["use"]: inp["use"] = false; game.try_use(self)
	if inp["seat"]: inp["seat"] = false; if veh != null: (veh as Vehicle).switch_seat(self)
	if veh != null: _drive_mode(dt); return
	# ---- نظر ----
	var look: Vector2 = inp["look"]; inp["look"] = Vector2.ZERO
	var sens := 0.0042 * (0.5 if ads and float(weapon()["zoom"]) >= 2.0 else 1.0)
	yaw -= look.x * sens; pitch = clampf(pitch - look.y * sens, -1.5, 1.5)
	rotation.y = yaw; cam.rotation.x = pitch + kick * 0.02
	# ---- حركة ----
	var mv: Vector2 = inp["mv"]
	ads = bool(inp["ads"])
	sprinting = bool(inp["sprint"]) and mv.y < -0.3 and not ads
	var sp := 7.0 if sprinting else 4.6
	if ads: sp *= 0.6
	var dirv := (global_transform.basis * Vector3(mv.x, 0, mv.y))
	dirv.y = 0.0
	if dirv.length() > 1.0: dirv = dirv.normalized()
	velocity.x = dirv.x * sp; velocity.z = dirv.z * sp
	if is_on_floor():
		velocity.y = -1.0
		if inp["jump"]: velocity.y = 6.2
	else: velocity.y -= 18.0 * dt
	inp["jump"] = false
	_step_up2(Vector3(velocity.x, 0, velocity.z) * dt)
	move_and_slide()
	var lim_x: float = game.map_w * 0.5 - 0.4; var lim_z: float = game.map_d * 0.5 - 0.4
	position.x = clampf(position.x, -lim_x, lim_x); position.z = clampf(position.z, -lim_z, lim_z)
	var spd := Vector2(velocity.x, velocity.z).length()
	bob += spd * dt * 1.6
	# ---- سلاح ----
	kick = maxf(0.0, kick - dt * 8.0); fire_hold = maxf(0.0, fire_hold - dt)
	sway = sway.lerp(Vector2.ZERO, minf(1.0, dt * 8.0)) + look * 0.0005
	sway = sway.clamp(Vector2(-0.04, -0.04), Vector2(0.04, 0.04))
	draw_t = maxf(0.0, draw_t - dt * 3.5)
	var rp := 0.0
	if reload_t > 0.0:
		var tot := maxf(0.1, float(weapon()["reload"])); rp = clampf(1.0 - reload_t / tot, 0.0, 1.0)
	if dbg_rp >= 0.0: rp = dbg_rp
	var rl := clampf(rp / 0.12, 0.0, 1.0) * clampf((1.0 - rp) / 0.1, 0.0, 1.0) if rp > 0.0 else 0.0
	var rpose := reload_pose(rp) if rp > 0.0 else {"lh": lh_home, "magoff": Vector3.ZERO, "magvis": true, "bolt": 0.0}
	if mag != null: mag.position = mag_home + (rpose["magoff"] as Vector3); mag.visible = bool(rpose["magvis"])
	if bolt != null: bolt.position = Vector3(0, 0, 0.07 * float(rpose["bolt"]))
	lh.position = lh.position.lerp(rpose["lh"], minf(1.0, dt * (30.0 if rp > 0.0 else 12.0)))
	rh.position = rh_home
	if flash: flash.visible = kick > 0.8
	var spr_pose := sprinting and fire_hold <= 0.0 and rp <= 0.0
	var tgt_pos := Vector3(0.0, -sight_y * 0.62 - 0.035 + ads_adj, -0.42) if ads else Vector3(0.2, -0.22, -0.45)
	if spr_pose: tgt_pos = Vector3(0.14, -0.3, -0.38)
	tgt_pos += Vector3(-sway.x, sway.y, 0) * (0.3 if ads else 1.0) + Vector3(-0.03 * rl, -0.35 * draw_t + 0.04 * rl, 0.05 * float(rpose["bolt"]) + 0.04 * rl)
	var bobv := Vector3(sin(bob) * 0.006, absf(cos(bob)) * 0.008, 0) * (1.0 if is_on_floor() else 0.0) * clampf(Vector2(velocity.x, velocity.z).length() / 4.6, 0, 1.4)
	if spr_pose: bobv *= 2.2
	gun_parts.position = gun_parts.position.lerp(tgt_pos + bobv + Vector3(0, 0, kick * 0.04), minf(1.0, dt * 14.0))
	gun_parts.rotation.z = lerpf(gun_parts.rotation.z, rl * 0.35 + (0.15 if spr_pose else 0.0), minf(1.0, dt * 10.0))
	gun_parts.rotation.x = lerpf(gun_parts.rotation.x, (-0.5 if spr_pose else 0.0) + kick * 0.06 + rl * 0.12 + draw_t * 0.6, minf(1.0, dt * 10.0))
	gun_parts.rotation.y = lerpf(gun_parts.rotation.y, (0.45 if spr_pose else 0.0), minf(1.0, dt * 10.0))
	_arm_to(arm_r, SHOULDER_R, rh); _arm_to(arm_l, SHOULDER_L, lh)
	var z := float(weapon()["zoom"])
	var tfov := 75.0 / (z if ads and z > 1.0 else 1.0)
	cam.fov = lerpf(cam.fov, tfov, minf(1.0, dt * 14.0))
	var scoped := ads and z >= 3.4
	gun_parts.visible = not scoped; arm_r.visible = not ads; arm_l.visible = not ads
	if inp["reload"]: start_reload(); inp["reload"] = false
	if inp["next"]: game.switch_weapon(self, 1); inp["next"] = false
	if inp["fire"]:  # الإطلاق مسموح أثناء الجري
		var w := weapon()
		if try_fire(cam.global_position, -cam.global_transform.basis.z, ads, spd * 0.0004):
			kick = 1.0; fire_hold = 0.35; pitch += float(w["recoilV"]) * (1.0 if ads else 1.3) * 1.5; yaw += randf_range(-1, 1) * float(w["recoilH"]) * 1.5
			game.hud.fire_flash(); game.eject_shell(gun_parts.to_global(ej_pt), gun_parts.global_transform.basis)
		if not bool(w["auto"]): inp["fire"] = false

func on_enter_vehicle(v: Vehicle) -> void:
	ads = false; sprinting = false; kick = 0.0
	gun_parts.visible = false; arm_r.visible = false; arm_l.visible = false
	yaw = v.h; pitch = -0.2; chase_idle = 0.0; game.inp["fire"] = false

func on_exit_vehicle() -> void:
	cam.position = Vector3(0, EYE, 0); cam.rotation = Vector3(pitch, 0, 0); rotation.y = yaw; cam.fov = 75.0
	gun_parts.visible = true; arm_r.visible = true; arm_l.visible = true; pitch = 0.0; velocity = Vector3.ZERO
	game.inp["fire"] = false

# قيادة/رماية من مركبة: كاميرا خلفية (أو من فوق الرشاش) وتحكّم بالنظر فقط؛ الحركة والإطلاق تتولاها المركبة
func _drive_mode(dt: float) -> void:
	var inp: Dictionary = game.inp
	var v: Vehicle = veh as Vehicle
	var look: Vector2 = inp["look"]; inp["look"] = Vector2.ZERO
	var sens := 0.0042
	yaw -= look.x * sens; pitch -= look.y * sens
	var fp := v.vtype == 0 and veh_seat == 1
	chase_idle = 0.0 if look.length() > 0.5 else chase_idle + dt
	if v.vtype == 0 and veh_seat == 0 and chase_idle > 1.2:
		yaw = lerp_angle(yaw, v.h, minf(1.0, dt * 1.6)); pitch = lerpf(pitch, -0.2, minf(1.0, dt * 1.6))
	pitch = clampf(pitch, -0.5, 0.9) if fp else clampf(pitch, -1.0, 0.55)
	var b := Basis.from_euler(Vector3(pitch, yaw, 0))
	var pos: Vector3
	if fp:
		pos = v.mg_yaw.global_position + Vector3(0, 0.55, 0) + b * Vector3(0, 0, 1.1)
	else:
		var pivot: Vector3 = v.global_position + Vector3(0, 2.1 if v.vtype == 0 else 2.8, 0)
		var want: Vector3 = pivot + b * Vector3(0, 0, 7.5 if v.vtype == 0 else 10.5)
		var q := PhysicsRayQueryParameters3D.create(pivot, want, 1, [v.get_rid()])
		var r := get_world_3d().direct_space_state.intersect_ray(q)
		pos = want if r.is_empty() else (r["position"] as Vector3) + (pivot - want).normalized() * 0.4
	cam.global_transform = Transform3D(b, pos)
	cam.fov = lerpf(cam.fov, 68.0 if v.vtype == 1 and not fp else 75.0, minf(1.0, dt * 6.0))
	rotation.y = yaw

# موضع الفوهة في العالم (بداية الرصاصة المرئية)
func muzzle_pos() -> Vector3:
	return muzzle.global_position if muzzle != null and is_instance_valid(muzzle) else cam.global_position

func _step_up2(h: Vector3) -> void:
	if h.length() < 0.001 or not is_on_floor(): return
	if test_move(global_transform, h):
		var up := global_transform.translated(Vector3(0, 0.55, 0))
		if not test_move(up, h) and not test_move(global_transform, Vector3(0, 0.55, 0)):
			global_position.y += 0.55

func respawn(p: Vector3) -> void:
	global_position = p; velocity = Vector3.ZERO; hp = 100.0; alive = true; spawn_prot = 2.0; spawn_seq += 1
	cam.position.y = EYE; setup_weapon(wi); rebuild_viewmodel()
