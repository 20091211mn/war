class_name MapBuilder
extends RefCounted
# يبني الخريطة من بيانات الصناديق: أرض + صناديق صلبة (MultiMesh لكل لون) + ديكور + بيئة/شمس

static func _col(c: Array) -> Color:
	return Color(c[0], c[1], c[2])

static func build(m: Dictionary, root: Node3D) -> Dictionary:
	var th: Dictionary = m["theme"]
	var cols: Array = th["cols"]
	var W: float = m["W"]; var D: float = m["D"]
	var out := {"boxes": m["boxes"], "W": W, "D": D}
	# ---- الأرض ----
	var gb := StaticBody3D.new(); gb.name = "Ground"; gb.collision_layer = 1
	var gs := CollisionShape3D.new(); var gsh := BoxShape3D.new(); gsh.size = Vector3(W + 40, 2, D + 40); gs.shape = gsh; gs.position = Vector3(0, -1, 0); gb.add_child(gs)
	var gm := MeshInstance3D.new(); var gmesh := BoxMesh.new(); gmesh.size = Vector3(W + 400, 0.2, D + 400); gm.mesh = gmesh; gm.position = Vector3(0, -0.1, 0)
	var gmat := StandardMaterial3D.new(); gmat.albedo_color = _col(th["ground"]); gmat.roughness = 1.0; gmat.metallic_specular = 0.0; gm.material_override = gmat; gm.cast_shadow = GeometryInstance3D.SHADOW_CASTING_SETTING_OFF; gb.add_child(gm)
	root.add_child(gb)
	# ---- صناديق صلبة وديكور: MultiMesh لكل لون ----
	var body := StaticBody3D.new(); body.name = "Solids"; body.collision_layer = 1; root.add_child(body)
	var groups := {}
	for b in m["boxes"]:
		_add_box(body, groups, b, cols, true)
	for b in m["deco"]:
		if b[3] - b[0] <= 0.0: continue
		_add_box(body, groups, b, cols, false)
	for k in groups:
		var arr: Array = groups[k]
		var mm := MultiMesh.new(); mm.transform_format = MultiMesh.TRANSFORM_3D; mm.mesh = BoxMesh.new(); (mm.mesh as BoxMesh).size = Vector3.ONE
		mm.instance_count = arr.size()
		for i in arr.size(): mm.set_instance_transform(i, arr[i])
		var mi := MultiMeshInstance3D.new(); mi.multimesh = mm
		var mat := StandardMaterial3D.new(); mat.albedo_color = _col(cols[k % cols.size()]); mat.roughness = 0.95; mat.metallic_specular = 0.1
		mi.material_override = mat; root.add_child(mi)
	# ---- البيئة والشمس ----
	var env := Environment.new()
	var sky := Sky.new(); var sm := ProceduralSkyMaterial.new()
	sm.sky_top_color = _col(th["sky"]); sm.sky_horizon_color = _col(th["hor"]); sm.ground_horizon_color = _col(th["hor"]); sm.ground_bottom_color = _col(th["ground"]) * 0.7
	sky.sky_material = sm; env.sky = sky; env.background_mode = Environment.BG_SKY
	env.ambient_light_source = Environment.AMBIENT_SOURCE_COLOR; env.ambient_light_color = Color(1, 1, 1); env.ambient_light_energy = float(th["amb"]) * 0.3
	env.fog_enabled = true; env.fog_light_color = _col(th["fog"]); env.tonemap_mode = Environment.TONE_MAPPER_FILMIC
	var we := WorldEnvironment.new(); we.environment = env; we.name = "Env"; root.add_child(we)
	var sun := DirectionalLight3D.new(); sun.name = "Sun"
	var sd: Array = th["sun"]; var dir := Vector3(sd[0], sd[1], sd[2]).normalized()
	sun.light_energy = float(th["sunI"]) * 0.45; sun.directional_shadow_max_distance = 55.0; sun.directional_shadow_mode = DirectionalLight3D.SHADOW_ORTHOGONAL; sun.shadow_bias = 0.05
	root.add_child(sun); sun.look_at_from_position(dir * 50.0, Vector3.ZERO, Vector3.UP)
	out["env"] = env; out["sun"] = sun; out["theme"] = th
	return out

static func _add_box(body: StaticBody3D, groups: Dictionary, b: Array, cols: Array, solid: bool) -> void:
	var sx: float = b[3] - b[0]; var sy: float = b[4] - b[1]; var sz: float = b[5] - b[2]
	if sx <= 0.0 or sy <= 0.0 or sz <= 0.0: return
	var c := Vector3((b[0] + b[3]) * 0.5, (b[1] + b[4]) * 0.5, (b[2] + b[5]) * 0.5)
	var k: int = int(b[6])
	if not groups.has(k): groups[k] = []
	groups[k].append(Transform3D(Basis.from_scale(Vector3(sx, sy, sz)), c))
	if solid:
		var cs := CollisionShape3D.new(); var sh := BoxShape3D.new(); sh.size = Vector3(sx, sy, sz); cs.shape = sh; cs.position = c; body.add_child(cs)
