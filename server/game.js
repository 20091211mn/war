// منطق اللعبة على الخادم: اللوبي، المباراة، الإصابات، القنابل، والبوتات
import { WEAPONS, PRIMARIES, computeStats, sanitizeLoadout, damageAt, defaultLoadout, sanitizeAtt } from '../shared/weapons.js';
import { buildMap, MAP_COUNT, MAP_NAMES } from '../shared/maps.js';
import { PLAYER, rayPlayer, raycastMap, moveBody } from '../shared/physics.js';
import { buildNav, findPath, cellOf, cellCenter, nearestFree } from '../shared/nav.js';

export const MAX_HUMANS = 16;
const SCORE_LIMITS = [10, 20, 30, 50, 100];
const TIME_LIMITS = [180, 300, 600, 900];
const RESPAWN_MS = 3000, PROT_MS = 2000, END_MS = 12000;
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
    this.cfg = { map: 0, scoreLimit: 30, timeLimit: 600, fill: 0 };
    this.map = null; this.nav = null;
    this.grenades = []; this.nextGren = 1;
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
      grenades: 2, lastGren: 0, epoch: 0, msgs: 0,
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
    if (this.phase !== 'lobby') {
      this.sendTo(p, { t: 'start', map: this.cfg.map, cfg: this.cfg, tl: this.timeLeft() });
      this.rebalanceBots();
      if (this.phase === 'playing') this.spawn(p);
    }
    this.sendLobby();
    return p;
  }

  disconnect(p) {
    if (!p || !this.players.has(p.id)) return;
    this.players.delete(p.id);
    p.conn = null;
    if (this.hostId === p.id) { const h = this.humans()[0]; this.hostId = h ? h.id : 0; }
    this.bcast({ t: 'left', id: p.id });
    if (this.humans().length === 0) this.reset();
    else { this.rebalanceBots(); this.sendLobby(); }
  }

  reset() {
    for (const [id, q] of [...this.players]) if (q.bot) this.players.delete(id);
    this.phase = 'lobby'; this.map = null; this.nav = null; this.grenades = []; this.score = [0, 0];
    for (const q of this.players.values()) { q.alive = false; q.kills = 0; q.deaths = 0; }
  }

  timeLeft() { return this.phase === 'lobby' ? this.cfg.timeLimit : Math.max(0, Math.ceil(this.cfg.timeLimit - (this.now() - this.startAt) / 1000)); }

  sendLobby() {
    const ps = [];
    for (const q of this.players.values()) ps.push({ id: q.id, n: q.name, tm: q.team, k: q.kills, d: q.deaths, b: q.bot ? 1 : 0, p: q.lo.p, s: q.lo.s, a: q.lo.a });
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
        if (t >= 0) p.team = t;
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
        this.sendLobby();
        break;
      }
      case 'start': if (isHost) this.startMatch(); break;
      case 'back': if (isHost && this.phase !== 'lobby') this.toLobby(); break;
      case 'st': {
        if (this.phase !== 'playing' || !p.alive || m.e !== p.epoch) break;
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
        if (this.phase !== 'playing' || !p.alive || !Array.isArray(m.d)) break;
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
      case 'rl': this.bcastExcept(p, { t: 'rl', id: p.id }); break;
      case 'gren': {
        if (this.phase !== 'playing' || !p.alive || p.grenades <= 0 || !Array.isArray(m.d)) break;
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
  bcastExcept(p, obj) { const s = JSON.stringify(obj); for (const q of this.players.values()) if (q !== p && q.conn) q.conn.send(s); }

  // ---------- المباراة ----------
  startMatch() {
    if (this.phase !== 'lobby') return;
    this.map = buildMap(this.cfg.map); this.nav = buildNav(this.map);
    this.score = [0, 0]; this.grenades = []; this.startAt = this.now();
    for (const q of this.players.values()) { q.kills = 0; q.deaths = 0; q.alive = false; }
    this.phase = 'playing';
    this.bcast({ t: 'start', map: this.cfg.map, cfg: this.cfg, tl: this.cfg.timeLimit });
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
    p.hp = 100; p.alive = true; p.crouch = false; p.ads = false; p.moving = false; p.sprint = false; p.w = 0;
    p.grenades = 2; p.prot = this.now() + PROT_MS; p.epoch++;
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
    const dmg = new Map(), ends = [];
    for (const [dx, dy, dz] of dirs) {
      let best = raycastMap(map, ox, oy, oz, dx, dy, dz, maxT);
      if (best === Infinity) best = maxT;
      let hitP = null, hitHead = false;
      for (const q of this.players.values()) {
        if (q === p || !q.alive || q.team === p.team || now < q.prot) continue;
        const r = rayPlayer(q, ox, oy, oz, dx, dy, dz, best);
        if (r && r.t < best) { best = r.t; hitP = q; hitHead = r.head; }
      }
      if (hitP) {
        const e = dmg.get(hitP) || { d: 0, h: false };
        e.d += damageAt(s, best) * (hitHead ? s.head : 1); e.h = e.h || hitHead;
        dmg.set(hitP, e);
      }
      ends.push([r2(ox + dx * best), r2(oy + dy * best), r2(oz + dz * best), hitP ? 1 : 0]);
    }
    if (!s.muted) p.reveal = now + 1500;
    this.bcastExcept(p, { t: 'shot', id: p.id, w: s.id, o: [r2(ox), r2(oy), r2(oz)], e: ends, s: s.muted ? 1 : 0 });
    for (const [q, e] of dmg) this.hurt(q, e.d, p, e.h, s.id, p.x, p.z);
  }

  hurt(v, amount, a, head, wid, fx, fz) {
    if (this.phase !== 'playing' || !v.alive || this.now() < v.prot) return;
    amount = Math.round(amount);
    if (amount <= 0) return;
    v.hp -= amount;
    const msg = { t: 'hit', v: v.id, a: a.id, d: amount, h: head ? 1 : 0, fx: r2(fx), fz: r2(fz) };
    this.sendTo(v, msg);
    if (a !== v) this.sendTo(a, msg);
    if (v.hp <= 0) this.kill(v, a, wid, head);
  }

  kill(v, a, wid, head) {
    v.hp = 0; v.alive = false; v.deaths++; v.respawnAt = this.now() + RESPAWN_MS;
    if (a !== v) { a.kills++; this.score[a.team]++; }
    this.bcast({ t: 'kill', k: a.id, v: v.id, w: wid, h: head ? 1 : 0 });
    this.checkEnd();
  }

  checkEnd() {
    if (this.phase !== 'playing') return;
    if (this.score[0] >= this.cfg.scoreLimit || this.score[1] >= this.cfg.scoreLimit) this.endMatch();
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

  explode(g) {
    const R = 7;
    this.bcast({ t: 'boom', x: r2(g.x), y: r2(g.y), z: r2(g.z) });
    const owner = this.players.get(g.owner);
    for (const q of [...this.players.values()]) {
      if (!q.alive) continue;
      if (q.team === g.team && q.id !== g.owner) continue;
      const cy = q.y + 1.0, d = Math.hypot(q.x - g.x, cy - g.y, q.z - g.z);
      if (d > R) continue;
      const t = raycastMap(this.map, g.x, g.y + 0.2, g.z, (q.x - g.x) / (d || 1), (cy - g.y - 0.2) / (d || 1), (q.z - g.z) / (d || 1), d);
      if (t !== Infinity && t < d - 0.3) continue;
      const dmg = 140 * Math.pow(1 - d / R, 1.2);
      this.hurt(q, dmg, owner || q, false, 'gren', g.x, g.z);
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
        if (b.target !== best.id) b.reactAt = now + 450 + Math.random() * 450;
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
    b.moving = !!(vx || vz);
    if (b.vis && distT > 0.1) { b.yaw = Math.atan2(-dxT, -dzT); const dy = (tq ? tq.y + 1.3 : 1.3) - eyeY; b.pitch = Math.atan2(dy, distT); }
    else if (vx || vz) { b.yaw = Math.atan2(-vx, -vz); b.pitch = 0; }
    // إطلاق النار
    if (b.vis && tq && now >= b.reactAt && now >= b.nextShot && distT < sl.range[1] * 0.85) {
      const aimY = tq.y + (Math.random() < 0.18 ? 1.6 : 1.1);
      let dx = tq.x - b.x, dy = aimY - eyeY, dz = tq.z - b.z;
      const dist = Math.hypot(dx, dy, dz);
      const err = 0.022 + 0.05 * Math.min(1, dist / 40);
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
      else if (p.bot) this.botThink(p, dt, now);
    }
    this.simGrenades(dt);
    this.sendSnap();
  }

  sendSnap() {
    const now = this.now(), ps = [];
    for (const p of this.players.values()) {
      const f = (p.crouch ? 1 : 0) | (p.ads ? 2 : 0) | (p.alive ? 4 : 0) | (p.moving ? 8 : 0) | (now < p.reveal ? 16 : 0) | (p.sprint ? 32 : 0);
      ps.push([p.id, r2(p.x), r2(p.y), r2(p.z), r3(p.yaw), r3(p.pitch), f, p.hp, p.w, p.grenades]);
    }
    this.bcast({ t: 'snap', tl: this.timeLeft(), sc: this.score, ps, gr: this.grenades.map(g => [g.id, r2(g.x), r2(g.y), r2(g.z)]) }, true);
  }
}
