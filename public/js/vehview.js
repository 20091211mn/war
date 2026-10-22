// رسم الشاص (تويوتا لاند كروزر بيك أب) والذخيرة المتساقطة وتأثيرات الدخان والنار
import { trs, mul, tmp } from './gl.js';
import { SEATS, l2w } from '/shared/vehicle.js';

const BODY = [0.78, 0.72, 0.58], DARK = [0.13, 0.13, 0.14], GLASS = [0.25, 0.36, 0.45], RUBBER = [0.06, 0.06, 0.07], HUB = [0.55, 0.56, 0.58];
const WHEELS = [[-0.95, -1.55], [0.95, -1.55], [-0.95, 1.5], [0.95, 1.5]];

export function drawVehicle(eng, v, now, far, night) {
  const dead = !!(v.flags & 2), burn = !!(v.flags & 1);
  const hpk = Math.max(0, Math.min(1, v.hp / 450));
  const tint = dead ? 0.16 : 0.55 + 0.45 * hpk;           // يسوَدّ مع الضرر
  const col = (c) => [c[0] * tint, c[1] * tint, c[2] * tint];
  const V = trs(tmp(), v.x, 0, v.z, v.h, 0, 1, 1, 1);
  const part = (x, y, z, sx, sy, sz, c, unlit, alpha) => {
    const m = mul(tmp(), V, trs(tmp(), x, y, z, 0, 0, sx, sy, sz));
    eng.cubeAt(m, c[0], c[1], c[2], unlit, alpha);
  };
  const body = col(BODY), dark = col(DARK);
  if (far) { // تفصيل منخفض للبعيد
    part(0, 0.6, 0, 1.9, 0.5, 4.9, body); part(0, 1.3, -0.15, 1.9, 0.8, 1.5, body); part(0, 0.95, 1.55, 1.9, 0.3, 1.9, dark);
    for (const [x, z] of WHEELS) part(x, 0.35, z, 0.34, 0.7, 0.7, RUBBER);
    return;
  }
  // الهيكل السفلي
  part(0, 0.5, 0, 1.7, 0.2, 4.9, [0.1, 0.1, 0.11]);
  // غطاء المحرك والرفارف
  part(0, 0.86, -1.7, 1.96, 0.46, 1.6, body);
  part(0, 1.12, -1.55, 1.2, 0.05, 0.9, dark);
  part(-0.97, 0.98, -1.55, 0.14, 0.34, 1.1, body); part(0.97, 0.98, -1.55, 0.14, 0.34, 1.1, body);
  // الواجهة الأمامية والمصد
  part(0, 0.74, -2.52, 1.9, 0.38, 0.1, dark);
  part(0, 0.5, -2.58, 2.0, 0.16, 0.14, [0.45, 0.46, 0.48]);
  part(-0.68, 0.94, -2.53, 0.26, 0.2, 0.06, [1, 0.95, 0.75], !dead, 1);
  part(0.68, 0.94, -2.53, 0.26, 0.2, 0.06, [1, 0.95, 0.75], !dead, 1);
  // الكابينة
  part(0, 0.72, -0.15, 2.0, 0.66, 1.5, body);
  part(0, 1.86, -0.15, 1.96, 0.1, 1.2, body);                     // السقف
  for (const x of [-0.93, 0.93]) { part(x, 1.45, -0.74, 0.08, 0.8, 0.08, body); part(x, 1.45, 0.4, 0.08, 0.8, 0.1, body); }
  part(0, 1.46, -0.76, 1.8, 0.78, 0.04, GLASS, false, dead ? 0.15 : 0.42);   // زجاج أمامي
  part(0, 1.46, 0.45, 1.8, 0.6, 0.04, GLASS, false, dead ? 0.15 : 0.42);     // زجاج خلفي
  // مقاعد وعجلة قيادة
  for (const x of [-0.45, 0.45]) { part(x, 0.62, -0.25, 0.7, 0.14, 0.6, [0.2, 0.2, 0.19]); part(x, 0.95, 0.12, 0.7, 0.55, 0.12, [0.22, 0.22, 0.21]); }
  part(-0.45, 1.0, -0.66, 0.42, 0.05, 0.42, [0.05, 0.05, 0.05]);
  part(0, 0.98, -0.72, 1.7, 0.12, 0.2, dark);                     // طبلون
  // الصندوق (بيك أب)
  part(0, 0.88, 1.55, 1.9, 0.08, 1.9, dark);
  part(-0.96, 1.08, 1.55, 0.08, 0.42, 1.9, body); part(0.96, 1.08, 1.55, 0.08, 0.42, 1.9, body);
  part(0, 1.08, 2.46, 1.9, 0.42, 0.08, body); part(0, 1.08, 0.66, 1.9, 0.42, 0.08, body);
  part(0, 0.62, 1.6, 1.6, 0.28, 1.6, [0.1, 0.1, 0.1]);             // خزان الوقود
  part(-0.7, 0.62, 2.52, 0.3, 0.12, 0.05, [0.9, 0.1, 0.1], !dead);  // أضواء خلفية
  part(0.7, 0.62, 2.52, 0.3, 0.12, 0.05, [0.9, 0.1, 0.1], !dead);
  if (night && !dead && !far) { // كشافات أمامية
    for (const x of [-0.68, 0.68]) {
      const m = mul(tmp(), V, trs(tmp(), x, 0.9, -7.5, 0, 0.0, 0.9, 0.7, 10));
      eng.cubeAt(m, 1, 0.95, 0.7, true, 0.08, true);
    }
  }
  // العجلات
  WHEELS.forEach(([x, z], i) => {
    const blown = (v.tires >> i) & 1, r = blown ? 0.31 : 0.38, steer = i < 2 ? v.steer : 0;
    const W = mul(tmp(), V, trs(tmp(), x, r, z, steer, v.spin, 1, 1, 1));
    eng.cylAt(mul(tmp(), W, trs(tmp(), 0, 0, 0, 0, 0, 0.34, r * 2, r * 2)), RUBBER[0], RUBBER[1], RUBBER[2]);
    eng.cylAt(mul(tmp(), W, trs(tmp(), x > 0 ? 0.03 : -0.03, 0, 0, 0, 0, 0.34, r * 1.1, r * 1.1)), HUB[0], HUB[1], HUB[2]);
    if (blown) eng.cubeAt(mul(tmp(), V, trs(tmp(), x, 0.05, z, 0, 0, 0.4, 0.06, 0.8)), 0.05, 0.05, 0.05, false);
  });
}

// تأثيرات: دخان/نار بحسب حالة المركبة. fx = كائن Effects
export function vehicleFx(fx, v, dt) {
  const dead = !!(v.flags & 2), burn = !!(v.flags & 1), hp = v.hp;
  v._t = (v._t || 0) + dt;
  const rate = dead ? 14 : burn ? 20 : hp < 250 ? 6 : 0;
  if (!rate) return;
  v._acc = (v._acc || 0) + dt * rate;
  while (v._acc > 1) {
    v._acc -= 1;
    const [x, , z] = l2w(v, (Math.random() - 0.5) * 0.8, 0, -1.6 + (Math.random() - 0.5) * 0.8);
    const dark = dead || burn;
    fx.smoke(x, 1.2, z, dark ? [0.07, 0.07, 0.07] : [0.55, 0.55, 0.55]);
    if ((burn || dead) && Math.random() < 0.55) fx.flame(x + (Math.random() - 0.5) * 0.4, 1.0 + Math.random() * 0.3, z + (Math.random() - 0.5) * 0.4);
  }
}

export function drawDrop(eng, d, now, mine) {
  const bob = Math.sin(now / 300 + d.id) * 0.05;
  const y = d.y + 0.18 + bob, rot = now / 700 + d.id;
  const c = mine ? [0.95, 0.8, 0.2] : [0.45, 0.47, 0.4];
  eng.cubeAt(trs(tmp(), d.x, y, d.z, rot, 0, 0.34, 0.2, 0.22), c[0], c[1], c[2], !!mine);
  eng.cubeAt(trs(tmp(), d.x, y + 0.11, d.z, rot, 0, 0.2, 0.03, 0.24), 0.15, 0.15, 0.15, false);
  if (mine) eng.cubeAt(trs(tmp(), d.x, y + 0.9 + bob, d.z, rot, 0, 0.06, 0.5, 0.06), 1, 0.9, 0.3, true, 0.35, true);
}
