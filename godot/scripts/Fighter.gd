class_name Fighter
extends CharacterBody3D
# أساس اللاعب والبوتات: صحة، فريق، سلاح (إطلاق Hitscan بانتشار وارتداد وسقوط ضرر حسب المدى)

var game: Node
var team := 0
var hp := 100.0
var alive := true
var wi := 0
var ammo := 30
var reload_tot := 1.0
var rl_empty := false
var rl_shell_n := 0
var reserve := 120
var cd := 0.0
var reload_t := 0.0
var air_vy := 0.0
var was_air := false
var reveal := 0.0
var jet: Node = null
var kills := 0
var deaths := 0
var fname := "BOT"
var recoil := 0.0
var spawn_prot := 0.0
var veh: Node = null   # المركبة التي أركبها
var veh_seat := 0
var spawn_seq := 0   # يزيد مع كل ولادة (لمزامنة إعادة الظهور عبر الشبكة)
const EYE := 1.62

func setup_weapon(i: int) -> void:
	wi = i
	var w: Dictionary = game.mod_weapon(i) if self == game.player else game.weapons[i]
	ammo = int(w["mag"]); reserve = int(w["mag"]) * int(w["mags"])
	cd = 0.0; reload_t = 0.0

func weapon() -> Dictionary:
	if self == game.player: return game.mod_weapon(wi)
	return game.weapons[wi]

func eye_pos() -> Vector3:
	return global_position + Vector3(0, EYE, 0)

func _physics_process(dt: float) -> void:
	cd = maxf(0.0, cd - dt)
	spawn_prot = maxf(0.0, spawn_prot - dt); reveal = maxf(0.0, reveal - dt)
	recoil = maxf(0.0, recoil - dt * 3.0)
	if reload_t > 0.0:
		reload_t -= dt
		var w := weapon()
		if String(w["kind"]) == "shotgun":  # قذيفة قذيفة (0.6 ث لكل واحدة) ويمكن مقاطعتها بالإطلاق
			var due := int(maxf(0.0, reload_tot - reload_t - 0.45) / 0.6) + (99 if reload_t <= 0.0 else 0)
			while rl_shell_n < due and ammo < int(w["mag"]) and reserve > 0: ammo += 1; reserve -= 1; rl_shell_n += 1
		elif reload_t <= 0.0:
			var need := int(w["mag"]) - ammo; var take := mini(need, reserve)
			ammo += take; reserve -= take

func start_reload() -> void:
	var w := weapon()
	if reload_t > 0.0 or ammo >= int(w["mag"]) or reserve <= 0: return
	rl_empty = ammo <= 0; rl_shell_n = 0
	if String(w["kind"]) == "shotgun": reload_tot = 0.45 + 0.6 * mini(int(w["mag"]) - ammo, reserve)
	else: reload_tot = float(w["reload"]) * (1.0 if rl_empty else 0.8)  # تعبئة تكتيكية أسرع من الفارغة
	reload_t = reload_tot
	if String(w["kind"]) != "shotgun": game.sfx_reload_seq(global_position, reload_tot, rl_empty)
	elif self == game.player: game.sfx_play("reload", global_position)

func try_fire(origin: Vector3, dir: Vector3, ads: bool, extra_spread: float) -> bool:
	if not alive or cd > 0.0: return false
	if reload_t > 0.0:
		if String(weapon()["kind"]) == "shotgun" and ammo > 0: reload_t = 0.0  # مقاطعة تعبئة الشوتغن
		else: return false
	if ammo <= 0:
		if reserve <= 0 and self == game.player: game.sfx_play("dry", global_position, -2.0)
		start_reload(); return false
	var w := weapon()
	ammo -= 1; cd = 60.0 / float(w["rpm"])
	recoil = minf(1.0, recoil + 0.35)
	var spread: float = (float(w["ads"]) if ads else float(w["hip"])) + extra_spread + recoil * 0.004 * float(w.get("recm", 1.0))
	for p in int(w["pellets"]):
		var d := dir
		var right := dir.cross(Vector3.UP).normalized()
		var up := right.cross(dir).normalized()
		d = (dir + right * randf_range(-1, 1) * spread + up * randf_range(-1, 1) * spread).normalized()
		game.hitscan(self, origin, d, w)
	game.sfx_shot(origin, self == game.player, -4.0 if self != game.player else 0.0)
	if ammo <= 0 and reserve > 0: start_reload()
	return true

func track_fall() -> void:
	# ضرر السقوط: نحو 10.5 م/ث بلا ضرر، ويقتل عند ~17.5 م/ث (حوالي 8.5 م ارتفاع)
	if is_on_floor():
		if was_air and air_vy < -10.5 and alive and jet == null and veh == null:
			var dmg := (-air_vy - 10.5) * 14.0
			if self == game.player: game.hud.say(Settings.t("سقوط من ارتفاع!"))
			game.sfx_play("hit", global_position)
			take_damage(dmg, null, false)
		was_air = false; air_vy = 0.0
	else:
		was_air = true; air_vy = minf(air_vy, velocity.y)

func take_damage(d: float, src: Fighter, head: bool) -> void:
	if not alive or spawn_prot > 0.0: return
	if game.lan != null and game.lan.role == 2: return  # العميل لا يطبّق الضرر: المضيف هو الحَكَم
	hp -= d
	if self == game.player:
		game.hud.flash_damage()
		if src != null and src != self: game.hud.hit_from(src.global_position)
	if hp <= 0.0:
		alive = false; deaths += 1
		if veh != null: (veh as Vehicle).leave(self)
		if src != null and src != self: src.kills += 1
		game.on_kill(src, self, head)
