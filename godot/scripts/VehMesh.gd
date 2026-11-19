class_name VehMesh
extends RefCounted
# توليد ميش هياكل المركبات: تحويل مقطع عرضي (loft) على طول z — يعطي غطاء محرك مائلاً وزجاجاً أمامياً وأقواس عجلات حقيقية
# بدل الصناديق. الأمام = -z، العرض ±x، الارتفاع y.

static func _lerp_pts(c: Array, z: float) -> float:
	if z <= c[0].x: return c[0].y
	for i in range(1, c.size()):
		if z <= c[i].x:
			var t: float = (z - c[i - 1].x) / maxf(c[i].x - c[i - 1].x, 0.0001)
			return lerpf(c[i - 1].y, c[i].y, t)
	return c[c.size() - 1].y

# نصف عرض الجسم عند ارتفاع y (يضيق فوق خط الحزام = tumblehome)
static func hw_at(y: float, top: float, belt := 1.08) -> float:
	if y <= belt:
		return 0.9 + 0.1 * clampf((y - 0.42) / 0.14, 0.0, 1.0)   # حافة سفلية مستديرة
	return lerpf(1.0, 0.84, clampf((y - belt) / maxf(top - belt, 0.05), 0.0, 1.0)) if top > belt + 0.1 else 0.96

# arches: [[zc, r, hmax]] ؛ top_ctrl: نقاط (z,y) للسقف
static func body(z0: float, z1: float, top_ctrl: Array, arches: Array, base_y := 0.42, open_top := false, belt := 1.08, ws := 1.0) -> ArrayMesh:
	var zs: Array = []
	var z := z0
	while z < z1: zs.append(z); z += 0.22
	zs.append(z1)
	for a in arches:
		var k: float = -a[1] - 0.04
		while k <= a[1] + 0.04:
			zs.append(a[0] + k); k += 0.07
	zs = zs.filter(func(v): return v >= z0 - 0.0001 and v <= z1 + 0.0001)
	zs.sort()
	var uniq: Array = []
	for v in zs:
		if uniq.is_empty() or v - uniq[uniq.size() - 1] > 0.02: uniq.append(v)
	var rings: Array = []
	for zv in uniq:
		var top := _lerp_pts(top_ctrl, zv)
		var bot := base_y
		for a in arches:
			var dz: float = absf(zv - a[0])
			if dz < a[1]: bot = maxf(bot, base_y + sqrt(1.0 - (dz / a[1]) * (dz / a[1])) * (a[2] - base_y))
		bot = minf(bot, top - 0.05)
		var b1 := minf(top, maxf(bot + 0.1, 0.0)); var bel := minf(top, belt)
		var ys := [bot, b1, minf(top, 0.8), bel, lerpf(bel, top, 0.4), lerpf(bel, top, 0.75), top]
		ys.sort()
		var ring: Array = []
		for y in ys: ring.append(Vector3(hw_at(y, top, belt) * ws, y, zv))
		rings.append(ring)
	var st := SurfaceTool.new(); st.begin(Mesh.PRIMITIVE_TRIANGLES)
	var n: int = rings[0].size()
	# حلقة مغلقة: يسار من الأسفل إلى الأعلى، ثم عبر السقف، ثم يمين من الأعلى إلى الأسفل، ثم القاع
	var loop := func(ring: Array) -> Array:
		var pts: Array = []
		for i in n: pts.append(Vector3(-ring[i].x, ring[i].y, ring[i].z))
		for i in range(n - 1, -1, -1): pts.append(Vector3(ring[i].x, ring[i].y, ring[i].z))
		return pts
	var loops: Array = rings.map(func(r): return loop.call(r))
	var m: int = loops[0].size()
	for ri in loops.size() - 1:
		for i in m:
			var j := (i + 1) % m
			if open_top and i == n - 1: continue        # لا سقف
			var a: Vector3 = loops[ri][i]; var b: Vector3 = loops[ri][j]; var c: Vector3 = loops[ri + 1][i]; var d: Vector3 = loops[ri + 1][j]
			for v in [a, c, b, b, c, d]: st.add_vertex(v)
	# غطاءا الطرفين
	for end in [0, loops.size() - 1]:
		var L: Array = loops[end]; var cen := Vector3.ZERO
		for p in L: cen += p
		cen /= float(L.size())
		for i in m:
			var j := (i + 1) % m
			if end == 0: for v in [L[i], cen, L[j]]: st.add_vertex(v)
			else: for v in [L[i], L[j], cen]: st.add_vertex(v)
	st.generate_normals()
	return st.commit()

# مستطيل زجاج مائل بين نقطتين على اليسار واليمين (للزجاج الأمامي/الخلفي)
static func quad(p0: Vector3, p1: Vector3, p2: Vector3, p3: Vector3) -> ArrayMesh:
	var st := SurfaceTool.new(); st.begin(Mesh.PRIMITIVE_TRIANGLES)
	for v in [p0, p1, p2, p0, p2, p3]: st.add_vertex(v)
	st.generate_normals(); return st.commit()
