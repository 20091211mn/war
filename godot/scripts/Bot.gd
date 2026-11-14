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
var seat_drop := 0.0
var veh_goal: Node = null     # مركبة يتجه لركوبها
var vtarget: Node = null      # مركبة معادية يستهدفها (للقاذف)
var flee_t := 0.0
var path := PackedVector3Array()
var path_i := 0
var path_t := 0.0
var path_goal := Vector3.INF
var last_seen := Vector3.INF     # آخر موضع لعدو رأيته
var last_seen_t := 0.0
var hurt_t := 0.0
var hurt_from := Vector3.ZERO
var cover_goal := Vector3.INF
var cover_t := 0.0
var flee_from := Vector3.ZERO
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
	model.position.y -= seat_drop
	kick = maxf(0.0, kick - dt * 9.0)
	gunn.position = Vector3(0.13, 0.42 + sin(walk_ph * 2.0) * 0.012 * k, -0.4 + kick * 0.05)
	gunn.rotation.x = aim_pitch + kick * 0.05 + 0.12 * k
	if flash != null: flash.visible = kick > 0.8 and Settings.quality >= 1
	_arm_to(arm_r, Vector3(0.27, 0.5, 0), rh); _arm_to(arm_l, Vector3(-0.27, 0.5, 0), lh)

func _physics_process(dt: float) -> void:
	if off or jet != null: return
	if net_ctrl: _puppet(dt); return
	super._physics_process(dt)
	if not alive: return
	if veh != null: _seated(dt); return
	think -= dt; strafe_t -= dt; flee_t = maxf(0.0, flee_t - dt); hurt_t = maxf(0.0, hurt_t - dt); cover_t = maxf(0.0, cover_t - dt); last_seen_t = maxf(0.0, last_seen_t - dt)
	if think <= 0.0:
		think = randf_range(0.25, 0.5); _pick_target(); _consider_vehicle(); _vehicle_threat()
	var mv := Vector3.ZERO
	var aim := Vector3.ZERO
	var see := false
	var tpos := Vector3.ZERO; var has_t := false; var aimp := Vector3.ZERO
	if target != null and target.alive: has_t = true; tpos = target.global_position; aimp = target.eye_pos()
	elif vtarget != null and is_instance_valid(vtarget) and not (vtarget as Vehicle).dead: has_t = true; tpos = vtarget.global_position; aimp = tpos + Vector3(0, 1.1, 0)
	if has_t:
		var to := tpos - global_position; to.y = 0.0
		var d := to.length()
		aim = aimp - eye_pos()
		see = _los_pos(aimp)
		if see: last_seen = tpos; last_seen_t = 8.0
		yaw = lerp_angle(yaw, atan2(-to.x, -to.z), minf(1.0, dt * 8.0))
		var want := 14.0 + (hash(fname) % 8)
		if d > want + 4: mv = to.normalized() if see else _goal_dir(tpos, dt)
		elif d < want - 6: mv = -to.normalized()
		if strafe_t <= 0.0: strafe = -strafe; strafe_t = randf_range(0.8, 2.2)
		mv += Vector3(-to.z, 0, to.x).normalized() * strafe * (0.7 if see else 0.2)
	elif last_seen_t > 0.0 and last_seen != Vector3.INF:   # فقدت العدو: أتحرّى آخر موضع رأيته فيه
		var to := last_seen - global_position; to.y = 0.0
		if to.length() < 3.0: last_seen_t = 0.0
		mv = _goal_dir(last_seen, dt); yaw = lerp_angle(yaw, atan2(-mv.x, -mv.z), minf(1.0, dt * 6.0))
	else:
		var to := wander - global_position; to.y = 0.0
		if to.length() < 3.0: wander = _next_goal(); to = wander - global_position
		mv = _goal_dir(wander, dt); yaw = lerp_angle(yaw, atan2(-mv.x, -mv.z), minf(1.0, dt * 6.0))
	if cover_t > 0.0 and cover_goal != Vector3.INF:   # أنسحب إلى غطاء وأعيد التعبئة
		var tc := cover_goal - global_position; tc.y = 0.0
		if tc.length() > 1.5: mv = _goal_dir(cover_goal, dt)
		else:
			mv = Vector3.ZERO
			if ammo < int(weapon()["mag"]): start_reload()
		if not see: yaw = lerp_angle(yaw, atan2(-mv.x, -mv.z) if mv.length() > 0.1 else yaw, minf(1.0, dt * 6.0))
	if flee_t > 0.0:   # هروب من مركبة معادية (لا يملك قاذفاً)
		var away := global_position - flee_from; away.y = 0.0
		mv = away.normalized() + Vector3(-away.z, 0, away.x).normalized() * strafe * 0.8
	if veh_goal != null and is_instance_valid(veh_goal):
		var vg := veh_goal as Vehicle
		if vg.seat_for(self) < 0: veh_goal = null
		else:
			var l := vg.to_local(global_position)
			var dd := Vector2(maxf(0.0, absf(l.x) - float(vg.P["hw"])), maxf(0.0, absf(l.z) - float(vg.P["hl"]))).length()
			if dd < 2.2: vg.enter(self, vg.seat_for(self)); veh_goal = null; return
			mv = _goal_dir(vg.global_position, dt)
			if not has_t or not see: yaw = lerp_angle(yaw, atan2(-mv.x, -mv.z), minf(1.0, dt * 6.0))
	else: veh_goal = null
	mv = _avoid(mv)
	mv.y = 0.0
	if mv.length() > 1.0: mv = mv.normalized()
	var sp := 4.9
	velocity.x = mv.x * sp; velocity.z = mv.z * sp
	velocity.y = velocity.y - 18.0 * dt if not is_on_floor() else -1.0
	_step_up(Vector3(velocity.x, 0, velocity.z) * dt)
	move_and_slide()
	track_fall()
	var lim_x: float = game.map_w * 0.5 - 0.5; var lim_z: float = game.map_d * 0.5 - 0.5
	position.x = clampf(position.x, -lim_x, lim_x); position.z = clampf(position.z, -lim_z, lim_z)
	rotation.y = yaw
	var spd := Vector2(velocity.x, velocity.z).length()
	var camp: Vector3 = game.cam_pos()
	var far := global_position.distance_to(camp) > Settings.lod_dist()
	if gun_key != wi and gunn != null: _build_gun()
	# إطلاق
	if see and has_t:
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
		aim_pitch = clampf(asin(clampf(aim.normalized().y, -1.0, 1.0)), -0.6, 0.6) if (see and has_t) else 0.0
		_animate(dt, spd, see)

# هدف تجوال ذكي: أحياناً نحو قاعدة العدو (هجوم) أو الوسط أو نقطة عشوائية
func _next_goal() -> Vector3:
	var r := randf()
	var mp: Dictionary = game.maps[game.map_idx]
	if r < 0.45:
		var arr: Array = mp["blue"] if team == 0 else mp["red"]
		var p: Array = arr[randi() % arr.size()]
		return Vector3(float(p[0]) + randf_range(-8, 8), 0.3, float(p[1]) + randf_range(-8, 8))
	if r < 0.65: return Vector3(randf_range(-12, 12), 0.3, randf_range(-12, 12))
	return game.random_point()

func set_seated(on: bool, vt: int, seat: int) -> void:
	veh_goal = null; target = null; vtarget = null
	if not on:
		seat_drop = 0.0; model.position.y = 0.0; model.rotation.x = 0.0
		for l in legs: (l as Node3D).visible = true
		visible = alive; return
	for l in legs: (l as Node3D).visible = false
	seat_drop = 0.4 if (vt == 0 and seat != 1) else (0.35 if vt == 1 else 0.0)
	visible = not (vt == 1 and seat == 0)
	if vt == 0 and seat == 1: for l in legs: (l as Node3D).visible = true

func _seated(dt: float) -> void:
	if gun_key != wi and gunn != null: _build_gun()
	var far := global_position.distance_to(game.cam_pos()) > Settings.lod_dist()
	if far: return
	aim_pitch = 0.0
	var gv := veh as Vehicle
	if gv.gunner == self: aim_pitch = gv.tp
	_animate(dt, 0.0, false)
	if gv.gunner == self and gv.vtype == 0: kick = 1.0 if gv.mg_cd > 0.07 else kick

func _pick_target() -> void:
	var best: Fighter = null; var bd := 1e9
	for f in game.fighters:
		if f.team == team or not f.alive: continue
		var d := global_position.distance_to(f.global_position)
		if d < bd and d < 70.0 and _los(f): best = f; bd = d
	target = best
	vtarget = null
	if String(weapon()["kind"]) == "launcher":
		var vb := 70.0
		for v in game.vehicles:
			if not is_instance_valid(v) or v.dead or v.team < 0 or v.team == team or v.occupant_count() == 0: continue
			var dv: float = global_position.distance_to(v.global_position)
			if dv < vb and _los_pos(v.global_position + Vector3(0, 1.1, 0)): vtarget = v; vb = dv

# اتجاه الحركة نحو هدف: عبر NavMesh إن جاهزة (يلتف حول المباني) وإلا خط مستقيم
func _goal_dir(goal: Vector3, dt: float) -> Vector3:
	var flat := goal - global_position; flat.y = 0.0
	if not game.nav_ready or flat.length() < 2.0: return flat.normalized()
	path_t -= dt
	if path_t <= 0.0 or path_goal.distance_to(goal) > 4.0:
		path_t = randf_range(0.5, 0.9); path_goal = goal
		path = NavigationServer3D.map_get_path(game.nav_map, global_position, goal, true); path_i = 1
	while path_i < path.size() - 1 and Vector2(path[path_i].x - global_position.x, path[path_i].z - global_position.z).length() < 1.1: path_i += 1
	if path_i < path.size():
		var d := path[path_i] - global_position; d.y = 0.0
		if d.length() > 0.05: return d.normalized()
	return flat.normalized()

# أقرب نقطة غطاء: لا يراها المهاجم (حاجز بينهما) وقريبة من موقعي
func _find_cover(threat: Vector3) -> Vector3:
	var best := Vector3.INF; var bd := 1e9
	var space := get_world_3d().direct_space_state
	for i in 14:
		var a := TAU * i / 14.0 + randf() * 0.3; var r := randf_range(4.0, 11.0)
		var p := global_position + Vector3(cos(a) * r, 0.0, sin(a) * r)
		p.x = clampf(p.x, -game.map_w * 0.5 + 2.0, game.map_w * 0.5 - 2.0); p.z = clampf(p.z, -game.map_d * 0.5 + 2.0, game.map_d * 0.5 - 2.0)
		if game.nav_ready:
			p = NavigationServer3D.map_get_closest_point(game.nav_map, p)
			if Vector2(p.x - global_position.x, p.z - global_position.z).length() > 14.0: continue
		var q := PhysicsRayQueryParameters3D.create(threat + Vector3(0, 1.5, 0), p + Vector3(0, 1.4, 0), 1)
		if space.intersect_ray(q).is_empty(): continue     # مكشوف
		var d := global_position.distance_to(p)
		if d < bd: bd = d; best = p
	return best

func play_death(from: Vector3) -> void:
	collision_layer = 0
	var away := (global_position - from); away.y = 0.0
	var loc := global_transform.basis.inverse() * (away.normalized() if away.length() > 0.1 else Vector3.BACK)
	var tw := create_tween(); tw.set_parallel(true)
	# يسقط بعيداً عن مصدر الإطلاق: دوران حول المحور الأفقي المتعامد مع الاتجاه
	var axis := Vector3(loc.z, 0, -loc.x).normalized() if absf(loc.x) + absf(loc.z) > 0.01 else Vector3.RIGHT
	var tb := model.transform; var tgt := Transform3D(Basis(axis, deg_to_rad(88.0)) * tb.basis, tb.origin + Vector3(loc.x, 0.0, loc.z) * 0.5 + Vector3(0, 0.12, 0))
	tw.tween_method(func(t: float): model.transform = tb.interpolate_with(tgt, t), 0.0, 1.0, 0.45).set_trans(Tween.TRANS_QUAD).set_ease(Tween.EASE_IN)

func _flinch() -> void:
	if model == null or not alive: return
	var tw := create_tween(); tw.tween_property(model, "rotation:x", -0.18, 0.05); tw.tween_property(model, "rotation:x", 0.0, 0.18)

func take_damage(d: float, src: Fighter, head: bool) -> void:
	var was := alive
	super.take_damage(d, src, head)
	if was and alive and d > 4.0: _flinch()
	if was and alive and src != null and src != self and not net_ctrl:
		hurt_t = 3.0; hurt_from = src.global_position; last_seen = src.global_position; last_seen_t = 8.0
		if hp < 45.0 and cover_t <= 0.0 and veh == null:
			var c := _find_cover(hurt_from)
			if c != Vector3.INF: cover_goal = c; cover_t = randf_range(3.0, 5.0)

func _los_pos(p: Vector3) -> bool:
	var q := PhysicsRayQueryParameters3D.create(eye_pos(), p - Vector3(0, 0.3, 0), 1)
	return get_world_3d().direct_space_state.intersect_ray(q).is_empty()

func _consider_vehicle() -> void:
	if veh != null or game.vehicles.is_empty(): return
	if veh_goal != null and is_instance_valid(veh_goal) and (veh_goal as Vehicle).seat_for(self) >= 0: return
	veh_goal = null
	if hash(fname) % 5 == 0 or hp < 40.0: return    # بعضهم يفضّل المشي
	var best: Vehicle = null; var bd := 55.0
	for v in game.vehicles:
		if not is_instance_valid(v): continue
		var s: int = v.seat_for(self)
		if s < 0 or s > 1: continue
		if s == 1 and not (v.driver is Bot or v.driver == game.player): continue
		var d: float = global_position.distance_to(v.global_position)
		if d < bd: best = v; bd = d
	veh_goal = best

# مركبة معادية مأهولة قريبة ومرئية وأنا بلا قاذف → أتراجع وأتفادى خط النار
func _vehicle_threat() -> void:
	if String(weapon()["kind"]) == "launcher": return
	for v in game.vehicles:
		if not is_instance_valid(v) or v.dead or v.team < 0 or v.team == team or v.occupant_count() == 0: continue
		if v.vtype == 0 and v.gunner == null and absf(v.speed) < 6.0: continue
		var d: float = global_position.distance_to(v.global_position)
		if d < (50.0 if v.vtype == 1 else 30.0) and _los_pos(v.global_position + Vector3(0, 1.2, 0)):
			flee_t = 1.4; flee_from = v.global_position; return

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
	if veh != null: (veh as Vehicle).leave(self)
	if model != null: model.transform = Transform3D.IDENTITY
	collision_layer = 2 if jet == null else 0
	global_position = p; velocity = Vector3.ZERO; hp = 100.0; alive = true; visible = true; spawn_prot = 2.0; target = null; spawn_seq += 1
	setup_weapon(8 if (not game.vehicles.is_empty() and randf() < 0.16) else game.random_weapon()); wander = game.random_point(); veh_goal = null; vtarget = null; flee_t = 0.0
