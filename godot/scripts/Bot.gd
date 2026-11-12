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
var legs: Array = []
var arms: Array = []
var gunn: Node3D
var far_hide: Array = []

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

func _build_model() -> void:
	model = Node3D.new(); add_child(model)
	var tc := Color(0.85, 0.2, 0.2) if team == 0 else Color(0.2, 0.45, 0.9)
	var dk := tc * 0.55
	var skin := Color(0.88, 0.7, 0.55)
	_box(model, Vector3(0.5, 0.65, 0.28), Vector3(0, 1.15, 0), tc)               # الجذع
	_box(model, Vector3(0.24, 0.26, 0.24), Vector3(0, 1.62, 0), skin)            # الرأس
	_box(model, Vector3(0.27, 0.12, 0.27), Vector3(0, 1.78, 0), dk)              # الخوذة
	for s in [-1, 1]:
		var leg := Node3D.new(); leg.position = Vector3(0.14 * s, 0.82, 0); model.add_child(leg); _box(leg, Vector3(0.2, 0.82, 0.22), Vector3(0, -0.41, 0), dk); legs.append(leg)
	gunn = Node3D.new(); gunn.position = Vector3(0.12, 1.28, -0.35); model.add_child(gunn)
	_box(gunn, Vector3(0.07, 0.1, 0.6), Vector3(0, 0, -0.1), Color(0.12, 0.12, 0.13))
	for s in [-1, 1]:
		var arm := _box(model, Vector3(0.13, 0.13, 0.42), Vector3(0.2 * s, 1.3, -0.25), tc * 0.8); arm.rotation.y = -0.25 * s; arms.append(arm); far_hide.append(arm)
	far_hide.append(gunn)
	for l in legs: far_hide.append(l)

func _physics_process(dt: float) -> void:
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
	# حركة الأطراف
	var spd := Vector2(velocity.x, velocity.z).length()
	walk_ph += spd * dt * 2.2
	for i in legs.size(): (legs[i] as Node3D).rotation.x = sin(walk_ph + i * PI) * 0.7 * clampf(spd / 4.0, 0, 1)
	gunn.position.y = 1.28 + sin(walk_ph * 2.0) * 0.012 * clampf(spd / 4.0, 0, 1) + sin(game.time * 1.6 + position.x) * 0.006
	# إطلاق
	if see and target != null:
		var dd := aim.length()
		var w := weapon()
		burst -= dt
		if burst > 0.0:
			var err := acc * (1.0 + dd * 0.02)
			var dir := (aim.normalized() + Vector3(randf_range(-err, err), randf_range(-err, err), randf_range(-err, err))).normalized()
			if dd < 90.0 and try_fire(eye_pos(), dir, false, 0.0): pass
		else:
			rest -= dt
			if rest <= 0.0: burst = randf_range(0.5, 1.4); rest = randf_range(0.4, 1.2)
	if ammo <= 0 and reserve <= 0: reserve = int(weapon()["mag"]) * 2
	# تفاصيل حسب الجودة والمسافة
	var camp: Vector3 = game.cam_pos()
	var far := global_position.distance_to(camp) > Settings.lod_dist()
	for n in far_hide: (n as Node3D).visible = not far

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
	global_position = p; velocity = Vector3.ZERO; hp = 100.0; alive = true; visible = true; spawn_prot = 2.0; target = null
	setup_weapon(game.random_weapon()); wander = game.random_point()
