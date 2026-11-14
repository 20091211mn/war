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
	var gmat: Material = _ground_mat(th, m)
	gm.material_override = gmat; gm.cast_shadow = GeometryInstance3D.SHADOW_CASTING_SETTING_OFF; gb.add_child(gm)
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
	var sky := Sky.new()
	if int(m.get("idx", -1)) == 0 or str(m.get("name", "")) == "المستودع":
		var sm := ProceduralSkyMaterial.new()
		sm.sky_top_color = _col(th["sky"]); sm.sky_horizon_color = _col(th["hor"]); sm.ground_horizon_color = _col(th["hor"]); sm.ground_bottom_color = _col(th["ground"]) * 0.7
		sky.sky_material = sm
	else:
		var sm2 := ShaderMaterial.new(); sm2.shader = _sky_shader()
		sm2.set_shader_parameter("top", _col(th["sky"])); sm2.set_shader_parameter("hor", _col(th["hor"])); sm2.set_shader_parameter("gnd", _col(th["ground"]) * 0.8)
		sm2.set_shader_parameter("cloud", 0.35 if str(m.get("name", "")).contains("الشاص") or str(m.get("name", "")).contains("الجيزة") else 0.5)
		sky.sky_material = sm2
	env.sky = sky; env.background_mode = Environment.BG_SKY
	env.ambient_light_source = Environment.AMBIENT_SOURCE_COLOR; env.ambient_light_color = Color(1, 1, 1); env.ambient_light_energy = float(th["amb"]) * 0.3
	env.fog_enabled = true; env.fog_sky_affect = 0.0; env.fog_light_color = _col(th["fog"]); env.tonemap_mode = Environment.TONE_MAPPER_FILMIC
	var we := WorldEnvironment.new(); we.environment = env; we.name = "Env"; root.add_child(we)
	var sun := DirectionalLight3D.new(); sun.name = "Sun"
	var sd: Array = th["sun"]; var dir := Vector3(sd[0], sd[1], sd[2]).normalized()
	sun.light_energy = float(th["sunI"]) * 0.45; sun.directional_shadow_max_distance = 55.0; sun.directional_shadow_mode = DirectionalLight3D.SHADOW_ORTHOGONAL; sun.shadow_bias = 0.05
	root.add_child(sun); sun.look_at_from_position(dir * 50.0, Vector3.ZERO, Vector3.UP)
	_horizon(root, m, th, W, D)
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

# ---------------------------------------------------------------- سماء بسحب + شمس، أرض متنوّعة، أفق 360°
static var _sky_sh: Shader = null
static var _gnd_sh: Shader = null

static func _sky_shader() -> Shader:
	if _sky_sh != null: return _sky_sh
	_sky_sh = Shader.new()
	_sky_sh.code = """shader_type sky;
uniform vec3 top : source_color = vec3(0.3, 0.55, 0.9);
uniform vec3 hor : source_color = vec3(0.9, 0.9, 0.9);
uniform vec3 gnd : source_color = vec3(0.5, 0.5, 0.4);
uniform float cloud = 0.5;
float hash(vec2 p){ return fract(sin(dot(p, vec2(127.1, 311.7))) * 43758.5453); }
float noise(vec2 p){ vec2 i = floor(p); vec2 f = fract(p); f = f * f * (3.0 - 2.0 * f);
	return mix(mix(hash(i), hash(i + vec2(1.0, 0.0)), f.x), mix(hash(i + vec2(0.0, 1.0)), hash(i + vec2(1.0, 1.0)), f.x), f.y); }
float fbm(vec2 p){ float v = 0.0; float a = 0.5; for (int i = 0; i < 4; i++){ v += a * noise(p); p = p * 2.03 + 11.7; a *= 0.5; } return v; }
void sky(){
	vec3 d = normalize(EYEDIR); float h = d.y;
	vec3 col = mix(hor, top, pow(clamp(h, 0.0, 1.0), 0.5));
	col = mix(col, mix(hor, top, 0.25), smoothstep(0.0, 0.08, h) * 0.0);
	if (h < 0.0) col = mix(hor, gnd, clamp(-h * 5.0, 0.0, 1.0));
	vec3 sd = normalize(LIGHT0_DIRECTION); float sdot = max(dot(d, sd), 0.0);
	col += vec3(1.0, 0.92, 0.75) * (pow(sdot, 900.0) * 10.0 + pow(sdot, 40.0) * 0.25 + pow(sdot, 6.0) * 0.07);
	if (h > 0.01) {
		vec2 uv = d.xz / (h + 0.22) * 1.5 + vec2(TIME * 0.006, TIME * 0.002);
		float c = fbm(uv); c = smoothstep(1.0 - cloud, 1.0 - cloud + 0.3, c);
		float shade = mix(0.78, 1.0, clamp(fbm(uv + vec2(0.07, 0.04)) * 1.4, 0.0, 1.0));
		col = mix(col, vec3(1.0) * shade, c * 0.9 * smoothstep(0.01, 0.2, h));
	}
	COLOR = col;
}"""
	return _sky_sh

static func _ground_mat(th: Dictionary, m: Dictionary) -> Material:
	if _gnd_sh == null:
		_gnd_sh = Shader.new()
		_gnd_sh.code = """shader_type spatial;
render_mode cull_back, diffuse_lambert, specular_disabled;
uniform vec3 base : source_color = vec3(0.5);
uniform float ripple = 0.0;
varying vec3 wp;
float hash(vec2 p){ return fract(sin(dot(p, vec2(127.1, 311.7))) * 43758.5453); }
float noise(vec2 p){ vec2 i = floor(p); vec2 f = fract(p); f = f * f * (3.0 - 2.0 * f);
	return mix(mix(hash(i), hash(i + vec2(1.0, 0.0)), f.x), mix(hash(i + vec2(0.0, 1.0)), hash(i + vec2(1.0, 1.0)), f.x), f.y); }
void vertex(){ wp = (MODEL_MATRIX * vec4(VERTEX, 1.0)).xyz; }
void fragment(){
	float n = noise(wp.xz * 0.12) * 0.6 + noise(wp.xz * 0.9) * 0.28 + noise(wp.xz * 5.0) * 0.12;
	float k = 0.86 + n * 0.28;
	float r = ripple * (sin(wp.x * 0.7 + noise(wp.xz * 0.2) * 9.0) * 0.5 + 0.5) * 0.07;
	ALBEDO = base * (k - r);
	ROUGHNESS = 1.0;
}"""
	var mt := ShaderMaterial.new(); mt.shader = _gnd_sh
	mt.set_shader_parameter("base", _col(th["ground"]))
	var nm := str(m.get("name", ""))
	mt.set_shader_parameter("ripple", 1.0 if (nm.contains("الصحراء") or nm.contains("الشاص") or nm.contains("الجيزة") or nm.contains("الصحر")) else 0.0)
	return mt

static func _mm(root: Node3D, mesh: Mesh, col: Color, tf: Array, shadow := false) -> void:
	if tf.is_empty(): return
	var mm := MultiMesh.new(); mm.transform_format = MultiMesh.TRANSFORM_3D; mm.mesh = mesh; mm.instance_count = tf.size()
	for i in tf.size(): mm.set_instance_transform(i, tf[i])
	var mi := MultiMeshInstance3D.new(); mi.multimesh = mm
	var mat := StandardMaterial3D.new(); mat.albedo_color = col; mat.roughness = 1.0; mat.metallic_specular = 0.0; mi.material_override = mat
	mi.cast_shadow = GeometryInstance3D.SHADOW_CASTING_SETTING_OFF; mi.name = "Horizon"; root.add_child(mi)

static func _horizon(root: Node3D, m: Dictionary, th: Dictionary, W: float, D: float) -> void:
	var nm := str(m.get("name", ""))
	if nm == "المستودع": return
	var rng := RandomNumberGenerator.new(); rng.seed = hash(nm)
	var fogF: float = float(th["fogF"]); var gc := _col(th["ground"])
	var big := W > 1000.0
	var cone := CylinderMesh.new(); cone.top_radius = 0.02; cone.bottom_radius = 1.0; cone.height = 1.0; cone.radial_segments = 7; cone.rings = 1
	var cube := BoxMesh.new(); cube.size = Vector3.ONE
	var dome := SphereMesh.new(); dome.radius = 1.0; dome.height = 2.0; dome.radial_segments = 10; dome.rings = 4
	var rx := W * 0.5; var rz := D * 0.5
	var off := fogF * 0.42 if not big else 700.0
	var N := 46
	var mount: Array = []; var caps: Array = []; var near: Array = []
	var desert := nm.contains("الصحراء") or nm.contains("الشاص") or nm.contains("الجيزة")
	var polar := nm.contains("القطب")
	var city := nm.contains("المدينة") and not nm.contains("القديمة")
	var old := nm.contains("القديمة")
	var port := nm.contains("الإسكندرية")
	var airb := big
	if desert or polar or airb:
		var col := _col(th["hor"]).lerp(gc, 0.45)
		if polar: col = Color(0.62, 0.7, 0.82)
		if airb: col = Color(0.36, 0.46, 0.4)
		for i in N:
			var a := TAU * (float(i) + rng.randf() * 0.8) / N
			var d := off * (1.0 + rng.randf() * 0.5)
			var px := cos(a) * (rx + d); var pz := sin(a) * (rz + d)
			var h := rng.randf_range(0.06, 0.16) * (fogF * 0.55) * (1.4 if polar else 1.0) if not desert else rng.randf_range(0.03, 0.09) * fogF * 0.5
			var wd := h * rng.randf_range(2.2, 3.6)
			if desert:   # كثبان وهضاب
				mount.append(Transform3D(Basis.from_scale(Vector3(wd * 1.5, h * 0.9, wd * 0.9)).rotated(Vector3.UP, a), Vector3(px, -h * 0.15, pz)))
			else:
				mount.append(Transform3D(Basis.from_scale(Vector3(wd, h, wd)).rotated(Vector3.UP, rng.randf() * TAU), Vector3(px, h * 0.5 - 0.5, pz)))
				if polar or airb and h > fogF * 0.1: caps.append(Transform3D(Basis.from_scale(Vector3(wd * 0.34, h * 0.34, wd * 0.34)).rotated(Vector3.UP, rng.randf() * TAU), Vector3(px, h * 0.83, pz)))
		_mm(root, dome if desert else cone, col, mount)
		_mm(root, cone, Color(0.95, 0.97, 1.0), caps)
	if city or old or port:
		var bc: Array = [[], [], []]
		var base := Color(0.52, 0.55, 0.62) if city else (Color(0.9, 0.87, 0.78) if old else Color(0.55, 0.58, 0.62))
		var tall := 1.0 if city else (0.35 if old else 0.5)
		for i in 130:
			var a := TAU * float(i) / 130.0 + rng.randf() * 0.04
			if port and sin(a) > 0.2: continue   # جهة البحر
			var d := off * (0.9 + rng.randf() * 0.7)
			var hh := rng.randf_range(10.0, 42.0) * tall; var ww := rng.randf_range(8.0, 20.0)
			bc[i % 3].append(Transform3D(Basis.from_scale(Vector3(ww, hh, ww * rng.randf_range(0.8, 1.4))).rotated(Vector3.UP, a), Vector3(cos(a) * (rx + d), hh * 0.5 - 0.3, sin(a) * (rz + d))))
		_mm(root, cube, base, bc[0]); _mm(root, cube, base.lightened(0.12), bc[1]); _mm(root, cube, base.darkened(0.12), bc[2])
		if old:   # مآذن
			var mn: Array = []; var tops: Array = []
			for i in 9:
				var a := TAU * float(i) / 9.0 + 0.3; var d := off * 1.05
				var hh := rng.randf_range(34.0, 50.0)
				mn.append(Transform3D(Basis.from_scale(Vector3(3.0, hh, 3.0)), Vector3(cos(a) * (rx + d), hh * 0.5, sin(a) * (rz + d))))
				tops.append(Transform3D(Basis.from_scale(Vector3(3.6, 6.0, 3.6)), Vector3(cos(a) * (rx + d), hh + 2.5, sin(a) * (rz + d))))
			_mm(root, cube, Color(0.95, 0.93, 0.86), mn); _mm(root, cone, Color(0.2, 0.55, 0.6), tops)
		if port:   # بحر + سفن ورافعات
			var sea := MeshInstance3D.new(); var pm := PlaneMesh.new(); pm.size = Vector2(W * 4.0, D * 2.0); sea.mesh = pm
			var sm := StandardMaterial3D.new(); sm.albedo_color = Color(0.1, 0.36, 0.55); sm.roughness = 0.18; sm.metallic = 0.3; sm.metallic_specular = 0.9; sea.material_override = sm
			sea.position = Vector3(0, -0.12, rz + D + 6.0); sea.cast_shadow = GeometryInstance3D.SHADOW_CASTING_SETTING_OFF; sea.name = "Sea"; root.add_child(sea)
			var ships: Array = []; var cr: Array = []
			for i in 6:
				var sx := rng.randf_range(-rx * 1.3, rx * 1.3); var sz := rz + rng.randf_range(60.0, off * 1.2)
				ships.append(Transform3D(Basis.from_scale(Vector3(rng.randf_range(34, 60), 8.0, 10.0)), Vector3(sx, 3.5, sz)))
				ships.append(Transform3D(Basis.from_scale(Vector3(12.0, 8.0, 8.0)), Vector3(sx - 8.0, 11.0, sz)))
			for i in 5: cr.append(Transform3D(Basis.from_scale(Vector3(2.5, 36.0, 2.5)), Vector3(-rx + i * (W / 4.0), 18.0, rz + 14.0)))
			_mm(root, cube, Color(0.28, 0.32, 0.38), ships); _mm(root, cube, Color(0.8, 0.35, 0.18), cr)
