class_name Gyro
extends RefCounted
# تصويب بالجيروسكوب. Godot على أندرويد/iOS يعطي القيم في إحداثيات الشاشة الحالية (x يمين، y أعلى، z خارج الشاشة)
# فلا حاجة لتخمين اتجاه الهاتف. الدوران الرأسي (yaw) يُحسب حول محور الجاذبية الحقيقي (لا محور الشاشة) كي يبقى صحيحاً
# حتى لو مال الهاتف للأعلى/الأسفل؛ والدوران العمودي (pitch) حول المحور الأفقي للشاشة (x).
# + معايرة انحراف (bias) تلقائية أثناء السكون، ومنطقة ميتة، وتنعيم خفيف يختفي عند الحركة السريعة.
static var bias := Vector3.ZERO
static var _calib_t := 0.0
static var _calib_sum := Vector3.ZERO
static var _calib_n := 0
static var _sm := Vector2.ZERO
const DEAD := 0.012          # rad/s: ما دونه ضجيج المستشعر
const STILL := 0.06          # rad/s: تحت هذا يعدّ الهاتف ساكناً (يُعايَر الانحراف)

# معايرة يدوية: ثبّت الهاتف ثانية واحدة
static func calibrate() -> void:
	_calib_t = 1.0; _calib_sum = Vector3.ZERO; _calib_n = 0

# يرجع (yaw, pitch) بالراديان لهذا الإطار، قبل الحساسية.
static func step(dt: float, g_raw: Vector3, grav: Vector3) -> Vector2:
	var g := g_raw - bias
	# معايرة: أثناء المعايرة اليدوية أو إن كان الجهاز ساكناً جداً نحدّث الانحراف ببطء
	if _calib_t > 0.0:
		_calib_t -= dt; _calib_sum += g_raw; _calib_n += 1
		if _calib_t <= 0.0 and _calib_n > 0: bias = _calib_sum / float(_calib_n)
		return Vector2.ZERO
	if g.length() < STILL: bias = bias.lerp(g_raw, clampf(dt * 0.5, 0.0, 1.0)); g = g_raw - bias
	# محور الجاذبية (للأعلى): نقرأه من المستشعر مع توحيد الإشارة بحيث يكون "أعلى الشاشة" موجباً
	var yaw_rate := g.y
	if grav.length() > 3.0:
		var u := grav.normalized()
		if u.y < 0.0: u = -u
		yaw_rate = g.dot(u)
	var pitch_rate := g.x
	var r := Vector2(yaw_rate, pitch_rate)
	for i in 2:
		var a := absf(r[i])
		r[i] = 0.0 if a < DEAD else signf(r[i]) * (a - DEAD)
	# تنعيم: قوي عند الحركة البطيئة (يزيل الرجفة) وصفر عند السريعة (بلا تأخير)
	var k := clampf(r.length() / 0.6, 0.0, 1.0)
	_sm = _sm.lerp(r, clampf(0.35 + 0.65 * k, 0.0, 1.0))
	return _sm * dt
