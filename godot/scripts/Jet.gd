class_name Jet
extends Node3D
# مقاتلة F-35 — فيزياء طيران (دفع + سحب + رفع يعتمد زاوية الهجوم + انهيار)، مدفع، ذكاء اصطناعي.
# محوّلة من نسخة Kotlin القديمة مع نموذج ثلاثي الأبعاد ودخان ولهب احتراق لاحق.

var game: Node
var team := 0
var pilot: Fighter = null
var vel := Vector3.ZERO
var yaw := 0.0
var pitch := 0.0
var roll := 0.0
var th := 0.7
var hp := 300.0
var ab := 0.0
var gun := 600
var gun_cd := 0.0
var gun_f := 0.0
var fire := 0.0
var alive := true
var spare := false
var gnd := false
var stall := false
var ai_t := 0.0
var wob := 0.0
var evade := 0.0
var tgt: Jet = null
var last_by: Fighter = null
var glow: MeshInstance3D
var smoke: CPUParticles3D
var ctl := {"rc": 0.0, "pc": 0.0, "yc": 0.0, "thd": 0.0, "ab": false, "fire": false}
var hum: AudioStreamPlayer3D
var banks: Array = []

const G := 9.8
const KL := 0.00165
const CD0 := 0.00065
const KI := 0.00015

func speed() -> float: return vel.length()
func fwd() -> Vector3: return Vector3(-sin(yaw) * cos(pitch), sin(pitch), -cos(yaw) * cos(pitch))
func human() -> bool: return pilot != null and pilot == game.player

static func make(g: Node, tm: int, parent: Node = null) -> Jet:
	var j := Jet.new(); j.game = g; j.team = tm; j._build(); (parent if parent != null else g.world).add_child(j); return j

func _mat(c: Color, met := 0.55, rough := 0.4) -> StandardMaterial3D:
	var m := StandardMaterial3D.new(); m.albedo_color = c; m.metallic = met; m.roughness = rough; return m

func _part(mesh: Mesh, m: Material, pos: Vector3, rot := Vector3.ZERO, sc := Vector3.ONE) -> MeshInstance3D:
	var n := MeshInstance3D.new(); n.mesh = mesh; n.material_override = m; n.position = pos; n.rotation = rot; n.scale = sc; add_child(n); return n

func _build() -> void:
	var body := _mat(Color(0.42, 0.44, 0.47)); var dark := _mat(Color(0.2, 0.21, 0.24)); var tc := Color(0.9, 0.15, 0.12) if team == 0 else Color(0.15, 0.4, 0.95)
	var mark := _mat(tc, 0.2, 0.6)
	var fus := CapsuleMesh.new(); fus.radius = 0.85; fus.height = 11.0
	_part(fus, body, Vector3(0, 0, 0), Vector3(PI / 2, 0, 0), Vector3(1.0, 1.0, 1.0))
	var nose := CylinderMesh.new(); nose.top_radius = 0.04; nose.bottom_radius = 0.8; nose.height = 4.2
	_part(nose, body, Vector3(0, 0, -7.2), Vector3(-PI / 2, 0, 0))
	var wing := BoxMesh.new(); wing.size = Vector3(5.4, 0.14, 3.6)
	for s in [-1, 1]:
		_part(wing, body, Vector3(3.6 * s, -0.15, 1.2), Vector3(0, 0.38 * s, 0))
		var tail := BoxMesh.new(); tail.size = Vector3(0.12, 2.5, 2.4)
		_part(tail, mark, Vector3(1.0 * s, 1.5, 4.0), Vector3(0, 0, -0.38 * s))
		var stab := BoxMesh.new(); stab.size = Vector3(2.6, 0.1, 1.8)
		_part(stab, body, Vector3(1.9 * s, 0.0, 4.6), Vector3(0, 0.3 * s, 0))
		var intake := BoxMesh.new(); intake.size = Vector3(0.7, 0.9, 2.4)
		_part(intake, dark, Vector3(0.85 * s, -0.25, -1.6))
	var can := SphereMesh.new(); can.radius = 0.6; can.height = 1.2
	var glass := _mat(Color(0.1, 0.25, 0.35, 1.0), 0.9, 0.1)
	_part(can, glass, Vector3(0, 0.62, -2.6), Vector3.ZERO, Vector3(0.75, 0.6, 2.4))
	var nz := CylinderMesh.new(); nz.top_radius = 0.6; nz.bottom_radius = 0.7; nz.height = 1.0
	_part(nz, dark, Vector3(0, 0, 5.8), Vector3(PI / 2, 0, 0))
	var gm := CylinderMesh.new(); gm.top_radius = 0.5; gm.bottom_radius = 0.1; gm.height = 3.0
	var gmat := StandardMaterial3D.new(); gmat.shading_mode = BaseMaterial3D.SHADING_MODE_UNSHADED; gmat.albedo_color = Color(1, 0.55, 0.2, 0.7); gmat.transparency = BaseMaterial3D.TRANSPARENCY_ALPHA
	glow = _part(gm, gmat, Vector3(0, 0, 7.4), Vector3(-PI / 2, 0, 0)); glow.visible = false
	smoke = CPUParticles3D.new(); smoke.amount = 40; smoke.lifetime = 2.5; smoke.emitting = false; smoke.local_coords = false
	smoke.direction = Vector3(0, 1, 0); smoke.spread = 25.0; smoke.initial_velocity_min = 1.0; smoke.initial_velocity_max = 3.0
	var sm := SphereMesh.new(); sm.radius = 0.8; sm.height = 1.6; sm.radial_segments = 6; sm.rings = 3; smoke.mesh = sm
	var smat := StandardMaterial3D.new(); smat.albedo_color = Color(0.1, 0.1, 0.1, 0.7); smat.transparency = BaseMaterial3D.TRANSPARENCY_ALPHA; smat.shading_mode = BaseMaterial3D.SHADING_MODE_UNSHADED
	smoke.material_override = smat; smoke.scale_amount_min = 0.8; smoke.scale_amount_max = 2.2; smoke.position = Vector3(0, 0.5, 3); add_child(smoke)
	var lt := OmniLight3D.new(); lt.light_color = Color(1, 0.6, 0.3); lt.light_energy = 0.0; lt.omni_range = 40.0; lt.position = Vector3(0, 0, 8); lt.name = "ablight"; add_child(lt)
	hum = AudioStreamPlayer3D.new(); hum.stream = Jet.roar(); hum.unit_size = 40.0; hum.max_distance = 700.0; hum.volume_db = -6.0; add_child(hum); hum.pitch_scale = 0.6

func _ready() -> void:
	hum.play()

func _apply_tf() -> void:
	basis = Basis(Vector3.UP, yaw) * Basis(Vector3.RIGHT, pitch) * Basis(Vector3.BACK, roll)

# ---------------------------------------------------------------- فيزياء الطيران
func step(dt: float, rc: float, pc0: float, yc: float, thd: float, aft: bool) -> bool:
	var pc := pc0
	th = clampf(th + thd * 0.6 * dt, 0.0, 1.0); ab += ((1.0 if (aft and th > 0.9) else 0.0) - ab) * minf(1.0, 4.0 * dt)
	var cp := cos(pitch); var sp := sin(pitch); var cy := cos(yaw); var sy := sin(yaw)
	var f := Vector3(-sy * cp, sp, -cy * cp); var r := Vector3(cy, 0, -sy)
	var u0 := r.cross(f) * -1.0   # (r01*f2 - r02*f1, r02*f0 - r00*f2, r00*f1 - r01*f0) = r × f مع r01=0
	u0 = Vector3(0 * f.z - r.z * f.y, r.z * f.x - r.x * f.z, r.x * f.y - 0 * f.x)
	var s := speed(); var thrust := 15.0 * th + 12.0 * ab
	if gnd:
		var hs := Vector2(vel.x, vel.z).length(); var ax := thrust * 0.8 - 0.7 - CD0 * hs * hs; hs = maxf(0.0, hs + ax * dt)
		if th < 0.02: hs = maxf(0.0, hs - 6.0 * dt)
		yaw -= rc * 0.6 * minf(1.0, hs / 18.0) * dt; roll *= exp(-6.0 * dt)
		if hs > 55.0 and pc > 0.1: pitch += pc * 0.5 * dt
		else: pitch = maxf(0.0, pitch * exp(-3.0 * dt))
		pitch = clampf(pitch, 0.0, 0.45)
		vel.x = -sin(yaw) * hs; vel.z = -cos(yaw) * hs
		var L := KL * hs * hs * minf(1.0, pitch * 5.2 + 0.05)
		if L > G: vel.y += (L - G) * dt
		else: vel.y = 0.0
		stall = false
	else:
		var auth := clampf((s - 22.0) / 60.0, 0.12, 1.0)
		roll += rc * 2.4 * dt * auth
		if absf(rc) < 0.05: roll -= roll * 0.8 * dt
		roll = clampf(roll, -1.5, 1.5)
		var cr := cos(roll); var sr := sin(roll)
		var up := u0 * cr - r * sr; var rt := r * cr + u0 * sr
		var vf := vel.dot(f); var vu := vel.dot(up); var vr := vel.dot(rt)
		var aoa := atan2(-vu, maxf(1.0, vf)); var CL := aoa * 5.2; stall = absf(aoa) > 0.30 or s < 40.0
		if absf(aoa) > 0.30:
			var sg := 1.0 if aoa > 0 else -1.0; CL = sg * maxf(0.4, 0.30 * 5.2 - (absf(aoa) - 0.30) * 6.0)
		var al := absf(aoa)
		if al > 0.2 and ((pc > 0 and aoa > 0) or (pc < 0 and aoa < 0)): pc *= clampf((0.30 - al) / 0.10, 0.0, 1.0)
		var q := pc * 0.9 * auth; var ry := yc * 0.25 * auth
		var n := f + up * q * dt + rt * ry * dt
		if s > 15.0: n += (vel / s - f) * 1.3 * dt
		if s < 40.0: n.y -= 0.55 * dt
		n = n.normalized(); pitch = clampf(asin(clampf(n.y, -1.0, 1.0)), -1.35, 1.35); yaw = atan2(-n.x, -n.z)
		var L2 := KL * s * s * CL; var D := (CD0 + KI * CL * CL) * s * s
		var a := f * thrust + up * L2 - rt * vr * 2.5 + Vector3(0, -G, 0)
		if s > 0.5: a -= vel / s * D
		vel += a * dt
	var hw: float = game.map_w * 0.5; var hd: float = game.map_d * 0.5
	var ex := absf(position.x) - hw; var ez := absf(position.z) - hd
	if ex > 0: vel.x -= (1.0 if position.x > 0 else -1.0) * minf(15.0, ex * 0.3) * dt
	if ez > 0: vel.z -= (1.0 if position.z > 0 else -1.0) * minf(15.0, ez * 0.3) * dt
	if position.y > 300.0: vel.y -= (position.y - 300.0) * 0.2 * dt + 20.0 * dt
	var old := position
	position += vel * dt
	position.x = clampf(position.x, -hw - 80.0, hw + 80.0); position.z = clampf(position.z, -hd - 80.0, hd + 80.0)
	if position.y <= 0.0:
		var vd := -vel.y
		if not gnd and (vd > 8.5 or absf(roll) > 0.5 or (pitch < -0.35 and s > 30.0)): return true
		position.y = 0.0
		if vel.y < 0: vel.y = 0.0
		gnd = true
	elif position.y > 0.4: gnd = false
	if position.y > 260.0 and vel.y > 0: vel.y -= 20.0 * dt
	if position.y < 40.0:
		var q := PhysicsRayQueryParameters3D.create(old + Vector3(0, 1.5, 0), position + Vector3(0, 1.5, 0) + vel.normalized() * 2.0, 1)
		if not game.get_world_3d().direct_space_state.intersect_ray(q).is_empty(): return true
	_apply_tf()
	return false

# ---------------------------------------------------------------- المدفع
func shoot(aim: Jet = null) -> void:
	if gun_cd > 0.0 or gun <= 0: return
	gun_cd = 0.045; gun -= 1
	var d := fwd() + Vector3(randf() - 0.5, randf() - 0.5, randf() - 0.5) * 0.016
	if aim != null and aim.alive:
		var er: float = [0.03, 0.02, 0.012][game.diff]
		d = (aim.position + aim.vel * 0.25 - position).normalized() + Vector3(randf() - 0.5, randf() - 0.5, randf() - 0.5) * er
	d = d.normalized()
	var o := position + d * 9.0 + Vector3(0, -0.3, 0)
	var best := 700.0; var hj: Jet = null
	for t in game.jets:
		var T := t as Jet
		if T == self or not T.alive or T.team == team: continue
		var p := T.position + Vector3(0, 1, 0) - o; var tc := p.dot(d)
		if tc < 0 or tc >= best: continue
		if (p - d * tc).length_squared() < 45.0: best = tc; hj = T
	var q := PhysicsRayQueryParameters3D.create(o, o + d * best, 1)
	var h: Dictionary = game.get_world_3d().direct_space_state.intersect_ray(q)
	if not h.is_empty(): best = o.distance_to(h["position"]); hj = null
	elif o.y + d.y * best < 0.0 and d.y < 0: best = maxf(1.0, -o.y / d.y)
	game.jet_tracer(o, o + d * minf(best, 260.0))
	if hj != null:
		hj.hurt(16.0, pilot, true); game._spark(o + d * best, 3)
	if human() or randf() < 0.12: game.sfx_play("shot", position)

func hurt(dmg: float, by: Fighter, inc: bool) -> void:
	if not alive or dmg <= 0.0: return
	hp -= dmg; last_by = by
	if inc and randf() < 0.3: fire = maxf(fire, 5.0)
	if human(): game.hud.flash_damage()
	if by == game.player: game.hud.hitmark()
	if hp <= 0.0: game.jet_die(self, by)

# ---------------------------------------------------------------- ذكاء الطيار الآلي
func ai(dt: float) -> void:
	ai_t -= dt; var skill: float = [0.7, 0.85, 1.0][game.diff]
	if ai_t <= 0.0:
		ai_t = 0.6; var keep := tgt != null and is_instance_valid(tgt) and tgt.alive and tgt.pilot != null and wob > 0.0; wob -= 0.6
		if not keep:
			wob = 7.0; tgt = null; var bs := 1.0e9
			for t in game.jets:
				var T := t as Jet
				if not T.alive or T.team == team or T.pilot == null: continue
				var d := position.distance_to(T.position)
				if d < bs: bs = d; tgt = T
	var tp := Vector3(0, 140, 0); var have := tgt != null and is_instance_valid(tgt) and tgt.alive; var dist := 1.0e9; var cosA := -1.0
	var f := fwd()
	if have:
		var lead := minf(1.2, Vector2(tgt.position.x - position.x, tgt.position.z - position.z).length() / 300.0); tp = tgt.position + tgt.vel * lead
		var dv := tp - position; dist = dv.length(); cosA = dv.dot(f) / (dist + 1e-4)
	var W2: float = game.map_w * 0.5 - 260.0; var D2: float = game.map_d * 0.5 - 220.0; var oob := absf(position.x) > W2 or absf(position.z) > D2
	if oob or not have:
		if oob: tp.x = 0.0; tp.z = 0.0
		if not have and not oob: tp.x = (-1.0 if team != 0 else 1.0) * 200.0; tp.z = 0.0
		tp.y = maxf(tp.y, 130.0)
	tp.y = maxf(tp.y, 95.0); var dx := tp.x - position.x; var dy := tp.y - position.y; var dz := tp.z - position.z
	var hdist := sqrt(dx * dx + dz * dz) + 1e-3; var ey := wrapf(atan2(-dx, -dz) - yaw, -PI, PI); var des := clampf(atan2(dy, hdist), -0.3, 0.35)
	if position.y > 220.0: des = minf(des, -0.15)
	if position.y < 90.0: des = maxf(des, 0.1)
	var turn_m := absf(ey) > 0.25 and position.y > 100.0
	var rmax := 0.5 if position.y < 100.0 else clampf(1.3 + vel.y * 0.02, 0.8, 1.45)
	if absf(ey) > 2.4:
		if evade == 0.0: evade = 1.0 if ey > 0 else -1.0
		ey = evade * 2.4
	else: evade = 0.0
	var rl := clampf(ey * 2.2, -rmax, rmax); var rc: float = clampf((rl - roll) * 2.5, -1.0, 1.0) * skill
	var cr := maxf(0.4, cos(roll))
	var pc: float = (((1.0 if absf(roll) > 0.7 else 0.4) if turn_m else clampf((des - pitch) * 2.2 + (1.0 / cr - 1.0) * 0.6, -1.0, 1.0))) * skill
	if position.y > 190.0: pc = minf(pc, 0.3 - (position.y - 190.0) * 0.02)
	if pitch < -0.45: pc = 1.0
	if pitch > 0.5: pc = minf(pc, -0.1)
	if vel.y < 0 and position.y < vel.y * vel.y / 20.0 + 70.0 and not gnd: pc = 1.0; rc = clampf(-roll * 2.0, -1.0, 1.0)
	if position.y < 22.0: pc = 1.0; rc = 0.0
	var s := speed(); var thd := 1.0 if s < 105.0 else (-1.0 if s > 135.0 else 0.0); var aft: bool = have and dist > 700.0 and game.diff > 0
	if stall and position.y > 120.0 and pitch > 0.0: pc = -0.4; rc = 0.0; thd = 1.0
	if position.y < 60.0 and vel.y < -12.0: vel.y *= 1.0 - 3.0 * dt
	if step(dt, rc, pc, 0.0, thd, aft): game.jet_die(self, null); return
	if position.y < 24.0 and not gnd:
		position.y = 24.0
		if vel.y < 0: vel.y = 0.0
		if pitch < 0: pitch *= 0.5
	if have and dist < 650.0 and cosA > 0.8 and randf() < (0.5 if game.diff == 0 else 0.9): shoot(tgt)

# ---------------------------------------------------------------- الحلقة
func _physics_process(dt: float) -> void:
	if not alive: return
	if gun_cd > 0.0: gun_cd -= dt
	elif gun < 600: gun_f += dt * (10.0 if human() else 40.0)
	if gun_f >= 1.0: var add := int(gun_f); gun = mini(600, gun + add); gun_f -= add
	if human():
		var c := ctl
		if step(dt, c["rc"], c["pc"], c["yc"], c["thd"], c["ab"]): game.jet_die(self, null); return
		if c["fire"]: shoot()
	elif pilot != null: ai(dt)
	elif not (gnd and speed() < 1.0):
		if step(dt, 0, 0, 0, 0, false): game.jet_die(self, null); return
	else: _apply_tf()
	if fire > 0.0:
		fire -= dt; hp -= 12.0 * dt
		if hp <= 0.0: game.jet_die(self, last_by); return
	smoke.emitting = fire > 0.0 or hp < 120.0
	glow.visible = ab > 0.2 or th > 0.6; glow.scale = Vector3(1, 1.0 + ab * 1.2, 1) * (0.7 + th * 0.3)
	(get_node("ablight") as OmniLight3D).light_energy = ab * 3.0
	hum.pitch_scale = 0.55 + th * 0.5 + ab * 0.25 + speed() / 400.0
	if pilot != null and not human(): pilot.global_position = position

static var _roar: AudioStreamWAV = null
static func roar() -> AudioStreamWAV:
	if _roar != null: return _roar
	var rate := 22050; var n := rate * 2; var b := PackedByteArray(); b.resize(n * 2)
	var lp := 0.0; var lp2 := 0.0
	for i in n:
		var x := randf() * 2.0 - 1.0
		lp += (x - lp) * 0.12; lp2 += (lp - lp2) * 0.25
		var v := lp2 * 2.2 + sin(float(i) / rate * TAU * 62.0) * 0.18
		var fade := minf(1.0, minf(float(i), float(n - i)) / 800.0)
		b.encode_s16(i * 2, int(clampf(v * fade, -1.0, 1.0) * 14000.0))
	var w := AudioStreamWAV.new(); w.format = AudioStreamWAV.FORMAT_16_BITS; w.mix_rate = rate; w.data = b; w.loop_mode = AudioStreamWAV.LOOP_FORWARD; w.loop_begin = 0; w.loop_end = n
	_roar = w; return w
