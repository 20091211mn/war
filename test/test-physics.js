// يقارن الاجتياز السريع (شبكة التسريع) بالبحث الكامل على كل الخرائط
import { buildMap, MAP_COUNT } from '../shared/maps.js';
import { raycastMap, rayBox, blocked, groundAt, PLAYER } from '../shared/physics.js';
let fails = 0;
let seed = 12345; const rnd = () => { seed = (seed * 1664525 + 1013904223) >>> 0; return seed / 4294967296; };
const brute = (m, ox, oy, oz, dx, dy, dz, maxT) => { let best = maxT; for (const b of m.boxes) { const t = rayBox(ox, oy, oz, dx, dy, dz, b.x0, b.y0, b.z0, b.x1, b.y1, b.z1); if (t < best) best = t; } return best >= maxT ? Infinity : best; };
const bruteBlocked = (m, x, y, z, hh) => { const r = PLAYER.r, st = PLAYER.step; for (const b of m.boxes) if (x + r > b.x0 && x - r < b.x1 && z + r > b.z0 && z - r < b.z1 && y + hh > b.y0 && y + st < b.y1) return true; return false; };
const bruteGround = (m, x, z, y) => { const r = PLAYER.r, lim = y + PLAYER.step + 1e-4; let g = 0; for (const b of m.boxes) if (b.y1 <= lim && b.y1 > g && x + r > b.x0 && x - r < b.x1 && z + r > b.z0 && z - r < b.z1) g = b.y1; return g; };
for (let id = 0; id < MAP_COUNT; id++) {
  const m = buildMap(id); let bad = 0, n = 0; const t0 = Date.now();
  for (let i = 0; i < 4000; i++) {
    const ox = (rnd() - 0.5) * m.W, oz = (rnd() - 0.5) * m.D, oy = rnd() * 3 + 0.2;
    const a = rnd() * 6.283, p = (rnd() - 0.5) * 1.2, dx = Math.cos(a) * Math.cos(p), dz = Math.sin(a) * Math.cos(p), dy = Math.sin(p), mt = 5 + rnd() * 250;
    const A = raycastMap(m, ox, oy, oz, dx, dy, dz, mt), B = brute(m, ox, oy, oz, dx, dy, dz, mt); n++;
    if (!(A === B || Math.abs(A - B) < 1e-9)) bad++;
    if (blocked(m, ox, 0, oz, PLAYER.h) !== bruteBlocked(m, ox, 0, oz, PLAYER.h)) bad++;
    if (groundAt(m, ox, oz, 0) !== bruteGround(m, ox, oz, 0)) bad++;
  }
  console.log(`خريطة ${id}: ${n} اختبار، أخطاء ${bad}`); if (bad) fails++;
}
console.log(fails ? 'فشل' : 'نجح اختبار الفيزياء ✓'); process.exit(fails ? 1 : 0);
