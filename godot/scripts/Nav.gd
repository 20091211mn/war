class_name Nav
extends RefCounted
# شبكة تنقّل (NavMesh) تُخبز وقت التشغيل من صناديق الخريطة: البوتات تسير حول المباني بمسار حقيقي بدل ملامسة الجدران

static func bake(game: Node, m: Dictionary, root: Node3D) -> void:
	var map_rid: RID = root.get_world_3d().navigation_map
	NavigationServer3D.map_set_cell_size(map_rid, 0.5); NavigationServer3D.map_set_cell_height(map_rid, 0.25)
	var nm := NavigationMesh.new()
	nm.cell_size = 0.5; nm.cell_height = 0.25; nm.agent_radius = 0.5; nm.agent_height = 1.75; nm.agent_max_climb = 0.5; nm.agent_max_slope = 40.0
	var W: float = m["W"]; var D: float = m["D"]
	var f := PackedVector3Array()
	var hx := W * 0.5; var hz := D * 0.5
	f.append_array([Vector3(-hx, 0, -hz), Vector3(hx, 0, -hz), Vector3(hx, 0, hz), Vector3(-hx, 0, -hz), Vector3(hx, 0, hz), Vector3(-hx, 0, hz)])
	for b in m["boxes"]:
		if float(b[4]) - float(b[1]) < 0.3 or float(b[1]) > 2.2: continue    # حاجز منخفض جداً أو سقف عالٍ لا يعيقان
		_add_box(f, b)
	var geo := NavigationMeshSourceGeometryData3D.new()
	geo.add_faces(f, Transform3D.IDENTITY)
	var w := root
	NavigationServer3D.bake_from_source_geometry_data_async(nm, geo, func():
		if not is_instance_valid(w) or game.world != w: return
		var rg := NavigationRegion3D.new(); rg.navigation_mesh = nm; w.add_child(rg)
		game.nav_map = rg.get_navigation_map(); game.nav_ready = true)

static func _add_box(f: PackedVector3Array, b: Array) -> void:
	var x0: float = b[0]; var y0: float = b[1]; var z0: float = b[2]; var x1: float = b[3]; var y1: float = b[4]; var z1: float = b[5]
	var v := [Vector3(x0, y0, z0), Vector3(x1, y0, z0), Vector3(x1, y1, z0), Vector3(x0, y1, z0), Vector3(x0, y0, z1), Vector3(x1, y0, z1), Vector3(x1, y1, z1), Vector3(x0, y1, z1)]
	var quads := [[3, 2, 1, 0], [4, 5, 6, 7], [7, 6, 2, 3], [0, 1, 5, 4], [4, 7, 3, 0], [1, 2, 6, 5]]   # كل وجه بترتيب يجعل الاتجاه نحو الخارج
	for q in quads:
		f.append_array([v[q[0]], v[q[1]], v[q[2]], v[q[0]], v[q[2]], v[q[3]]])
