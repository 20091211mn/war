class_name Lan
extends Node
# لعب جماعي على الشبكة المحلية (LAN) بين أي أجهزة: iOS وأندرويد وسطح المكتب.
# المضيف هو الحَكَم (خادم موثوق): يحاكي البوتات والإصابات والنقاط ويبثّ لقطات 15 مرة/ثانية؛ العميل يرسل حالته 20 مرة/ثانية وطلقاته.
# الاكتشاف: بثّ UDP (broadcast) + مسح مباشر لشبكة /24 (لأن iOS يمنع استقبال البثّ بدون صلاحيات خاصة).

const GPORT := 9330   # منفذ اللعب (ENet)
const DPORT := 9331   # منفذ الاكتشاف (UDP)
const NAMES := ["Falcon", "Lion", "Wolf", "Eagle", "Tiger", "Cobra", "Viper", "Shark"]
const LIMS := [10, 20, 30, 50]
const MINS := [0, 5, 10, 15]
const BOTS := [0, 2, 4, 6]

var game: Node
var role := 0                      # 0 بلا شبكة، 1 مضيف، 2 عميل
var room := {"name": 0, "maxp": 6, "lim": 2, "mins": 0, "bots": 2, "style": 0}   # فهارس الخيارات
var my_slot := 0
var peer_slot := {}                # peer id -> خانة
var hosts: Array = []              # الغرف المكتشفة
var status := ""
var host_root: Control
var join_root: Control
var _host_rows: Array = []
var _join_list: VBoxContainer
var _join_status: Label
var _disc: PacketPeerUDP           # مقبس الاكتشاف (مضيف: يرد على المسح؛ عميل: يمسح ويستمع)
var _disc_b: PacketPeerUDP         # عميل: استماع لبثّ المضيفين على DPORT
var _scan_on := false
var _sweep := 0
var _t_beacon := 0.0
var _t_snap := 0.0
var _t_state := 0.0
var _t_list := 0.0
var _prev_hp := 100.0
var _seq := {}
var _ip_base := ""
var _my_ip := ""
var match_id := 0
var last_cfg := {}

# ------------------------------------------------------------ إعدادات الغرفة
func maxp_e() -> int: return 2 if int(room["style"]) == 1 else 2 + int(room["maxp"])
func bots_e() -> int: return 0 if int(room["style"]) == 1 else mini(BOTS[int(room["bots"])], maxp_e() - 1)
func cfg() -> Dictionary:
	return {"map": game.map_idx, "n": maxp_e(), "lim": LIMS[int(room["lim"])], "mins": MINS[int(room["mins"])], "bots": bots_e(), "mid": match_id, "name": NAMES[int(room["name"])]}

func screens() -> Array: return [host_root, join_root]

# ------------------------------------------------------------ واجهة
func add_menu_buttons(panel: Control) -> void:
	var row := HBoxContainer.new(); row.add_theme_constant_override("separation", 10); panel.add_child(row)
	var b1 := Button.new(); b1.custom_minimum_size = Vector2(0, 66); b1.size_flags_horizontal = Control.SIZE_EXPAND_FILL; b1.add_theme_font_size_override("font_size", 22); b1.text = "LAN · " + Settings.t("إنشاء غرفة"); UiTheme.style_ghost(b1); UiTheme.juicy(b1)
	b1.pressed.connect(show_host); row.add_child(b1)
	var b2 := Button.new(); b2.custom_minimum_size = Vector2(0, 66); b2.size_flags_horizontal = Control.SIZE_EXPAND_FILL; b2.add_theme_font_size_override("font_size", 22); b2.text = "LAN · " + Settings.t("الانضمام"); UiTheme.style_ghost(b2); UiTheme.juicy(b2)
	b2.pressed.connect(show_join); row.add_child(b2)

func _screen(title: String) -> VBoxContainer:
	var root := Control.new(); root.set_anchors_preset(Control.PRESET_FULL_RECT); root.visible = false; game.ui_root.add_child(root)
	var dim := ColorRect.new(); dim.color = Color(0.03, 0.05, 0.09, 0.9); dim.set_anchors_preset(Control.PRESET_FULL_RECT); root.add_child(dim)
	var cc := CenterContainer.new(); cc.set_anchors_preset(Control.PRESET_FULL_RECT); root.add_child(cc)
	var vb: VBoxContainer = game._card(cc, 640)
	var t: Node = game._glass_label(title, 40); vb.add_child(t)
	vb.set_meta("root", root)
	return vb

func build_ui() -> void:
	# ---- شاشة إنشاء غرفة ----
	var hv := _screen("LAN · " + Settings.t("إنشاء غرفة")); host_root = hv.get_meta("root")
	var defs := [["اسم الغرفة", "name", 8], ["أقصى لاعبين", "maxp", 7], ["حد النقاط", "lim", 4], ["الوقت", "mins", 4], ["البوتات", "bots", 4], ["النمط", "style", 2]]
	for d in defs:
		var key: String = d[1]; var cnt: int = d[2]
		var box := VBoxContainer.new(); box.add_theme_constant_override("separation", 4); hv.add_child(box)
		var lab := Label.new(); lab.add_theme_font_size_override("font_size", 18); lab.add_theme_color_override("font_color", UiTheme.MUTED); box.add_child(lab)
		var hb := HBoxContainer.new(); hb.add_theme_constant_override("separation", 8); box.add_child(hb)
		var bp := Button.new(); bp.text = "◀"; bp.custom_minimum_size = Vector2(72, 58); UiTheme.style_ghost(bp); UiTheme.juicy(bp); hb.add_child(bp)
		var val := Label.new(); val.size_flags_horizontal = Control.SIZE_EXPAND_FILL; val.horizontal_alignment = HORIZONTAL_ALIGNMENT_CENTER; val.add_theme_font_size_override("font_size", 28); hb.add_child(val)
		var bn := Button.new(); bn.text = "▶"; bn.custom_minimum_size = Vector2(72, 58); UiTheme.style_ghost(bn); UiTheme.juicy(bn); hb.add_child(bn)
		bp.pressed.connect(func(): room[key] = (int(room[key]) + cnt - 1) % cnt; _refresh_host())
		bn.pressed.connect(func(): room[key] = (int(room[key]) + 1) % cnt; _refresh_host())
		_host_rows.append({"lab": lab, "val": val, "key": key, "ar": d[0]})
	var go := Button.new(); go.custom_minimum_size = Vector2(0, 84); go.add_theme_font_size_override("font_size", 30); UiTheme.style_accent(go); UiTheme.juicy(go); go.text = Settings.t("أنشئ الغرفة وابدأ"); go.pressed.connect(host_start); hv.add_child(go)
	var back := Button.new(); back.custom_minimum_size = Vector2(0, 64); back.add_theme_font_size_override("font_size", 24); UiTheme.style_ghost(back); UiTheme.juicy(back); back.text = Settings.t("رجوع"); back.pressed.connect(func(): host_root.visible = false); hv.add_child(back)
	# ---- شاشة الانضمام ----
	var jv := _screen("LAN · " + Settings.t("الانضمام")); join_root = jv.get_meta("root")
	_join_status = Label.new(); _join_status.add_theme_font_size_override("font_size", 20); _join_status.add_theme_color_override("font_color", UiTheme.MUTED); jv.add_child(_join_status)
	_join_list = VBoxContainer.new(); _join_list.add_theme_constant_override("separation", 10); jv.add_child(_join_list)
	var jb := Button.new(); jb.custom_minimum_size = Vector2(0, 64); jb.add_theme_font_size_override("font_size", 24); UiTheme.style_ghost(jb); UiTheme.juicy(jb); jb.text = Settings.t("رجوع"); jb.pressed.connect(func(): _scan_stop(); join_root.visible = false); jv.add_child(jb)
	_refresh_host()

func _refresh_host() -> void:
	var st := int(room["style"])
	for r in _host_rows:
		var key: String = r["key"]; var v := int(room[key]); var txt := ""
		match key:
			"name": txt = NAMES[v]
			"maxp": txt = str(maxp_e())
			"lim": txt = str(LIMS[v])
			"mins": txt = ("%d min" % MINS[v]) if MINS[v] > 0 else Settings.t("بلا حد")
			"bots": txt = str(bots_e()) if bots_e() > 0 else Settings.t("بدون بوتات")
			"style": txt = Settings.t("مبارزة 1 ضد 1") if st == 1 else Settings.t("فرق")
		(r["lab"] as Label).text = Settings.t(r["ar"]); (r["val"] as Label).text = txt

func show_host() -> void:
	_refresh_host(); host_root.visible = true; join_root.visible = false

func show_join() -> void:
	host_root.visible = false; join_root.visible = true; _scan_start(); _rebuild_list()

# ------------------------------------------------------------ مضيف
func host_start() -> bool:
	leave()
	var pr := ENetMultiplayerPeer.new()
	var err := pr.create_server(GPORT, maxi(1, maxp_e() - 1))
	if err != OK:
		status = "ERR %d" % err; push_warning("LAN: create_server failed %d" % err); return false
	multiplayer.multiplayer_peer = pr; role = 1; my_slot = 0; match_id += 1; peer_slot.clear(); _seq.clear()
	_hook()
	_disc = PacketPeerUDP.new(); _disc.set_broadcast_enabled(true); _disc.bind(DPORT, "*")
	last_cfg = cfg()
	game.start_lan_match(1, 0, last_cfg)
	return true

func host_restart() -> void:
	if role != 1: return
	match_id += 1; last_cfg = cfg(); last_cfg["map"] = game.map_idx
	game.start_lan_match(1, 0, last_cfg)
	var used := peer_slot.duplicate()
	for pid in used: _attach_human(int(pid), int(used[pid]), true)
	for pid in used: rpc_id(int(pid), "n_restart", int(used[pid]), last_cfg)

func _hook() -> void:
	if not multiplayer.peer_connected.is_connected(_on_conn): multiplayer.peer_connected.connect(_on_conn)
	if not multiplayer.peer_disconnected.is_connected(_on_disc): multiplayer.peer_disconnected.connect(_on_disc)
	if not multiplayer.connected_to_server.is_connected(_on_connected): multiplayer.connected_to_server.connect(_on_connected)
	if not multiplayer.connection_failed.is_connected(_on_failed): multiplayer.connection_failed.connect(_on_failed)
	if not multiplayer.server_disconnected.is_connected(_on_lost): multiplayer.server_disconnected.connect(_on_lost)

func _on_conn(_pid: int) -> void: pass

func _on_disc(pid: int) -> void:
	if role != 1 or not peer_slot.has(pid): return
	var slot: int = peer_slot[pid]; peer_slot.erase(pid); _release(slot)
	send_feed("P%d ✕ LEFT" % slot)

# خانة لاعب جديد: الخانات الفارغة أولاً ثم استبدال بوت؛ مع موازنة الفرق
func _pick_slot() -> int:
	var f: Array = game.fighters
	var cnt := [0, 0]
	for s in f.size():
		var b := f[s] as Bot
		if s == 0 or (b != null and b.net_ctrl): cnt[(f[s] as Fighter).team] += 1
	var pref := 0 if cnt[0] <= cnt[1] else 1
	for pk in 2:
		for pass_i in 2:
			for s in range(1, f.size()):
				var b := f[s] as Bot
				if b == null or b.net_ctrl: continue
				if (pk == 0) != b.off: continue
				if b.team == (pref if pass_i == 0 else 1 - pref): return s
	return -1

func _attach_human(pid: int, slot: int, keep := false) -> void:
	var b := game.fighters[slot] as Bot
	b.off = false; b.net_ctrl = true; b.human = true; b.target = null; b.fname = "P%d" % slot; b.visible = true
	b.global_position = game.spawn_point(b.team); b.net_pos = b.global_position; b.hp = 100.0; b.alive = true; b.spawn_prot = 2.0; b.spawn_seq += 1
	b.setup_weapon(0); b.kills = 0; b.deaths = 0
	peer_slot[pid] = slot

func _release(slot: int) -> void:
	if slot <= 0 or slot >= game.fighters.size(): return
	var b := game.fighters[slot] as Bot
	b.net_ctrl = false; b.human = false
	if last_cfg.get("bots", 0) > 0 and slot <= int(last_cfg["bots"]):
		b.respawn(game.spawn_point(b.team))
	else:
		b.off = true; b.alive = false; b.visible = false; b.global_position = Vector3(0, -50, 0)

func respawn_human(b: Bot) -> void:
	b.global_position = game.spawn_point(b.team); b.net_pos = b.global_position; b.hp = 100.0; b.alive = true; b.visible = true; b.spawn_prot = 2.0; b.spawn_seq += 1; b.setup_weapon(b.wi)

@rpc("any_peer", "reliable")
func n_hello(_v: int) -> void:
	if role != 1: return
	var pid := multiplayer.get_remote_sender_id()
	var slot := _pick_slot()
	if slot < 0:
		rpc_id(pid, "n_full"); return
	_attach_human(pid, slot)
	rpc_id(pid, "n_welcome", slot, last_cfg)
	send_feed("P%d ✚ JOINED" % slot)

@rpc("authority", "reliable")
func n_full() -> void:
	status = Settings.t("الغرفة ممتلئة"); leave(); _show_status()

@rpc("authority", "reliable")
func n_welcome(slot: int, c: Dictionary) -> void:
	my_slot = slot; last_cfg = c; _scan_stop()
	game.start_lan_match(2, slot, c)

@rpc("authority", "reliable")
func n_restart(slot: int, c: Dictionary) -> void:
	my_slot = slot; last_cfg = c
	game.start_lan_match(2, slot, c)

# ------------------------------------------------------------ عميل
func join(ip: String) -> void:
	leave()
	var pr := ENetMultiplayerPeer.new()
	var err := pr.create_client(ip, GPORT)
	if err != OK: status = "ERR %d" % err; _show_status(); return
	multiplayer.multiplayer_peer = pr; role = 2; _hook(); status = Settings.t("جارٍ الاتصال…"); _show_status()

func _on_connected() -> void:
	if role == 2: rpc_id(1, "n_hello", 1)

func _on_failed() -> void:
	status = Settings.t("تعذّر الاتصال"); leave(); _show_status()

func _on_lost() -> void:
	status = Settings.t("انقطع الاتصال بالمضيف"); var was := role == 2; leave()
	if was and game.state != "menu": game._show_menu(); show_join(); _show_status()

func leave() -> void:
	if multiplayer.multiplayer_peer != null and not (multiplayer.multiplayer_peer is OfflineMultiplayerPeer):
		multiplayer.multiplayer_peer.close()
	multiplayer.multiplayer_peer = OfflineMultiplayerPeer.new()
	role = 0; peer_slot.clear()
	if _disc != null and role == 0 and not _scan_on: _disc.close(); _disc = null

func _show_status() -> void:
	if _join_status != null: _join_status.text = status

# ------------------------------------------------------------ اكتشاف الغرف
func _local_ip() -> String:
	for a in IP.get_local_addresses():
		var s := String(a)
		if s.begins_with("192.168.") or s.begins_with("10.") or (s.begins_with("172.") and s.split(".").size() == 4 and int(s.split(".")[1]) >= 16 and int(s.split(".")[1]) <= 31): return s
	return ""

func _scan_start() -> void:
	hosts.clear(); _scan_on = true; _sweep = 1; status = Settings.t("جارٍ البحث عن غرف…")
	_my_ip = _local_ip(); _ip_base = ""
	if _my_ip != "": var p := _my_ip.split("."); _ip_base = "%s.%s.%s." % [p[0], p[1], p[2]]
	if _disc != null: _disc.close()
	_disc = PacketPeerUDP.new(); _disc.set_broadcast_enabled(true); _disc.bind(0, "*")
	_disc_b = PacketPeerUDP.new(); _disc_b.set_broadcast_enabled(true)
	if _disc_b.bind(DPORT, "*") != OK: _disc_b = null
	_show_status()

func _scan_stop() -> void:
	_scan_on = false
	if _disc != null and role != 1: _disc.close(); _disc = null
	if _disc_b != null: _disc_b.close(); _disc_b = null

func _beacon() -> PackedByteArray:
	var c := last_cfg
	return JSON.stringify({"v": 1, "n": c.get("name", "WZ"), "m": c.get("map", 0), "p": peer_slot.size() + 1, "x": c.get("n", 2), "l": c.get("lim", 30), "t": c.get("mins", 0), "b": c.get("bots", 0), "s": int(room["style"])}).to_utf8_buffer()

func _poll(sock: PacketPeerUDP) -> void:
	while sock != null and sock.get_available_packet_count() > 0:
		var ip := sock.get_packet_ip(); var port := sock.get_packet_port(); var pk := sock.get_packet()
		var txt := pk.get_string_from_utf8()
		if role == 1 and sock == _disc and txt == "WZ?":
			sock.set_dest_address(ip, port); sock.put_packet(_beacon())
		elif _scan_on and txt.begins_with("{"):
			var d: Variant = JSON.parse_string(txt)
			if d is Dictionary and int(d.get("v", 0)) == 1: _add_host(ip, d)

func _add_host(ip: String, d: Dictionary) -> void:
	for h in hosts:
		if h["ip"] == ip: h["info"] = d; h["seen"] = 0.0; return
	hosts.append({"ip": ip, "info": d, "seen": 0.0})

func _rebuild_list() -> void:
	for c in _join_list.get_children(): c.queue_free()
	if hosts.is_empty():
		var l := Label.new(); l.text = "…"; l.add_theme_font_size_override("font_size", 22); _join_list.add_child(l)
	for h in hosts:
		var d: Dictionary = h["info"]; var ip: String = h["ip"]
		var b := Button.new(); b.custom_minimum_size = Vector2(0, 76); b.add_theme_font_size_override("font_size", 22); UiTheme.style_accent(b); UiTheme.juicy(b)
		var tag := (" · " + Settings.t("مبارزة 1 ضد 1")) if int(d.get("s", 0)) == 1 else ((" · " + Settings.t("البوتات")) if int(d.get("b", 0)) > 0 else "")
		b.text = "%s   %d/%d   %dK%s%s" % [d.get("n", "?"), int(d.get("p", 1)), int(d.get("x", 2)), int(d.get("l", 30)), (" · %dm" % int(d.get("t", 0))) if int(d.get("t", 0)) > 0 else "", tag]
		b.pressed.connect(func(): join(ip)); _join_list.add_child(b)
	_show_status()

# ------------------------------------------------------------ الحلقة
func _process(dt: float) -> void:
	_poll(_disc); _poll(_disc_b)
	if role == 1 and _disc != null:
		_t_beacon -= dt
		if _t_beacon <= 0.0:
			_t_beacon = 1.0
			for dst in ["255.255.255.255", (_ip_base_of_host() + "255")]:
				_disc.set_dest_address(dst, DPORT); _disc.put_packet(_beacon())
	if _scan_on and _disc != null:
		for i in 10:
			if _ip_base != "" and _sweep <= 254:
				var ip := _ip_base + str(_sweep); _sweep += 1
				if ip != _my_ip: _disc.set_dest_address(ip, DPORT); _disc.put_packet("WZ?".to_utf8_buffer())
		_t_list -= dt
		if _t_list <= 0.0:
			_t_list = 0.8; _disc.set_dest_address("127.0.0.1", DPORT); _disc.put_packet("WZ?".to_utf8_buffer())
			_disc.set_dest_address("255.255.255.255", DPORT); _disc.put_packet("WZ?".to_utf8_buffer())
			if _sweep > 254: _sweep = 1
			for h in hosts: h["seen"] += 0.8
			hosts = hosts.filter(func(h): return h["seen"] < 5.0)
			_rebuild_list()

func _ip_base_of_host() -> String:
	var ip := _local_ip()
	if ip == "": return "255.255.255."
	var p := ip.split("."); return "%s.%s.%s." % [p[0], p[1], p[2]]

func _physics_process(dt: float) -> void:
	if role == 0 or game.state == "menu" or game.player == null: return
	if role == 1:
		_t_snap -= dt
		if _t_snap <= 0.0: _t_snap = 1.0 / 15.0; _send_snap()
	else:
		_t_state -= dt
		if _t_state <= 0.0:
			_t_state = 1.0 / 20.0
			var p: Player = game.player
			rpc_id(1, "n_state", [p.global_position, p.rotation.y, p.pitch, p.wi, (1 if p.ads else 0) | (2 if p.sprinting else 0)])

# ------------------------------------------------------------ رسائل اللعب
@rpc("any_peer", "unreliable_ordered")
func n_state(a: Array) -> void:
	if role != 1: return
	var pid := multiplayer.get_remote_sender_id()
	if not peer_slot.has(pid): return
	var b := game.fighters[peer_slot[pid]] as Bot
	if b == null or not b.alive: return
	b.net_pos = a[0]; b.net_yaw = a[1]; b.net_aim = a[2]
	if int(a[3]) != b.wi: b.wi = int(a[3])

@rpc("any_peer", "reliable")
func n_fire(origin: Vector3, dir: Vector3, wi: int) -> void:
	if role != 1: return
	var pid := multiplayer.get_remote_sender_id()
	if not peer_slot.has(pid): return
	var b := game.fighters[peer_slot[pid]] as Bot
	if b == null or not b.alive or wi < 0 or wi >= game.weapons.size(): return
	b.wi = wi; game.hitscan(b, origin, dir.normalized(), game.weapons[wi])

func send_fire(origin: Vector3, dir: Vector3, wi: int) -> void:
	if role == 2: rpc_id(1, "n_fire", origin, dir, wi)

func send_fx(slot: int, origin: Vector3, dir: Vector3) -> void:
	if role != 1 or peer_slot.is_empty(): return
	rpc("n_fx", slot, origin, dir)

@rpc("authority", "unreliable")
func n_fx(slot: int, origin: Vector3, dir: Vector3) -> void:
	if slot == my_slot: return
	game.fx_shot(slot, origin, dir)

func send_hit(slot: int, head: bool) -> void:
	for pid in peer_slot:
		if peer_slot[pid] == slot: rpc_id(int(pid), "n_hit", head)

@rpc("authority", "reliable")
func n_hit(_head: bool) -> void:
	game.hud.hitmark()

func send_feed(txt: String) -> void:
	game.hud.add_feed(txt)
	if role == 1 and not peer_slot.is_empty(): rpc("n_feed", txt)

@rpc("authority", "reliable")
func n_feed(txt: String) -> void:
	game.hud.add_feed(txt)

func _send_snap() -> void:
	if peer_slot.is_empty(): return
	var ents: Array = []
	for f in game.fighters:
		var fl := 0
		if f.alive: fl |= 1
		var aim := 0.0
		if f == game.player: aim = game.player.pitch
		elif f is Bot:
			var b := f as Bot
			if b.off: fl |= 2
			aim = b.net_aim if b.net_ctrl else b.aim_pitch
		ents.append([f.global_position, f.rotation.y, aim, f.wi, fl, int(f.hp), f.kills, f.deaths, f.spawn_seq])
	var over := 1 if game.state == "over" else 0
	var winner := 0 if game.score[0] >= game.score[1] else 1
	rpc("n_snap", ents, [game.score[0], game.score[1], game.time_left, over, winner, match_id])

@rpc("authority", "unreliable_ordered")
func n_snap(ents: Array, meta: Array) -> void:
	if role != 2 or game.state == "menu" or int(meta[5]) != int(last_cfg.get("mid", -1)): return
	var g := game
	g.score = [int(meta[0]), int(meta[1])]; g.time_left = float(meta[2])
	for i in mini(ents.size(), g.fighters.size()):
		var e: Array = ents[i]; var f: Fighter = g.fighters[i]
		if not is_instance_valid(f): continue
		var fl := int(e[4])
		f.kills = int(e[6]); f.deaths = int(e[7])
		if i == my_slot:
			var p: Player = g.player
			var hp := float(e[5])
			if hp < _prev_hp - 0.5 and fl & 1: g.hud.flash_damage()
			_prev_hp = hp; p.hp = hp
			if int(e[8]) != p.spawn_seq:
				p.respawn(e[0]); p.spawn_seq = int(e[8])
			if fl & 1 == 0 and p.alive: p.alive = false
		else:
			var b := f as Bot
			b.off = (fl & 2) != 0; b.hp = float(e[5]); b.wi = int(e[3]); b.net_aim = e[2]; b.net_yaw = e[1]; b.net_pos = e[0]
			var al := (fl & 1) != 0
			if al and not b.alive: b.global_position = e[0]
			b.alive = al; b.visible = al and not b.off
	if int(meta[3]) == 1 and g.state == "play": g._end_match()
