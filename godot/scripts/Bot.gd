class_name Bot
extends Fighter
# بوت بسيط: يطارد أقرب عدو مرئي، يتحرك جانبياً، يطلق رشقات بدقة حسب الصعوبة، ويتفادى الجدران

var target: Fighter = null
var wander := Vector3.ZERO
var think := 0.0
var strafe := 1.0
var strafe_t := 0.0
var walk_ph := 0.0
var burst := 0.0
var rest := 0.0
var yaw := 0.0
var acc := 0.05
var model: Node3D
var torso: Node3D
var legs: Array = []        # مفاصل الورك
var knees: Array = []       # مفاصل الركبة
var gunn: Node3D
var muzzle: Node3D
var flash: Node3D
var rh: Node3D
var lh: Node3D
var arm_r: MeshInstance3D
var arm_l: MeshInstance3D
var far_hide: Array = []
var kick := 0.0
var run_k := 0.0            # 0..1 شدّة الجري (تنعيم)
var aim_pitch := 0.0
var gun_key := -1
# ---- شبكة LAN ----
var net_ctrl := false       # لاعب بشري (على المضيف) أو دمية (على العميل): لا ذكاء ولا فيزياء، يتبع الحالة المستلمة
var human := false
var off := false            # خانة فارغة في الغرفة
var net_pos := Vector3.ZERO
var net_yaw := 0.0
var net_aim := 0.0

func _ready() -> void:
	collision_layer = 2; collision_mask = 1
	var cs := CollisionShape3D.new(); var sh := CapsuleShape3D.new(); sh.radius = 0.35; sh.height = 1.75; cs.shape = sh; cs.position = Vector3(0, 0.875, 0); add_child(cs)
	floor_snap_length = 0.4
	_build_model()
	strafe = 1.0 if randf() > 0.5 else -1.0

func _mat(c: Color) -> StandardMaterial3D:
	var m := StandardMaterial3D.new(); m.albedo_color = c; m.roughness = 0.9; return m

func _box(parent: Node3D, size: Vector3, pos: Vector3, c: Color) -> MeshInstance3D:
	var mi := MeshInstance3D.new(); var bm := BoxMesh.new(); bm.size = size; mi.mesh = bm; mi.position = pos; mi.material_override = _mat(c); parent.add_child(mi); return mi

func _pivot(parent: Node3D, pos: Vector3) -> Node3D:
	var n := Node3D.new(); n.position = pos; parent.add_child(n); return n

# جسم بشري متمفصل: وركان وركبتان وصدر ورأس وذراعان تمسكان السلاح
func _build_model() -> void:
	model = Node3D.new(); add_child(model)
	var tc := Color(0.85, 0.2, 0.2) if team == 0 else Color(0.2, 0.45, 0.9)
	var dk := tc * 0.55; var skin := Color(0.88, 0.7, 0.55); var boot := Color(0.1, 0.1, 0.11)
	torso = _pivot(model, Vector3(0, 0.88, 0))
	_box(torso, Vector3(0.46, 0.6, 0.26), Vector3(0, 0.32, 0), tc)                    # الجذع
	_box(torso, Vector3(0.4, 0.3, 0.08), Vector3(0, 0.36, 0.16), dk)                  # حقيبة/سترة
	_box(torso, Vector3(0.24, 0.26, 0.24), Vector3(0, 0.8, 0), skin)                  # الرأس
	_box(torso, Vector3(0.27, 0.12, 0.27), Vector3(0, 0.95, 0), dk)                   # الخوذة
	for sd in [-1, 1]:
		var hip := _pivot(model, Vector3(0.11 * sd, 0.88, 0)); legs.append(hip)
		_box(hip, Vector3(0.17, 0.44, 0.19), Vector3(0, -0.22, 0), dk)
		var kn := _pivot(hip, Vector3(0, -0.44, 0)); knees.append(kn)
		_box(kn, Vector3(0.15, 0.42, 0.17), Vector3(0, -0.21, 0), dk * 0.9)
		_box(kn, Vector3(0.16, 0.09, 0.27), Vector3(0, -0.43, -0.04), boot)
	gunn = _pivot(torso, Vector3(0.13, 0.42, -0.4))
	rh = _pivot(gunn, Vector3(0, -0.09, 0.14)); lh = _pivot(gunn, Vector3(0, -0.05, -0.3))
	for h in [rh, lh]: _box(h, Vector3(0.09, 0.09, 0.1), Vector3.ZERO, skin)
	arm_r = _box(model, Vector3(1, 1, 1), Vector3.ZERO, tc * 0.8); arm_l = _box(model, Vector3(1, 1, 1), Vector3.ZERO, tc * 0.8)
	far_hide = [arm_r, arm_l, gunn] + legs
	_build_gun()

func _build_gun() -> void:
	gun_key = wi
	for c in gunn.get_children():
		if c != rh and c != lh: c.queue_free()
	var kind := "rifle"
	if game != null and wi < game.weapons.size(): kind = String(game.weapons[wi]["kind"])
	var L := 0.6; var H := 0.1
	match kind:
		"pistol", "magnum": L = 0.3; H = 0.12
		"smg": L = 0.48
		"shotgun": L = 0.8
		"sniper": L = 1.0
		"lmg": L = 0.85; H = 0.12
		"dmr": L = 0.9
		"launcher": L = 0.95; H = 0.16
	var dark := Color(0.12, 0.12, 0.13)
	_box(gunn, Vector3(0.06, H, L), Vector3(0, 0, -L * 0.5 + 0.18), dark)
	_box(gunn, Vector3(0.025, 0.025, L * 0.55), Vector3(0, H * 0.3, -L * 0.95 + 0.18), Color(0.2, 0.2, 0.22))
	if kind not in ["pistol", "magnum", "launcher", "shotgun"]: _box(gunn, Vector3(0.05, 0.14, 0.07), Vector3(0, -0.12, -L * 0.4 + 0.18), Color(0.1, 0.1, 0.11))
	if kind in ["sniper", "dmr"]: _box(gunn, Vector3(0.05, 0.05, 0.28), Vector3(0, H * 0.8, -L * 0.4 + 0.18), Color(0.08, 0.08, 0.09))
	muzzle = _pivot(gunn, Vector3(0, 0, -L * 1.1 + 0.18))
	flash = _pivot(muzzle, Vector3.ZERO)
	var fm := StandardMaterial3D.new(); fm.shading_mode = BaseMaterial3D.SHADING_MODE_UNSHADED; fm.albedo_color = Color(1, 0.85, 0.45, 0.9); fm.transparency = BaseMaterial3D.TRANSPARENCY_ALPHA; fm.blend_mode = BaseMaterial3D.BLEND_MODE_ADD; fm.cull_mode = BaseMaterial3D.CULL_DISABLED
	for r in [0.0, PI * 0.5]:
		var q := QuadMesh.new(); q.size = Vector2(0.3, 0.3); var qi := MeshInstance3D.new(); qi.mesh = q; qi.material_override = fm; qi.rotation.z = r; qi.cast_shadow = GeometryInstance3D.SHADOW_CASTING_SETTING_OFF; flash.add_child(qi)
	flash.visible = false
	rh.position = Vector3(0, -0.09, 0.0); lh.position = Vector3(0, -0.06, -L * 0.5 - 0.02)

func _puppet(dt: float) -> void:
	if gun_key != wi and gunn != null: _build_gun()
	spawn_prot = maxf(0.0, spawn_prot - dt)
	if not alive: return
	var old := global_position
	if old.distance_to(net_pos) > 6.0: global_position = net_pos
	else: global_position = old.lerp(net_pos, minf(1.0, dt * 12.0))
	yaw = lerp_angle(yaw, net_yaw, minf(1.0, dt * 12.0)); rotation.y = yaw
	var spd := Vector2(global_position.x - old.x, global_position.z - old.z).length() / maxf(dt, 0.0001)
	aim_pitch = net_aim
	var far := global_position.distance_to(game.cam_pos()) > Settings.lod_dist()
	for n in far_hide: (n as Node3D).visible = not far
	if not far: _animate(dt, minf(spd, 6.0), false)

func muzzle_pos() -> Vector3:
	return muzzle.global_position if muzzle != null and is_instance_valid(muzzle) else eye_pos()

func _arm_to(arm: MeshInstance3D, shoulder_local: Vector3, hand: Node3D) -> void:
	var sh := torso.to_global(shoulder_local); var s := model.to_local(sh); var h := model.to_local(hand.global_position)
	var d := h - s; var ln := d.length()
	if ln < 0.01: return
	arm.transform = Transform3D(Basis.looking_at(d / ln, Vector3.UP) * Basis.from_scale(Vector3(0.12, 0.12, ln)), s + d * 0.5)

# حركة الجسم: جري بركبتين، ميلان للأمام، اهتزاز، ارتداد السلاح، تصويب نحو الهدف
func _animate(dt: float, spd: float, firing: bool) -> void:
	var want := clampf(spd / 4.0, 0.0, 1.0)
	run_k = lerpf(run_k, want, minf(1.0, dt * 10.0))
	walk_ph += spd * dt * 2.6
	var k := run_k
	for i in 2:
		var ph := walk_ph + i * PI
		(legs[i] as Node3D).rotation.x = sin(ph) * 0.95 * k
		(knees[i] as Node3D).rotation.x = -(0.15 + 0.55 * (0.5 + 0.5 * sin(ph - 1.3))) * 1.5 * k
	model.rotation.x = -0.12 * k
	model.position.y = absf(sin(walk_ph)) * 0.07 * k
	torso.rotation.y = sin(walk_ph) * 0.07 * k
	kick = maxf(0.0, kick - dt * 9.0)
	gunn.position = Vector3(0.13, 0.42 + sin(walk_ph * 2.0) * 0.012 * k, -0.4 + kick * 0.05)
	gunn.rotation.x = aim_pitch + kick * 0.05 + 0.12 * k
	if flash != null: flash.visible = kick > 0.8 and Settings.quality >= 1
	_arm_to(arm_r, Vector3(0.27, 0.5, 0), rh); _arm_to(arm_l, Vector3(-0.27, 0.5, 0), lh)

func _physics_process(dt: float) -> void:
	if off: return
	if net_ctrl: _puppet(dt); return
	super._physics_process(dt)
	if not alive: return
	think -= dt; strafe_t -= dt
	if think <= 0.0:
		think = randf_range(0.25, 0.5); _pick_target()
	var mv := Vector3.ZERO
	var aim := Vector3.ZERO
	var see := false
	if target != null and target.alive:
		var to := target.global_position - global_position; to.y = 0.0
		var d := to.length()
		aim = (target.eye_pos() - eye_pos())
		see = _los(target)
		yaw = lerp_angle(yaw, atan2(-to.x, -to.z), minf(1.0, dt * 8.0))
		var want := 14.0 + (hash(fname) % 8)
		if d > want + 4: mv = to.normalized()
		elif d < want - 6: mv = -to.normalized()
		if strafe_t <= 0.0: strafe = -strafe; strafe_t = randf_range(0.8, 2.2)
		mv += Vector3(-to.z, 0, to.x).normalized() * strafe * 0.7
	else:
		var to := wander - global_position; to.y = 0.0
		if to.length() < 3.0: wander = game.random_point(); to = wander - global_position
		mv = to.normalized(); yaw = lerp_angle(yaw, atan2(-mv.x, -mv.z), minf(1.0, dt * 6.0))
	mv = _avoid(mv)
	mv.y = 0.0
	if mv.length() > 1.0: mv = mv.normalized()
	var sp := 4.2
	velocity.x = mv.x * sp; velocity.z = mv.z * sp
	velocity.y = velocity.y - 18.0 * dt if not is_on_floor() else -1.0
	_step_up(Vector3(velocity.x, 0, velocity.z) * dt)
	move_and_slide()
	var lim_x: float = game.map_w * 0.5 - 0.5; var lim_z: float = game.map_d * 0.5 - 0.5
	position.x = clampf(position.x, -lim_x, lim_x); position.z = clampf(position.z, -lim_z, lim_z)
	rotation.y = yaw
	var spd := Vector2(velocity.x, velocity.z).length()
	var camp: Vector3 = game.cam_pos()
	var far := global_position.distance_to(camp) > Settings.lod_dist()
	if gun_key != wi and gunn != null: _build_gun()
	# إطلاق
	if see and target != null:
		var dd := aim.length()
		var w := weapon()
		burst -= dt
		if burst > 0.0:
			var err := acc * (1.0 + dd * 0.02)
			var dir := (aim.normalized() + Vector3(randf_range(-err, err), randf_range(-err, err), randf_range(-err, err))).normalized()
			if dd < 90.0 and try_fire(eye_pos(), dir, false, 0.0): kick = 1.0
		else:
			rest -= dt
			if rest <= 0.0: burst = randf_range(0.5, 1.4); rest = randf_range(0.4, 1.2)
	if ammo <= 0 and reserve <= 0: reserve = int(weapon()["mag"]) * 2
	# تفاصيل حسب الجودة والمسافة
	for n in far_hide: (n as Node3D).visible = not far
	if not far:
		aim_pitch = clampf(asin(clampf(aim.normalized().y, -1.0, 1.0)), -0.6, 0.6) if (see and target != null) else 0.0
		_animate(dt, spd, see)

func _pick_target() -> void:
	var best: Fighter = null; var bd := 1e9
	for f in game.fighters:
		if f.team == team or not f.alive: continue
		var d := global_position.distance_to(f.global_position)
		if d < bd and d < 70.0 and _los(f): best = f; bd = d
	target = best

func _los(f: Fighter) -> bool:
	var q := PhysicsRayQueryParameters3D.create(eye_pos(), f.eye_pos() - Vector3(0, 0.3, 0), 1)
	return get_world_3d().direct_space_state.intersect_ray(q).is_empty()

func _avoid(mv: Vector3) -> Vector3:
	if mv.length() < 0.1: return mv
	var origin := global_position + Vector3(0, 0.6, 0)
	var fwd := mv.normalized()
	var space := get_world_3d().direct_space_state
	var q := PhysicsRayQueryParameters3D.create(origin, origin + fwd * 1.6, 1)
	if space.intersect_ray(q).is_empty(): return mv
	for a in [0.9, -0.9, 1.6, -1.6]:
		var d := fwd.rotated(Vector3.UP, a)
		var q2 := PhysicsRayQueryParameters3D.create(origin, origin + d * 1.6, 1)
		if space.intersect_ray(q2).is_empty(): return d
	return -fwd

func _step_up(h: Vector3) -> void:
	if h.length() < 0.001 or not is_on_floor(): return
	if test_move(global_transform, h):
		var up := global_transform.translated(Vector3(0, 0.55, 0))
		if not test_move(up, h) and not test_move(global_transform, Vector3(0, 0.55, 0)):
			global_position.y += 0.55

func respawn(p: Vector3) -> void:
	global_position = p; velocity = Vector3.ZERO; hp = 100.0; alive = true; visible = true; spawn_prot = 2.0; target = null; spawn_seq += 1
	setup_weapon(game.random_weapon()); wander = game.random_point()
