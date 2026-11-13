class_name Drone
extends Node3D
# مسيّرة استطلاع: كاميرا حرّة، تكشف الأعداء عبر الجدران، وتنفجر عند الضغط (انتحارية)

var game: Node
var owner_f: Fighter
var cam: Camera3D
var vel := Vector3.ZERO
var yaw := 0.0
var pitch := -0.2
var t := 25.0
var spot := 0.0
var rotors: Array = []
var hum: AudioStreamPlayer3D

static func make(g: Node, s: Fighter, pos: Vector3, yw: float) -> Drone:
	var d := Drone.new(); d.game = g; d.owner_f = s; d.yaw = yw
	var m := StandardMaterial3D.new(); m.albedo_color = Color(0.12, 0.13, 0.15); m.metallic = 0.6; m.roughness = 0.45
	var body := MeshInstance3D.new(); var bm := BoxMesh.new(); bm.size = Vector3(0.22, 0.07, 0.3); body.mesh = bm; body.material_override = m; d.add_child(body)
	var eye := MeshInstance3D.new(); var em := SphereMesh.new(); em.radius = 0.04; em.height = 0.08; eye.mesh = em; eye.position = Vector3(0, -0.04, -0.15)
	var emat := StandardMaterial3D.new(); emat.shading_mode = BaseMaterial3D.SHADING_MODE_UNSHADED; emat.albedo_color = Color(1, 0.2, 0.1); eye.material_override = emat; d.add_child(eye)
	for sx in [-1, 1]:
		for sz in [-1, 1]:
			var arm := MeshInstance3D.new(); var am := BoxMesh.new(); am.size = Vector3(0.3, 0.02, 0.02); arm.mesh = am; arm.material_override = m
			arm.position = Vector3(0.14 * sx, 0, 0.14 * sz); arm.rotation.y = 0.785 * sx * sz; d.add_child(arm)
			var rt := MeshInstance3D.new(); var cm := CylinderMesh.new(); cm.top_radius = 0.11; cm.bottom_radius = 0.11; cm.height = 0.005; rt.mesh = cm
			var rm := StandardMaterial3D.new(); rm.albedo_color = Color(0.7, 0.7, 0.75, 0.35); rm.transparency = BaseMaterial3D.TRANSPARENCY_ALPHA; rt.material_override = rm
			rt.position = Vector3(0.27 * sx, 0.03, 0.27 * sz) * 0.62; d.add_child(rt); d.rotors.append(rt)
	d.cam = Camera3D.new(); d.cam.fov = 85.0; d.cam.near = 0.05; d.cam.far = 400.0; d.add_child(d.cam)
	g.world.add_child(d); d.global_position = pos
	return d

func step(dt: float, inp: Dictionary) -> bool:  # يرجع false عند الانتهاء
	t -= dt
	if t <= 0.0: game.hud.say(Settings.t("نفدت بطارية المسيّرة")); return false
	var look: Vector2 = inp["look"]; inp["look"] = Vector2.ZERO
	yaw -= look.x * 0.0042; pitch = clampf(pitch - look.y * 0.0042, -1.4, 1.4)
	rotation = Vector3(0, yaw, 0); cam.rotation = Vector3(pitch, 0, 0)
	var mv: Vector2 = inp["mv"]
	var f := Vector3(-sin(yaw) * cos(pitch), sin(pitch), -cos(pitch) * cos(yaw)); var r := Vector3(cos(yaw), 0, -sin(yaw))
	var up := (1.0 if bool(inp["jump"]) else 0.0) - (1.0 if bool(inp["crouch"]) else 0.0); inp["jump"] = false
	var tv := (f * -mv.y + r * mv.x) * 12.0 + Vector3(0, up * 6.0, 0)
	vel = vel.lerp(tv, minf(1.0, 6.0 * dt))
	var d := vel * dt; var L := d.length()
	if L > 1e-5:
		var q := PhysicsRayQueryParameters3D.create(global_position, global_position + d.normalized() * (L + 0.5), 1)
		if not get_world_3d().direct_space_state.intersect_ray(q).is_empty(): vel = Vector3.ZERO
		else:
			var np := global_position + d
			var hw: float = game.map_w * 0.5 - 0.5; var hd: float = game.map_d * 0.5 - 0.5
			if absf(np.x) < hw and absf(np.z) < hd and np.y > 0.5 and np.y < 30.0 and np.distance_to(owner_f.global_position) <= 70.0: global_position = np
			else: vel = Vector3.ZERO
	for rt in rotors: (rt as Node3D).rotation.y += dt * 60.0
	spot -= dt
	if spot <= 0.0:
		spot = 0.6
		for f2 in game.fighters:
			if not is_instance_valid(f2) or not f2.alive or f2.team == owner_f.team: continue
			var tp: Vector3 = f2.global_position + Vector3(0, 1.2, 0); var dd := global_position.distance_to(tp)
			if dd > 45.0: continue
			var q := PhysicsRayQueryParameters3D.create(global_position, tp, 1)
			if get_world_3d().direct_space_state.intersect_ray(q).is_empty(): f2.reveal = 2.5
	return true

func detonate() -> void:
	var p := global_position
	game.blast(owner_f, p, 7.0, 140.0, 220.0); game.fx_blast(p, 0.9)

func shut() -> void:
	queue_free()
