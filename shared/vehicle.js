// المركبة: شاص تيوتا (لاند كروزر 70 بيك أب) — أبعاد، مقاعد، صناديق إصابة، فيزياء قيادة
import { rayBox } from './physics.js';

export const VEH = {
  hp: 450, L: 3.0, hw: 1.0, maxFwd: 26, maxRev: 7, accel: 7.5, brake: 16, maxSteer: 0.52,
  tireHp: 30, crashMin: 4,
};
// مقاعد (محلي: x يمين، y ارتفاع قدم اللاعب، z للخلف) — الأمام = -z
export const SEATS = [
  { x: -0.45, y: 0.35, z: -0.25, kind: 'driver' },
  { x: 0.45, y: 0.35, z: -0.25, kind: 'front' },
  { x: -0.55, y: 0.85, z: 1.2, kind: 'bed' },
  { x: 0.55, y: 0.85, z: 1.2, kind: 'bed' },
  { x: 0, y: 0.85, z: 1.9, kind: 'bed' },
];
const WHEELS = [[-0.95, -1.55], [0.95, -1.55], [-0.95, 1.5], [0.95, 1.5]]; // FL, FR, RL, RR

const BODY = [
  [-1, 0.3, -2.5, 1, 1.1, -0.9, 'body'],
  [-1, 0.3, -0.9, 1, 1.05, 0.6, 'body'],
  [-0.95, 1.8, -0.7, 0.95, 1.9, 0.4, 'body'],
  [-1, 0.3, 0.6, 1, 0.85, 2.5, 'tank'],
  [-1, 0.85, 0.6, -0.9, 1.25, 2.5, 'body'],
  [0.9, 0.85, 0.6, 1, 1.25, 2.5, 'body'],
  [-1, 0.85, 2.4, 1, 1.25, 2.5, 'body'],
  [-1, 0.85, 0.6, 1, 1.25, 0.7, 'body'],
];
const WBOX = WHEELS.map(([x, z], i) => [x - 0.2, 0, z - 0.4, x + 0.2, 0.76, z + 0.4, 'w' + i]);

export function l2w(v, lx, ly, lz) {
  const c = Math.cos(v.h), s = Math.sin(v.h);
  return [v.x + c * lx + s * lz, ly, v.z - s * lx + c * lz];
}
export function w2l(v, wx, wz) {
  const c = Math.cos(v.h), s = Math.sin(v.h), dx = wx - v.x, dz = wz - v.z;
  return [c * dx - s * dz, s * dx + c * dz];
}
export function seatWorld(v, i) { const s = SEATS[i]; return l2w(v, s.x, s.y, s.z); }
export function vehForward(v) { return [-Math.sin(v.h), -Math.cos(v.h)]; }

// تقاطع شعاع مع المركبة. يرجع {t, part} أو null
export function rayVehicle(v, ox, oy, oz, dx, dy, dz, maxT) {
  const c = Math.cos(v.h), s = Math.sin(v.h);
  const px = ox - v.x, pz = oz - v.z;
  const lox = c * px - s * pz, loz = s * px + c * pz, ldx = c * dx - s * dz, ldz = s * dx + c * dz;
  let best = maxT, part = null;
  for (const b of BODY) { const t = rayBox(lox, oy, loz, ldx, dy, ldz, b[0], b[1], b[2], b[3], b[4], b[5]); if (t < best) { best = t; part = b[6]; } }
  for (const b of WBOX) { const t = rayBox(lox, oy, loz, ldx, dy, ldz, b[0], b[1], b[2], b[3], b[4], b[5]); if (t < best + 0.02 && t < maxT) { best = Math.min(best, t); part = b[6]; } }
  return part ? { t: best, part } : null;
}

function circleHit(map, cx, cz, r) {
  const lx = map.W / 2 - r, lz = map.D / 2 - r;
  if (cx > lx || cx < -lx || cz > lz || cz < -lz) return true;
  for (const b of map.boxes) {
    if (b.y1 <= 0.55 || b.y0 >= 1.6) continue;
    const nx = cx < b.x0 ? b.x0 : cx > b.x1 ? b.x1 : cx, nz = cz < b.z0 ? b.z0 : cz > b.z1 ? b.z1 : cz;
    const dx = cx - nx, dz = cz - nz;
    if (dx * dx + dz * dz < r * r) return true;
  }
  return false;
}
const SAMPLES = [-2.2, -1.1, 0, 1.1, 2.2];
export function vehCollides(map, x, z, h, dyn) {
  const bx = Math.sin(h), bz = Math.cos(h); // اتجاه الخلف
  for (const k of SAMPLES) {
    const cx = x + bx * k, cz = z + bz * k;
    if (circleHit(map, cx, cz, VEH.hw)) return true;
    if (dyn) for (const o of dyn) { // مركبات أخرى: دوائر عند -1.6 و 0 و 1.6
      const ob = Math.sin(o[2]), oc = Math.cos(o[2]);
      for (const j of [-1.6, 0, 1.6]) { const dx = cx - (o[0] + ob * j), dz = cz - (o[1] + oc * j); if (dx * dx + dz * dz < 3.3) return true; }
    }
  }
  return false;
}
// دفع لاعب (أو بوت) خارج جسم المركبة. يرجع true إن دُفع
export function pushOutVeh(v, p) {
  if (p.y > 1.85 || p.y < -1) return false;
  const [lx, lz] = w2l(v, p.x, p.z), hx = VEH.hw + 0.38, hz = 2.5 + 0.38;
  if (Math.abs(lx) >= hx || Math.abs(lz) >= hz) return false;
  const px = hx - Math.abs(lx), pz = hz - Math.abs(lz);
  let nx = lx, nz = lz;
  if (px < pz) nx = Math.sign(lx || 1) * hx; else nz = Math.sign(lz || 1) * hz;
  const c = Math.cos(v.h), s = Math.sin(v.h);
  p.x = v.x + c * nx + s * nz; p.z = v.z - s * nx + c * nz;
  return true;
}
export function tiresBlown(mask) { return (mask & 1) + ((mask >> 1) & 1) + ((mask >> 2) & 1) + ((mask >> 3) & 1); }

// v: {x,z,h,vx,vz,steer,tires}. inp: {thr,steer,hb}. يرجع سرعة الاصطدام (0 إن لم يحدث)
export function vehStep(map, v, inp, dt) {
  const fx = -Math.sin(v.h), fz = -Math.cos(v.h), rx = Math.cos(v.h), rz = -Math.sin(v.h);
  let vf = v.vx * fx + v.vz * fz, vl = v.vx * rx + v.vz * rz;
  const nb = tiresBlown(v.tires | 0);
  const maxFwd = Math.max(5, VEH.maxFwd * (1 - 0.17 * nb)), accel = VEH.accel * (1 - 0.1 * nb);
  const thr = inp.thr || 0;
  if (thr > 0) {
    if (vf < -0.5) vf += VEH.brake * dt;
    else vf += accel * thr * dt * Math.max(0, 1 - vf / maxFwd);
  } else if (thr < 0) {
    if (vf > 0.5) vf -= VEH.brake * dt;
    else vf -= accel * 0.6 * -thr * dt * Math.max(0, 1 - -vf / VEH.maxRev);
  } else {
    const roll = (1.5 + 0.012 * vf * vf) * dt;
    vf -= Math.sign(vf) * Math.min(Math.abs(vf), roll);
  }
  if (vf > maxFwd) vf = Math.max(maxFwd, vf - 12 * dt);
  if (inp.hb) vf -= Math.sign(vf) * Math.min(Math.abs(vf), 9 * dt);
  const grip = (inp.hb ? 1.4 : 7) * (1 - 0.12 * nb);
  vl *= Math.exp(-grip * dt);
  const maxS = VEH.maxSteer * (1 - Math.min(0.72, Math.abs(vf) / 38));
  v.steer += ((inp.steer || 0) * maxS - v.steer) * Math.min(1, dt * 6);
  let yawRate = vf / VEH.L * Math.tan(v.steer) * (inp.hb ? 1.35 : 1);
  const pull = ((v.tires >> 1 & 1) + (v.tires >> 3 & 1)) - ((v.tires & 1) + (v.tires >> 2 & 1));
  yawRate += pull * 0.09 * Math.min(1, Math.abs(vf) / 10) * Math.sign(vf || 1);
  let nh = v.h - yawRate * dt;
  let crash = 0;
  if (!vehCollides(map, v.x, v.z, nh, v.dyn)) v.h = nh; else { if (Math.abs(vf) > 1) crash = Math.max(crash, Math.abs(vf) * 0.4); }
  const nfx = -Math.sin(v.h), nfz = -Math.cos(v.h), nrx = Math.cos(v.h), nrz = -Math.sin(v.h);
  let nvx = nfx * vf + nrx * vl, nvz = nfz * vf + nrz * vl;
  let nx = v.x + nvx * dt, nz = v.z + nvz * dt, hit = false;
  if (!vehCollides(map, nx, v.z, v.h, v.dyn)) v.x = nx; else { hit = true; nvx *= -0.2; }
  if (!vehCollides(map, v.x, nz, v.h, v.dyn)) v.z = nz; else { hit = true; nvz *= -0.2; }
  if (hit) { crash = Math.max(crash, Math.abs(vf)); nvx *= 0.6; nvz *= 0.6; }
  v.vx = nvx; v.vz = nvz;
  v.speed = nvx * nfx + nvz * nfz;
  // لو علقت داخل جسم (نادر): ادفعها للخارج
  if (vehCollides(map, v.x, v.z, v.h, v.dyn)) { for (let k = 1; k <= 12; k++) for (const [ax, az] of [[1, 0], [-1, 0], [0, 1], [0, -1]]) { if (!vehCollides(map, v.x + ax * k * 0.25, v.z + az * k * 0.25, v.h, v.dyn)) { v.x += ax * k * 0.25; v.z += az * k * 0.25; k = 99; break; } } }
  return crash;
}
