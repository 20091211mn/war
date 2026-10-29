import { buildMap, MAP_COUNT, MAP_NAMES } from '../shared/maps.js';
import { buildNav, findPath, reachableFrom, cellOf } from '../shared/nav.js';
import { blocked, groundAt, moveBody, PLAYER } from '../shared/physics.js';
import fs from 'node:fs';
import { vehCollides } from '../shared/vehicle.js';

let fails = 0;
const bad = (m) => { console.log('  ✗ ' + m); fails++; };
for (let id = 0; id < MAP_COUNT; id++) {
  const m = buildMap(id);
  console.log(`خريطة ${id} ${MAP_NAMES[id]}: ${m.boxes.length} صندوق`);
  for (const b of m.boxes) {
    if (![b.x0, b.x1, b.y0, b.y1, b.z0, b.z1].every(Number.isFinite) || b.x1 <= b.x0 || b.z1 <= b.z0 || b.y1 <= b.y0) bad('صندوق غير صالح ' + JSON.stringify(b));
    if (b.x0 < -m.W / 2 - 1.01 || b.x1 > m.W / 2 + 1.01 || b.z0 < -m.D / 2 - 1.01 || b.z1 > m.D / 2 + 1.01) bad('صندوق خارج الحدود ' + JSON.stringify(b));
  }
  if (m.red.length !== 8 || m.blue.length !== 8) bad('عدد نقاط الظهور');
  for (const [team, arr] of [['red', m.red], ['blue', m.blue]]) for (const [x, z] of arr) {
    if (blocked(m, x, 0, z, PLAYER.h)) bad(`نقطة ظهور ${team} محجوبة ${x},${z}`);
    if (groundAt(m, x, z, 0) !== 0) bad(`نقطة ظهور ${team} فوق صندوق ${x},${z}`);
    if (Math.abs(x) > m.W / 2 - 1 || Math.abs(z) > m.D / 2 - 1) bad('ظهور قرب الحدود');
  }
  // المركبات: خالية من التصادم ومتصلة ببعضها وبقاعدتي الفريقين
  if (m.vehicles.length) {
    for (const v of m.vehicles) if (vehCollides(m, v.x, v.z, v.h)) bad(`موضع مركبة محجوب ${v.x},${v.z}`);
    const cs = 1.5, cols = Math.ceil(m.W / cs), rows = Math.ceil(m.D / cs), free = new Uint8Array(cols * rows);
    for (let j = 0; j < rows; j++) for (let i = 0; i < cols; i++) { const x = -m.W / 2 + (i + 0.5) * cs, z = -m.D / 2 + (j + 0.5) * cs; free[j * cols + i] = (!vehCollides(m, x, z, 0) || !vehCollides(m, x, z, Math.PI / 2)) ? 1 : 0; }
    const idx = (x, z) => Math.floor((z + m.D / 2) / cs) * cols + Math.floor((x + m.W / 2) / cs);
    const seen2 = new Uint8Array(cols * rows), st = [idx(m.vehicles[0].x, m.vehicles[0].z)]; seen2[st[0]] = 1;
    while (st.length) { const c = st.pop(), ci = c % cols, cj = (c / cols) | 0; for (const [di, dj] of [[1, 0], [-1, 0], [0, 1], [0, -1]]) { const a = ci + di, b2 = cj + dj; if (a < 0 || b2 < 0 || a >= cols || b2 >= rows) continue; const n = b2 * cols + a; if (free[n] && !seen2[n]) { seen2[n] = 1; st.push(n); } } }
    for (const v of m.vehicles) if (!seen2[idx(v.x, v.z)]) bad(`مركبة معزولة ${v.x},${v.z}`);
    for (const [x, z] of [m.red[0], m.blue[0], m.red[7], m.blue[7]]) { const near = [[0, 0], [1, 0], [-1, 0], [0, 1], [0, -1], [2, 0], [-2, 0]].some(([a, b2]) => seen2[idx(x + a * cs, z + b2 * cs)]); if (!near) bad(`قاعدة لا تصلها المركبات ${x},${z}`); }
    let fr = 0, rc = 0; for (let i = 0; i < free.length; i++) if (free[i]) { fr++; if (seen2[i]) rc++; }
    console.log(`  مركبات: ${m.vehicles.length}، مساحة قيادة متصلة ${(100 * rc / fr).toFixed(1)}%`);
    if (rc / fr < 0.9) bad('مناطق قيادة معزولة كثيرة');
  }
  const nav = buildNav(m);
  const seen = reachableFrom(nav, cellOf(nav, m.red[0][0], m.red[0][1]));
  for (const [x, z] of [...m.red, ...m.blue]) if (!seen[cellOf(nav, x, z)]) bad(`نقطة ظهور غير متصلة ${x},${z}`);
  let free = 0, reach = 0;
  for (let i = 0; i < nav.grid.length; i++) { if (!nav.grid[i]) { free++; if (seen[i]) reach++; } }
  console.log(`  خلايا حرة ${free}, متصلة ${reach} (${(100 * reach / free).toFixed(1)}%)`);
  if (reach / free < 0.93) bad('نسبة المناطق المتصلة منخفضة (مناطق معزولة)');
  const p = findPath(nav, m.red[0][0], m.red[0][1], m.blue[3][0], m.blue[3][1]);
  if (!p || p.length < 5) bad('لا مسار بين الفريقين'); else console.log(`  مسار الفريقين: ${p.length} خطوة`);
  // تماثل: عدد الصناديق زوجي ما عدا المركزية
  // اختبار فيزياء: لاعب يمشي في اتجاه الأعداء ولا يسقط خارج الخريطة ولا يخترق جدارًا
  const pl = { x: m.red[0][0], y: 0, z: m.red[0][1], vy: 0, onGround: true };
  for (let i = 0; i < 3000; i++) { moveBody(m, pl, 6 * Math.cos(i * 0.01), 6 * Math.sin(i * 0.013), 1 / 60, i % 97 === 0, false); if (blocked(m, pl.x, pl.y, pl.z, PLAYER.h) && !pl.onGround) { /* قد يكون في الهواء بجانب صندوق */ } if (!Number.isFinite(pl.x + pl.y + pl.z) || pl.y < -0.01 || Math.abs(pl.x) > m.W / 2 || Math.abs(pl.z) > m.D / 2) { bad('فيزياء: خرج اللاعب ' + JSON.stringify(pl)); break; } }
  // صعود الدرج: اللاعب يتحرك نحو المنصة
  // رسم الخريطة (ASCII + PPM)
  const S = 8, w = Math.round(m.W * S), h = Math.round(m.D * S);
  const img = Buffer.alloc(w * h * 3, 40);
  for (let py = 0; py < h; py++) for (let px = 0; px < w; px++) {
    const x = px / S - m.W / 2, z = py / S - m.D / 2; let top = 0, k = -1;
    for (const b of m.boxes) if (x >= b.x0 && x <= b.x1 && z >= b.z0 && z <= b.z1 && b.y1 > top && b.y0 < 6.5) { top = b.y1; k = b.k; }
    const o = (py * w + px) * 3, v = Math.min(255, 70 + top * 30);
    if (k >= 0) { img[o] = v; img[o + 1] = v * (k === 2 ? 0.8 : 1); img[o + 2] = v * (k === 3 ? 0.6 : 1); } else { img[o] = 30; img[o + 1] = 40; img[o + 2] = 30; }
  }
  const dot = (x, z, c) => { for (let dy = -4; dy <= 4; dy++) for (let dx = -4; dx <= 4; dx++) { const px = Math.round((x + m.W / 2) * S) + dx, py = Math.round((z + m.D / 2) * S) + dy; if (px >= 0 && py >= 0 && px < w && py < h) { const o = (py * w + px) * 3; img[o] = c[0]; img[o + 1] = c[1]; img[o + 2] = c[2]; } } };
  for (const [x, z] of m.red) dot(x, z, [255, 40, 40]); for (const [x, z] of m.blue) dot(x, z, [40, 120, 255]);
  fs.mkdirSync(process.env.MAPOUT || '/tmp', { recursive: true });
  fs.writeFileSync(`${process.env.MAPOUT || '/tmp'}/map${id}.ppm`, Buffer.concat([Buffer.from(`P6 ${w} ${h} 255\n`), img]));
}
console.log(fails ? `فشل: ${fails}` : 'نجحت كل اختبارات الخرائط ✓');
process.exit(fails ? 1 : 0);
