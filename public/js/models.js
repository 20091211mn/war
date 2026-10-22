// نماذج الأسلحة واللاعبين والمؤثرات (كلها مجسّمات مكعبة بدون ملفات)
import { trs, mul, tmp, basis } from './gl.js';

const C = { d: [0.14, 0.14, 0.16], g: [0.34, 0.35, 0.38], b: [0.05, 0.05, 0.06], w: [0.42, 0.27, 0.13], t: [0.5, 0.45, 0.32], o: [0.9, 0.5, 0.1], s: [0.86, 0.68, 0.55] };
// أجزاء السلاح: [x,y,z,sx,sy,sz,لون] — الاتجاه الأمامي = -Z ، mz = موضع الفوهة، sy = ارتفاع المصوّب الحديدي
export const GUNS = {
  rifle: { mz: -0.44, sy: 0.058, parts: [[0, 0, -0.02, 0.05, 0.07, 0.36, 'd'], [0, 0.005, -0.31, 0.02, 0.02, 0.2, 'b'], [0, -0.008, -0.17, 0.06, 0.055, 0.18, 'g'], [0, -0.01, 0.24, 0.04, 0.085, 0.16, 'd'], [0, -0.05, 0.07, 0.03, 0.09, 0.04, 'd'], [0, 0.05, 0.0, 0.015, 0.02, 0.05, 'b']], mag: [0, -0.085, -0.01, 0.035, 0.11, 0.06] },
  smg: { mz: -0.3, sy: 0.052, parts: [[0, 0, -0.02, 0.05, 0.07, 0.26, 'd'], [0, 0.005, -0.2, 0.02, 0.02, 0.12, 'b'], [0, -0.01, 0.2, 0.03, 0.06, 0.14, 'g'], [0, -0.05, 0.06, 0.03, 0.09, 0.04, 'd']], mag: [0, -0.1, -0.03, 0.03, 0.14, 0.05] },
  shotgun: { mz: -0.52, sy: 0.05, parts: [[0, 0, -0.04, 0.05, 0.06, 0.3, 'd'], [0, 0.01, -0.36, 0.03, 0.03, 0.4, 'b'], [0, -0.03, -0.3, 0.028, 0.028, 0.32, 'g'], [0, -0.025, -0.2, 0.055, 0.045, 0.12, 'w'], [0, -0.01, 0.24, 0.04, 0.1, 0.2, 'w'], [0, -0.05, 0.06, 0.03, 0.08, 0.04, 'w']], mag: null },
  sniper: { mz: -0.8, sy: 0.058, parts: [[0, 0, -0.02, 0.05, 0.07, 0.42, 'd'], [0, 0.005, -0.52, 0.022, 0.022, 0.5, 'b'], [0, -0.01, 0.26, 0.05, 0.1, 0.26, 'w'], [0, -0.05, 0.08, 0.03, 0.09, 0.04, 'd'], [0, -0.04, -0.4, 0.015, 0.06, 0.015, 'b']], mag: [0, -0.07, -0.02, 0.03, 0.06, 0.07] },
  lmg: { mz: -0.52, sy: 0.07, parts: [[0, 0, -0.03, 0.06, 0.085, 0.4, 'd'], [0, 0.01, -0.36, 0.025, 0.025, 0.3, 'b'], [0, -0.015, -0.24, 0.07, 0.06, 0.2, 'g'], [0, -0.01, 0.27, 0.045, 0.09, 0.18, 'd'], [0, -0.05, 0.08, 0.03, 0.09, 0.04, 'd'], [0, 0.055, -0.05, 0.03, 0.02, 0.1, 'b']], mag: [0, -0.1, 0.0, 0.09, 0.09, 0.1] },
  dmr: { mz: -0.66, sy: 0.058, parts: [[0, 0, -0.02, 0.05, 0.07, 0.46, 'd'], [0, 0.005, -0.4, 0.02, 0.02, 0.3, 'b'], [0, -0.008, -0.26, 0.055, 0.05, 0.22, 'w'], [0, -0.01, 0.28, 0.04, 0.09, 0.2, 'w'], [0, -0.05, 0.08, 0.03, 0.09, 0.04, 'd']], mag: [0, -0.08, -0.02, 0.03, 0.09, 0.06] },
  pistol: { mz: -0.12, sy: 0.045, parts: [[0, 0.015, -0.03, 0.032, 0.035, 0.18, 'd'], [0, -0.012, -0.03, 0.03, 0.03, 0.15, 'g'], [0, -0.06, 0.05, 0.03, 0.09, 0.045, 'b']], mag: null },
  magnum: { mz: -0.22, sy: 0.05, parts: [[0, 0.0, -0.03, 0.04, 0.055, 0.2, 'g'], [0, 0.015, -0.17, 0.028, 0.03, 0.2, 'd'], [0, -0.06, 0.06, 0.035, 0.1, 0.05, 'w'], [0, 0.0, 0.0, 0.05, 0.055, 0.06, 'b']], mag: null },
};
const OPT = { red: [0.03, 0.035, 0.05, 0.075], x2: [0.034, 0.034, 0.12, 0.08], x4: [0.04, 0.04, 0.18, 0.085], x8: [0.045, 0.045, 0.24, 0.09] };
export const sightTop = (st) => { const g = GUNS[st.kind], o = st.att.optic; return o !== 'iron' && OPT[o] ? OPT[o][3] : g.sy; };

// يرسم السلاح في مصفوفة المجموعة G
export function drawGun(eng, G, st, flash) {
  const g = GUNS[st.kind], a = st.att;
  const part = (x, y, z, sx, sy, sz, col, unlit) => {
    const m = mul(tmp(), G, trs(tmp(), x, y, z, 0, 0, sx, sy, sz));
    eng.cubeAt(m, col[0], col[1], col[2], unlit);
  };
  for (const p of g.parts) part(p[0], p[1], p[2], p[3], p[4], p[5], C[p[6]]);
  if (g.mag) {
    const k = a.mag === 'ext' ? 1.4 : 1;
    part(g.mag[0], g.mag[1] - (k - 1) * g.mag[4] * 0.5, g.mag[2], g.mag[3], g.mag[4] * k, g.mag[5], a.mag === 'fast' ? C.o : C.b);
  }
  if (a.optic !== 'iron' && OPT[a.optic]) {
    const o = OPT[a.optic];
    part(0, o[3] - o[1] / 2 + 0.0, -0.04, o[0], o[1], o[2], C.b);
    part(0, o[3] - o[1] / 2, -0.04 - o[2] / 2, o[0] * 0.8, o[1] * 0.8, 0.004, a.optic === 'red' ? [0.9, 0.1, 0.1] : [0.3, 0.5, 0.8], true);
  }
  if (a.muzzle === 'supp') part(0, 0.005, g.mz + 0.0, 0.034, 0.034, 0.16, C.b);
  else if (a.muzzle === 'comp') part(0, 0.005, g.mz + 0.02, 0.036, 0.036, 0.045, C.g);
  if (a.under === 'grip') part(0, -0.07, g.mz * 0.5 - 0.02, 0.028, 0.08, 0.03, C.d);
  else if (a.under === 'laser') { part(0, -0.03, g.mz * 0.5, 0.02, 0.02, 0.06, C.b); part(0, -0.03, g.mz * 0.5 - 0.035, 0.008, 0.008, 0.01, [1, 0.1, 0.1], true); }
  if (flash > 0) {
    const mzz = g.mz - (a.muzzle === 'supp' ? 0.16 : 0) - 0.03, s = (st.comp ? 1.4 : 1) * (a.muzzle === 'supp' ? 0.35 : 1) * (0.07 + 0.05 * flash);
    const m = mul(tmp(), G, trs(tmp(), 0, 0.005, mzz, 0, 0, s, s, s * 1.8));
    eng.cubeAt(m, 1, 0.8, 0.35, true, 0.85 * flash, true);
  }
}

// ---- اللاعب (من منظور الآخرين) ----
const TEAM = [[0.82, 0.14, 0.14], [0.15, 0.36, 0.9]];
const TEAM_D = [[0.5, 0.08, 0.08], [0.08, 0.2, 0.55]];
function pivot(o, P, x, y, z, rx, ox, oy, oz, sx, sy, sz) {
  const cx = Math.cos(rx), sX = Math.sin(rx);
  return mul(o, P, trs(tmp(), x + ox, y + oy * cx - oz * sX, z + oy * sX + oz * cx, 0, rx, sx, sy, sz));
}
export function drawAvatar(eng, a, t) {
  // a: {x,y,z,yaw,pitch,crouch,team,moving,sprint,stats,deadT(<0 حي), flash, phase, sit, lean}
  const dead = a.deadT >= 0;
  const cs = a.crouch ? 0.69 : 1;
  let P;
  if (dead) {
    const k = Math.min(1, a.deadT / 0.35);
    P = trs(tmp(), a.x, a.y + 0.18 * k, a.z, a.yaw, -Math.PI / 2 * k, 1, 1, 1);
  } else P = trs(tmp(), a.x, a.y, a.z, a.yaw, 0, 1, cs, 1);
  const tc = TEAM[a.team] || TEAM[0], td = TEAM_D[a.team] || TEAM_D[0], pants = [0.17, 0.18, 0.15], boot = [0.08, 0.07, 0.07], vest = [0.2, 0.22, 0.2], glove = [0.1, 0.1, 0.11];
  if (a.far && !dead) { // مستوى تفصيل منخفض للبعيد: 5 مكعبات فقط
    const cb = (x, y, z, sx, sy, sz, c) => eng.cubeAt(mul(tmp(), P, trs(tmp(), x, y, z, 0, 0, sx, sy, sz)), c[0], c[1], c[2], false);
    cb(0, 0.42, 0, 0.4, 0.84, 0.24, pants); cb(0, 1.1, 0, 0.5, 0.6, 0.3, tc); cb(0, 1.58, 0, 0.24, 0.26, 0.24, td);
    if (a.stats) cb(0.12, 1.2, -0.35, 0.06, 0.08, 0.6, vest);
    return;
  }
  const mv = a.moving && !dead ? 1 : 0, sw = Math.sin(a.phase) * 0.75 * mv;
  const cube = (m, c, unlit) => eng.cubeAt(m, c[0], c[1], c[2], unlit);
  // ساقان: فخذ + ساق + حذاء (وضع الجلوس: الفخذ أفقي)
  const legs = (x, rot) => {
    if (a.sit && !dead) {
      cube(mul(tmp(), P, trs(tmp(), x, 0.62, -0.2, 0, 0, 0.19, 0.18, 0.5)), pants);
      cube(mul(tmp(), P, trs(tmp(), x, 0.3, -0.42, 0, 0, 0.17, 0.5, 0.18)), pants);
      cube(mul(tmp(), P, trs(tmp(), x, 0.06, -0.5, 0, 0, 0.2, 0.12, 0.34)), boot);
    } else {
      cube(pivot(tmp(), P, x, 0.8, 0, rot, 0, -0.3, 0, 0.2, 0.62, 0.22), pants);
      cube(pivot(tmp(), P, x, 0.8, 0, rot, 0, -0.74, -0.02, 0.21, 0.16, 0.3), boot);
    }
  };
  legs(-0.12, sw); legs(0.12, -sw);
  // الجزء العلوي (قد يميل خارج المركبة)
  const U = a.lean ? mul(tmp(), P, trs(tmp(), a.lean[0], 0.02, a.lean[1], 0, 0, 1, 1, 1)) : P;
  cube(mul(tmp(), U, trs(tmp(), 0, 1.1, 0, 0, 0, 0.44, 0.58, 0.25)), tc);                 // قميص
  cube(mul(tmp(), U, trs(tmp(), 0, 1.13, 0.015, 0, 0, 0.5, 0.46, 0.29)), vest);           // سترة واقية
  cube(mul(tmp(), U, trs(tmp(), 0, 1.22, -0.15, 0, 0, 0.36, 0.2, 0.05)), [0.26, 0.28, 0.25]); // لوح أمامي
  for (const px of [-0.14, 0, 0.14]) cube(mul(tmp(), U, trs(tmp(), px, 0.98, -0.16, 0, 0, 0.1, 0.12, 0.06)), [0.16, 0.17, 0.15]); // جيوب
  cube(mul(tmp(), U, trs(tmp(), 0, 1.0, 0.2, 0, 0, 0.36, 0.42, 0.14)), [0.18, 0.2, 0.16]);  // حقيبة ظهر
  cube(mul(tmp(), U, trs(tmp(), 0, 0.92, 0, 0, 0, 0.5, 0.1, 0.3)), [0.1, 0.1, 0.11]);       // حزام
  cube(mul(tmp(), U, trs(tmp(), 0, 1.44, 0, 0, 0, 0.11, 0.1, 0.11)), C.s);                  // رقبة
  // الرأس: وجه + خوذة + نظارة
  const HP = mul(tmp(), U, trs(tmp(), 0, 1.58, 0, 0, a.pitch * 0.5, 1, 1, 1));
  cube(mul(tmp(), HP, trs(tmp(), 0, 0, 0, 0, 0, 0.21, 0.23, 0.22)), C.s);
  eng.sphereAt(mul(tmp(), HP, trs(tmp(), 0, 0.06, 0.01, 0, 0, 0.34, 0.27, 0.35)), td[0] * 0.9, td[1] * 0.9, td[2] * 0.9, false);
  cube(mul(tmp(), HP, trs(tmp(), 0, 0.02, -0.115, 0, 0, 0.23, 0.07, 0.03)), [0.05, 0.06, 0.08]);
  cube(mul(tmp(), HP, trs(tmp(), 0, -0.1, -0.08, 0, 0, 0.18, 0.05, 0.08)), [0.12, 0.12, 0.12]);
  const st = a.stats;
  const ap = a.sprint ? 0.2 : a.pitch;
  const arm = (x, rot, len = 0.54) => {
    cube(pivot(tmp(), U, x, 1.36, 0, rot, 0, -len / 2, 0, 0.12, len, 0.12), td);
    cube(pivot(tmp(), U, x, 1.36, 0, rot, 0, -len - 0.02, 0, 0.1, 0.1, 0.1), glove);
  };
  if (st && !dead) {
    const base = a.sprint ? 0.9 : Math.PI / 2;
    arm(0.27, base + ap - 0.12); arm(-0.27, base + ap + 0.12);
    const G = pivot(tmp(), U, 0.12, 1.3, 0, ap, 0, -0.05, -0.38, 1, 1, 1);
    drawGun(eng, G, st, a.flash || 0);
  } else { arm(0.27, 0.05); arm(-0.27, -0.05); }
}

// ---- مجسم السلاح بمنظور اللاعب ----
export function drawViewmodel(eng, vm) {
  const st = vm.stats, g = GUNS[st.kind], k = vm.ads;
  const top = sightTop(st);
  const hip = [0.17, -0.17, -0.33], ads = [0, -top - 0.002, -0.36];
  let x = hip[0] + (ads[0] - hip[0]) * k, y = hip[1] + (ads[1] - hip[1]) * k, z = hip[2] + (ads[2] - hip[2]) * k;
  x += vm.bobX + vm.swayX; y += vm.bobY + vm.swayY; z += vm.kick * 0.045;
  let rx = vm.kick * 0.05, ry = 0;
  if (vm.reload >= 0) { const s = Math.sin(vm.reload * Math.PI); y -= s * 0.13; rx -= s * 0.5; x += s * 0.03; }
  if (vm.swap < 1) { const s = 1 - vm.swap; y -= s * 0.25; rx -= s * 0.6; }
  if (vm.sprint) { y -= 0.06; rx -= 0.35; ry = 0.4; x -= 0.04; }
  eng.beginViewmodel();
  const G = trs(tmp(), x, y, z, ry, rx, 1, 1, 1);
  drawGun(eng, G, st, vm.flash);
  // الأيدي والأكمام
  const sleeve = TEAM_D[vm.team] || TEAM_D[0], skin = C.s;
  const part = (px, py, pz, sx, sy, sz, c) => eng.cubeAt(mul(tmp(), G, trs(tmp(), px, py, pz, 0, 0, sx, sy, sz)), c[0], c[1], c[2], false);
  const gz = st.kind === 'pistol' || st.kind === 'magnum' ? 0.04 : 0.06;
  part(0.0, -0.075, gz, 0.05, 0.05, 0.06, skin);
  part(0.015, -0.12, gz + 0.12, 0.065, 0.06, 0.28, sleeve);
  if (st.kind !== 'pistol' && st.kind !== 'magnum') {
    part(-0.005, -0.05, g.mz * 0.42, 0.055, 0.05, 0.07, skin);
    part(-0.03, -0.09, g.mz * 0.42 + 0.2, 0.06, 0.055, 0.3, sleeve);
  } else part(-0.03, -0.075, gz + 0.01, 0.05, 0.05, 0.06, skin);
  eng.endViewmodel();
}

// ---- المؤثرات ----
export class Effects {
  constructor() { this.tracers = []; this.parts = []; this.booms = []; this.smokes = []; this.flames = []; this.shells = []; }
  shell(x, y, z, yaw, big) { if (this.shells.length >= 30) this.shells.shift(); const rx = Math.cos(yaw), rz = -Math.sin(yaw), v = 1.8 + Math.random() * 1.2; this.shells.push({ x, y, z, vx: rx * v + (Math.random() - 0.5), vy: 1.6 + Math.random(), vz: rz * v + (Math.random() - 0.5), age: 0, life: 2.5, big, rot: Math.random() * 6 }); }
  smoke(x, y, z, col) { if (this.smokes.length < 36) this.smokes.push({ x, y, z, vx: (Math.random() - 0.5) * 0.5, vz: (Math.random() - 0.5) * 0.5, age: 0, life: 1.8 + Math.random() * 1.4, col }); }
  flame(x, y, z) { if (this.flames.length < 24) this.flames.push({ x, y, z, age: 0, life: 0.35 + Math.random() * 0.25 }); }
  tracer(o, e, muted, hit) { if (this.tracers.length < 24) this.tracers.push({ o, e, age: 0, muted }); }
  spark(x, y, z, col, n = 5, spd = 3) {
    for (let i = 0; i < n && this.parts.length < 70; i++) {
      this.parts.push({ x, y, z, vx: (Math.random() - 0.5) * spd, vy: Math.random() * spd * 0.8, vz: (Math.random() - 0.5) * spd, life: 0.25 + Math.random() * 0.25, age: 0, col, s: 0.03 + Math.random() * 0.03 });
    }
  }
  boom(x, y, z) { this.booms.push({ x, y, z, age: 0 }); this.spark(x, y, z, [1, 0.6, 0.15], 18, 12); this.spark(x, y, z, [0.3, 0.3, 0.3], 8, 6); }
  update(dt) {
    for (const t of this.tracers) t.age += dt; this.tracers = this.tracers.filter(t => t.age < 0.14);
    for (const p of this.parts) { p.age += dt; p.vy -= 12 * dt; p.x += p.vx * dt; p.y += p.vy * dt; p.z += p.vz * dt; }
    this.parts = this.parts.filter(p => p.age < p.life);
    for (const b of this.booms) b.age += dt; this.booms = this.booms.filter(b => b.age < 0.7);
    for (const q of this.smokes) { q.age += dt; q.y += dt * 1.6; q.x += q.vx * dt; q.z += q.vz * dt; } this.smokes = this.smokes.filter(q => q.age < q.life);
    for (const q of this.shells) { q.age += dt; q.rot += dt * 14; if (q.y > 0.03) { q.vy -= 14 * dt; q.x += q.vx * dt; q.y += q.vy * dt; q.z += q.vz * dt; if (q.y <= 0.03) { q.y = 0.03; q.vy = q.vy < -2 ? -q.vy * 0.35 : 0; q.vx *= 0.4; q.vz *= 0.4; if (q.vy === 0) q.vx = q.vz = 0; } } }
    this.shells = this.shells.filter(q => q.age < q.life);
    for (const q of this.flames) { q.age += dt; q.y += dt * 1.2; } this.flames = this.flames.filter(q => q.age < q.life);
  }
  draw(eng) {
    for (const t of this.tracers) {
      const dx = t.e[0] - t.o[0], dy = t.e[1] - t.o[1], dz = t.e[2] - t.o[2], L = Math.hypot(dx, dy, dz) || 1;
      const a = Math.min(1, t.age / 0.05), b = Math.max(0, Math.min(1, (t.age - 0.02) / 0.05));
      const s = Math.max(a, b) * L, tl = b * L; const len = s - tl; if (len < 0.05) continue;
      const mx = t.o[0] + dx / L * (tl + len / 2), my = t.o[1] + dy / L * (tl + len / 2), mz = t.o[2] + dz / L * (tl + len / 2);
      const m = basis(tmp(), mx, my, mz, dx / L, dy / L, dz / L, 0.02, 0.02, len);
      eng.cubeAt(m, 1, 0.85, 0.5, true, t.muted ? 0.25 : 0.8, true);
    }
    for (const p of this.parts) {
      const f = 1 - p.age / p.life;
      eng.cubeAt(trs(tmp(), p.x, p.y, p.z, 0, 0, p.s, p.s, p.s), p.col[0], p.col[1], p.col[2], true, f);
    }
    for (const q of this.shells) { const l = q.big ? 0.09 : 0.055, w = q.big ? 0.03 : 0.02; eng.cubeAt(trs(tmp(), q.x, q.y, q.z, q.rot, 0, w, w, l), 0.85, 0.65, 0.2, false); }
    for (const q of this.smokes) { const k = q.age / q.life, sz = 0.5 + k * 1.8; eng.sphereAt(trs(tmp(), q.x, q.y, q.z, 0, 0, sz, sz, sz), q.col[0], q.col[1], q.col[2], true, (1 - k) * 0.5); }
    for (const q of this.flames) { const k = q.age / q.life, sz = 0.5 * (1 - k * 0.7); eng.sphereAt(trs(tmp(), q.x, q.y, q.z, 0, 0, sz, sz * 1.5, sz), 1, 0.55 - 0.3 * k, 0.1, true, 0.85 * (1 - k), true); }
    for (const b of this.booms) {
      const k = b.age / 0.7, s = 2 + k * 11;
      eng.sphereAt(trs(tmp(), b.x, b.y, b.z, 0, 0, s, s, s), 1, 0.55 - 0.3 * k, 0.12, true, (1 - k) * 0.6, true);
    }
  }
}
export function drawGrenade(eng, x, y, z) {
  eng.sphereAt(trs(tmp(), x, y, z, 0, 0, 0.16, 0.2, 0.16), 0.2, 0.3, 0.18, false);
}
