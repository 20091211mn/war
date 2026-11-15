class_name GunModel
extends RefCounted
# موديلات الأسلحة الحقيقية (M4/AWM/SVD/RPG) محوّلة من GLB عبر tools/gunmodels_to_godot.py
# كل موديل 4 أسطح: 0 أمامي، 1 مخزن، 2 ترباس، 3 كعب — لتحريك المخزن والترباس عند التعبئة

static var _cache := {}
const IDS := [0, 2, 3, 4, 5, 6, 8]

static func has(wi: int) -> bool: return wi in IDS

# يعيد {"meshes": [4 ArrayMesh أو null], "mag_c": Vector3, "bolt_c": Vector3, "lo": Vector3, "hi": Vector3}
static func get_model(wi: int) -> Dictionary:
	if _cache.has(wi): return _cache[wi]
	var out := {}
	var f := FileAccess.open("res://assets/guns/%d.bin" % wi, FileAccess.READ)
	if f == null: _cache[wi] = out; return out
	var b := f.get_buffer(f.get_length())
	var nv := int(b.decode_u32(0)); var ni := int(b.decode_u32(4))
	var grp := [int(b.decode_s32(8)), int(b.decode_s32(12)), int(b.decode_s32(16)), int(b.decode_s32(20))]
	var o := 24
	var P := b.slice(o, o + nv * 12).to_float32_array(); o += nv * 12
	var N := b.slice(o, o + nv * 12).to_float32_array(); o += nv * 12
	var C := b.slice(o, o + nv * 3); o += nv * 3; o += (4 - o % 4) % 4
	var I := b.slice(o, o + ni * 4).to_int32_array()
	var mat := StandardMaterial3D.new(); mat.vertex_color_use_as_albedo = true; mat.roughness = 0.5; mat.metallic = 0.45; mat.cull_mode = BaseMaterial3D.CULL_DISABLED
	var meshes: Array = []; var off := 0
	var lo := Vector3(1e9, 1e9, 1e9); var hi := Vector3(-1e9, -1e9, -1e9)
	var cent: Array = []
	for g in 4:
		var cnt: int = grp[g]
		if cnt <= 0: meshes.append(null); cent.append(Vector3.ZERO); continue
		var remap := PackedInt32Array(); remap.resize(nv); remap.fill(-1)
		var vp := PackedVector3Array(); var vn := PackedVector3Array(); var vc := PackedColorArray(); var ix := PackedInt32Array()
		var mn := Vector3(1e9, 1e9, 1e9); var mx := Vector3(-1e9, -1e9, -1e9)
		for k in cnt:
			var gi: int = I[off + k]; var li := remap[gi]
			if li < 0:
				li = vp.size(); remap[gi] = li
				var p := Vector3(P[gi * 3], P[gi * 3 + 1], P[gi * 3 + 2]); vp.append(p)
				vn.append(Vector3(N[gi * 3], N[gi * 3 + 1], N[gi * 3 + 2]))
				vc.append(Color(C[gi * 3] / 255.0, C[gi * 3 + 1] / 255.0, C[gi * 3 + 2] / 255.0))
				mn = mn.min(p); mx = mx.max(p)
			ix.append(li)
		off += cnt
		var arr := []; arr.resize(Mesh.ARRAY_MAX)
		arr[Mesh.ARRAY_VERTEX] = vp; arr[Mesh.ARRAY_NORMAL] = vn; arr[Mesh.ARRAY_COLOR] = vc; arr[Mesh.ARRAY_INDEX] = ix
		var am := ArrayMesh.new(); am.add_surface_from_arrays(Mesh.PRIMITIVE_TRIANGLES, arr); am.surface_set_material(0, mat)
		meshes.append(am); cent.append((mn + mx) * 0.5); lo = lo.min(mn); hi = hi.max(mx)
	out = {"meshes": meshes, "mag_c": cent[1], "bolt_c": cent[2], "lo": lo, "hi": hi}
	_cache[wi] = out
	return out
