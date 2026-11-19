class_name DesertFx
extends RefCounted
# مرحلة «صحراء الشاص» الواقعية: رمل بتموّجات رياح وحصى، مواد صخر/طين/معدن مع تعرية وغبار وانسداد ظلّي عند الحواف،
# كثبان ناعمة وهضاب بعيدة، نخيل حقيقي (جذع مخروطي + سعف منحنٍ يتمايل)، ماء بانعكاس، صخور وشجيرات وأعشاب جافة،
# وغبار تذروه الرياح. كل التفاصيل بلا تصادم (زخرفية) كي لا تتأثر الفيزياء أو مسارات الذكاء الاصطناعي.
# كل عناصر التوزيع بذرة ثابتة => نفس الشكل عند جميع اللاعبين.

# ملفات تعريف الخرائط: ground = sand/asphalt/snow/grass/concrete ؛ kinds = نوع مادة كل لون (0 جص/حجر، 1 معدن، 2 خشب، 3 صخر، 5 زجاج)
# dust = لون الغبار/الثلج فوق الأسطح ؛ custom = أفق مخصّص (كثبان+هضاب) ؛ palms = نخيل+ماء ؛ extras = صخور/أعشاب
static func profile(m: Dictionary) -> Dictionary:
	var nm := str(m.get("name", ""))
	var sand := Color(0.84, 0.7, 0.46)
	if nm.contains("الشاص"): return {"ground": 0, "dust": sand, "kinds": [0, 0, 0, 1, 0, 2, 0, 0, 0], "custom": true, "palms": true, "extras": 1.0, "wind": 1, "rust": 0.4, "win": 0}
	if nm.contains("أطلال"): return {"ground": 0, "dust": sand, "kinds": [0, 0, 0, 3, 0, 2, 0, 0, 0], "custom": true, "palms": true, "extras": 0.8, "wind": 1, "rust": 0.3, "win": 2}
	if nm.contains("الجيزة"): return {"ground": 0, "dust": Color(0.88, 0.74, 0.48), "kinds": [3, 3, 3, 3, 0, 2, 3, 0, 0], "custom": true, "palms": true, "extras": 0.8, "wind": 1, "rust": 0.2, "win": 0}
	if nm.contains("القديمة"): return {"ground": 0, "dust": Color(0.86, 0.78, 0.62), "kinds": [0, 0, 0, 0, 0, 2, 0, 0, 0], "custom": false, "palms": false, "extras": 0.4, "wind": 0, "rust": 0.2, "win": 2}
	if nm.contains("المدينة"): return {"ground": 1, "dust": Color(0.6, 0.58, 0.54), "kinds": [0, 0, 5, 1, 0, 2, 5, 0, 0], "custom": false, "palms": false, "extras": 0.5, "wind": 0, "rust": 0.5, "win": 1}
	if nm.contains("القطب"): return {"ground": 2, "dust": Color(0.93, 0.96, 1.0), "kinds": [1, 1, 1, 3, 0, 2, 5, 1, 1], "custom": false, "palms": false, "extras": 0.5, "wind": 2, "rust": 0.1, "win": 1}
	if nm.contains("الإسكندرية"): return {"ground": 1, "dust": Color(0.66, 0.64, 0.6), "kinds": [1, 1, 1, 1, 0, 2, 1, 1, 1], "custom": false, "palms": false, "extras": 0.4, "wind": 0, "rust": 0.45, "win": 1}
	if nm.contains("جوية"): return {"ground": 3, "dust": Color(0.62, 0.6, 0.52), "kinds": [0, 1, 1, 0, 0, 2, 1, 0, 0], "custom": false, "palms": false, "extras": 0.0, "wind": 0, "rust": 0.3, "win": 0}
	if nm == "المستودع": return {"ground": 4, "dust": Color(0.55, 0.54, 0.52), "kinds": [0, 1, 1, 1, 0, 2, 1, 1, 1], "custom": false, "palms": false, "extras": 0.0, "wind": 0, "rust": 0.6, "win": 0}
	return {}

static func is_real(m: Dictionary) -> bool:
	return not profile(m).is_empty()

# ------------------------------------------------------------------ شيدرات
const NOISE := """
float hash(vec2 p){ return fract(sin(dot(p, vec2(127.1, 311.7))) * 43758.5453); }
float noise(vec2 p){ vec2 i = floor(p); vec2 f = fract(p); f = f * f * (3.0 - 2.0 * f);
	return mix(mix(hash(i), hash(i + vec2(1.0, 0.0)), f.x), mix(hash(i + vec2(0.0, 1.0)), hash(i + vec2(1.0, 1.0)), f.x), f.y); }
"""
static var _g_sh: Shader; static var _s_sh: Shader; static var _w_sh: Shader; static var _l_sh: Shader; static var _b_sh: Shader

static func _sh(cache: Shader, code: String) -> Shader:
	if cache != null: return cache
	var s := Shader.new(); s.code = code; return s

static func ground_mat(th: Dictionary, mode := 0) -> ShaderMaterial:
	if _g_sh == null:
		_g_sh = Shader.new(); _g_sh.code = "shader_type spatial;\nrender_mode diffuse_lambert, specular_schlick_ggx;\n" + """
uniform vec3 base : source_color = vec3(0.84, 0.7, 0.46);
uniform float detail = 1.0;
uniform int mode = 0;   // 0 رمل، 1 أسفلت، 2 ثلج، 3 عشب/تراب، 4 خرسانة
varying vec3 wp; varying vec3 wn;
""" + NOISE + """
// مسافة حافة خلية فورونوي (للتشققات) والمعرّف
vec2 vor(vec2 q){
	vec2 g = floor(q); vec2 f = fract(q); float d1 = 8.0; float d2 = 8.0; float id = 0.0;
	for (int i = -1; i <= 1; i++) for (int j = -1; j <= 1; j++) {
		vec2 o = vec2(float(i), float(j)); vec2 h = vec2(hash(g + o), hash(g + o + 17.3));
		float d = length(o + h - f);
		if (d < d1) { d2 = d1; d1 = d; id = hash(g + o + 5.1); } else if (d < d2) { d2 = d; }
	}
	return vec2(d2 - d1, id);
}
void vertex(){ wp = (MODEL_MATRIX * vec4(VERTEX, 1.0)).xyz; wn = normalize((MODEL_MATRIX * vec4(NORMAL, 0.0)).xyz); }
void fragment(){
	vec2 p = wp.xz;
	float dist = length(VERTEX);
	float fade = 1.0 - smoothstep(25.0, 110.0, dist);
	float large = noise(p * 0.018); float mid = noise(p * 0.09 + 7.0);
	float grain = noise(p * 13.0) * 0.5 + noise(p * 41.0) * 0.5;
	vec3 col = base; float rough = 1.0; float spec = 0.3;
	vec3 n = wn;
	if (mode == 0) {
		vec3 pale = base * vec3(1.12, 1.1, 1.06); vec3 dark = base * vec3(0.82, 0.76, 0.68);
		col = mix(dark, pale, large * 0.65 + mid * 0.35);
		vec2 wd = normalize(vec2(0.82, 0.57));
		float warp = noise(p * 0.32) * 4.5 + noise(p * 0.06) * 10.0;
		float rp = dot(p, wd) * 2.3 + warp;
		float rip = (sin(rp) * 0.5 + 0.5); rip *= rip;
		col *= 0.93 + rip * 0.13 * fade + (grain - 0.5) * 0.14 * detail;
		if (detail > 0.5 && fade > 0.01) {
			float patch = smoothstep(0.58, 0.74, noise(p * 0.21 + 3.0));
			vec2 q = p * 2.6; vec2 g = floor(q); vec2 f = fract(q); float md = 1.0; float pid = 0.0;
			for (int i = -1; i <= 1; i++) for (int j = -1; j <= 1; j++) {
				vec2 o = vec2(float(i), float(j)); vec2 h = vec2(hash(g + o), hash(g + o + 17.3));
				float d = length(o + h - f); if (d < md) { md = d; pid = hash(g + o + 5.1); }
			}
			float peb = smoothstep(0.13, 0.07, md) * patch;
			col = mix(col, vec3(0.58, 0.5, 0.4) * (0.78 + pid * 0.5), peb * fade * 0.7);
		}
		col = mix(col, col * vec3(0.8, 0.72, 0.62), smoothstep(0.62, 0.8, noise(p * 0.045 + 19.0)) * 0.55);
		float k = 0.16 * fade; n = normalize(wn + vec3(-cos(rp) * k * wd.x, 0.0, -cos(rp) * k * wd.y));
	} else if (mode == 1) {
		// أسفلت: حبيبات + تشققات + بقع زيت/رطوبة + ترقيعات
		col = base * (0.82 + large * 0.3 + mid * 0.18);
		col *= 0.9 + (grain - 0.5) * 0.35 * detail + step(0.93, hash(floor(p * 45.0))) * 0.25 * fade;
		vec2 v = vor(p * 0.28); float crack = smoothstep(0.05, 0.0, v.x) * step(0.45, v.y) * fade;
		col *= 1.0 - crack * 0.55;
		float st = smoothstep(0.58, 0.8, noise(p * 0.13 + 4.0)); col = mix(col, col * 0.55, st * 0.5); rough = mix(0.85, 0.35, st);
		float pt = step(0.7, hash(floor(p * 0.07))); col *= mix(1.0, 0.86, pt);
		spec = 0.4;
	} else if (mode == 2) {
		// ثلج: ظلال زرقاء، كثبان ريح، بريق بلّوري
		col = mix(base * vec3(0.8, 0.86, 0.96), base * vec3(1.03, 1.03, 1.04), large * 0.6 + mid * 0.4);
		vec2 wd = normalize(vec2(0.7, 0.7)); float rp = dot(p, wd) * 1.6 + noise(p * 0.25) * 9.0;
		float rip = sin(rp) * 0.5 + 0.5; col *= 0.95 + rip * 0.07 * fade;
		float sp = step(0.987, hash(floor(p * 70.0 + floor(TIME * 0.0)))) * fade; col += vec3(0.5, 0.55, 0.6) * sp * detail;
		float k = 0.1 * fade; n = normalize(wn + vec3(-cos(rp) * k * wd.x, 0.0, -cos(rp) * k * wd.y));
		spec = 0.5; rough = 0.55;
	} else if (mode == 3) {
		// عشب جاف وتراب
		vec3 dirt = base * vec3(1.08, 1.0, 0.82); vec3 gr = base * vec3(0.9, 1.0, 0.85);
		float m2 = smoothstep(0.3, 0.7, noise(p * 0.012 + 11.0) * 0.7 + noise(p * 0.09) * 0.3);
		col = mix(dirt, gr, m2); col *= 0.82 + large * 0.3 + (grain - 0.5) * 0.25 * detail;
		col *= 0.95 + 0.1 * sin(p.x * 3.0 + noise(p * 0.5) * 6.0) * fade;
	} else {
		// خرسانة: بلاطات ووصلات وبقع
		col = base * (0.85 + large * 0.25 + (grain - 0.5) * 0.18 * detail);
		vec2 sl = abs(fract(p / 6.0) - 0.5); float joint = smoothstep(0.488, 0.5, max(sl.x, sl.y)); col *= 1.0 - joint * 0.45 * fade;
		col = mix(col, col * 0.6, smoothstep(0.62, 0.82, noise(p * 0.2 + 2.0)) * 0.5);
		spec = 0.35; rough = 0.7;
	}
	ALBEDO = col; ROUGHNESS = rough; SPECULAR = spec;
	NORMAL = normalize((VIEW_MATRIX * vec4(n, 0.0)).xyz);
}"""
	var mt := ShaderMaterial.new(); mt.shader = _g_sh
	mt.set_shader_parameter("base", Color(th["ground"][0], th["ground"][1], th["ground"][2]))
	mt.set_shader_parameter("detail", 1.0 if Settings.quality >= 2 else 0.0)
	mt.set_shader_parameter("mode", mode)
	return mt

# kind: 0 طين/حجر رملي، 1 معدن صدئ، 2 خشب/لحاء، 3 صخر أحمر (هضاب)
static func solid_mat(col: Color, kind: int, edge := 1.0, dust := Color(0.84, 0.7, 0.46), rust := 0.4, win := 0) -> ShaderMaterial:
	if _s_sh == null:
		_s_sh = Shader.new(); _s_sh.code = "shader_type spatial;\nrender_mode diffuse_lambert, specular_schlick_ggx;\n" + """
uniform vec3 base : source_color = vec3(0.7, 0.6, 0.45);
uniform vec3 sand : source_color = vec3(0.84, 0.7, 0.46);
uniform float rustk = 0.4;
uniform int win = 0;
uniform int kind = 0;
uniform float edge = 1.0;
varying vec3 wp; varying vec3 wn; varying vec3 lp; varying vec3 sc; varying vec2 yr;
""" + NOISE + """
void vertex(){
	wp = (MODEL_MATRIX * vec4(VERTEX, 1.0)).xyz; wn = normalize((MODEL_MATRIX * vec4(NORMAL, 0.0)).xyz); lp = VERTEX;
	sc = vec3(length(MODEL_MATRIX[0].xyz), length(MODEL_MATRIX[1].xyz), length(MODEL_MATRIX[2].xyz));
	yr = vec2(MODEL_MATRIX[3].y - sc.y * 0.5, MODEL_MATRIX[3].y + sc.y * 0.5);
}
void fragment(){
	vec2 xz = wp.xz + wp.y * 0.37;
	float n1 = noise(xz * 0.35); float n2 = noise(xz * 2.3 + wp.y * 1.7); float n3 = noise((xz + wp.y) * 9.0);
	vec3 col = base * (0.88 + n1 * 0.22 + n2 * 0.1 + (n3 - 0.5) * 0.09);
	float up = clamp(wn.y, 0.0, 1.0); float side = 1.0 - up;
	float thin = step(sc.y, 0.12);
	float rough = 0.95; float met = 0.0;
	if (kind == 0 || kind == 3) {
		// طبقات أفقية (ترسّبات) + بقع تعرية رأسية
		float band = noise(vec2(wp.y * (kind == 3 ? 2.2 : 4.5), (wp.x + wp.z) * 0.03));
		col *= 0.9 + band * (kind == 3 ? 0.26 : 0.16);
		float st = noise(vec2((wp.x + wp.z) * 3.1, wp.y * 0.45));
		col *= 1.0 - smoothstep(0.55, 0.92, st) * 0.2 * side;
		// بنية لبن/طوب: خطوط مونة خافتة
		if (kind == 0) { float cr = step(0.93, fract(wp.y * 2.4 + 0.5)) * side; col *= 1.0 - cr * 0.09; }
	} else if (kind == 1) {
		float r = smoothstep(0.6, 0.85, noise(xz * 1.4 + wp.y * 2.1));
		col = mix(col, vec3(0.45, 0.22, 0.1) * (0.8 + n3 * 0.5), r * rustk * (1.0 - thin));
		col *= 0.9 + 0.1 * sin((wp.x + wp.z) * 18.0);   // تعرّج حاويات
		rough = 0.55; met = 0.25 * (1.0 - r);
	} else if (kind == 5) {
		// زجاج/ألواح عاكسة: داكن أملس مع تدرّج رأسي
		col *= 0.8 + 0.2 * smoothstep(0.0, 8.0, wp.y); rough = 0.15; met = 0.3;
		float ug = abs(wn.x) > abs(wn.z) ? wp.z : wp.x; vec2 gc = abs(fract(vec2(ug / 1.5, wp.y / 1.6)) - 0.5); col *= 1.0 - step(0.47, max(gc.x, gc.y)) * 0.45 * side;
	} else if (kind == 2) {
		col *= 0.8 + 0.3 * noise(vec2(wp.x * 3.0 + wp.z * 3.0, wp.y * 14.0)); rough = 0.9;
	}
	// واجهات المباني: نوافذ وإطارات وقاعدة (plinth) وكورنيش — للأجسام العالية فقط
	float bld = step(4.2, sc.y) * step(2.8, min(sc.x, sc.z)) * smoothstep(0.55, 0.85, side);
	vec3 emit = vec3(0.0);
	if (bld > 0.5) {
		float hr = wp.y - yr.x; float top = yr.y;
		float u = abs(wn.x) > abs(wn.z) ? wp.z : wp.x;
		col *= mix(1.0, 0.8, 1.0 - smoothstep(0.0, 1.0, hr));                    // قاعدة أغمق
		col *= mix(1.0, 0.86, smoothstep(top - 0.5, top - 0.3, wp.y));           // حزام الكورنيش
		if (win == 1 && kind == 0 && hr > 1.8 && wp.y < top - 1.0) {
			vec2 cs = vec2(2.7, 3.2); vec2 q = vec2(u / cs.x, (wp.y - yr.x - 1.2) / cs.y); vec2 id = floor(q); vec2 f = fract(q);
			float r = hash(id + 3.1);
			float inw = step(0.2, f.x) * step(f.x, 0.8) * step(0.25, f.y) * step(f.y, 0.82);
			float fr = step(0.16, f.x) * step(f.x, 0.84) * step(0.21, f.y) * step(f.y, 0.86);
			vec3 glass = mix(vec3(0.1, 0.17, 0.26), vec3(0.45, 0.6, 0.75), smoothstep(0.25, 0.82, f.y) * 0.6 + r * 0.15);
			if (r > 0.88) { glass = vec3(0.95, 0.8, 0.45); emit = glass * 0.5; }
			else if (r < 0.2) glass = mix(glass, vec3(0.75, 0.7, 0.6), smoothstep(0.5, 0.8, f.y) * 0.8);
			col = mix(col, col * 0.7, fr - inw); col = mix(col, glass, inw); rough = mix(rough, 0.12, inw); met = mix(met, 0.4, inw);
			col = mix(col, col * 1.12, step(0.8, f.y) * step(0.14, f.x) * step(f.x, 0.86) * (1.0 - inw) * 0.0); // عتبة
		} else if (win == 2 && kind == 0 && hr > 1.5 && wp.y < top - 0.8) {
			vec2 cs = vec2(3.4, 3.0); vec2 q = vec2(u / cs.x, (wp.y - yr.x - 1.5) / cs.y); vec2 id = floor(q); vec2 f = fract(q);
			float aw = abs(f.x - 0.5);
			float arch = step(aw, 0.15) * step(f.y, 0.55 - smoothstep(0.1, 0.15, aw) * 0.25) * step(0.1, f.y);
			float rim = step(aw, 0.2) * step(f.y, 0.62 - smoothstep(0.1, 0.2, aw) * 0.25) * step(0.06, f.y);
			col = mix(col, col * 1.1, rim - arch); col = mix(col, vec3(0.06, 0.05, 0.05) * (0.6 + hash(id) * 0.6), arch);
		}
	}
	// غبار الرمل: على الأسطح العلوية ويتراكم عند القاعدة
	col = mix(col, sand * (0.88 + n2 * 0.22), up * (0.5 + n1 * 0.4) * (1.0 - thin * 0.75));
	float drift = smoothstep(1.2, 0.0, wp.y + (n1 - 0.5) * 0.8) * side;
	col = mix(col, sand * 0.96, drift * 0.5);
	if (edge > 0.5 && thin < 0.5) {
		vec3 d = (0.5 - abs(lp)) * sc;
		float mn = min(d.x, min(d.y, d.z)); float mx = max(d.x, max(d.y, d.z)); float med = d.x + d.y + d.z - mn - mx;
		col *= mix(0.76, 1.0, smoothstep(0.0, 0.24, med));
		col += vec3(0.05) * (1.0 - smoothstep(0.0, 0.05, med)) * up;
	}
	col *= mix(0.7, 1.0, smoothstep(0.0, 0.8, wp.y + thin));
	ALBEDO = col; ROUGHNESS = rough; METALLIC = met; SPECULAR = 0.2; EMISSION = emit;
}"""
	var m := ShaderMaterial.new(); m.shader = _s_sh
	m.set_shader_parameter("base", col); m.set_shader_parameter("kind", kind); m.set_shader_parameter("edge", edge); m.set_shader_parameter("sand", dust); m.set_shader_parameter("rustk", rust)
	return m

static func water_mat() -> ShaderMaterial:
	if _w_sh == null:
		_w_sh = Shader.new(); _w_sh.code = "shader_type spatial;\nrender_mode blend_mix, depth_draw_always, specular_schlick_ggx, cull_back;\n" + """
varying vec3 wp;
""" + NOISE + """
void vertex(){ wp = (MODEL_MATRIX * vec4(VERTEX, 1.0)).xyz; }
void fragment(){
	vec2 p = wp.xz;
	float t = TIME;
	float a = noise(p * 1.3 + vec2(t * 0.25, t * 0.12)); float b = noise(p * 3.1 - vec2(t * 0.2, -t * 0.3));
	vec3 n = normalize(vec3((a - 0.5) * 0.35 + (b - 0.5) * 0.2, 1.0, (b - 0.5) * 0.3 + (a - 0.5) * 0.2));
	NORMAL = normalize((VIEW_MATRIX * vec4(n, 0.0)).xyz);
	float fr = pow(1.0 - clamp(dot(normalize(NORMAL), normalize(VIEW)), 0.0, 1.0), 3.0);
	vec3 deep = vec3(0.04, 0.27, 0.3); vec3 shallow = vec3(0.16, 0.5, 0.45);
	vec3 col = mix(deep, shallow, 0.4 + a * 0.4);
	ALBEDO = mix(col, vec3(0.7, 0.85, 0.95), fr * 0.7);
	ROUGHNESS = 0.06; SPECULAR = 0.9; METALLIC = 0.1;
	ALPHA = 0.9;
}"""
	var m := ShaderMaterial.new(); m.shader = _w_sh; return m

# أوراق/أعشاب: ألوان رؤوس (COLOR) وتمايل في الريح حسب UV.y (0 قاعدة، 1 طرف)
static func leaf_mat() -> ShaderMaterial:
	if _l_sh == null:
		_l_sh = Shader.new(); _l_sh.code = "shader_type spatial;\nrender_mode cull_disabled, diffuse_lambert, specular_disabled;\n" + """
varying float tip;
void vertex(){
	tip = UV.y;
	vec3 wp0 = (MODEL_MATRIX * vec4(0.0, 0.0, 0.0, 1.0)).xyz;
	float ph = wp0.x * 0.7 + wp0.z * 0.9;
	VERTEX.x += sin(TIME * 1.6 + ph) * 0.05 * UV.y * UV.y;
	VERTEX.z += cos(TIME * 1.3 + ph * 1.3) * 0.04 * UV.y * UV.y;
}
void fragment(){
	ALBEDO = COLOR.rgb * (0.78 + 0.28 * tip);
	ROUGHNESS = 0.9;
}"""
	var m := ShaderMaterial.new(); m.shader = _l_sh; return m

static func bark_mat() -> ShaderMaterial:
	if _b_sh == null:
		_b_sh = Shader.new(); _b_sh.code = "shader_type spatial;\nrender_mode diffuse_lambert, specular_disabled;\nvarying vec3 wp;\n" + NOISE + """
void vertex(){ wp = (MODEL_MATRIX * vec4(VERTEX, 1.0)).xyz; }
void fragment(){
	float ring = smoothstep(0.35, 0.5, abs(fract(wp.y * 3.2) - 0.5));
	float v = noise(vec2(atan(wp.x, wp.z) * 0.0 + (wp.x + wp.z) * 14.0, wp.y * 2.5));
	vec3 c = vec3(0.34, 0.24, 0.14) * (0.75 + v * 0.45);
	c *= 1.0 - ring * 0.35;
	c = mix(c, vec3(0.84, 0.7, 0.46) * 0.9, smoothstep(0.7, 0.0, wp.y) * 0.45);
	ALBEDO = c; ROUGHNESS = 0.95;
}"""
	var m := ShaderMaterial.new(); m.shader = _b_sh; return m

# ------------------------------------------------------------------ تصفية صناديق الديكور
# تُرجع true إن كان الصندوق سيُستبدل ببناء خاص (خارج الحدود: كتل الأفق / سعف النخيل / ماء)
static func take_deco(b: Array, W: float, D: float, dx: Dictionary) -> bool:
	var cx := (float(b[0]) + float(b[3])) * 0.5; var cz := (float(b[2]) + float(b[5])) * 0.5
	var pr: Dictionary = dx["prof"]
	if pr["custom"] and (absf(cx) > W * 0.5 + 0.5 or absf(cz) > D * 0.5 + 0.5): return true
	var k := int(b[6])
	if not pr["palms"]: return false
	if k == 4:
		var key := "%d,%d" % [roundi(cx * 4.0), roundi(cz * 4.0)]
		if not dx["crowns"].has(key): dx["crowns"][key] = Vector3(cx, float(b[1]), cz)
		return true
	if k == 8 and float(b[4]) - float(b[1]) < 0.2:
		dx["water"].append([cx, cz, float(b[3]) - float(b[0]), float(b[5]) - float(b[2])]); return true
	return false

static func is_palm_trunk(b: Array) -> bool:
	return int(b[6]) == 5 and absf(float(b[3]) - float(b[0]) - 0.7) < 0.02 and absf(float(b[5]) - float(b[2]) - 0.7) < 0.02

static func new_dx(prof := {}) -> Dictionary:
	return {"crowns": {}, "water": [], "trunks": [], "prof": prof}

# ------------------------------------------------------------------ مولّدات الميش
static func _frond_mesh() -> ArrayMesh:
	var st := SurfaceTool.new(); st.begin(Mesh.PRIMITIVE_TRIANGLES)
	var segs := 9; var L := 3.1
	var prev_l := Vector3.ZERO; var prev_r := Vector3.ZERO
	for i in segs + 1:
		var t := float(i) / segs
		var c := Vector3(t * L, sin(t * PI * 0.6) * 0.95 - t * t * 2.5, 0.0)    # محور السعفة المنحني (يتدلّى عند الطرف)
		var w := 0.5 * sin(clampf(t, 0.0, 1.0) * PI * 0.92 + 0.12) * (0.55 if i % 2 == 1 else 1.0)    # حافة مسننة
		var l := c + Vector3(0, -w * 0.45, w); var r := c + Vector3(0, -w * 0.45, -w)
		if i > 0:
			var col0 := Color(0.12, 0.3, 0.1).lerp(Color(0.38, 0.5, 0.18), t)
			st.set_color(col0); st.set_uv(Vector2(0, float(i - 1) / segs)); st.add_vertex(prev_l)
			st.set_color(col0); st.set_uv(Vector2(0, float(i - 1) / segs)); st.add_vertex(prev_r)
			st.set_color(col0); st.set_uv(Vector2(0, t)); st.add_vertex(l)
			st.set_color(col0); st.set_uv(Vector2(0, float(i - 1) / segs)); st.add_vertex(prev_r)
			st.set_color(col0); st.set_uv(Vector2(0, t)); st.add_vertex(r)
			st.set_color(col0); st.set_uv(Vector2(0, t)); st.add_vertex(l)
		prev_l = l; prev_r = r
	st.generate_normals()
	return st.commit()

static func _tuft_mesh(blades: int, h: float, w: float, col: Color, seed_v: int) -> ArrayMesh:
	var rng := RandomNumberGenerator.new(); rng.seed = seed_v
	var st := SurfaceTool.new(); st.begin(Mesh.PRIMITIVE_TRIANGLES)
	for i in blades:
		var a := rng.randf() * TAU; var lean := rng.randf_range(0.15, 0.7); var hh := h * rng.randf_range(0.55, 1.0)
		var d := Vector3(cos(a), 0, sin(a)); var side := Vector3(-d.z, 0, d.x) * w * 0.5
		var base := d * rng.randf_range(0.0, 0.12)
		var tipp := base + d * lean * hh * 0.6 + Vector3(0, hh, 0)
		var mid := base.lerp(tipp, 0.5) + d * lean * 0.08 * hh
		var cb := col.darkened(0.35); var ct := col.lightened(0.1)
		for v in [[base - side, cb, 0.0], [base + side, cb, 0.0], [mid - side * 0.6, col, 0.5], [base + side, cb, 0.0], [mid + side * 0.6, col, 0.5], [mid - side * 0.6, col, 0.5],
				[mid - side * 0.6, col, 0.5], [mid + side * 0.6, col, 0.5], [tipp, ct, 1.0]]:
			st.set_color(v[1]); st.set_uv(Vector2(0, v[2])); st.add_vertex(v[0])
	st.generate_normals()
	return st.commit()

static func _rock_mesh(seed_v: int, flat: float) -> ArrayMesh:
	var sph := SphereMesh.new(); sph.radius = 0.5; sph.height = 1.0; sph.radial_segments = 11; sph.rings = 7
	var mdt := MeshDataTool.new(); var am := ArrayMesh.new(); am.add_surface_from_arrays(Mesh.PRIMITIVE_TRIANGLES, sph.get_mesh_arrays())
	mdt.create_from_surface(am, 0)
	var fn := FastNoiseLite.new(); fn.seed = seed_v; fn.frequency = 2.3; fn.fractal_octaves = 3
	for i in mdt.get_vertex_count():
		var v := mdt.get_vertex(i); var u := v.normalized()
		var r := 0.5 * (1.0 + fn.get_noise_3dv(u * 1.4) * 0.55)
		var p := u * r
		p.y *= flat
		if p.y < -0.12 * flat: p.y = -0.12 * flat    # قاعدة مسطّحة تجلس على الأرض
		mdt.set_vertex(i, p)
	am.clear_surfaces(); mdt.commit_to_surface(am)
	var st := SurfaceTool.new(); st.create_from(am, 0); st.generate_normals()
	return st.commit()

# ------------------------------------------------------------------ البناء الرئيسي
static func _blocked(boxes: Array, x: float, z: float, pad: float) -> bool:
	for q in boxes:
		if float(q[1]) < 3.0 and float(q[4]) > 0.05 and x > float(q[0]) - pad and x < float(q[3]) + pad and z > float(q[2]) - pad and z < float(q[5]) + pad: return true
	return false

static func _mm(root: Node3D, mesh: Mesh, mat: Material, tf: Array, shadow := false, name := "DFx") -> void:
	if tf.is_empty(): return
	var mm := MultiMesh.new(); mm.transform_format = MultiMesh.TRANSFORM_3D; mm.mesh = mesh; mm.instance_count = tf.size()
	for i in tf.size(): mm.set_instance_transform(i, tf[i])
	var mi := MultiMeshInstance3D.new(); mi.multimesh = mm; mi.material_override = mat; mi.name = name
	mi.cast_shadow = GeometryInstance3D.SHADOW_CASTING_SETTING_ON if shadow else GeometryInstance3D.SHADOW_CASTING_SETTING_OFF
	root.add_child(mi)

static func build(m: Dictionary, root: Node3D, dx: Dictionary, th: Dictionary, sun: DirectionalLight3D, env: Environment) -> void:
	var W: float = m["W"]; var D: float = m["D"]; var boxes: Array = m["boxes"]
	var q := Settings.quality
	var full := q >= 3
	var rng := RandomNumberGenerator.new(); rng.seed = 4404
	var pr: Dictionary = dx["prof"]; var gm: int = pr["ground"]
	# ---- جو ----
	match gm:
		0:
			env.fog_density = 0.0011; env.fog_sun_scatter = 0.2; env.adjustment_contrast = 1.1; env.adjustment_saturation = 1.04
			if sun != null: sun.light_color = Color(1.0, 0.94, 0.82); sun.directional_shadow_max_distance = 75.0
		1:
			env.fog_density = 0.0009; env.fog_sun_scatter = 0.12; env.adjustment_contrast = 1.1; env.adjustment_saturation = 1.0
			if sun != null: sun.light_color = Color(1.0, 0.96, 0.9); sun.directional_shadow_max_distance = 70.0
		2:
			env.fog_density = 0.0018; env.fog_sun_scatter = 0.1; env.adjustment_contrast = 1.06; env.adjustment_saturation = 0.95
			if sun != null: sun.light_color = Color(0.93, 0.96, 1.0); sun.directional_shadow_max_distance = 70.0
		3:
			env.fog_density = 0.0006; env.adjustment_contrast = 1.08; env.adjustment_saturation = 1.06
			if sun != null: sun.light_color = Color(1.0, 0.96, 0.88)
		_:
			env.adjustment_contrast = 1.08
	# ---- كثبان ناعمة حول الحدود + هضاب بعيدة ----
	if pr["custom"]:
		_dunes(root, th, W, D, q)
		_mesas(root, W, D, rng)
	# ---- نخيل ----
	var fr := _frond_mesh(); var lm := leaf_mat()
	var fronds: Array = []; var trunks: Array = []
	for key in dx["crowns"]:
		var c: Vector3 = dx["crowns"][key]
		var lean := Vector3(rng.randf_range(-0.25, 0.25), 0, rng.randf_range(-0.25, 0.25))
		var hh := c.y
		# جذع مخروطي مائل قليلاً (التصادم يبقى صندوق الجذع الأصلي)
		trunks.append(Transform3D(Basis.from_euler(Vector3(lean.z * 0.12, 0, -lean.x * 0.12)).scaled(Vector3(1, hh, 1)), Vector3(c.x, hh * 0.5, c.z)))
		var nf := 11
		for i in nf:
			var a := TAU * float(i) / nf + rng.randf() * 0.4
			var sc := rng.randf_range(0.85, 1.2)
			var b := Basis.from_euler(Vector3(0, a, 0)) * Basis(Vector3(0, 0, 1), rng.randf_range(-0.1, 0.35)) * Basis.from_scale(Vector3(sc, sc * rng.randf_range(0.65, 1.5), sc))
			fronds.append(Transform3D(b, Vector3(c.x + lean.x * 0.15, c.y + 0.05, c.z + lean.z * 0.15)))
		for i in 5:   # سعف فتيّ منتصب في القلب يكسر شكل المظلة
			var a2 := TAU * float(i) / 5.0 + rng.randf()
			var b2 := Basis.from_euler(Vector3(0, a2, 0)) * Basis(Vector3(0, 0, 1), rng.randf_range(0.8, 1.15)) * Basis.from_scale(Vector3(0.55, 0.7, 0.55))
			fronds.append(Transform3D(b2, Vector3(c.x + lean.x * 0.15, c.y + 0.1, c.z + lean.z * 0.15)))
	var trunk := CylinderMesh.new(); trunk.top_radius = 0.2; trunk.bottom_radius = 0.34; trunk.height = 1.0; trunk.radial_segments = 8; trunk.rings = 1
	_mm(root, trunk, bark_mat(), trunks, true, "PalmTrunk")
	_mm(root, fr, lm, fronds, false, "PalmFronds")
	# ---- ماء ----
	var wm := water_mat()
	for w in dx["water"]:
		var mi := MeshInstance3D.new(); var pm := PlaneMesh.new(); pm.size = Vector2(w[2], w[3]); mi.mesh = pm; mi.material_override = wm
		mi.position = Vector3(w[0], 0.035, w[1]); mi.cast_shadow = GeometryInstance3D.SHADOW_CASTING_SETTING_OFF; mi.name = "Water"; root.add_child(mi)
	# ---- صخور صغيرة/متوسطة (زخرفية، ≤ 0.5م) ----
	var ex: float = pr["extras"]
	var rocks_n := int((260 if full else 120) * ex)
	var rm := [_rock_mesh(11, 0.75), _rock_mesh(23, 0.6), _rock_mesh(37, 0.9)]
	var rt: Array = [[], [], []]
	var tries := 0
	while tries < rocks_n * 6 and (rt[0].size() + rt[1].size() + rt[2].size()) < rocks_n:
		tries += 1
		var x := rng.randf_range(-W * 0.5 + 2.0, W * 0.5 - 2.0); var z := rng.randf_range(-D * 0.5 + 2.0, D * 0.5 - 2.0)
		var s := rng.randf_range(0.14, 0.5) * (1.6 if rng.randf() < 0.1 else 1.0)
		if _blocked(boxes, x, z, s * 0.6 + 0.2): continue
		var tf := Transform3D(Basis.from_euler(Vector3(rng.randf_range(-0.2, 0.2), rng.randf() * TAU, rng.randf_range(-0.2, 0.2))).scaled(Vector3(s * rng.randf_range(1.0, 1.5), s, s * rng.randf_range(0.9, 1.3))), Vector3(x, s * 0.12, z))
		rt[tries % 3].append(tf)
	var rcols := [Color(0.5, 0.44, 0.37), Color(0.6, 0.5, 0.38), Color(0.42, 0.38, 0.34)]
	if gm == 1 or gm == 4: rcols = [Color(0.45, 0.45, 0.44), Color(0.55, 0.54, 0.52), Color(0.36, 0.36, 0.37)]
	elif gm == 2: rcols = [Color(0.34, 0.36, 0.4), Color(0.42, 0.44, 0.48), Color(0.3, 0.32, 0.36)]
	for i in 3: _mm(root, rm[i], solid_mat(rcols[i], 3, 0.0, pr["dust"], 0.0), rt[i], false, "Rocks")
	# ---- شجيرات وأعشاب جافة ----
	var gcol := Color(0.74, 0.62, 0.32) if gm == 0 else Color(0.5, 0.54, 0.32)
	var dry := _tuft_mesh(9, 0.55, 0.09, gcol, 5)
	var shrub := _tuft_mesh(16, 0.9, 0.12, Color(0.36, 0.38, 0.2), 9)
	var tuf: Array = []; var shr: Array = []
	var tn := int((520 if full else 220) * ex) if gm != 2 else 0
	tries = 0
	while tries < tn * 5 and (tuf.size() + shr.size()) < tn:
		tries += 1
		var x := rng.randf_range(-W * 0.5 + 2.0, W * 0.5 - 2.0); var z := rng.randf_range(-D * 0.5 + 2.0, D * 0.5 - 2.0)
		if _blocked(boxes, x, z, 0.5): continue
		# تتجمع قرب الواحة (z≈±48) والوادي
		var oasis := false
		for w in dx["water"]:
			if absf(x - w[0]) < w[2] * 0.5 + 8.0 and absf(z - w[1]) < w[3] * 0.5 + 8.0: oasis = true; break
		if not oasis and rng.randf() < (0.55 if not dx["water"].is_empty() else 0.2): continue
		var s := rng.randf_range(0.7, 1.5)
		var tf := Transform3D(Basis.from_euler(Vector3(0, rng.randf() * TAU, 0)).scaled(Vector3(s, s * rng.randf_range(0.8, 1.3), s)), Vector3(x, 0.0, z))
		if rng.randf() < 0.28: shr.append(tf)
		else: tuf.append(tf)
	_mm(root, dry, lm, tuf, false, "DryGrass"); _mm(root, shrub, lm, shr, false, "Shrubs")
	# ---- غبار تذروه الرياح ----
	if q >= 2 and int(pr["wind"]) > 0: _wind_dust(root, W, D, q, int(pr["wind"]))

static func _dunes(root: Node3D, th: Dictionary, W: float, D: float, q: int) -> void:
	var fn := FastNoiseLite.new(); fn.seed = 77; fn.frequency = 0.0042; fn.fractal_octaves = 3; fn.noise_type = FastNoiseLite.TYPE_SIMPLEX
	var fn2 := FastNoiseLite.new(); fn2.seed = 91; fn2.frequency = 0.012
	var ext := 620.0; var n := 96 if q >= 3 else 64
	var rx := W * 0.5; var rz := D * 0.5
	var hs: Array = []
	for j in n + 1:
		for i in n + 1:
			var x := -rx - ext + (2.0 * (rx + ext)) * float(i) / n; var z := -rz - ext + (2.0 * (rz + ext)) * float(j) / n
			var out := maxf(absf(x) - rx, absf(z) - rz)
			var ramp := smoothstep(6.0, 170.0, out)
			var rg := 1.0 - absf(fn.get_noise_2d(x, z))
			var h := (pow(rg, 2.2) * 34.0 + fn2.get_noise_2d(x, z) * 3.0) * ramp
			if out < 6.0: h = 0.0
			hs.append(h)
	var st := SurfaceTool.new(); st.begin(Mesh.PRIMITIVE_TRIANGLES)
	for j in n:
		for i in n:
			var i0 := j * (n + 1) + i; var i1 := i0 + 1; var i2 := i0 + n + 1; var i3 := i2 + 1
			if maxf(maxf(hs[i0], hs[i1]), maxf(hs[i2], hs[i3])) < 0.03: continue
			var v := func(idx: int) -> Vector3:
				var ii := idx % (n + 1); var jj := idx / (n + 1)
				return Vector3(-rx - ext + (2.0 * (rx + ext)) * float(ii) / n, hs[idx] - 0.03, -rz - ext + (2.0 * (rz + ext)) * float(jj) / n)
			for idx in [i0, i2, i1, i1, i2, i3]: st.add_vertex(v.call(idx))
	st.index(); st.generate_normals()
	var mi := MeshInstance3D.new(); mi.mesh = st.commit(); mi.material_override = ground_mat(th); mi.name = "Dunes"; mi.cast_shadow = GeometryInstance3D.SHADOW_CASTING_SETTING_OFF
	root.add_child(mi)

static func _mesas(root: Node3D, W: float, D: float, rng: RandomNumberGenerator) -> void:
	var tiers: Array = []
	var N := 14
	for i in N:
		var a := TAU * (float(i) + rng.randf() * 0.6) / N
		var rr := rng.randf_range(430.0, 700.0)
		var cx := cos(a) * (W * 0.5 + rr); var cz := sin(a) * (D * 0.5 + rr)
		var w := rng.randf_range(50.0, 120.0); var h := rng.randf_range(24.0, 62.0)
		var y := -2.0
		for t in 3:
			var th := h * (0.45 if t == 0 else 0.3)
			var sc := w * (1.0 - t * 0.2)
			tiers.append(Transform3D(Basis.from_euler(Vector3(0, a + float(t) * 0.2, 0)).scaled(Vector3(sc, th, sc * rng.randf_range(0.7, 1.1))), Vector3(cx, y + th * 0.5, cz)))
			y += th
	var cyl := CylinderMesh.new(); cyl.top_radius = 0.42; cyl.bottom_radius = 0.5; cyl.height = 1.0; cyl.radial_segments = 9; cyl.rings = 1
	_mm(root, cyl, solid_mat(Color(0.66, 0.46, 0.3), 3, 0.0), tiers, false, "Mesas")

static func _wind_dust(root: Node3D, W: float, D: float, q: int, kind := 1) -> void:
	var p := GPUParticles3D.new(); p.name = "WindDust" if kind == 1 else "Snow"
	var pm := ParticleProcessMaterial.new(); pm.emission_shape = ParticleProcessMaterial.EMISSION_SHAPE_BOX; pm.gravity = Vector3.ZERO
	var qm := QuadMesh.new(); var mat := StandardMaterial3D.new()
	mat.shading_mode = BaseMaterial3D.SHADING_MODE_UNSHADED; mat.transparency = BaseMaterial3D.TRANSPARENCY_ALPHA; mat.billboard_mode = BaseMaterial3D.BILLBOARD_ENABLED
	if kind == 1:
		p.amount = 700 if q >= 3 else 350; p.lifetime = 7.0; p.preprocess = 7.0
		p.visibility_aabb = AABB(Vector3(-W * 0.5, -2, -D * 0.5), Vector3(W, 12, D)); p.position = Vector3(0, 1.2, 0)
		pm.emission_box_extents = Vector3(W * 0.5, 2.5, D * 0.5); pm.direction = Vector3(0.85, 0.04, 0.5); pm.spread = 12.0
		pm.initial_velocity_min = 4.0; pm.initial_velocity_max = 9.0; pm.scale_min = 0.5; pm.scale_max = 1.6
		qm.size = Vector2(0.09, 0.09); mat.albedo_color = Color(0.93, 0.82, 0.6, 0.4)
	else:
		p.amount = 1000 if q >= 3 else 500; p.lifetime = 5.0; p.preprocess = 5.0
		p.visibility_aabb = AABB(Vector3(-W * 0.5, -2, -D * 0.5), Vector3(W, 20, D)); p.position = Vector3(0, 12.0, 0)
		pm.emission_box_extents = Vector3(W * 0.5, 1.0, D * 0.5); pm.direction = Vector3(0.35, -1.0, 0.15); pm.spread = 10.0
		pm.initial_velocity_min = 2.5; pm.initial_velocity_max = 4.5; pm.scale_min = 0.6; pm.scale_max = 1.6; pm.turbulence_enabled = true; pm.turbulence_noise_strength = 0.4
		qm.size = Vector2(0.07, 0.07); mat.albedo_color = Color(1, 1, 1, 0.85)
	pm.turbulence_enabled = true
	p.process_material = pm; qm.material = mat; p.draw_pass_1 = qm; p.cast_shadow = GeometryInstance3D.SHADOW_CASTING_SETTING_OFF
	root.add_child(p)
