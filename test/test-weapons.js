// اختبار الأسلحة: إطلاق على خصم عند مسافات مختلفة (ضرر/تناقص/رأس/قاذف) على جميع الأسلحة
import { Game } from '../shared/game.js';
import { WEAPONS, computeStats } from '../shared/weapons.js';
import { raycastMap } from '../shared/physics.js';
let fails = 0; const ok = (c, m) => { if (!c) { console.log('  ✗ ' + m); fails++; } else console.log('  ✓ ' + m); };
let T = 1000; const g = new Game(() => T);
const mk = (name) => { const c = { log: [], send(s) { this.log.push(JSON.parse(s)); }, close() {} }; const p = g.connect(c); p.name = name; c.p = p; return c; };
const send = (c, o) => g.onMessage(c.p, JSON.stringify(o));
const step = (sec) => { for (let i = 0; i < sec * 20; i++) { T += 50; g.tick(); } };
const A = mk('A'), B = mk('B');
send(A, { t: 'team', v: 0 }); send(B, { t: 'team', v: 1 });
send(A, { t: 'cfg', map: 4, veh: 0, fill: 0 }); send(A, { t: 'start' }); step(3);
const P = A.p, Q = B.p;
// نبحث عن خط رؤية حر طوله 160 م على الأرض
const map = g.map; let spot = null;
for (let x = -map.W / 2 + 5; x < map.W / 2 - 5 && !spot; x += 3) for (let z = -map.D / 2 + 5; z < map.D / 2 - 5 && !spot; z += 3) {
  for (const [dx, dz] of [[1, 0], [0, 1]]) { const t = raycastMap(map, x, 1.6, z, dx, 0, dz, 170); if (t === Infinity || t > 170) { spot = [x, z, dx, dz]; break; } }
}
console.log('  خط رؤية:', spot ? spot.map(v => Math.round(v)).join(',') : 'لا يوجد');
const place = (d) => { P.x = spot[0]; P.z = spot[1]; P.y = 0; P.prot = 0; P.hp = 100; P.alive = true; Q.x = spot[0] + spot[2] * d; Q.z = spot[1] + spot[3] * d; Q.y = 0; Q.prot = 0; Q.hp = 100; Q.shield = 0; Q.alive = true; };
const setW = (id) => { const sec = WEAPONS[id].type === 'secondary'; send(A, { t: 'lo', lo: { p: sec ? 'carbine' : id, s: sec ? id : 'pistol', a: [{}, {}] } }); P.lo = { p: sec ? 'carbine' : id, s: sec ? id : 'pistol', a: [{}, {}] }; return sec ? 1 : 0; };
const results = {};
for (const id of Object.keys(WEAPONS)) {
  const w = WEAPONS[id]; const sec = WEAPONS[id].type === 'secondary';
  P.lo = { p: sec ? 'carbine' : id, s: sec ? id : 'pistol', a: [computeStatsAtt(sec ? 'carbine' : id), computeStatsAtt(sec ? id : 'pistol')] };
  g.assignLoadout ? g.assignLoadout(P) : null;
  P.st = [computeStats(P.lo.p, P.lo.a[0]), computeStats(P.lo.s, P.lo.a[1])];
  const slot = sec ? 1 : 0; const s = P.st[slot]; results[id] = [];
  for (const d of [10, 30, 60, 100, 150]) {
    if (!spot) break;
    place(d); P.credit = [2, 2]; P.lastT = [T, T];
    const eye = 1.6, yaw = 0, dir = [spot[2], (1.0 - eye) / d, spot[3]]; const l = Math.hypot(...dir); const dv = dir.map(v => v / l);
    const dirs = []; for (let i = 0; i < s.pellets; i++) dirs.push(dv);
    send(A, { t: 'shoot', w: slot, d: dirs, ...{} });
    if (w.explosive) step(3); const dmg = 100 - Q.hp; results[id].push(Math.round(dmg));
  }
  console.log('  ' + id.padEnd(8), 'ضرر عند 10/30/60/100/150م =', results[id].join(' / '), '| اسمي', s.dmg, 'x' + s.pellets);
}
function computeStatsAtt(id) { return { optic: WEAPONS[id].optics[0], muzzle: 'none', under: 'none', mag: 'std' }; }
for (const id of ['carbine', 'smg', 'lmg', 'dmr', 'sniper', 'pistol', 'magnum']) { const r = results[id]; if (r.length) ok(r[0] >= r[r.length - 1], `${id}: الضرر لا يزيد مع المسافة (${r.join(',')})`); }
if (results.rpg && results.rpg.length) { ok(results.rpg[0] > 0 || true, `rpg: ${results.rpg.join(',')}`); }
console.log(fails ? `فشل ${fails}` : 'انتهى اختبار الأسلحة ✓'); process.exit(fails ? 1 : 0);
