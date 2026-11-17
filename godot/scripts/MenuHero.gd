class_name MenuHero
extends SubViewportContainer
# معرض سلاح سينمائي خلف القائمة: موديل GLB حقيقي يدور ببطء بإضاءة درامية (دفء من الخلف + تبريد من الأمام) وغبار عائم.
# يبدّل السلاح كل بضع ثوانٍ بتلاشٍ ناعم. شفاف فيظهر فوق مشهد الخلفية.
var sv: SubViewport
var pivot: Node3D
var cam: Camera3D
var ids: Array = [0, 3, 4, 2, 5, 6, 8]
var cur := 0
var t := 0.0
var cycle_t := 0.0
var names: Callable
var label: Label
var sub: Label
var fade := 1.0
var _dir := 0

func _init() -> void:
	set_anchors_preset(Control.PRESET_FULL_RECT); stretch = true; mouse_filter = Control.MOUSE_FILTER_IGNORE
	sv = SubViewport.new(); sv.own_world_3d = true; sv.transparent_bg = true; sv.msaa_3d = Viewport.MSAA_2X
	sv.render_target_update_mode = SubViewport.UPDATE_ALWAYS; add_child(sv)
	var env := Environment.new(); env.background_mode = Environment.BG_CLEAR_COLOR
	var sky := Sky.new(); var pm := ProceduralSkyMaterial.new()
	pm.sky_top_color = Color(0.04, 0.06, 0.12); pm.sky_horizon_color = Color(0.75, 0.6, 0.5); pm.ground_horizon_color = Color(0.5, 0.4, 0.35); pm.ground_bottom_color = Color(0.02, 0.03, 0.06); pm.sky_curve = 0.18
	sky.sky_material = pm; env.sky = sky; env.ambient_light_source = Environment.AMBIENT_SOURCE_SKY; env.ambient_light_energy = 0.35; env.reflected_light_source = Environment.REFLECTION_SOURCE_SKY
	env.tonemap_mode = Environment.TONE_MAPPER_FILMIC; env.glow_enabled = true; env.glow_intensity = 0.7; env.glow_bloom = 0.06; env.glow_hdr_threshold = 0.9
	var we := WorldEnvironment.new(); we.environment = env; sv.add_child(we)
	cam = Camera3D.new(); cam.fov = 28.0; cam.position = Vector3(0, 0.05, 2.1); cam.near = 0.05; cam.far = 20.0; sv.add_child(cam)
	var key := DirectionalLight3D.new(); key.light_color = Color(1.0, 0.82, 0.62); key.light_energy = 1.7; key.rotation_degrees = Vector3(-35, 32, 0); sv.add_child(key)
	var rim := DirectionalLight3D.new(); rim.light_color = Color(0.55, 0.72, 1.0); rim.light_energy = 1.3; rim.rotation_degrees = Vector3(-10, 158, 0); sv.add_child(rim)
	var warm := OmniLight3D.new(); warm.light_color = Color(1.0, 0.5, 0.2); warm.light_energy = 1.2; warm.omni_range = 5.0; warm.position = Vector3(-1.2, -0.3, -1.0); sv.add_child(warm)
	pivot = Node3D.new(); sv.add_child(pivot)
	_dust()
	set_process(true)

func _dust() -> void:
	var p := CPUParticles3D.new(); p.amount = 70; p.lifetime = 9.0; p.preprocess = 9.0; p.emission_shape = CPUParticles3D.EMISSION_SHAPE_BOX; p.emission_box_extents = Vector3(1.6, 0.9, 1.0)
	p.direction = Vector3(0.3, 1, 0); p.spread = 40.0; p.initial_velocity_min = 0.02; p.initial_velocity_max = 0.1; p.gravity = Vector3.ZERO; p.scale_amount_min = 0.5; p.scale_amount_max = 1.5
	var q := QuadMesh.new(); q.size = Vector2(0.012, 0.012); var m := StandardMaterial3D.new(); m.shading_mode = BaseMaterial3D.SHADING_MODE_UNSHADED; m.vertex_color_use_as_albedo = true; m.transparency = BaseMaterial3D.TRANSPARENCY_ALPHA; m.blend_mode = BaseMaterial3D.BLEND_MODE_ADD; m.billboard_mode = BaseMaterial3D.BILLBOARD_ENABLED; q.material = m; p.mesh = q
	var g := Gradient.new(); g.set_color(0, Color(1, 0.75, 0.45, 0.0)); g.set_color(1, Color(1, 0.7, 0.4, 0.0)); g.add_point(0.3, Color(1, 0.8, 0.55, 0.9)); g.add_point(0.7, Color(1, 0.75, 0.5, 0.6)); p.color_ramp = g
	p.position = Vector3(0, 0, -0.2); sv.add_child(p)

func show_weapon(i: int) -> void:
	for c in pivot.get_children(): c.queue_free()
	cur = i % ids.size(); var wi: int = ids[cur]
	var mdl := GunModel.get_model(wi)
	if mdl.is_empty(): return
	var lo: Vector3 = mdl["lo"]; var hi: Vector3 = mdl["hi"]; var ctr := (lo + hi) * 0.5
	var inner := Node3D.new(); pivot.add_child(inner); inner.position = -ctr
	for m in mdl["meshes"]:
		if m == null: continue
		var mi := MeshInstance3D.new(); mi.mesh = m; mi.cast_shadow = GeometryInstance3D.SHADOW_CASTING_SETTING_OFF; inner.add_child(mi)
	var len := maxf(hi.z - lo.z, 0.3); var s := 0.86 / len   # نفس الطول البصري لكل الأسلحة
	pivot.scale = Vector3.ONE * s; fade = 0.0
	if label != null and names.is_valid(): label.text = str(names.call(wi, 0)); sub.text = str(names.call(wi, 1))

func _process(dt: float) -> void:
	t += dt; cycle_t += dt
	if pivot.get_child_count() == 0: show_weapon(0)
	if cycle_t > 7.0: cycle_t = 0.0; show_weapon(cur + 1)
	fade = minf(1.0, fade + dt * 1.6)
	var k := fade * fade * (3.0 - 2.0 * fade)
	var yaw := deg_to_rad(-62.0 + sin(t * 0.32) * 28.0) + (1.0 - k) * 0.7
	pivot.rotation = Vector3(deg_to_rad(8.0 + sin(t * 0.5) * 2.0), yaw, deg_to_rad(-3.0 + sin(t * 0.41) * 1.5))
	pivot.position = Vector3(0, sin(t * 0.9) * 0.018 - (1.0 - k) * 0.05, (1.0 - k) * 0.5)
	modulate.a = k
	if label != null: label.modulate.a = k; sub.modulate.a = k
