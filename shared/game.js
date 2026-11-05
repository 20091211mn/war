// منطق اللعبة على الخادم: اللوبي، المباراة، الإصابات، القنابل، والبوتات
import { WEAPONS, PRIMARIES, computeStats, sanitizeLoadout, damageAt, defaultLoadout, sanitizeAtt } from './weapons.js';
import { buildMap, MAP_COUNT, MAP_NAMES } from './maps.js';
import { PLAYER, rayPlayer, raycastMap, moveBody, blocked, groundAt } from './physics.js';
import { VEH, SEATS, VT, TANK, vp, seatsOf, rayVehicle, seatWorld, w2l, l2w, vehStep, vehCollides, pushOutVeh } from './vehicle.js';
import { extras, DRONE, raySphere } from './extras.js';
import { buildNav, findPath, cellOf, cellCenter, nearestFree } from './nav.js';

export const MAX_HUMANS = 16;
const SCORE_LIMITS = [10, 20, 30, 50, 100];
const TIME_LIMITS = [180, 300, 600, 900];
const PROT_MS = 2000, END_MS = 12000, VEH_RESPAWN_MS = 30000, DROP_LIFE_MS = 40000;
const RESP_OPTS = [1, 3, 5, 8, 10];
const BOT_WEAPONS = ['carbine', 'smg', 'lmg', 'dmr', 'shotgun'];
const r2 = (v) => Math.round(v * 100) / 100;
const r3 = (v) => Math.round(v * 1000) / 1000;
const fin = (v) => typeof v === 'number' && Number.isFinite(v);
const clamp = (v, a, b) => (v < a ? a : v > b ? b : v);

export class Game {
  constructor(now = () => Date.now()) {
    this.now = now;
    this.players = new Map();
    this.nextId = 1; this.nextBot = 1;
    this.phase = 'lobby';
    this.cfg = { map: 0, scoreLimit: 30, timeLimit: 600, fill: 0, ff: 0, veh: 1, diff: 1, resp: 3, regen: 1, mode: 0, tod: 0, streaks: 1 };
    this.kits = []; this.zones = []; this.uav = [0, 0]; this.zoneIdx = 1; this.zoneAt = 0; this.zoneTick = 0; this.zoneState = 3;
    this.vehs = []; this.drops = []; this.nextDrop = 1;
    this.map = null; this.nav = null;
    this.grenades = []; this.nextGren = 1; this.c4 = []; this.nextC4 = 1;
    this.score = [0, 0];
    this.startAt = 0; this.endAt = 0; this.hostId = 0;
    this.lastLobby = 0; this.lastSec = 0;
  }

  // ---------- أدوات الإرسال ----------
  sendTo(p, obj) { if (p && p.conn) p.conn.send(JSON.stringify(obj)); }
  bcast(obj, droppable = false) {
    const s = JSON.stringify(obj);
    for (const p of this.players.values()) if (p.conn) p.conn.send(s, droppable);
  }
  humans() { return [...this.players.values()].filter(p => !p.bot); }

  mkPlayer(name, bot) {
    const id = this.nextId++;
    return {
      id, name: name || ('لاعب' + id), team: 0, bot, conn: null, lo: defaultLoadout(), st: [null, null],
      x: 0, y: 0, z: 0, vy: 0, onGround: true, yaw: 0, pitch: 0, crouch: false, ads: false, moving: false, sprint: false, w: 0,
      hp: 100, alive: false, kills: 0, deaths: 0, respawnAt: 0, prot: 0, reveal: 0, credit: [1, 1], lastT: [0, 0],
      grenades: 2, lastGren: 0, epoch: 0, msgs: 0, veh: -1, seat: 0, lastHurt: 0, lastChat: 0, am: null, runT: 0,
      // بوت
      target: 0, scanAt: 0, reactAt: 0, nextShot: 0, burst: 0, vis: false, seenAt: 0, tx: 0, tz: 0,
      path: null, pi: 0, pathAt: 0, goal: null, strafe: 1, strafeAt: 0, lastX: 0, lastZ: 0, stuckAt: 0,
    };
  }

  // ---------- الاتصال ----------
  connect(conn) {
    if (this.humans().length >= MAX_HUMANS) { conn.send(JSON.stringify({ t: 'full' })); conn.close(); return null; }
    const p = this.mkPlayer('', false);
    p.conn = conn;
    const cnt = [0, 0];
    for (const q of this.players.values()) cnt[q.team]++;
    p.team = cnt[1] < cnt[0] ? 1 : 0;
    this.players.set(p.id, p);
    if (!this.hostId || !this.players.has(this.hostId)) this.hostId = p.id;
    this.sendTo(p, { t: 'welcome', id: p.id, maps: MAP_NAMES });
    this.sys(p.name + ' انضم إلى الغرفة');
    if (this.phase !== 'lobby') {
      this.sendTo(p, { t: 'start', map: this.cfg.map, cfg: this.cfg, tl: this.timeLeft(), ...this.matchInfo() });
      this.rebalanceBots();
      if (this.phase === 'playing') this.spawn(p);
    }
    this.sendLobby();
    return p;
  }

  disconnect(p) {
    if (!p || !this.players.has(p.id)) return;
    if (p.veh >= 0) this.dropSeat(p);
    this.c4Clear(p); this.players.delete(p.id);
    p.conn = null;
    this.sys(p.name + ' غادر');
    if (this.hostId === p.id) { const h = this.humans()[0]; this.hostId = h ? h.id : 0; }
    this.bcast({ t: 'left', id: p.id });
    if (this.humans().length === 0) this.reset();
    else { this.rebalanceBots(); this.sendLobby(); }
  }

  reset() {
    for (const [id, q] of [...this.players]) if (q.bot) this.players.delete(id);
    this.phase = 'lobby'; this.map = null; this.nav = null; this.grenades = []; this.c4 = []; this.score = [0, 0]; this.vehs = []; this.drops = []; this.kits = []; this.zones = []; this.uav = [0, 0];
    for (const q of this.players.values()) { q.alive = false; q.kills = 0; q.deaths = 0; q.veh = -1; }
  }

  timeLeft() { return this.phase === 'lobby' ? this.cfg.timeLimit : Math.max(0, Math.ceil(this.cfg.timeLimit - (this.now() - this.startAt) / 1000)); }

  sendLobby() {
    const ps = [];
    for (const q of this.players.values()) ps.push({ id: q.id, n: q.name, tm: q.team, k: q.kills, d: q.deaths, b: q.bot ? 1 : 0, p: q.lo.p, s: q.lo.s, a: q.lo.a, v: q.veh >= 0 ? 1 : 0 });
    this.bcast({ t: 'lobby', ph: this.phase, cfg: this.cfg, host: this.hostId, ps });
  }

  // ---------- الرسائل ----------
  onMessage(p, raw) {
    if (!p || !this.players.has(p.id)) return;
    if (++p.msgs > 600) return; // حماية من الإغراق
    let m;
    try { m = JSON.parse(raw); } catch { return; }
    if (!m || typeof m !== 'object' || typeof m.t !== 'string') return;
    const isHost = p.id === this.hostId;
    switch (m.t) {
      case 'hello': {
        if (typeof m.n === 'string') p.name = m.n.replace(/[\u0000-\u001f<>&"]/g, '').trim().slice(0, 14) || p.name;
        if (m.lo) p.lo = sanitizeLoadout(m.lo);
        this.sendLobby();
        break;
      }
      case 'team': {
        if (this.phase !== 'lobby') break;
        const t = m.v === 1 ? 1 : m.v === 0 ? 0 : -1;
        const tgt = isHost && fin(m.id) && this.players.get(m.id | 0) ? this.players.get(m.id | 0) : p;
        if (tgt !== p) { if (t >= 0) tgt.team = t; }
        else if (t >= 0) p.team = t;
        else { const c = [0, 0]; for (const q of this.players.values()) if (q !== p) c[q.team]++; p.team = c[1] < c[0] ? 1 : 0; }
        this.sendLobby();
        break;
      }
      case 'lo': { p.lo = sanitizeLoadout(m.lo); this.sendLobby(); break; }
      case 'cfg': {
        if (!isHost || this.phase !== 'lobby') break;
        const c = this.cfg;
        if (fin(m.map)) c.map = clamp(m.map | 0, 0, MAP_COUNT - 1);
        if (SCORE_LIMITS.includes(m.scoreLimit)) c.scoreLimit = m.scoreLimit;
        if (TIME_LIMITS.includes(m.timeLimit)) c.timeLimit = m.timeLimit;
        if (fin(m.fill)) c.fill = clamp(m.fill | 0, 0, 8);
        if (m.ff === 0 || m.ff === 1) c.ff = m.ff;
        if (m.veh === 0 || m.veh === 1) c.veh = m.veh;
        if (m.regen === 0 || m.regen === 1) c.regen = m.regen;
        if (fin(m.diff)) c.diff = clamp(m.diff | 0, 0, 2);
        if (RESP_OPTS.includes(m.resp)) c.resp = m.resp;
        if (m.mode === 0 || m.mode === 1) c.mode = m.mode;
        if (m.tod === 0 || m.tod === 1 || m.tod === 2) c.tod = m.tod;
        if (m.streaks === 0 || m.streaks === 1) c.streaks = m.streaks;
        this.sendLobby();
        break;
      }
      case 'chat': {
        if (typeof m.m !== 'string') break;
        const txt = m.m.replace(/[\u0000-\u001f<>&]/g, '').trim().slice(0, 120);
        const now = this.now();
        if (!txt || now - p.lastChat < 600) break;
        p.lastChat = now;
        const o = JSON.stringify({ t: 'chat', id: p.id, n: p.name, tm: p.team, m: txt, to: m.to === 1 ? 1 : 0 });
        for (const q of this.players.values()) if (q.conn && (m.to !== 1 || q.team === p.team)) q.conn.send(o);
        break;
      }
      case 'kick': {
        if (!isHost) break;
        const q = this.players.get(m.id | 0);
        if (q && !q.bot && q !== p) { this.sendTo(q, { t: 'kicked' }); const c = q.conn; this.disconnect(q); if (c) c.close(); }
        break;
      }
      case 'give': {
        if (!isHost) break;
        const q = this.players.get(m.id | 0);
        if (q && !q.bot) { this.hostId = q.id; this.sys(q.name + ' أصبح مضيف الغرفة'); this.sendLobby(); }
        break;
      }
      case 'shuffle': {
        if (!isHost || this.phase !== 'lobby') break;
        const hs = this.humans().sort(() => Math.random() - 0.5);
        hs.forEach((q, i) => { q.team = i % 2; });
        this.sendLobby();
        break;
      }
      case 'restart': {
        if (!isHost) break;
        if (this.phase !== 'lobby') this.toLobby();
        if (fin(m.map)) this.cfg.map = clamp(m.map | 0, 0, MAP_COUNT - 1);
        this.startMatch();
        break;
      }
      case 'use': this.useVeh(p); break;
      case 'drone': case 'dp': case 'dboom': this.onDroneMsg(p, m); break;
      case 'seat': this.switchSeat(p, m.s | 0); break;
      case 'v': this.onDriver(p, m); break;
      case 'start': if (isHost) this.startMatch(); break;
      case 'back': if (isHost && this.phase !== 'lobby') this.toLobby(); break;
      case 'st': {
        if (this.phase !== 'playing' || !p.alive || m.e !== p.epoch) break;
        if (Array.isArray(m.am) && m.am.length === 4) p.am = m.am.map(v => clamp(fin(v) ? v | 0 : 0, 0, 999));
        if (p.veh >= 0) {
          if (!fin(m.yaw) || !fin(m.pitch)) break;
          p.yaw = m.yaw; p.pitch = clamp(m.pitch, -1.6, 1.6);
          const f = m.f | 0; p.ads = !!(f & 2); p.w = m.w === 1 ? 1 : 0;
          break;
        }
        if (!fin(m.x) || !fin(m.y) || !fin(m.z) || !fin(m.yaw) || !fin(m.pitch)) break;
        const mp = this.map;
        p.x = clamp(m.x, -mp.W / 2, mp.W / 2); p.z = clamp(m.z, -mp.D / 2, mp.D / 2); p.y = clamp(m.y, -2, 40);
        p.yaw = m.yaw; p.pitch = clamp(m.pitch, -1.6, 1.6);
        const f = m.f | 0;
        p.crouch = !!(f & 1); p.ads = !!(f & 2); p.moving = !!(f & 4); p.sprint = !!(f & 8);
        p.w = m.w === 1 ? 1 : 0;
        break;
      }
      case 'shoot': {
        if (this.phase !== 'playing' || !p.alive || !Array.isArray(m.d) || (p.veh >= 0 && p.seat === 0)) break;
        const w = m.w === 1 ? 1 : 0, s = p.st[w];
        if (!s) break;
        const now = this.now();
        p.credit[w] = Math.min(2, p.credit[w] + (now - p.lastT[w]) / (s.interval * 1000));
        p.lastT[w] = now;
        if (p.credit[w] < 0.95) break;
        p.credit[w] -= 1;
        const dirs = [];
        for (const d of m.d.slice(0, s.pellets)) {
          if (!Array.isArray(d) || !fin(d[0]) || !fin(d[1]) || !fin(d[2])) continue;
          const l = Math.hypot(d[0], d[1], d[2]);
          if (l < 0.5) continue;
          dirs.push([d[0] / l, d[1] / l, d[2] / l]);
        }
        if (dirs.length) { p.w = w; this.processShot(p, w, dirs); }
        break;
      }
      case 'cannon': this.onCannon(p, m); break;
      case 'c4': this.c4Place(p, m); break;
      case 'c4x': this.c4Det(p); break;
      case 'rl': this.bcastExcept(p, { t: 'rl', id: p.id }); break;
      case 'gren': {
        if (this.phase !== 'playing' || !p.alive || p.grenades <= 0 || !Array.isArray(m.d) || (p.veh >= 0 && p.seat === 0)) break;
        const now = this.now(); if (now - p.lastGren < 600) break;
        const l = Math.hypot(m.d[0], m.d[1], m.d[2]);
        if (!fin(l) || l < 0.5) break;
        p.lastGren = now; p.grenades--;
        const dx = m.d[0] / l, dy = m.d[1] / l, dz = m.d[2] / l;
        const eyeY = p.y + (p.crouch ? PLAYER.eyec : PLAYER.eye);
        this.grenades.push({ id: this.nextGren++, x: p.x + dx * 0.5, y: eyeY + dy * 0.5 - 0.2, z: p.z + dz * 0.5, vx: dx * 14 + 0, vy: dy * 14 + 2.5, vz: dz * 14, owner: p.id, team: p.team, fuse: 2.6 });
        break;
      }
      case 'ping': this.sendTo(p, { t: 'pong', c: m.c }); break;
      default: break;
    }
  }
  sys(text) { this.bcast({ t: 'chat', id: 0, n: '', tm: -1, m: text, to: 0 }); }
  bcastExcept(p, obj) { const s = JSON.stringify(obj); for (const q of this.players.values()) if (q !== p && q.conn) q.conn.send(s); }

  // ---------- المباراة ----------
  startMatch() {
    if (this.phase !== 'lobby') return;
    this.map = buildMap(this.cfg.map); this.nav = buildNav(this.map);
    this.score = [0, 0]; this.grenades = []; this.c4 = []; this.drops = []; this.startAt = this.now();
    this.vehs = this.cfg.veh ? this.map.vehicles.map((sp, i) => this.mkVeh(sp, i)) : [];
    this.linkVehs(); this.initExtras();
    for (const q of this.players.values()) { q.kills = 0; q.streak = 0; q.shield = 0; q.deaths = 0; q.alive = false; q.veh = -1; }
    this.phase = 'playing';
    this.bcast({ t: 'start', map: this.cfg.map, cfg: this.cfg, tl: this.cfg.timeLimit, ...this.matchInfo() });
    for (const q of this.humans()) this.spawn(q);
    this.rebalanceBots();
    this.sendLobby();
  }

  toLobby() {
    this.reset();
    this.bcast({ t: 'tolobby' });
    this.sendLobby();
  }

  rebalanceBots() {
    if (this.phase === 'lobby') return;
    for (const team of [0, 1]) {
      const hum = [...this.players.values()].filter(q => !q.bot && q.team === team).length;
      const bots = [...this.players.values()].filter(q => q.bot && q.team === team);
      const want = Math.max(0, this.cfg.fill - hum);
      while (bots.length > want) { const b = bots.pop(); this.players.delete(b.id); this.bcast({ t: 'left', id: b.id }); }
      while (bots.length < want) {
        const b = this.mkPlayer('Bot ' + this.nextBot++, true); b.team = team;
        const pw = BOT_WEAPONS[Math.floor(Math.random() * BOT_WEAPONS.length)];
        b.lo = sanitizeLoadout({ p: pw, s: 'pistol', a: [sanitizeAtt(pw, {}), sanitizeAtt('pistol', {})] });
        this.players.set(b.id, b); bots.push(b);
        if (this.phase === 'playing') this.spawn(b);
      }
    }
    this.sendLobby();
  }

  spawnPoint(p) {
    const list = p.team === 0 ? this.map.red : this.map.blue;
    const others = [...this.players.values()].filter(q => q !== p && q.alive);
    let best = null, bs = -1;
    for (const [x, z] of list) {
      let md = 1e9, near = false;
      for (const q of others) {
        const d = Math.hypot(q.x - x, q.z - z);
        if (d < 1.2) near = true;
        if (q.team !== p.team && d < md) md = d;
      }
      const sc = (near ? -100 : 0) + Math.min(md, 80) + Math.random() * 6;
      if (sc > bs) { bs = sc; best = [x, z]; }
    }
    return best;
  }

  spawn(p) {
    if (!this.map) return;
    const [x, z] = this.spawnPoint(p);
    p.x = x; p.y = 0; p.z = z; p.vy = 0; p.onGround = true;
    p.yaw = p.team === 0 ? -Math.PI / 2 : Math.PI / 2; p.pitch = 0;
    p.hp = 100; p.alive = true; p.veh = -1; p.seat = 0; p.lastHurt = 0; p.am = null; p.crouch = false; p.ads = false; p.moving = false; p.sprint = false; p.w = 0;
    p.grenades = 2; this.c4Clear(p); p.prot = this.now() + PROT_MS; p.epoch++;
    p.st = [computeStats(p.lo.p, p.lo.a[0]), computeStats(p.lo.s, p.lo.a[1])];
    p.credit = [1, 1]; p.lastT = [this.now(), this.now()];
    p.target = 0; p.path = null; p.goal = null; p.vis = false; p.burst = 0;
    this.bcast({ t: 'spawn', id: p.id, x, y: 0, z, yaw: p.yaw, e: p.epoch, tm: p.team, p: p.lo.p, s: p.lo.s, a: p.lo.a });
  }

  // ---------- القتال ----------
  processShot(p, slot, dirs) {
    const s = p.st[slot], map = this.map, now = this.now();
    const ox = p.x, oy = p.y + (p.crouch ? PLAYER.eyec : PLAYER.eye), oz = p.z;
    const maxT = Math.min(s.range[1] * 1.6, 300);
    const dmg = new Map(), dd = new Map(), ends = [], vd = new Map(), ff = !!this.cfg.ff;
    const ownV = p.veh >= 0 ? this.vehs[p.veh] : null;
    for (const [dx, dy, dz] of dirs) {
      let best = raycastMap(map, ox, oy, oz, dx, dy, dz, maxT);
      if (best === Infinity) best = maxT;
      let hitP = null, hitHead = false, hitV = null, part = null, hitD = null;
      for (const v of this.vehs) {
        if (v === ownV) continue;
        const r = rayVehicle(v, ox, oy, oz, dx, dy, dz, best);
        if (r && r.t < best) { best = r.t; hitV = v; part = r.part; }
      }
      for (const q of this.players.values()) {
        if (q === p || !q.alive || (q.team === p.team && !ff) || now < q.prot) continue;
        const r = rayPlayer(q, ox, oy, oz, dx, dy, dz, best);
        if (r && r.t < best) { best = r.t; hitP = q; hitHead = r.head; hitV = null; hitD = null; }
      }
      for (const q of this.players.values()) {
        if (!q.drone || q === p || (q.team === p.team && !ff)) continue;
        const t = raySphere(ox, oy, oz, dx, dy, dz, q.drone.x, q.drone.y, q.drone.z, DRONE.r, best);
        if (t !== null && t < best) { best = t; hitD = q; hitP = null; hitV = null; }
      }
      if (hitP) {
        const e = dmg.get(hitP) || { d: 0, h: false };
        e.d += damageAt(s, best) * (hitHead ? s.head : 1) * (hitP.team === p.team ? 0.5 : 1); e.h = e.h || hitHead;
        dmg.set(hitP, e);
      } else if (hitD) {
        dd.set(hitD, (dd.get(hitD) || 0) + damageAt(s, best));
      } else if (hitV) {
        const e = vd.get(hitV) || [];
        e.push([part, damageAt(s, best)]);
        vd.set(hitV, e);
      }
      ends.push([r2(ox + dx * best), r2(oy + dy * best), r2(oz + dz * best), hitP ? 1 : (hitV || hitD) ? 2 : 0]);
    }
    if (s.explosive && ends.length) { // قاذف: انفجار عند نقطة الاصطدام بدل الضرر المباشر
      dmg.clear(); vd.clear(); dd.clear();
      const e = ends[0]; this.explode({ x: e[0], y: e[1], z: e[2], owner: p.id, team: p.team }, 6.5, s.dmg, 340, s.id);
    }
    if (!s.muted) p.reveal = now + 5000; // يظهر على الرادار 5 ث بعد الإطلاق (أقل من 10 ث)
    this.bcastExcept(p, { t: 'shot', id: p.id, w: s.id, o: [r2(ox), r2(oy), r2(oz)], e: ends, s: s.muted ? 1 : 0 });
    for (const [q, e] of dmg) this.hurt(q, e.d, p, e.h, s.id, p.x, p.z);
    for (const [v, list] of vd) for (const [part, d] of list) this.shootVeh(v, part, d, p);
    for (const [q, d] of dd) this.hurtDrone(q, d, p);
  }

  hurt(v, amount, a, head, wid, fx, fz) {
    if (this.phase !== 'playing' || !v.alive || this.now() < v.prot) return;
    amount = Math.round(amount);
    if (amount <= 0) return;
    const total = amount;
    if (v.shield > 0) { const ab = Math.min(v.shield, amount); v.shield -= ab; amount -= ab; }
    v.hp -= amount; v.lastHurt = this.now();
    const msg = { t: 'hit', v: v.id, a: a.id, d: total, h: head ? 1 : 0, fx: r2(fx), fz: r2(fz) };
    this.sendTo(v, msg);
    if (a !== v) this.sendTo(a, msg);
    if (v.hp <= 0) this.kill(v, a, wid, head);
  }

  kill(v, a, wid, head) {
    v.hp = 0; v.alive = false; v.deaths++; v.respawnAt = this.now() + this.cfg.resp * 1000;
    if (a !== v && a.team !== v.team) { a.kills++; this.score[a.team]++; }
    this.onKillExtras(v, a); this.c4Clear(v);
    this.dropAmmo(v);
    if (v.veh >= 0) this.dropSeat(v);
    this.bcast({ t: 'kill', k: a.id, v: v.id, w: wid, h: head ? 1 : 0 });
    this.checkEnd();
  }

  checkEnd() {
    if (this.phase !== 'playing') return;
    if (this.score[0] >= this.limit() || this.score[1] >= this.limit()) this.endMatch();
    else if (this.timeLeft() <= 0) this.endMatch();
  }
  endMatch() {
    if (this.phase !== 'playing') return;
    this.phase = 'ended'; this.endAt = this.now() + END_MS;
    const win = this.score[0] === this.score[1] ? -1 : this.score[0] > this.score[1] ? 0 : 1;
    this.sendLobby();
    this.bcast({ t: 'end', win, sc: this.score });
  }

  // ---------- القنابل ----------
  simGrenades(dt) {
    const map = this.map, rr = 0.12;
    const inBox = (x, y, z) => { for (const b of map.boxes) if (x > b.x0 - rr && x < b.x1 + rr && y > b.y0 - rr && y < b.y1 + rr && z > b.z0 - rr && z < b.z1 + rr) return b; return null; };
    const keep = [];
    for (const g of this.grenades) {
      const sub = 4, h = dt / sub;
      for (let i = 0; i < sub; i++) {
        g.vy -= 18 * h;
        let nx = g.x + g.vx * h;
        if (inBox(nx, g.y, g.z)) { g.vx *= -0.45; g.vz *= 0.8; } else g.x = nx;
        let nz = g.z + g.vz * h;
        if (inBox(g.x, g.y, nz)) { g.vz *= -0.45; g.vx *= 0.8; } else g.z = nz;
        let ny = g.y + g.vy * h;
        const bb = inBox(g.x, ny, g.z);
        if (bb) { if (g.vy < 0) ny = bb.y1 + rr; else ny = bb.y0 - rr; g.vy *= -0.4; g.vx *= 0.75; g.vz *= 0.75; if (Math.abs(g.vy) < 1) g.vy = 0; g.y = ny; }
        else if (ny < rr) { g.y = rr; g.vy = Math.abs(g.vy) < 1.5 ? 0 : -g.vy * 0.4; g.vx *= 0.75; g.vz *= 0.75; }
        else g.y = ny;
      }
      g.fuse -= dt;
      if (g.fuse <= 0) this.explode(g); else keep.push(g);
    }
    this.grenades = keep;
  }

  explode(g, R = 7, pd = 140, vd = 260, wid = 'gren') {
    this.bcast({ t: 'boom', x: r2(g.x), y: r2(g.y), z: r2(g.z) });
    const owner = this.players.get(g.owner);
    for (const q of [...this.players.values()]) {
      if (!q.alive) continue;
      if (q.team === g.team && q.id !== g.owner && !this.cfg.ff) continue;
      const cy = q.y + 1.0, d = Math.hypot(q.x - g.x, cy - g.y, q.z - g.z);
      if (d > R) continue;
      const t = raycastMap(this.map, g.x, g.y + 0.2, g.z, (q.x - g.x) / (d || 1), (cy - g.y - 0.2) / (d || 1), (q.z - g.z) / (d || 1), d);
      if (t !== Infinity && t < d - 0.3) continue;
      const dmg = pd * Math.pow(1 - d / R, 1.2);
      this.hurt(q, dmg, owner || q, false, wid, g.x, g.z);
    }
    for (const v of this.vehs) {
      if (v.dead) continue;
      const d = Math.max(0, Math.hypot(v.x - g.x, g.y - 0.8, v.z - g.z) - 1.6);
      if (d < R) this.hurtVeh(v, vd * Math.pow(1 - d / R, 1.1), owner);
    }
  }

  // ---------- المركبات ----------
  // ---------- مدفع الدبابة ----------
  onCannon(p, m) {
    const v = this.vehOf(p);
    if (!v || v.type !== 1 || p.seat !== 0 || v.dead || this.phase !== 'playing' || !p.alive || !Array.isArray(m.d)) return;
    const now = this.now(); if (now < v.cannonAt) return;
    const [dx0, dy0, dz0] = m.d; if (![dx0, dy0, dz0].every(fin)) return;
    const l = Math.hypot(dx0, dy0, dz0); if (l < 0.5) return;
    const dx = dx0 / l, dy = dy0 / l, dz = dz0 / l;
    v.cannonAt = now + TANK.cd;
    const ox = v.x + dx * TANK.muzzleLen, oy = 1.65 + dy * TANK.muzzleLen, oz = v.z + dz * TANK.muzzleLen;
    let best = raycastMap(this.map, ox, oy, oz, dx, dy, dz, TANK.range); if (best === Infinity) best = TANK.range;
    for (const o of this.vehs) { if (o === v || o.dead) continue; const r = rayVehicle(o, ox, oy, oz, dx, dy, dz, best); if (r && r.t < best) best = r.t; }
    for (const q of this.players.values()) { if (q === p || !q.alive || q.veh >= 0) continue; const r = rayPlayer(q, ox, oy, oz, dx, dy, dz, best); if (r && r.t < best) best = r.t; }
    const ex = ox + dx * best, ey = Math.max(0.1, oy + dy * best), ez = oz + dz * best;
    this.bcast({ t: 'cshot', v: v.id, o: [r2(ox), r2(oy), r2(oz)], e: [r2(ex), r2(ey), r2(ez)] });
    this.cannonBoom(ex, ey, ez, p);
  }
  cannonBoom(x, y, z, owner) {
    const R = TANK.splash;
    this.bcast({ t: 'boom', x: r2(x), y: r2(y), z: r2(z) });
    for (const q of [...this.players.values()]) {
      if (!q.alive) continue;
      if (q.team === owner.team && q.id !== owner.id && !this.cfg.ff) continue;
      const d = Math.hypot(q.x - x, q.y + 1 - y, q.z - z); if (d > R) continue;
      this.hurt(q, TANK.dmg * Math.pow(1 - d / R, 0.9), owner, false, 'tank', x, z);
    }
    for (const o of this.vehs) { if (o.dead) continue; const d = Math.hypot(o.x - x, o.z - z); if (d < R + 1.5) this.hurtVeh(o, TANK.vdmg * (1 - d / (R + 1.5)), owner); }
  }

  mkVeh(sp, i) {
    const P = VT[sp.t === 1 ? 1 : 0];
    return { id: i, type: sp.t === 1 ? 1 : 0, cannonAt: 0, x: sp.x, z: sp.z, h: sp.h, vx: 0, vz: 0, steer: 0, tires: 0, speed: 0, hp: P.hp, tireHp: [P.tireHp, P.tireHp, P.tireHp, P.tireHp],
      seats: [0, 0, 0, 0, 0], team: -1, burn: false, dead: false, respawnAt: 0, sp, lastV: 0, lastAtk: 0, lastCrash: 0, dyn: null };
  }
  linkVehs() { for (const v of this.vehs) v.dyn = this.vehs.filter(o => o !== v).map(o => [o.x, o.z, o.h, o.type | 0]); }
  vehOf(p) { return p.veh >= 0 ? this.vehs[p.veh] || null : null; }

  useVeh(p) {
    if (this.phase !== 'playing' || !p.alive) return;
    if (p.veh >= 0) { this.exitVeh(p); return; }
    let best = null, bd = 2.8;
    for (const v of this.vehs) {
      if (v.dead) continue;
      const [lx, lz] = w2l(v, p.x, p.z);
      const P = vp(v), d = Math.hypot(Math.max(0, Math.abs(lx) - P.hw), Math.max(0, Math.abs(lz) - P.hl));
      if (d < bd && p.y < 2.3 && (v.team < 0 || v.team === p.team)) { bd = d; best = { v, lx, lz }; }
    }
    if (!best) return;
    const { v, lx, lz } = best;
    const order = lz < -0.2 ? (lx < 0 ? [0, 1, 2, 3, 4] : [1, 0, 3, 2, 4]) : (lx < 0 ? [2, 3, 4, 1, 0] : [3, 2, 4, 1, 0]);
    const s = (v.type === 1 ? [0, 1] : order).find(i => !v.seats[i]);
    if (s === undefined) return;
    this.sitIn(p, v, s);
  }
  sitIn(p, v, s) {
    if (p.veh >= 0) this.vehOf(p).seats[p.seat] = 0;
    v.seats[s] = p.id; p.veh = v.id; p.seat = s; v.team = p.team;
    p.crouch = true; p.moving = false; p.sprint = false; p.vy = 0; p.epoch++;
    this.placeOccupant(p, v);
    this.sendTo(p, { t: 'seat', v: v.id, s, e: p.epoch });
  }
  switchSeat(p, s) {
    const v = this.vehOf(p);
    if (!v || s < 0 || s >= seatsOf(v).length || v.seats[s] || s === p.seat) return;
    if (s === 0 && v.hp <= 0) return;
    v.seats[p.seat] = 0; v.seats[s] = p.id; p.seat = s;
    p.epoch++;
    this.placeOccupant(p, v);
    this.sendTo(p, { t: 'seat', v: v.id, s, e: p.epoch });
  }
  placeOccupant(p, v) { const [x, y, z] = seatWorld(v, p.seat); p.x = x; p.y = y; p.z = z; }
  dropSeat(p) {
    const v = this.vehOf(p);
    if (v) { v.seats[p.seat] = 0; if (!v.seats.some(Boolean)) v.team = -1; }
    p.veh = -1;
  }
  exitVeh(p) {
    const v = this.vehOf(p); if (!v) return;
    const ST = seatsOf(v), PV = vp(v), side = ST[p.seat].x < 0 ? -1 : 1;
    const cands = v.type === 1 ? [[PV.hw + 1, 0], [-PV.hw - 1, 0], [0, PV.hl + 1], [0, -PV.hl - 1]] : p.seat === 4 ? [[0, 3.3], [-2, 2], [2, 2]] : [[side * 1.9, ST[p.seat].z], [-side * 1.9, ST[p.seat].z], [0, 3.4], [0, -3.4]];
    let spot = null;
    for (const [lx, lz] of cands) {
      const [x, , z] = l2w(v, lx, 0, lz), y = groundAt(this.map, x, z, 0);
      if (!blocked(this.map, x, y, z, PLAYER.h)) { spot = [x, y, z]; break; }
    }
    if (!spot) spot = [p.x, groundAt(this.map, p.x, p.z, 0), p.z];
    const spd = Math.abs(v.speed);
    this.dropSeat(p);
    p.x = spot[0]; p.y = spot[1]; p.z = spot[2]; p.vy = 0; p.onGround = true; p.crouch = false; p.epoch++;
    this.sendTo(p, { t: 'exit', x: r2(p.x), y: r2(p.y), z: r2(p.z), e: p.epoch });
    if (spd > 12) this.hurt(p, (spd - 12) * 5, p, false, 'veh', p.x, p.z);
  }
  onDriver(p, m) {
    const v = this.vehOf(p);
    if (!v || p.seat !== 0 || this.phase !== 'playing' || !p.alive || m.e !== p.epoch) return;
    if (![m.x, m.z, m.h, m.vx, m.vz, m.s, m.sp].every(fin)) return;
    const now = this.now();
    const dt = clamp((now - (v.lastV || now - 100)) / 1000, 0.02, 0.5);
    const mp = this.map;
    const nx = clamp(m.x, -mp.W / 2, mp.W / 2), nz = clamp(m.z, -mp.D / 2, mp.D / 2);
    if (Math.hypot(nx - v.x, nz - v.z) > 34 * dt + 0.8) { this.sendTo(p, { t: 'vfix', x: r2(v.x), z: r2(v.z), h: r3(v.h), vx: r2(v.vx), vz: r2(v.vz) }); v.lastV = now; return; }
    v.lastV = now; v.x = nx; v.z = nz; v.h = m.h; v.vx = clamp(m.vx, -40, 40); v.vz = clamp(m.vz, -40, 40); v.steer = clamp(m.s, -0.6, 0.6); v.speed = clamp(m.sp, -40, 40);
    for (let i = 0; i < 5; i++) if (v.seats[i]) { const q = this.players.get(v.seats[i]); if (q) this.placeOccupant(q, v); }
    if (fin(m.cr) && m.cr > vp(v).crashMin && now - v.lastCrash > 450) { v.lastCrash = now; this.crashVeh(v, Math.min(m.cr, 35), p); }
  }
  crashVeh(v, speed, driver) {
    this.bcast({ t: 'vcrash', v: v.id, s: r2(speed) });
    const occ = speed > 9 ? (speed - 9) * 4 : 0;
    if (occ > 0) for (const id of [...v.seats]) { const q = id ? this.players.get(id) : null; if (q && q.alive) this.hurt(q, occ, q, false, 'veh', v.x, v.z); }
    this.hurtVeh(v, (speed - vp(v).crashMin) * 9, driver);
  }
  shootVeh(v, part, d, a) {
    if (v.dead) return;
    if (v.seats.some(id => { const q = id && this.players.get(id); return q && q.team === a.team && a.veh !== v.id && !this.cfg.ff; })) return;
    if (part[0] === 'w') {
      const i = +part[1];
      if (v.tires >> i & 1) return;
      v.tireHp[i] -= d;
      if (v.tireHp[i] <= 0) { v.tires |= 1 << i; this.bcast({ t: 'tire', v: v.id, i, m: v.tires }); }
      return;
    }
    this.hurtVeh(v, (part === 'tank' ? d * 1.3 : d * 0.35) * vp(v).armor, a);
  }
  hurtVeh(v, amount, a) {
    if (v.dead || amount <= 0) return;
    v.hp -= amount; if (a) v.lastAtk = a.id;
    if (v.hp <= 0) this.explodeVeh(v);
    else if (v.hp < 150) v.burn = true;
  }
  explodeVeh(v) {
    v.dead = true; v.burn = false; v.hp = 0; v.respawnAt = this.now() + VEH_RESPAWN_MS; v.tires = 15;
    this.bcast({ t: 'vboom', v: v.id, x: r2(v.x), y: 1, z: r2(v.z) });
    const atk = this.players.get(v.lastAtk);
    const R = 9;
    for (const id of [...v.seats]) { const q = id ? this.players.get(id) : null; if (q && q.alive) this.hurt(q, 400, atk || q, false, 'veh', v.x, v.z); }
    for (const q of [...this.players.values()]) {
      if (!q.alive || q.veh >= 0) continue;
      if (atk && q.team === atk.team && q !== atk && !this.cfg.ff) continue;
      const d = Math.hypot(q.x - v.x, q.y + 1 - 1, q.z - v.z);
      if (d > R) continue;
      this.hurt(q, 190 * Math.pow(1 - d / R, 1.1), atk || q, false, 'veh', v.x, v.z);
    }
    for (const o of this.vehs) {
      if (o === v || o.dead) continue;
      const d = Math.hypot(o.x - v.x, o.z - v.z);
      if (d < R) this.hurtVeh(o, 300 * (1 - d / R), atk);
    }
    // حطام: يتوقف
    v.vx *= 0.3; v.vz *= 0.3;
  }
  tickVehs(dt, now) {
    this.linkVehs();
    for (const v of this.vehs) {
      const driver = v.seats[0] ? this.players.get(v.seats[0]) : null;
      if (v.burn && !v.dead) { v.hp -= 13 * dt; if (v.hp <= 0) this.explodeVeh(v); }
      if (v.dead) {
        if (now >= v.respawnAt && !this.players.size === false) {
          const near = [...this.players.values()].some(q => q.alive && Math.hypot(q.x - v.sp.x, q.z - v.sp.z) < 5);
          if (!near) { const nv = this.mkVeh(v.sp, v.id); this.vehs[v.id] = nv; }
        }
      }
      if (!driver || !driver.alive) {
        if (Math.abs(v.vx) + Math.abs(v.vz) > 0.05) { vehStep(this.map, v, { thr: 0, hb: true, steer: 0 }, dt); }
        for (let i = 0; i < 5; i++) if (v.seats[i]) { const q = this.players.get(v.seats[i]); if (q) this.placeOccupant(q, v); }
      } else if (now - v.lastV > 400) {
        // السائق متأخر: تباطؤ طبيعي
        vehStep(this.map, v, { thr: 0, hb: false, steer: 0 }, dt);
        this.placeOccupant(driver, v);
      }
      // الدهس
      if (!v.dead && driver && Math.abs(v.speed) > 3.5) {
        for (const q of [...this.players.values()]) {
          if (!q.alive || q.veh >= 0 || (q.team === driver.team && !this.cfg.ff) || now < q.prot || now < q.runT) continue;
          const [lx, lz] = w2l(v, q.x, q.z);
          if (Math.abs(lx) < vp(v).hw + 0.55 && Math.abs(lz) < vp(v).hl + 0.5 && q.y < 1.4) { q.runT = now + 500; this.hurt(q, Math.abs(v.speed) * (v.type === 1 ? 16 : 9), driver, false, 'veh', v.x, v.z); }
        }
      }
    }
    this.linkVehs();
  }

  // ---------- الذخيرة المتساقطة والتجدد ----------
  dropAmmo(v) {
    if (!this.map) return;
    const y = v.veh >= 0 ? 0 : Math.max(0, v.y);
    const slots = [v.lo.p, v.lo.s];
    slots.forEach((w, i) => {
      const st = computeStats(w, v.lo.a[i]);
      let n = v.bot ? st.mag * 2 : v.am ? v.am[i * 2] + v.am[i * 2 + 1] : st.mag;
      if (!(n > 0)) return;
      n = Math.min(n, st.reserve);
      this.drops.push({ id: this.nextDrop++, x: v.x + (i ? 0.5 : -0.5), y, z: v.z, w, n, exp: this.now() + DROP_LIFE_MS });
    });
    while (this.drops.length > 30) this.drops.shift();
  }
  tickDrops(now) {
    this.drops = this.drops.filter(d => now < d.exp);
    if (!this.drops.length) return;
    for (const p of this.players.values()) {
      if (p.bot || !p.alive || p.veh >= 0) continue;
      for (let i = this.drops.length - 1; i >= 0; i--) {
        const d = this.drops[i];
        if (Math.abs(p.x - d.x) > 1.4 || Math.abs(p.z - d.z) > 1.4 || Math.abs(p.y - d.y) > 1.6) continue;
        const slot = p.lo.p === d.w ? 0 : p.lo.s === d.w ? 1 : -1;
        if (slot < 0) continue;
        const max = computeStats(d.w, p.lo.a[slot]).reserve;
        if (p.am && p.am[slot * 2 + 1] >= max) continue;
        this.drops.splice(i, 1);
        this.sendTo(p, { t: 'ammo', w: d.w, slot, n: d.n });
      }
    }
  }

  // ---------- البوتات ----------
  botThink(b, dt, now) {
    if (!b.alive) return;
    const map = this.map, eyeY = b.y + PLAYER.eye, sl = b.st[0];
    if (now >= b.scanAt) {
      b.scanAt = now + 250 + Math.random() * 250;
      let best = null, bd = 1e9;
      for (const q of this.players.values()) {
        if (!q.alive || q.team === b.team) continue;
        const ddx = q.x - b.x, ddz = q.z - b.z, d = Math.hypot(ddx, ddz);
        if (d > 70 || d > bd) continue;
        const dy = q.y + 1.3 - eyeY, dist = Math.hypot(d, dy);
        if (raycastMap(map, b.x, eyeY, b.z, ddx / dist, dy / dist, ddz / dist, dist) === Infinity) { best = q; bd = d; }
      }
      if (best) {
        if (b.target !== best.id) b.reactAt = now + (450 + Math.random() * 450) * [1.5, 1, 0.6][this.cfg.diff];
        b.target = best.id; b.seenAt = now; b.tx = best.x; b.tz = best.z; b.vis = true;
      } else b.vis = false;
    }
    const tq = b.target ? this.players.get(b.target) : null;
    if (b.target && (!tq || !tq.alive)) { b.target = 0; b.vis = false; }
    if (b.vis && tq) { b.tx = tq.x; b.tz = tq.z; }
    const kind = sl.kind, engage = kind === 'shotgun' ? 9 : kind === 'smg' ? 14 : 22;
    const dxT = b.tx - b.x, dzT = b.tz - b.z, distT = Math.hypot(dxT, dzT);
    let vx = 0, vz = 0, speed = 4.0;
    if (b.vis && distT <= engage) {
      if (now >= b.strafeAt) { b.strafe = Math.random() < 0.5 ? -1 : 1; b.strafeAt = now + 1200 + Math.random() * 1800; }
      const px = -dzT / (distT || 1), pz = dxT / (distT || 1);
      vx = px * b.strafe * 2.4; vz = pz * b.strafe * 2.4;
    } else {
      const chasing = b.target && (b.vis || now - b.seenAt < 6000);
      if (chasing) { if (!b.goal || Math.hypot(b.goal[0] - b.tx, b.goal[1] - b.tz) > 4) { b.goal = [b.tx, b.tz]; b.pathAt = 0; } }
      else if (!b.goal || Math.hypot(b.goal[0] - b.x, b.goal[1] - b.z) < 2) { this.randomGoal(b); }
      if (now >= b.pathAt || !b.path) { b.path = findPath(this.nav, b.x, b.z, b.goal[0], b.goal[1]); b.pi = 0; b.pathAt = now + 1500; if (!b.path) { this.randomGoal(b); } }
      if (b.path && b.pi < b.path.length) {
        const wp = b.path[b.pi], ddx = wp[0] - b.x, ddz = wp[1] - b.z, d = Math.hypot(ddx, ddz);
        if (d < 0.6) b.pi++; else { vx = ddx / d * speed; vz = ddz / d * speed; }
      } else if (!chasing) this.randomGoal(b);
    }
    // كشف التعثّر
    if (now >= b.stuckAt) {
      if ((vx || vz) && Math.hypot(b.x - b.lastX, b.z - b.lastZ) < 0.3) { b.pathAt = 0; b.strafe *= -1; if (!b.target) this.randomGoal(b); }
      b.lastX = b.x; b.lastZ = b.z; b.stuckAt = now + 1000;
    }
    const steps = 3, h = dt / steps;
    for (let i = 0; i < steps; i++) moveBody(map, b, vx, vz, h, false, false);
    for (const v of this.vehs) pushOutVeh(v, b);
    b.moving = !!(vx || vz);
    if (b.vis && distT > 0.1) { b.yaw = Math.atan2(-dxT, -dzT); const dy = (tq ? tq.y + 1.3 : 1.3) - eyeY; b.pitch = Math.atan2(dy, distT); }
    else if (vx || vz) { b.yaw = Math.atan2(-vx, -vz); b.pitch = 0; }
    // إطلاق النار
    if (b.vis && tq && now >= b.reactAt && now >= b.nextShot && distT < sl.range[1] * 0.85) {
      const aimY = tq.y + (Math.random() < 0.18 ? 1.6 : 1.1);
      let dx = tq.x - b.x, dy = aimY - eyeY, dz = tq.z - b.z;
      const dist = Math.hypot(dx, dy, dz);
      const err = (0.022 + 0.05 * Math.min(1, dist / 40)) * [1.7, 1, 0.55][this.cfg.diff];
      const dirs = [];
      const n = sl.pellets;
      for (let i = 0; i < n; i++) {
        const sp = n > 1 ? sl.hip : err;
        let ex = dx / dist + (Math.random() - 0.5) * 2 * sp, ey = dy / dist + (Math.random() - 0.5) * 2 * sp, ez = dz / dist + (Math.random() - 0.5) * 2 * sp;
        const l = Math.hypot(ex, ey, ez); dirs.push([ex / l, ey / l, ez / l]);
      }
      this.processShot(b, 0, dirs);
      b.burst++;
      b.nextShot = now + sl.interval * 1000 * (sl.auto ? 1 : 1.5);
      if (b.burst >= 4 + Math.floor(Math.random() * 5) || !sl.auto) { b.nextShot += 350 + Math.random() * 700; b.burst = 0; }
    }
  }

  randomGoal(b) {
    const nav = this.nav;
    for (let i = 0; i < 20; i++) {
      const idx = Math.floor(Math.random() * nav.grid.length);
      if (!nav.grid[idx]) { b.goal = cellCenter(nav, idx); b.pathAt = 0; return; }
    }
    b.goal = [b.x, b.z];
  }

  // ---------- الحلقة الرئيسية ----------
  tick() {
    const now = this.now(), dt = 0.05;
    if (this.phase === 'lobby') { for (const p of this.players.values()) p.msgs = 0; return; }
    if (now - this.lastSec >= 1000) {
      this.lastSec = now;
      for (const p of this.players.values()) p.msgs = 0;
      this.sendLobby();
    }
    if (this.phase === 'ended') {
      if (now >= this.endAt) this.toLobby();
      else this.sendSnap();
      return;
    }
    if (this.timeLeft() <= 0) { this.endMatch(); return; }
    for (const p of this.players.values()) {
      if (!p.alive && p.respawnAt && now >= p.respawnAt) { p.respawnAt = 0; this.spawn(p); }
      else if (p.bot && p.veh < 0) this.botThink(p, dt, now);
      if (p.alive && this.cfg.regen && p.hp < 100 && now - p.lastHurt > 6000) p.hp = Math.min(100, p.hp + 12 * dt);
    }
    this.tickVehs(dt, now);
    this.tickExtras(now);
    this.tickDrops(now);
    this.simGrenades(dt);
    this.sendSnap();
  }

  sendSnap() {
    const now = this.now(), ps = [];
    for (const p of this.players.values()) {
      const f = (p.crouch ? 1 : 0) | (p.ads ? 2 : 0) | (p.alive ? 4 : 0) | (p.moving ? 8 : 0) | (now < p.reveal ? 16 : 0) | (p.sprint ? 32 : 0);
      ps.push([p.id, r2(p.x), r2(p.y), r2(p.z), r3(p.yaw), r3(p.pitch), f, Math.round(p.hp), p.w, p.grenades, p.veh + 1, p.seat, Math.round(p.shield || 0), p.c4Left | 0]);
    }
    this.bcast({ t: 'snap', tl: this.timeLeft(), sc: this.score, ps, gr: this.grenades.map(g => [g.id, r2(g.x), r2(g.y), r2(g.z)]),
      vs: this.vehs.map(v => [v.id, r2(v.x), r2(v.z), r3(v.h), r2(v.speed), r2(v.steer), v.tires, Math.round(v.hp), (v.burn ? 1 : 0) | (v.dead ? 2 : 0), ...v.seats, v.type | 0]),
      dr: this.drops.map(d => [d.id, r2(d.x), r2(d.y), r2(d.z), d.w, d.n]), ...this.snapExtras() }, true);
  }
}
Object.assign(Game.prototype, extras);
