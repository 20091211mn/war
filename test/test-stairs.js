import { buildMap, MAP_COUNT } from '../shared/maps.js';
import { moveBody } from '../shared/physics.js';
// اختبار عام: كل درج مسجّل في climbs يجب أن يصعده اللاعب إلى ارتفاعه
let fails = 0, n = 0;
for (let id = 0; id < MAP_COUNT; id++) {
  const m = buildMap(id);
  for (const c of m.climbs) {
    n++; const p = { x: c.x, y: 0, z: c.z, vy: 0, onGround: true }; let mx = 0;
    for (let i = 0; i < 300; i++) { moveBody(m, p, c.vx * 4.6, c.vz * 4.6, 1 / 60, false, false); mx = Math.max(mx, p.y); }
    if (mx < c.h - 0.45) { console.log(`✗ خريطة ${id}: الدرج عند ${c.x.toFixed(1)},${c.z.toFixed(1)} بلغ ${mx.toFixed(2)} من ${c.h}`); fails++; }
  }
  console.log(`خريطة ${id}: ${m.climbs.length} درج`);
}
console.log(fails ? 'فشل' : `✓ كل الدرجات (${n}) قابلة للصعود`); process.exit(fails ? 1 : 0);
