class_name Fighter
extends CharacterBody3D
# أساس اللاعب والبوتات: صحة، فريق، سلاح (إطلاق Hitscan بانتشار وارتداد وسقوط ضرر حسب المدى)

var game: Node
var team := 0
var hp := 100.0
var alive := true
var wi := 0
var ammo := 30
var reserve := 120
var cd := 0.0
var reload_t := 0.0
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
	var w: Dictionary = game.weapons[i]
	ammo = int(w["mag"]); reserve = int(w["mag"]) * int(w["mags"])
	cd = 0.0; reload_t = 0.0

func weapon() -> Dictionary:
	return game.weapons[wi]

func eye_pos() -> Vector3:
	return global_position + Vector3(0, EYE, 0)

func _physics_process(dt: float) -> void:
	cd = maxf(0.0, cd - dt)
	spawn_prot = maxf(0.0, spawn_prot - dt)
	recoil = maxf(0.0, recoil - dt * 3.0)
	if reload_t > 0.0:
		reload_t -= dt
		if reload_t <= 0.0:
			var w := weapon(); var need := int(w["mag"]) - ammo; var take := mini(need, reserve)
			ammo += take; reserve -= take

func start_reload() -> void:
	var w := weapon()
	if reload_t > 0.0 or ammo >= int(w["mag"]) or reserve <= 0: return
	reload_t = float(w["reload"])
	if self == game.player: game.sfx_play("reload", global_position)

func try_fire(origin: Vector3, dir: Vector3, ads: bool, extra_spread: float) -> bool:
	if not alive or cd > 0.0 or reload_t > 0.0: return false
	if ammo <= 0:
		start_reload(); return false
	var w := weapon()
	ammo -= 1; cd = 60.0 / float(w["rpm"])
	recoil = minf(1.0, recoil + 0.35)
	var spread: float = (float(w["ads"]) if ads else float(w["hip"])) + extra_spread + recoil * 0.004
	for p in int(w["pellets"]):
		var d := dir
		var right := dir.cross(Vector3.UP).normalized()
		var up := right.cross(dir).normalized()
		d = (dir + right * randf_range(-1, 1) * spread + up * randf_range(-1, 1) * spread).normalized()
		game.hitscan(self, origin, d, w)
	game.sfx_play("shot", origin)
	if ammo <= 0 and reserve > 0: start_reload()
	return true

func take_damage(d: float, src: Fighter, head: bool) -> void:
	if not alive or spawn_prot > 0.0: return
	if game.lan != null and game.lan.role == 2: return  # العميل لا يطبّق الضرر: المضيف هو الحَكَم
	hp -= d
	if self == game.player: game.hud.flash_damage()
	if hp <= 0.0:
		alive = false; deaths += 1
		if veh != null: (veh as Vehicle).leave(self)
		if src != null and src != self: src.kills += 1
		game.on_kill(src, self, head)
