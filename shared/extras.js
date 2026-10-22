// مميزات إضافية على الخادم: حزم الإسعاف، وضع السيطرة على المنطقة، مكافآت سلسلة القتل
import { cellOf, cellCenter, nearestFree } from './nav.js';
import { groundAt, raycastMap } from './physics.js';

export const KIT_RESPAWN_MS = 25000, KIT_HEAL = 50, ZONE_R = 7, ZONE_ROT_MS = 60000, UAV_MS = 12000, SHIELD = 60;
export const DRONE = { hp: 40, ms: 25000, cd: 20000, cdLost: 40000, speed: 12, range: 70, spot: 45, r: 0.5, maxY: 30 };
export const ZONE_SCORE_MUL = 5;
// تصادم الطائرة مع الخريطة (مشترك بين العميل والخادم)
export function droneHit(map, x, y, z) {
  if (y < 0.45 || y > DRONE.maxY || Math.abs(x) > map.W / 2 - 0.5 || Math.abs(z) > map.D / 2 - 0.5) return true;
  for (const b of map.boxes) if (x + 0.35 > b.x0 && x - 0.35 < b.x1 && z + 0.35 > b.z0 && z - 0.35 < b.z1 && y + 0.2 > b.y0 && y - 0.2 < b.y1) return true;
  return false;
}
export function raySphere(ox, oy, oz, dx, dy, dz, cx, cy, cz, r, maxT) {
  const lx = cx - ox, ly = cy - oy, lz = cz - oz, tca = lx * dx + ly * dy + lz * dz;
  if (tca < 0) return null;
  const d2 = lx * lx + ly * ly + lz * lz - tca * tca; if (d2 > r * r) return null;
  const t = tca - Math.sqrt(r * r - d2); return t >= 0 && t < maxT ? t : null;
} // حد النقاط في وضع السيطرة = حد النقاط × 5

const r2d = (v) => Math.round(v * 100) / 100;
function rngf(seed) {
  let a = seed >>> 0;
  return () => { a = (a + 0x6D2B79F5) >>> 0; let t = a; t = Math.imul(t ^ (t >>> 15), t | 1); t ^= t + Math.imul(t ^ (t >>> 7), t | 61); return ((t ^ (t >>> 14)) >>> 0) / 4294967296; };
}

export const extras = {
  limit() { return this.cfg.scoreLimit * (this.cfg.mode === 1 ? ZONE_SCORE_MUL : 1); },

  freeSpot(x, z) { // أقرب نقطة حرة على الأرض المستوية
    const nav = this.nav, m = this.map;
    for (let r = 0; r < 6; r++) {
      const idx = nearestFree(nav, cellOf(nav, x, z));
      if (idx < 0) return null;
      const [cx, cz] = cellCenter(nav, idx);
      if (groundAt(m, cx, cz, 0) === 0) return [cx, cz];
      x += (r % 2 ? 1 : -1) * 2; z += 2;
    }
    return null;
  },

  initExtras() {
    const m = this.map, nav = this.nav, now = this.now();
    this.kits = []; this.zones = []; this.uav = [0, 0]; this.zoneIdx = 1; this.zoneAt = now; this.zoneTick = now; this.zoneState = 3;
    const rng = rngf(m.id * 131 + 7), picks = [];
    const spawns = [...m.red, ...m.blue];
    for (let tries = 0; tries < 600 && picks.length < 3; tries++) {
      const sp = this.freeSpot((rng() - 0.5) * (m.W - 26), (rng() - 0.5) * (m.D - 18));
      if (!sp) continue;
      const mir = this.freeSpot(-sp[0], -sp[1]);
      if (!mir || Math.hypot(mir[0] + sp[0], mir[1] + sp[1]) > 1.5) continue;
      if (spawns.some(([x, z]) => Math.hypot(x - sp[0], z - sp[1]) < 9 || Math.hypot(x - mir[0], z - mir[1]) < 9)) continue;
      if (Math.hypot(sp[0], sp[1]) < 8) continue;
      if (picks.some(([x, z]) => Math.hypot(x - sp[0], z - sp[1]) < 14 || Math.hypot(x + sp[0], z + sp[1]) < 14)) continue;
      picks.push(sp);
    }
    let id = 1;
    for (const [x, z] of picks) { this.kits.push({ id: id++, x, z, up: true, at: 0 }); this.kits.push({ id: id++, x: -x, z: -z, up: true, at: 0 }); }
    // مناطق السيطرة: طرف، منتصف، طرف مقابل
    const A = this.freeSpot(-m.W * 0.2, -m.D * 0.22), C = this.freeSpot(0, 0), B = A && this.freeSpot(-A[0], -A[1]);
    if (A && C && B) this.zones = [A, C, B];
  },

  matchInfo() {
    return { kits: (this.kits || []).map(k => [k.id, k.x, k.z]), zones: this.zones || [] };
  },

  snapExtras() {
    const o = { dn: this.droneSnap(), kt: (this.kits || []).map(k => (k.up ? 1 : 0)).join(''), uv: [Math.max(0, this.uav[0] - this.now()) | 0, Math.max(0, this.uav[1] - this.now()) | 0] };
    if (this.cfg.mode === 1 && this.zones.length) { const z = this.zones[this.zoneIdx]; o.zn = [this.zoneIdx, z[0], z[1], ZONE_R, this.zoneState, Math.max(0, Math.ceil((ZONE_ROT_MS - (this.now() - this.zoneAt)) / 1000))]; }
    return o;
  },

  tickExtras(now) {
    this.tickDrones(now);
    for (const k of this.kits) if (!k.up && now >= k.at) { k.up = true; this.bcast({ t: 'kit', id: k.id, up: 1 }); }
    for (const p of this.players.values()) {
      if (!p.alive || p.veh >= 0) continue;
      if (p.hp < 100) for (const k of this.kits) {
        if (!k.up || Math.abs(p.x - k.x) > 1.3 || Math.abs(p.z - k.z) > 1.3 || Math.abs(p.y) > 1.6) continue;
        p.hp = Math.min(100, p.hp + KIT_HEAL); k.up = false; k.at = now + KIT_RESPAWN_MS;
        this.bcast({ t: 'kit', id: k.id, up: 0, by: p.id });
        if (!p.bot) this.sendTo(p, { t: 'heal', n: KIT_HEAL });
        break;
      }
      // البوتات تبحث عن إسعاف عند الإصابة أو تتجه للمنطقة
      if (p.bot) {
        if (p.hp < 55 && now - (p.kitAt || 0) > 2500) {
          let best = null, bd = 45;
          for (const k of this.kits) { if (!k.up) continue; const d = Math.hypot(k.x - p.x, k.z - p.z); if (d < bd) { bd = d; best = k; } }
          p.kitAt = now; if (best && !p.vis) { p.goal = [best.x, best.z]; p.pathAt = 0; p.seekKit = now + 6000; }
        } else if (this.cfg.mode === 1 && this.zones.length && !p.target && p.id % 4 !== 0 && now >= (p.zoneAt || 0) && now >= (p.seekKit || 0)) {
          const z = this.zones[this.zoneIdx], a = Math.random() * 6.28, r = Math.random() * ZONE_R * 0.7;
          const g = this.freeSpot(z[0] + Math.cos(a) * r, z[1] + Math.sin(a) * r);
          p.zoneAt = now + 3500 + Math.random() * 2500; if (g) { p.goal = g; p.pathAt = 0; }
        }
      }
    }
    if (this.cfg.mode === 1 && this.zones.length) {
      if (now - this.zoneAt >= ZONE_ROT_MS) { this.zoneIdx = (this.zoneIdx + 1) % this.zones.length; this.zoneAt = now; const z = this.zones[this.zoneIdx]; this.bcast({ t: 'zone', i: this.zoneIdx, x: z[0], z: z[1] }); this.sys('تحركت منطقة السيطرة'); }
      if (now - this.zoneTick >= 1000) {
        this.zoneTick = now;
        const z = this.zones[this.zoneIdx], c = [0, 0];
        for (const p of this.players.values()) if (p.alive && p.veh < 0 && Math.hypot(p.x - z[0], p.z - z[1]) <= ZONE_R && Math.abs(p.y) < 4) c[p.team]++;
        this.zoneState = c[0] && c[1] ? 2 : c[0] ? 0 : c[1] ? 1 : 3;
        if (this.zoneState === 0 || this.zoneState === 1) { this.score[this.zoneState]++; this.checkEnd(); }
      }
    }
  },

  // ---------- الطائرة المسيّرة ----------
  launchDrone(p) {
    const now = this.now();
    if (this.phase !== 'playing' || !p.alive || p.veh >= 0 || p.drone || now < (p.droneCd || 0)) return false;
    p.drone = { x: p.x, y: p.y + 2, z: p.z, yaw: p.yaw, pitch: 0, hp: DRONE.hp, until: now + DRONE.ms, spotAt: 0, lastDp: now };
    if (droneHit(this.map, p.drone.x, p.drone.y, p.drone.z)) p.drone.y = p.y + 1.2;
    this.bcast({ t: 'dron', id: p.id, on: 1, ms: DRONE.ms });
    return true;
  },
  landDrone(p, why = 'land', cd = DRONE.cd) {
    if (!p.drone) return;
    p.drone = null; p.droneCd = this.now() + cd;
    this.bcast({ t: 'dron', id: p.id, on: 0, why, cd });
  },
  onDroneMsg(p, m) {
    if (m.t === 'drone') { if (m.on) this.launchDrone(p); else this.landDrone(p); return; }
    const d = p.drone; if (!d || this.phase !== 'playing' || !p.alive) return;
    if (m.t === 'dboom') {
      const g = { x: d.x, y: d.y, z: d.z, owner: p.id, team: p.team };
      this.landDrone(p, 'boom', DRONE.cdLost); this.explode(g); return;
    }
    // dp: تحديث الموضع مع تحقّق السرعة والتصادم
    const f = (v) => typeof v === 'number' && Number.isFinite(v);
    if (!f(m.x) || !f(m.y) || !f(m.z) || !f(m.yaw)) return;
    const now = this.now(), dt = Math.min(0.4, Math.max(0.02, (now - d.lastDp) / 1000)); d.lastDp = now;
    const dist = Math.hypot(m.x - d.x, m.y - d.y, m.z - d.z);
    if (dist > DRONE.speed * dt * 1.8 + 1 || droneHit(this.map, m.x, m.y, m.z) || Math.hypot(m.x - p.x, m.z - p.z) > DRONE.range + 5) {
      this.sendTo(p, { t: 'dfix', x: d.x, y: d.y, z: d.z }); return;
    }
    d.x = m.x; d.y = m.y; d.z = m.z; d.yaw = m.yaw; d.pitch = f(m.pitch) ? Math.max(-1.5, Math.min(1.5, m.pitch)) : 0;
  },
  hurtDrone(q, amount, a) {
    const d = q.drone; if (!d) return;
    d.hp -= amount;
    if (d.hp <= 0) {
      this.bcast({ t: 'boom', x: r2d(d.x), y: r2d(d.y), z: r2d(d.z), sm: 1 });
      this.landDrone(q, 'shot', DRONE.cdLost);
      this.sys(`${a.name} أسقط طائرة ${q.name}`);
    }
  },
  tickDrones(now) {
    for (const p of this.players.values()) {
      const d = p.drone; if (!d) continue;
      if (!p.alive || p.veh >= 0) { this.landDrone(p, 'lost', DRONE.cdLost); continue; }
      if (now >= d.until) { this.landDrone(p, 'battery', DRONE.cd); continue; }
      if (now >= d.spotAt) {
        d.spotAt = now + 600;
        for (const q of this.players.values()) {
          if (!q.alive || q.team === p.team) continue;
          const dx = q.x - d.x, dy = q.y + 1.2 - d.y, dz = q.z - d.z, L = Math.hypot(dx, dy, dz);
          if (L > DRONE.spot || L < 0.1) continue;
          if (raycastMap(this.map, d.x, d.y, d.z, dx / L, dy / L, dz / L, L) === Infinity) q.reveal = Math.max(q.reveal || 0, now + 2500);
        }
      }
    }
  },
  droneSnap() { const o = []; for (const p of this.players.values()) if (p.drone) o.push([p.id, r2d(p.drone.x), r2d(p.drone.y), r2d(p.drone.z), Math.round(p.drone.yaw * 100) / 100, Math.round(p.drone.hp)]); return o; },

  onKillExtras(v, a) {
    if (v.drone) this.landDrone(v, 'lost', DRONE.cdLost);
    v.streak = 0; v.shield = 0;
    if (a === v || a.team === v.team || !this.cfg.streaks) return;
    a.streak = (a.streak || 0) + 1;
    const now = this.now(), s = a.streak, msg = (k) => this.bcast({ t: 'ks', id: a.id, n: a.name, tm: a.team, k });
    if (s === 3) { this.uav[a.team] = now + UAV_MS; msg('uav'); }
    else if (s === 5) { a.shield = SHIELD; msg('shield'); }
    else if (s === 8) { a.hp = 100; a.shield = SHIELD; a.grenades = 4; msg('supply'); }
  },
};
