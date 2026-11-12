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
var flash: MeshInstance3D
var mag: MeshInstance3D
var lh: Node3D
var mag_home := Vector3.ZERO
var lh_home := Vector3.ZERO

func _ready() -> void:
	collision_layer = 2; collision_mask = 1
	var cs := CollisionShape3D.new(); var sh := CapsuleShape3D.new(); sh.radius = 0.35; sh.height = 1.75; cs.shape = sh; cs.position = Vector3(0, 0.875, 0); add_child(cs)
	floor_snap_length = 0.4
	cam = Camera3D.new(); cam.position = Vector3(0, EYE, 0); cam.fov = 75.0; cam.near = 0.05; cam.far = 400.0; add_child(cam)
	vm = Node3D.new(); cam.add_child(vm)
	rebuild_viewmodel()

func rebuild_viewmodel() -> void:
	if gun_parts != null: gun_parts.queue_free()
	gun_parts = Node3D.new(); vm.add_child(gun_parts)
	var w := weapon()
	var kind: String = w["kind"]
	var L := 0.62; var H := 0.09
	match kind:
		"pistol", "magnum": L = 0.3; H = 0.11
		"smg": L = 0.5
		"shotgun": L = 0.85
		"sniper": L = 1.05
		"lmg": L = 0.85; H = 0.12
		"dmr": L = 0.9
		"launcher": L = 0.95; H = 0.16
	var dark := Color(0.12, 0.12, 0.13)
	_part(Vector3(0.06, H, L), Vector3(0, 0, -L * 0.5), dark)                               # الجسم
	_part(Vector3(0.025, 0.025, L * 0.7), Vector3(0, H * 0.4, -L * 0.85), Color(0.2, 0.2, 0.22)) # السبطانة
	_part(Vector3(0.05, 0.13, 0.06), Vector3(0, -0.11, -L * 0.2), Color(0.18, 0.18, 0.2))     # القبضة
	if kind in ["rifle", "dmr", "lmg", "sniper", "smg"]: _part(Vector3(0.045, 0.12, 0.07), Vector3(0, -0.11, -L * 0.5), Color(0.22, 0.2, 0.15))
	if kind in ["sniper", "dmr"]: _part(Vector3(0.05, 0.05, 0.28), Vector3(0, H * 0.8, -L * 0.45), Color(0.08, 0.08, 0.09))
	var skin := Color(0.78, 0.6, 0.48); var sleeve := Color(0.16, 0.2, 0.3)
	_part(Vector3(0.075, 0.085, 0.1), Vector3(0.0, -0.17, -L * 0.2), skin)                    # يد القبضة
	_part(Vector3(0.085, 0.09, 0.42), Vector3(0.05, -0.27, 0.12), sleeve).rotation.x = -0.35  # ساعد أيمن
	lh = Node3D.new(); gun_parts.add_child(lh); lh_home = Vector3(0, 0, 0); lh.position = lh_home
	var hh := _part(Vector3(0.07, 0.07, 0.11), Vector3(-0.03, -0.06, -L * 0.72), skin); hh.reparent(lh, false)
	var fa := _part(Vector3(0.08, 0.08, 0.5), Vector3(-0.2, -0.2, -L * 0.2), sleeve); fa.rotation = Vector3(-0.1, 0.45, 0); fa.reparent(lh, false)
	mag = null
	if kind != "launcher" and kind != "shotgun":
		var mh := 0.2 if kind in ["lmg", "smg"] else 0.15
		mag_home = Vector3(0, -0.13 - mh * 0.3, -L * 0.52 - (0.08 if kind in ["pistol", "magnum"] else 0.0))
		mag = _part(Vector3(0.045, mh, 0.07), mag_home, Color(0.1, 0.1, 0.11))
	flash = _part(Vector3(0.1, 0.1, 0.1), Vector3(0, H * 0.4, -L * 1.25), Color(1, 0.85, 0.4))
	var fm := flash.material_override as StandardMaterial3D
	fm.emission_enabled = true; fm.emission = Color(1, 0.7, 0.2); fm.emission_energy_multiplier = 3.0; flash.visible = false
	draw_t = 1.0
	gun_parts.position = Vector3(0.2, -0.22, -0.45); gun_parts.scale = Vector3.ONE * 0.62

func _part(sz: Vector3, pos: Vector3, c: Color) -> MeshInstance3D:
	var mi := MeshInstance3D.new(); var bm := BoxMesh.new(); bm.size = sz; mi.mesh = bm; mi.position = pos
	var m := StandardMaterial3D.new(); m.albedo_color = c; m.roughness = 0.5; m.metallic = 0.4; mi.material_override = m
	mi.cast_shadow = GeometryInstance3D.SHADOW_CASTING_SETTING_OFF; gun_parts.add_child(mi); return mi

func _physics_process(dt: float) -> void:
	super._physics_process(dt)
	if not alive:
		cam.position.y = lerpf(cam.position.y, 0.3, dt * 4.0); return
	var inp: Dictionary = game.inp
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
	kick = maxf(0.0, kick - dt * 8.0)
	sway = sway.lerp(Vector2.ZERO, minf(1.0, dt * 8.0)) + look * 0.0005
	sway = sway.clamp(Vector2(-0.04, -0.04), Vector2(0.04, 0.04))
	draw_t = maxf(0.0, draw_t - dt * 3.5)
	var rl := 0.0; var bolt := 0.0
	var rp := 0.0
	if reload_t > 0.0:
		var tot := maxf(0.1, float(weapon()["reload"])); rp = clampf(1.0 - reload_t / tot, 0.0, 1.0)
		rl = clampf(rp / 0.15, 0.0, 1.0) * clampf((1.0 - rp) / 0.2, 0.0, 1.0)  # ينزل السلاح ويعود
		if rp > 0.85: bolt = sin((rp - 0.85) / 0.15 * PI)
	if mag != null:
		var mp := mag_home; var lp := Vector3.ZERO; mag.visible = true
		if rp > 0.0:
			if rp < 0.3: mp = mag_home + Vector3(0, -0.4 * (rp / 0.3), 0.03); lp = Vector3(0.05, -0.1 * (rp / 0.3), 0.18 * (rp / 0.3))
			elif rp < 0.5: mag.visible = false; lp = Vector3(0.05, -0.1, 0.18) * (1.0 - (rp - 0.3) / 0.2)
			elif rp < 0.75: var k := (rp - 0.5) / 0.25; mp = mag_home + Vector3(0, -0.4 * (1.0 - k), 0.03 * (1.0 - k)); lp = Vector3(0.05, -0.1, 0.18) * (1.0 - k) * 0.6
		mag.position = mp; lh.position = lh.position.lerp(lp, minf(1.0, dt * 16.0))
	elif lh: lh.position = lh.position.lerp(Vector3(0, -0.12 * rl, 0.1 * rl), minf(1.0, dt * 10.0))
	if flash: flash.visible = kick > 0.8
	var tgt_pos := Vector3(0.0, -0.13, -0.42) if ads else Vector3(0.2, -0.22, -0.45)
	tgt_pos += Vector3(-sway.x, sway.y, 0) * (0.3 if ads else 1.0) + Vector3(0, -0.35 * draw_t - 0.08 * rl, 0.05 * bolt)
	var bobv := Vector3(sin(bob) * 0.006, absf(cos(bob)) * 0.008, 0) * (1.0 if is_on_floor() else 0.0) * clampf(spd / 4.6, 0, 1.4)
	gun_parts.position = gun_parts.position.lerp(tgt_pos + bobv + Vector3(0, 0, kick * 0.04), minf(1.0, dt * 14.0))
	gun_parts.rotation.z = lerpf(gun_parts.rotation.z, rl * 0.35, minf(1.0, dt * 10.0))
	gun_parts.rotation.x = lerpf(gun_parts.rotation.x, (-0.35 if sprinting else 0.0) + kick * 0.06 + rl * 0.45 + draw_t * 0.6, minf(1.0, dt * 10.0))
	var z := float(weapon()["zoom"])
	var tfov := 75.0 / (z if ads and z > 1.0 else 1.0)
	cam.fov = lerpf(cam.fov, tfov, minf(1.0, dt * 14.0))
	gun_parts.visible = not (ads and z >= 3.4)
	if inp["reload"]: start_reload(); inp["reload"] = false
	if inp["next"]: game.switch_weapon(self, 1); inp["next"] = false
	if inp["fire"] and not sprinting:
		var w := weapon()
		if try_fire(cam.global_position, -cam.global_transform.basis.z, ads, spd * 0.0004):
			kick = 1.0; pitch += float(w["recoilV"]) * (1.0 if ads else 1.3) * 1.5; yaw += randf_range(-1, 1) * float(w["recoilH"]) * 1.5
			game.hud.fire_flash()
		if not bool(w["auto"]): inp["fire"] = false

func _step_up2(h: Vector3) -> void:
	if h.length() < 0.001 or not is_on_floor(): return
	if test_move(global_transform, h):
		var up := global_transform.translated(Vector3(0, 0.55, 0))
		if not test_move(up, h) and not test_move(global_transform, Vector3(0, 0.55, 0)):
			global_position.y += 0.55

func respawn(p: Vector3) -> void:
	global_position = p; velocity = Vector3.ZERO; hp = 100.0; alive = true; spawn_prot = 2.0
	cam.position.y = EYE; setup_weapon(wi); rebuild_viewmodel()
