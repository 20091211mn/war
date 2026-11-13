class_name Grenade
extends Node3D
# قنبلة يدوية: مقذوفة باليد، ترتد، فتيل 2.2 ثانية، وميض أحمر قبل الانفجار

var game: Node
var src: Fighter
var vel := Vector3.ZERO
var fuse := 2.2
var spin := Vector3.ZERO
var mat: StandardMaterial3D
var led: MeshInstance3D
var settled := false

static func make(g: Node, s: Fighter, pos: Vector3, v: Vector3) -> Grenade:
	var n := Grenade.new(); n.game = g; n.src = s; n.vel = v
	var body := MeshInstance3D.new(); var sm := SphereMesh.new(); sm.radius = 0.055; sm.height = 0.11; sm.radial_segments = 10; sm.rings = 5; body.mesh = sm
	n.mat = StandardMaterial3D.new(); n.mat.albedo_color = Color(0.2, 0.3, 0.16); n.mat.roughness = 0.6; n.mat.metallic = 0.4; body.material_override = n.mat; n.add_child(body)
	var cap := MeshInstance3D.new(); var cm := CylinderMesh.new(); cm.top_radius = 0.018; cm.bottom_radius = 0.022; cm.height = 0.04; cap.mesh = cm; cap.position.y = 0.06
	var cmat := StandardMaterial3D.new(); cmat.albedo_color = Color(0.55, 0.55, 0.5); cmat.metallic = 0.8; cap.material_override = cmat; n.add_child(cap)
	n.led = MeshInstance3D.new(); var lm := SphereMesh.new(); lm.radius = 0.018; lm.height = 0.036; n.led.mesh = lm; n.led.position = Vector3(0, 0.045, 0.045)
	var lmat := StandardMaterial3D.new(); lmat.shading_mode = BaseMaterial3D.SHADING_MODE_UNSHADED; lmat.albedo_color = Color(1, 0.1, 0.05); n.led.material_override = lmat; n.led.visible = false; n.add_child(n.led)
	n.spin = Vector3(randf_range(-9, 9), randf_range(-9, 9), randf_range(-9, 9))
	g.world.add_child(n); n.global_position = pos
	return n

func _physics_process(dt: float) -> void:
	fuse -= dt
	if fuse <= 0.0: _boom(); return
	led.visible = fuse < 1.0 and int(fuse * (8.0 if fuse > 0.4 else 16.0)) % 2 == 0
	if settled: return
	vel.y -= 16.0 * dt
	var a := global_position; var b := a + vel * dt
	var q := PhysicsRayQueryParameters3D.create(a, b, 1 | 2)
	var h := get_world_3d().direct_space_state.intersect_ray(q)
	if h.is_empty(): global_position = b
	else:
		var nrm: Vector3 = h["normal"]
		global_position = (h["position"] as Vector3) + nrm * 0.06
		vel = vel.bounce(nrm) * 0.45
		if nrm.y > 0.6: vel.x *= 0.7; vel.z *= 0.7
		spin *= 0.6
		if vel.length() < 1.2 and nrm.y > 0.6: settled = true; vel = Vector3.ZERO
		elif game.has_method("sfx_play") and vel.length() > 2.0: game.sfx_play("reload", global_position)
	if not settled: rotation += spin * dt

func _boom() -> void:
	game.blast(src if is_instance_valid(src) else game.player, global_position, 6.5, 150.0, 220.0)
	game.fx_blast(global_position, 0.8)
	queue_free()
