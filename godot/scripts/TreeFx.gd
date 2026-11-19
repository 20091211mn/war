class_name TreeFx
extends RefCounted
# أشجار حقيقية بميش واحد لكل نوع (جذع مخروطي + أغصان + تاج من كتل غير منتظمة): 0 شجرة عريضة الأوراق، 1 تنّوب مثلّج، 2 سنط (أكاسيا)
# تتمايل بالريح في الشيدر (INSTANCE_CUSTOM.x طور، .y تباين لون) وتحمل ثلجاً على الأسطح العلوية عند snow>0.

static var _sh: Shader
static var _meshes := {}

static func mat(snow := 0.0) -> ShaderMaterial:
	if _sh == null:
		_sh = Shader.new(); _sh.code = """shader_type spatial;
render_mode diffuse_lambert, specular_disabled;
uniform float snow = 0.0;
varying vec3 wn; varying vec3 wp; varying float var_;
void vertex(){
	float ph = INSTANCE_CUSTOM.x; var_ = INSTANCE_CUSTOM.y;
	float h = max(VERTEX.y - 1.8, 0.0);
	float s = sin(TIME * 1.25 + ph) * 0.05 + sin(TIME * 2.3 + ph * 1.7) * 0.02;
	VERTEX.x += s * h * 0.28; VERTEX.z += s * h * 0.2;
	wn = normalize((MODEL_MATRIX * vec4(NORMAL, 0.0)).xyz); wp = (MODEL_MATRIX * vec4(VERTEX, 1.0)).xyz;
}
void fragment(){
	vec3 c = COLOR.rgb * (0.88 + var_ * 0.24);
	float v = fract(sin(dot(floor(wp.xz * 3.0 + wp.y * 2.0), vec2(12.9, 78.2))) * 43758.5);
	c *= 0.93 + v * 0.14;
	c = mix(c, vec3(0.94, 0.97, 1.0), snow * smoothstep(0.35, 0.8, wn.y));
	ALBEDO = c; ROUGHNESS = 0.9;
}"""
	var m := ShaderMaterial.new(); m.shader = _sh; m.set_shader_parameter("snow", snow); return m

static func _tube(st: SurfaceTool, a: Vector3, b: Vector3, r0: float, r1: float, sides: int, col: Color) -> void:
	var ax := (b - a).normalized(); var up := Vector3.UP if absf(ax.y) < 0.9 else Vector3.RIGHT
	var u := ax.cross(up).normalized(); var v := ax.cross(u).normalized()
	for i in sides:
		var a0 := TAU * float(i) / sides; var a1 := TAU * float(i + 1) / sides
		var d0 := u * cos(a0) + v * sin(a0); var d1 := u * cos(a1) + v * sin(a1)
		var p00 := a + d0 * r0; var p01 := a + d1 * r0; var p10 := b + d0 * r1; var p11 := b + d1 * r1
		var c0 := col.darkened(0.25); var c1 := col
		for q in [[p00, c0], [p01, c0], [p10, c1], [p01, c0], [p11, c1], [p10, c1]]:
			st.set_color(q[1]); st.add_vertex(q[0])

static func _blob(st: SurfaceTool, c: Vector3, r: Vector3, lo: Color, hi: Color, seed_v: int, fn: FastNoiseLite) -> void:
	var seg := 9; var rng := 6
	var pts: Array = []
	for j in rng + 1:
		var th := PI * float(j) / rng
		for i in seg + 1:
			var ph := TAU * float(i) / seg
			var d := Vector3(sin(th) * cos(ph), cos(th), sin(th) * sin(ph))
			var k := 1.0 + fn.get_noise_3dv(d * 2.2 + Vector3(seed_v, seed_v * 0.7, 0)) * 0.34
			pts.append([c + Vector3(d.x * r.x, d.y * r.y, d.z * r.z) * k, lo.lerp(hi, clampf(d.y * 0.5 + 0.5, 0.0, 1.0))])
	for j in rng:
		for i in seg:
			var i0 := j * (seg + 1) + i; var i1 := i0 + 1; var i2 := i0 + seg + 1; var i3 := i2 + 1
			for idx in [i0, i1, i2, i1, i3, i2]:
				st.set_color(pts[idx][1]); st.add_vertex(pts[idx][0])

static func _cone(st: SurfaceTool, base: Vector3, r: float, h: float, sides: int, lo: Color, hi: Color) -> void:
	var apex := base + Vector3(0, h, 0)
	for i in sides:
		var a0 := TAU * float(i) / sides; var a1 := TAU * float(i + 1) / sides
		var r0 := r * (1.0 if i % 2 == 0 else 0.8); var r1 := r * (1.0 if (i + 1) % 2 == 0 else 0.8)
		var p0 := base + Vector3(cos(a0) * r0, -h * 0.12 * (1.0 if i % 2 == 0 else 0.0), sin(a0) * r0)
		var p1 := base + Vector3(cos(a1) * r1, -h * 0.12 * (1.0 if (i + 1) % 2 == 0 else 0.0), sin(a1) * r1)
		st.set_color(lo); st.add_vertex(p0); st.set_color(lo); st.add_vertex(p1); st.set_color(hi); st.add_vertex(apex)
		var cc := base + Vector3(0, -0.05, 0)
		st.set_color(lo.darkened(0.4)); st.add_vertex(p0); st.set_color(lo.darkened(0.4)); st.add_vertex(cc); st.set_color(lo.darkened(0.4)); st.add_vertex(p1)

static func mesh(kind: int) -> ArrayMesh:
	if _meshes.has(kind): return _meshes[kind]
	var st := SurfaceTool.new(); st.begin(Mesh.PRIMITIVE_TRIANGLES)
	var rng := RandomNumberGenerator.new(); rng.seed = 321 + kind
	var fn := FastNoiseLite.new(); fn.seed = 5 + kind; fn.frequency = 1.0
	var bark := Color(0.27, 0.19, 0.11)
	if kind == 0:
		_tube(st, Vector3(0, 0, 0), Vector3(0.05, 2.4, 0), 0.2, 0.12, 7, bark)
		for i in 3:
			var a := TAU * float(i) / 3.0 + 0.5
			_tube(st, Vector3(0.05, 2.0, 0), Vector3(cos(a) * 0.9, 3.5, sin(a) * 0.9), 0.09, 0.05, 5, bark)
		for i in 9:
			var a := rng.randf() * TAU; var rr := rng.randf_range(0.2, 1.3)
			var c := Vector3(cos(a) * rr, 3.5 + rng.randf_range(-0.2, 1.7), sin(a) * rr)
			var s := rng.randf_range(0.8, 1.3)
			_blob(st, c, Vector3(s, s * 0.75, s), Color(0.07, 0.26, 0.09).lerp(Color(0.14, 0.3, 0.08), rng.randf()), Color(0.28, 0.52, 0.15).lerp(Color(0.38, 0.55, 0.14), rng.randf()), i * 7, fn)
	elif kind == 1:
		_tube(st, Vector3(0, 0, 0), Vector3(0, 1.4, 0), 0.2, 0.1, 6, bark)
		var radii := [2.0, 1.75, 1.45, 1.15, 0.85, 0.55]
		for i in 6:
			var by := 0.9 + float(i) * 1.05
			_cone(st, Vector3(0, by, 0), radii[i], 1.9 - i * 0.1, 12, Color(0.05, 0.17, 0.1), Color(0.12, 0.33, 0.17))
	else:
		_tube(st, Vector3(0, 0, 0), Vector3(0.3, 2.0, 0.1), 0.24, 0.16, 7, bark)
		_tube(st, Vector3(0.3, 2.0, 0.1), Vector3(-0.1, 3.4, 0.2), 0.16, 0.1, 6, bark)
		for i in 5:
			var a := TAU * float(i) / 5.0
			_tube(st, Vector3(-0.1, 3.2, 0.2), Vector3(cos(a) * 1.5, 3.9, sin(a) * 1.5 + 0.2), 0.07, 0.04, 5, bark)
		for i in 7:
			var a := TAU * float(i) / 7.0 + rng.randf(); var rr := 0.0 if i == 0 else rng.randf_range(1.0, 1.8)
			_blob(st, Vector3(cos(a) * rr, 3.9 + rng.randf_range(-0.1, 0.3), sin(a) * rr + 0.2), Vector3(1.3, 0.38, 1.3), Color(0.22, 0.32, 0.1), Color(0.42, 0.5, 0.18), i * 11, fn)
	st.generate_normals()
	var m := st.commit(); _meshes[kind] = m; return m
