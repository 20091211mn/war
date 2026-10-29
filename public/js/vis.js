// رسم العناصر الإضافية: حزم الإسعاف ومنطقة السيطرة وحلقة الطائرة
import { trs, mul, tmp } from './gl.js';

export function drawKit(eng, k, now) {
  const bob = Math.sin(now / 400 + k.id) * 0.06, y = 0.55 + bob, ry = now / 900 + k.id;
  const B = trs(tmp(), k.x, y, k.z, ry, 0, 1, 1, 1);
  const part = (x, yy, z, sx, sy, sz, c, unlit) => eng.cubeAt(mul(tmp(), B, trs(tmp(), x, yy, z, 0, 0, sx, sy, sz)), c[0], c[1], c[2], unlit);
  part(0, 0, 0, 0.55, 0.38, 0.4, [0.95, 0.95, 0.95]);
  for (const z of [-0.205, 0.205]) { part(0, 0, z, 0.3, 0.09, 0.01, [0.9, 0.12, 0.12], true); part(0, 0, z, 0.09, 0.3, 0.01, [0.9, 0.12, 0.12], true); }
  part(0, 0.22, 0, 0.18, 0.05, 0.1, [0.5, 0.5, 0.5]);
  eng.sphereAt(trs(tmp(), k.x, 0.7 + bob, k.z, 0, 0, 1.1, 1.1, 1.1), 0.3, 1, 0.5, true, 0.06 + 0.03 * Math.sin(now / 300), true); // وهج
}

const ZCOL = [[1, 0.25, 0.25], [0.3, 0.55, 1], [0.8, 0.3, 1], [1, 1, 1]];
export function drawZone(eng, z, now) {
  const c = ZCOL[z.state] || ZCOL[3], pulse = 0.5 + 0.5 * Math.sin(now / 350), r = z.r * 2;
  eng.tubeAt(trs(tmp(), z.x, 6, z.z, 0, 0, r, 12, r), c[0], c[1], c[2], true, 0.1 + 0.05 * pulse, true);
  eng.tubeAt(trs(tmp(), z.x, 0.12, z.z, 0, 0, r, 0.25, r), c[0], c[1], c[2], true, 0.45 + 0.2 * pulse, true);
  eng.tubeAt(trs(tmp(), z.x, 1.4, z.z, 0, 0, r * 0.985, 0.08, r * 0.985), c[0], c[1], c[2], true, 0.25, true);
}

const DT = [[1, 0.25, 0.25], [0.3, 0.55, 1]];
export function drawDrone(eng, d, tm, now) {
  const hv = Math.sin(now / 260 + d.id) * 0.04, B = trs(tmp(), d.x, d.y + hv, d.z, d.yaw, 0.0, 1, 1, 1);
  const part = (m, c, unlit, a) => eng.cubeAt(m, c[0], c[1], c[2], unlit, a);
  part(mul(tmp(), B, trs(tmp(), 0, 0, 0, 0, 0, 0.3, 0.1, 0.34)), [0.12, 0.12, 0.14]);
  part(mul(tmp(), B, trs(tmp(), 0, 0.06, 0, 0, 0, 0.14, 0.05, 0.16)), [0.25, 0.27, 0.3]);
  const c = DT[tm] || DT[0], blink = 0.6 + 0.4 * Math.sin(now / 120);
  part(mul(tmp(), B, trs(tmp(), 0, -0.07, -0.12, 0, 0, 0.06, 0.05, 0.06)), [c[0] * blink, c[1] * blink, c[2] * blink], true);
  for (const [sx, sz] of [[-1, -1], [1, -1], [-1, 1], [1, 1]]) {
    part(mul(tmp(), B, trs(tmp(), sx * 0.19, 0, sz * 0.19, sx * sz * 0.785, 0, 0.36, 0.03, 0.04)), [0.2, 0.2, 0.22]);
    part(mul(tmp(), B, trs(tmp(), sx * 0.3, 0.05, sz * 0.3, now / 25 * sx, 0, 0.3, 0.012, 0.3)), [0.7, 0.75, 0.8], false, 0.35);
  }
}
