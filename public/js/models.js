// نماذج الأسلحة واللاعبين والمؤثرات (كلها مجسّمات مكعبة بدون ملفات)
import { trs, mul, tmp, basis } from './gl.js';

const C = { d: [0.14, 0.14, 0.16], g: [0.34, 0.35, 0.38], b: [0.05, 0.05, 0.06], w: [0.42, 0.27, 0.13], t: [0.5, 0.45, 0.32], o: [0.9, 0.5, 0.1], s: [0.86, 0.68, 0.55] };
// أجزاء السلاح: [x,y,z,sx,sy,sz,لون] — الاتجاه الأمامي = -Z ، mz = موضع الفوهة، sy = ارتفاع المصوّب الحديدي
export const GUNS = {
  rifle: { mz: -0.44, sy: 0.058, parts: [[0,0,-0.02,0.05,0.06,0.32,'d'],[0,-0.04,0.03,0.046,0.04,0.2,'d'],[0,0,-0.2,0.056,0.06,0.2,'g'],[0,0.034,-0.2,0.02,0.01,0.2,'b'],[0,0.004,-0.36,0.016,0.016,0.14,'b'],[0,0.004,-0.435,0.028,0.028,0.045,'b'],[0,0.04,-0.33,0.008,0.036,0.008,'b'],[0,0.045,0.07,0.03,0.03,0.012,'b'],[0,0,0.14,0.034,0.04,0.12,'g'],[0,-0.01,0.26,0.042,0.08,0.14,'d'],[0,-0.01,0.335,0.046,0.09,0.02,'b'],[0,-0.085,0.085,0.03,0.09,0.035,'d'],[0,-0.066,-0.01,0.012,0.012,0.08,'b'],[0.027,0.008,0.0,0.004,0.02,0.05,'b']], mag: [0,-0.1,-0.02,0.034,0.12,0.055] },
  smg: { mz: -0.3, sy: 0.052, parts: [[0,0,-0.02,0.05,0.06,0.24,'d'],[0,-0.03,-0.1,0.046,0.03,0.1,'g'],[0,0.004,-0.22,0.015,0.015,0.1,'b'],[0,0.004,-0.285,0.026,0.026,0.04,'b'],[0,0.04,-0.2,0.008,0.036,0.008,'b'],[0,0.04,0.06,0.028,0.026,0.012,'b'],[0,-0.005,0.17,0.02,0.02,0.12,'b'],[0,-0.04,0.19,0.02,0.02,0.1,'b'],[0,-0.02,0.23,0.04,0.06,0.02,'b'],[0,-0.07,0.06,0.028,0.08,0.035,'d'],[0,-0.058,-0.01,0.01,0.01,0.07,'b']], mag: [0,-0.11,-0.03,0.03,0.14,0.05] },
  shotgun: { mz: -0.52, sy: 0.05, parts: [[0,0,-0.04,0.05,0.06,0.3,'d'],[0,0.012,-0.36,0.026,0.026,0.4,'b'],[0,-0.022,-0.34,0.032,0.032,0.34,'b'],[0,-0.025,-0.2,0.06,0.045,0.14,'w'],[0,0.03,-0.36,0.014,0.008,0.38,'b'],[0,0.04,-0.52,0.008,0.02,0.008,'b'],[0,-0.01,0.2,0.04,0.09,0.16,'w'],[0,-0.02,0.3,0.044,0.1,0.02,'b'],[0,-0.07,0.06,0.03,0.08,0.035,'w'],[0,-0.05,0.0,0.01,0.01,0.07,'b'],[0.027,0.0,0.0,0.004,0.02,0.07,'b']], mag: null },
  sniper: { mz: -0.8, sy: 0.058, parts: [[0,0,-0.02,0.05,0.07,0.36,'d'],[0,0.005,-0.5,0.022,0.022,0.5,'b'],[0,0.005,-0.78,0.034,0.034,0.07,'b'],[0,-0.01,-0.3,0.045,0.05,0.2,'g'],[0.04,0.0,0.07,0.04,0.012,0.012,'b'],[0.062,0.0,0.07,0.015,0.015,0.015,'g'],[0,-0.01,0.26,0.05,0.1,0.26,'w'],[0,0.04,0.26,0.04,0.02,0.12,'w'],[0,-0.075,0.08,0.03,0.09,0.04,'w'],[0.025,-0.07,-0.4,0.008,0.08,0.008,'b'],[-0.025,-0.07,-0.4,0.008,0.08,0.008,'b'],[0,0.04,-0.74,0.008,0.036,0.008,'b']], mag: [0,-0.065,-0.02,0.03,0.06,0.07] },
  lmg: { mz: -0.52, sy: 0.07, parts: [[0,0,-0.03,0.06,0.085,0.4,'d'],[0,0.05,-0.05,0.05,0.015,0.3,'g'],[0,-0.015,-0.26,0.07,0.06,0.2,'g'],[0,0.01,-0.4,0.026,0.026,0.2,'b'],[0,0.01,-0.51,0.034,0.034,0.04,'b'],[0,0.035,-0.3,0.014,0.014,0.2,'b'],[0.025,-0.07,-0.42,0.008,0.08,0.008,'b'],[-0.025,-0.07,-0.42,0.008,0.08,0.008,'b'],[0,0.055,-0.5,0.01,0.04,0.01,'b'],[0,-0.01,0.27,0.045,0.09,0.18,'d'],[0,-0.075,0.08,0.03,0.09,0.04,'d'],[0,0.07,0.1,0.03,0.02,0.01,'b']], mag: [0,-0.1,0.0,0.09,0.09,0.1] },
  dmr: { mz: -0.66, sy: 0.058, parts: [[0,0,-0.02,0.05,0.065,0.4,'d'],[0,-0.006,-0.26,0.055,0.05,0.22,'w'],[0,0.005,-0.5,0.02,0.02,0.2,'b'],[0,0.005,-0.64,0.03,0.03,0.05,'b'],[0,0.04,-0.6,0.008,0.036,0.008,'b'],[0,0.042,0.1,0.028,0.028,0.012,'b'],[0,-0.01,0.28,0.04,0.09,0.2,'w'],[0,0.035,0.28,0.036,0.02,0.1,'w'],[0,-0.08,0.08,0.03,0.09,0.035,'w'],[0,-0.06,-0.01,0.01,0.01,0.08,'b'],[0,0.036,-0.1,0.018,0.008,0.22,'b']], mag: [0,-0.08,-0.02,0.03,0.09,0.06] },
  pistol: { mz: -0.12, sy: 0.045, parts: [[0,0.018,-0.03,0.032,0.034,0.18,'g'],[0,-0.012,-0.03,0.03,0.03,0.15,'d'],[0,0.018,-0.115,0.016,0.016,0.02,'b'],[0,0.04,-0.1,0.006,0.012,0.006,'b'],[0,0.04,0.05,0.022,0.012,0.008,'b'],[0,-0.06,0.05,0.03,0.09,0.045,'b'],[0,-0.032,0.0,0.008,0.01,0.05,'b'],[0.017,0.018,0.04,0.002,0.03,0.04,'b']], mag: null },
  magnum: { mz: -0.24, sy: 0.05, parts: [[0,0,-0.03,0.04,0.055,0.2,'g'],[0,0.015,-0.17,0.028,0.03,0.2,'d'],[0,-0.012,-0.17,0.022,0.02,0.18,'g'],[0,0,0.0,0.05,0.055,0.06,'b'],[0,0.04,-0.26,0.008,0.016,0.008,'b'],[0,0.037,0.04,0.022,0.012,0.008,'b'],[0,-0.06,0.06,0.035,0.1,0.05,'w'],[0,0.03,0.04,0.012,0.02,0.012,'b']], mag: null },
};
const OPT = { red: [0.03, 0.035, 0.05, 0.075], x2: [0.034, 0.034, 0.12, 0.08], x4: [0.04, 0.04, 0.18, 0.085], x8: [0.045, 0.045, 0.24, 0.09] };
export const sightTop = (st) => { const g = GUNS[st.kind], o = st.att.optic; return o !== 'iron' && OPT[o] ? OPT[o][3] : g.sy; };

// يرسم السلاح في مصفوفة المجموعة G
export function drawGun(eng, G, st, flash, opts) {
  const g = GUNS[st.kind], a = st.att;
  const part = (x, y, z, sx, sy, sz, col, unlit) => {
    const m = mul(tmp(), G, trs(tmp(), x, y, z, 0, 0, sx, sy, sz));
    eng.cubeAt(m, col[0], col[1], col[2], unlit);
  };
  for (let i = 0; i < g.parts.length; i++) { const p = g.parts[i], dz = opts && opts.shift && opts.shift[i] || 0; if (opts && opts.hideRear && p[2] - p[5] / 2 > 0.1 && i !== 0) continue; part(p[0], p[1], p[2] + dz, p[3], p[4], p[5], C[p[6]]); }
  if (g.mag && !(opts && opts.hideMag)) {
    const k = a.mag === 'ext' ? 1.4 : 1;
    part(g.mag[0], g.mag[1] - (k - 1) * g.mag[4] * 0.5, g.mag[2], g.mag[3], g.mag[4] * k, g.mag[5], a.mag === 'fast' ? C.o : C.b);
  }
  if (a.optic !== 'iron' && OPT[a.optic]) {
    const o = OPT[a.optic], zc = -0.04, top = o[3];
    part(0, 0.034 + 0.01, zc, 0.024, 0.014, o[2] * 0.6 + 0.04, C.b);           // قاعدة التثبيت
    if (a.optic === 'red') {                                                      // نقطة حمراء: علبة + نافذة
      part(0, top - 0.02, zc, 0.034, 0.04, 0.05, C.b); part(0, top - 0.02, zc - 0.026, 0.026, 0.028, 0.004, [0.5, 0.8, 1], true); part(0, top - 0.018, zc - 0.028, 0.006, 0.006, 0.003, [1, 0.1, 0.1], true);
    } else {                                                                      // منظار: أنبوب + جرس أمامي وخلفي + عدسة
      const L = o[2], R = o[0];
      part(0, top - R / 2 - 0.004, zc, R, R, L, C.b); part(0, top - R / 2 - 0.004, zc - L / 2 - 0.012, R * 1.35, R * 1.35, 0.03, C.d); part(0, top - R / 2 - 0.004, zc + L / 2 + 0.01, R * 1.2, R * 1.2, 0.025, C.d);
      part(0, top - R / 2 - 0.004, zc - L / 2 - 0.028, R * 1.05, R * 1.05, 0.004, [0.3, 0.55, 0.9], true);
      part(0, top - R - 0.016, zc - L * 0.2, 0.012, 0.012, 0.03, C.d); part(0, top - R - 0.016, zc + L * 0.2, 0.012, 0.012, 0.03, C.d);
    }
  }
  if (a.muzzle === 'supp') { part(0, 0.005, g.mz - 0.07, 0.036, 0.036, 0.2, C.b); for (let i = 0; i < 4; i++) part(0, 0.005, g.mz - 0.01 - i * 0.045, 0.04, 0.04, 0.006, C.d); }
  else if (a.muzzle === 'comp') { part(0, 0.005, g.mz - 0.01, 0.036, 0.036, 0.06, C.d); part(0.019, 0.005, g.mz - 0.01, 0.004, 0.02, 0.03, C.b); part(-0.019, 0.005, g.mz - 0.01, 0.004, 0.02, 0.03, C.b); part(0, 0.021, g.mz - 0.01, 0.02, 0.004, 0.03, C.b); }
  if (a.under === 'grip') { part(0, -0.075, g.mz * 0.5 - 0.02, 0.028, 0.09, 0.03, C.d); part(0, -0.032, g.mz * 0.5 - 0.02, 0.04, 0.012, 0.05, C.b); part(0, -0.118, g.mz * 0.5 - 0.02, 0.032, 0.01, 0.034, C.b); }
  else if (a.under === 'laser') { part(0.03, -0.015, g.mz * 0.5, 0.022, 0.026, 0.07, C.b); part(0.03, -0.015, g.mz * 0.5 - 0.04, 0.012, 0.012, 0.012, [1, 0.1, 0.1], true); part(0.03, -0.015, g.mz * 0.5 - 0.5, 0.004, 0.004, 0.9, [1, 0.1, 0.1], true); }
  if (flash > 0) {
    const mzz = g.mz - (a.muzzle === 'supp' ? 0.16 : 0) - 0.03, s = (st.comp ? 1.4 : 1) * (a.muzzle === 'supp' ? 0.35 : 1) * (0.07 + 0.05 * flash);
    const fl = (sx, sy, sz, dz, r, g2, b2, al) => eng.cubeAt(mul(tmp(), G, trs(tmp(), 0, 0.005, mzz + dz, 0, 0, sx, sy, sz)), r, g2, b2, true, al * flash, true);
    fl(s, s, s * 2.2, 0, 1, 0.95, 0.7, 0.95);          // قلب الوميض
    fl(s * 3.2, s * 0.35, s * 0.5, -0.01, 1, 0.75, 0.3, 0.8); // صليب أفقي
    fl(s * 0.35, s * 3.2, s * 0.5, -0.01, 1, 0.75, 0.3, 0.8); // صليب عمودي
    fl(s * 0.5, s * 0.5, s * 3.2, -0.12, 1, 0.6, 0.2, 0.6);   // لسان أمامي
  }
}


// ---- حركة التعبئة (مشتركة للاعب الأول والآخرين) ----
// t: 0..1 تقدّم التعبئة. الإحداثيات في فضاء السلاح (الأمام = -Z). تُرجع إزاحة السلاح ومواضع المخزن القديم/الجديد واليد اليسرى.
const _ez = (x) => (x <= 0 ? 0 : x >= 1 ? 1 : x * x * (3 - 2 * x));
const seg = (t, a, b) => _ez((t - a) / (b - a));
const mix3 = (a, b, k) => [a[0] + (b[0] - a[0]) * k, a[1] + (b[1] - a[1]) * k, a[2] + (b[2] - a[2]) * k];
export const RELOAD_SYNC = { out: 0.17, in: 0.5, seat: 0.585, pull: 0.69, slam: 0.83 }; // نقاط الصوت (كسر من مدة التعبئة)
const BRASS = [0.85, 0.62, 0.18];
export function reloadPose(kind, t) {
  const g = GUNS[kind], sg = kind === 'shotgun', pistol = kind === 'pistol' || kind === 'magnum';
  const M = g.mag ? [g.mag[0], g.mag[1], g.mag[2]] : [0, -0.08, 0.05], MS = g.mag ? [g.mag[3], g.mag[4], g.mag[5]] : [0.028, 0.09, 0.04];
  const H0 = pistol ? [-0.03, -0.075, 0.05] : [-0.005, -0.05, g.mz * 0.42];
  const tilt = seg(t, 0, 0.13) - seg(t, 0.88, 1);
  const P = { hideRear: true, y: 0.09 * tilt, x: -0.1 * tilt, z: -0.12 * tilt, rx: -0.2 * tilt, ry: 0, rz: -0.5 * tilt, hand: H0, hideMag: false, mags: [], shells: [], bolt: 0, shift: null, handle: false, tilt };
  const pouch = [-0.14, -0.17, 0.06];
  if (sg) {
    const W = [0.14, 0.30, 0.46, 0.62]; let hand = H0;
    const port = [0, -0.035, -0.02];
    let cur = null;
    for (let k = 0; k < 4; k++) {
      const a = W[k], b = a + 0.14;
      if (t >= a && t < b + 0.02) {
        const u = (t - a) / (b - a);
        const pos = u < 0.45 ? mix3(pouch, [-0.05, -0.2, 0.02], seg(u, 0, 0.45)) : mix3([-0.05, -0.2, 0.02], port, seg(u, 0.45, 0.9));
        if (u < 0.92) cur = pos;
        hand = [pos[0] - 0.01, pos[1] - 0.045, pos[2]];
      }
    }
    if (cur) P.shells.push(cur);
    const pump = seg(t, 0.78, 0.84) - seg(t, 0.84, 0.9);
    if (t >= 0.76 && t < 0.92) { P.shift = { 2: 0.12 * pump, 3: 0.12 * pump }; hand = [-0.005, -0.04 + 0.0, g.mz * 0.42 + 0.12 * pump]; }
    else if (t >= 0.9) hand = mix3([-0.005, -0.04, g.mz * 0.42], H0, seg(t, 0.9, 0.97));
    else if (t < 0.14) hand = mix3(H0, pouch, seg(t, 0.04, 0.14));
    P.hand = hand; return P;
  }
  const Mb = [M[0], M[1] - MS[1] * 0.5 - 0.02, M[2]];            // أسفل بئر المخزن
  const hOld = seg(t, 0.14, 0.30);
  if (t < 0.58) P.hideMag = true;
  // المخزن القديم: يُسحب ثم يسقط بجاذبية
  if (t >= 0.0 && t < 0.52) {
    if (t < 0.30) P.mags.push({ p: [M[0] - 0.01 * hOld, M[1] - 0.17 * hOld, M[2]], s: MS });
    else { const q = t - 0.30; if (q < 0.17) P.mags.push({ p: [M[0] - 0.01 - q * 0.5, M[1] - 0.17 - 12 * q * q, M[2] + q * 0.3], s: MS }); }
  }
  // اليد: تقترب ثم تسحب ثم تنتقل لجيب المخزن ثم تدخل الجديد
  let hand = H0, nm = null;
  if (t < 0.14) hand = mix3(H0, Mb, seg(t, 0.0, 0.14));
  else if (t < 0.30) hand = [Mb[0] - 0.01 * hOld, Mb[1] - 0.17 * hOld, Mb[2]];
  else if (t < 0.40) hand = mix3([Mb[0] - 0.01, Mb[1] - 0.17, Mb[2]], pouch, seg(t, 0.30, 0.40));
  else if (t < 0.58) {
    const k = t < 0.52 ? seg(t, 0.40, 0.52) : 1, slam = seg(t, 0.52, 0.575);
    const a = mix3(pouch, [Mb[0], Mb[1] - 0.04, Mb[2]], k), mpos = t < 0.52 ? a : mix3([Mb[0], Mb[1] - 0.04, Mb[2]], M, slam);
    nm = { p: mpos, s: MS }; hand = [mpos[0] - 0.015, mpos[1] - MS[1] * 0.5 - 0.03, mpos[2]];
  } else if (t < 0.69) hand = mix3([M[0] - 0.015, M[1] - 0.07, M[2]], pistol ? [0.0, 0.025, 0.06] : [0.0, 0.05, 0.05], seg(t, 0.585, 0.69));
  else if (t < 0.84) hand = pistol ? [0.0, 0.025, 0.06] : [0.0, 0.05, 0.05];
  else hand = mix3(pistol ? [0.0, 0.025, 0.06] : [0.0, 0.05, 0.05], H0, seg(t, 0.84, 0.95));
  if (nm) P.mags.push(nm);
  // بعد الدخول لحظة "دقّة" (ارتداد صغير)
  P.z += 0.022 * Math.max(0, Math.sin(Math.min(1, Math.max(0, (t - 0.57) / 0.06)) * Math.PI)) - 0.012 * Math.max(0, Math.sin(Math.min(1, Math.max(0, (t - 0.82) / 0.05)) * Math.PI));
  // الترباس / الشريحة: يُسحب ثم يُطلق
  const b = seg(t, 0.69, 0.78) - seg(t, 0.8, 0.83); P.bolt = 0.06 * b;
  if (pistol) { P.shift = { 0: P.bolt }; if (t >= 0.69 && t < 0.84) hand = [0.0, 0.025, 0.06 + P.bolt]; }
  else { P.handle = t >= 0.58 && t < 0.9; if (t >= 0.69 && t < 0.84) hand = [0.0, 0.05, 0.05 + P.bolt]; }
  P.hand = hand;
  return P;
}
// يرسم عناصر التعبئة المتحركة (مخازن/خراطيش/مقبض التلقيم) داخل فضاء السلاح G
export function drawReloadExtras(eng, G, st, P) {
  const part = (p, s, col, unlit) => eng.cubeAt(mul(tmp(), G, trs(tmp(), p[0], p[1], p[2], 0, 0, s[0], s[1], s[2])), col[0], col[1], col[2], unlit);
  const fast = st.att && st.att.mag === 'fast';
  for (const m of P.mags) part(m.p, m.s, fast ? C.o : C.b);
  for (const sh of P.shells) { part(sh, [0.02, 0.02, 0.07], BRASS); part([sh[0], sh[1], sh[2] + 0.03], [0.021, 0.021, 0.018], [0.7, 0.1, 0.08]); }
  if (P.handle) part([st.kind === 'sniper' ? 0.062 : 0.03, st.kind === 'sniper' ? 0.0 : 0.032, 0.05 + P.bolt], [0.014, 0.014, 0.04], C.g);
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
  let P;
  if (dead) {
    const k = Math.min(1, a.deadT / 0.4), e = 1 - (1 - k) * (1 - k);
    P = trs(tmp(), a.x, a.y + 0.17 * e, a.z, a.yaw, -Math.PI / 2 * e, 1, 1, 1);
  } else P = trs(tmp(), a.x, a.y, a.z, a.yaw, 0, 1, 1, 1);
  const tc = TEAM[a.team] || TEAM[0], td = TEAM_D[a.team] || TEAM_D[0], pants = [0.17, 0.18, 0.15], boot = [0.08, 0.07, 0.07], vest = [0.2, 0.22, 0.2], glove = [0.1, 0.1, 0.11];
  if (a.far && !dead) { // مستوى تفصيل منخفض للبعيد: 5 مكعبات فقط
    const cb = (x, y, z, sx, sy, sz, c) => eng.cubeAt(mul(tmp(), P, trs(tmp(), x, y, z, 0, 0, sx, sy, sz)), c[0], c[1], c[2], false);
    cb(0, 0.42, 0, 0.4, 0.84, 0.24, pants); cb(0, 1.1, 0, 0.5, 0.6, 0.3, tc); cb(0, 1.58, 0, 0.24, 0.26, 0.24, td);
    if (a.stats) cb(0.12, 1.2, -0.35, 0.06, 0.08, 0.6, vest);
    return;
  }
  const cube = (m, c, unlit) => eng.cubeAt(m, c[0], c[1], c[2], unlit);
  if (dead) { // استلقاء: أطراف مفتوحة قليلاً
    const k = Math.min(1, a.deadT / 0.4), sp = 0.12 * k;
    cube(mul(tmp(), P, trs(tmp(), -0.12 - sp, 0.4, 0.02, 0, 0, 0.2, 0.8, 0.22)), pants); cube(mul(tmp(), P, trs(tmp(), 0.12 + sp, 0.4, 0, 0, 0, 0.2, 0.8, 0.22)), pants);
    cube(mul(tmp(), P, trs(tmp(), 0, 1.1, 0, 0, 0, 0.44, 0.58, 0.25)), tc); cube(mul(tmp(), P, trs(tmp(), 0, 1.13, 0.015, 0, 0, 0.5, 0.46, 0.29)), vest);
    cube(mul(tmp(), P, trs(tmp(), 0, 1.0, 0.2, 0, 0, 0.36, 0.42, 0.14)), [0.18, 0.2, 0.16]);
    cube(mul(tmp(), P, trs(tmp(), 0, 1.58, 0, 0, 0, 0.21, 0.23, 0.22)), C.s); eng.sphereAt(mul(tmp(), P, trs(tmp(), 0, 1.64, 0.01, 0, 0, 0.34, 0.27, 0.35)), td[0] * 0.9, td[1] * 0.9, td[2] * 0.9, false);
    cube(mul(tmp(), P, trs(tmp(), 0.36 + sp * 2, 1.2, 0, 0, 0, 0.12, 0.54, 0.12)), td); cube(mul(tmp(), P, trs(tmp(), -0.36 - sp * 2, 1.2, 0, 0, 0, 0.12, 0.54, 0.12)), td);
    return;
  }
  // ---- مشية واقعية: فخذ + ساق بثني الركبة ، اهتزاز الجذع ، ميلان عند الركض ، انحناء حقيقي ----
  const mv = a.moving ? 1 : 0, cr = !!a.crouch, sit = !!a.sit;
  const amp = (a.sprint ? 0.95 : 0.68) * (cr ? 0.45 : 1) * mv;
  const hipY = 0.84 - (cr && !sit ? 0.34 : 0), bob = Math.abs(Math.sin(a.phase)) * 0.032 * mv * (a.sprint ? 1.7 : 1) * (cr ? 0.5 : 1);
  const L = sit ? 0 : (a.sprint ? 0.2 : 0) + (cr ? 0.1 : 0) + 0.02 * Math.sin(a.phase * 2) * mv;
  const legs = (x, ph) => {
    if (sit) {
      cube(mul(tmp(), P, trs(tmp(), x, 0.62, -0.2, 0, 0, 0.19, 0.18, 0.5)), pants);
      cube(mul(tmp(), P, trs(tmp(), x, 0.3, -0.42, 0, 0, 0.17, 0.5, 0.18)), pants);
      cube(mul(tmp(), P, trs(tmp(), x, 0.06, -0.5, 0, 0, 0.2, 0.12, 0.34)), boot);
      return;
    }
    const th = Math.sin(ph) * amp + (cr ? 0.85 : 0), flex = Math.max(0, Math.cos(ph)) * 0.85 * mv * (cr ? 0.5 : 1) + (cr ? 1.7 : 0), sa = th - flex;
    const hy = hipY + bob * 0.5, ky = hy - 0.42 * Math.cos(th), kz = -0.42 * Math.sin(th);
    cube(mul(tmp(), P, trs(tmp(), x, hy - 0.21 * Math.cos(th), -0.21 * Math.sin(th), 0, th, 0.2, 0.44, 0.22)), pants);
    cube(mul(tmp(), P, trs(tmp(), x, ky - 0.21 * Math.cos(sa), kz - 0.21 * Math.sin(sa), 0, sa, 0.18, 0.44, 0.2)), pants);
    const ay = ky - 0.42 * Math.cos(sa), az = kz - 0.42 * Math.sin(sa);
    cube(mul(tmp(), P, trs(tmp(), x, ay - 0.05, az - 0.08, 0, sa * 0.35, 0.21, 0.14, 0.3)), boot);
  };
  legs(-0.12, a.phase); legs(0.12, a.phase + Math.PI);
  // الجزء العلوي: يدور حول الورك (ميلان) وقد يميل خارج المركبة
  const U0 = a.lean ? mul(tmp(), P, trs(tmp(), a.lean[0], 0.02, a.lean[1], 0, 0, 1, 1, 1)) : P;
  const U = mul(tmp(), U0, trs(tmp(), 0, sit ? 0.84 : hipY + bob, 0, 0, L, 1, 1, 1));
  const Y0 = 0.84, up = (x, y, z, sx, sy, sz) => mul(tmp(), U, trs(tmp(), x, y - Y0, z, 0, 0, sx, sy, sz));
  cube(up(0, 1.1, 0, 0.44, 0.58, 0.25), tc);                 // قميص
  cube(up(0, 1.13, 0.015, 0.5, 0.46, 0.29), vest);           // سترة واقية
  cube(up(0, 1.22, -0.15, 0.36, 0.2, 0.05), [0.26, 0.28, 0.25]); // لوح أمامي
  for (const px of [-0.14, 0, 0.14]) cube(up(px, 0.98, -0.16, 0.1, 0.12, 0.06), [0.16, 0.17, 0.15]); // جيوب
  cube(up(0, 1.0, 0.2, 0.36, 0.42, 0.14), [0.18, 0.2, 0.16]);  // حقيبة ظهر
  cube(up(0, 0.92, 0, 0.5, 0.1, 0.3), [0.1, 0.1, 0.11]);       // حزام
  cube(up(0, 1.44, 0, 0.11, 0.1, 0.11), C.s);                  // رقبة
  // الرأس: وجه + خوذة + نظارة (يتبع النظر مع تعويض الميلان)
  const HP = mul(tmp(), U, trs(tmp(), 0, 1.58 - Y0, 0, 0, a.pitch * 0.5 - L, 1, 1, 1));
  cube(mul(tmp(), HP, trs(tmp(), 0, 0, 0, 0, 0, 0.21, 0.23, 0.22)), C.s);
  eng.sphereAt(mul(tmp(), HP, trs(tmp(), 0, 0.06, 0.01, 0, 0, 0.34, 0.27, 0.35)), td[0] * 0.9, td[1] * 0.9, td[2] * 0.9, false);
  cube(mul(tmp(), HP, trs(tmp(), 0, 0.02, -0.115, 0, 0, 0.23, 0.07, 0.03)), [0.05, 0.06, 0.08]);
  cube(mul(tmp(), HP, trs(tmp(), 0, -0.1, -0.08, 0, 0, 0.18, 0.05, 0.08)), [0.12, 0.12, 0.12]);
  const st = a.stats;
  const ap = a.sprint ? 0.2 : a.pitch, br = 0.015 * Math.sin(a.phase * 0.5 + (t || 0) * 2);
  const arm = (x, rot, len = 0.54) => {
    rot -= L;
    cube(pivot(tmp(), U, x, 1.36 - Y0, 0, rot, 0, -len / 2, 0, 0.12, len, 0.12), td);
    cube(pivot(tmp(), U, x, 1.36 - Y0, 0, rot, 0, -len - 0.02, 0, 0.1, 0.1, 0.1), glove);
  };
  if (st) {
    const base = a.sprint ? 0.9 : Math.PI / 2;
    const RP = a.rl >= 0 ? reloadPose(st.kind, a.rl) : null;
    arm(0.27, base + ap - 0.12 + br); arm(-0.27, base + ap + 0.12 + br + (RP ? RP.tilt * 0.55 : 0));
    let G = pivot(tmp(), U, 0.12, 1.3 - Y0, 0, ap - L, 0, -0.05, -0.38, 1, 1, 1);
    if (RP) G = mul(tmp(), mul(tmp(), G, trs(tmp(), RP.x * 0.6, RP.y * 0.7, RP.z, RP.ry * 0.8, RP.rx * 0.6, 1, 1, 1)), rotZ(RP.rz * 0.7));
    drawGun(eng, G, st, a.flash || 0, RP);
    if (RP) drawReloadExtras(eng, G, st, RP);
  } else { arm(0.27, 0.05); arm(-0.27, -0.05); }
}

const _rz = new Float32Array(16);
function rotZ(a) { const c = Math.cos(a), sn = Math.sin(a); _rz.fill(0); _rz[0] = c; _rz[1] = sn; _rz[4] = -sn; _rz[5] = c; _rz[10] = 1; _rz[15] = 1; return _rz; }

// ---- مجسم السلاح بمنظور اللاعب ----
export function drawViewmodel(eng, vm) {
  const st = vm.stats, g = GUNS[st.kind], k = vm.ads;
  const top = sightTop(st);
  const hip = [0.17, -0.17, -0.33], ads = [0, -top - 0.002, -0.36];
  let x = hip[0] + (ads[0] - hip[0]) * k, y = hip[1] + (ads[1] - hip[1]) * k, z = hip[2] + (ads[2] - hip[2]) * k;
  x += vm.bobX + vm.swayX; y += vm.bobY + vm.swayY; z += vm.kick * 0.045;
  let rx = vm.kick * 0.05, ry = 0.07 * (1 - k); { const tt = performance.now() / 1000; y += Math.sin(tt * 1.7) * 0.0025 * (1 - k * 0.7); x += Math.sin(tt * 0.9) * 0.0015; }
  const RP = vm.reload >= 0 ? reloadPose(st.kind, vm.reload) : null;
  if (RP) { x += RP.x; y += RP.y; z += RP.z; rx += RP.rx; ry += RP.ry; }
  if (vm.swap < 1) { const s = 1 - vm.swap; y -= s * 0.25; rx -= s * 0.6; }
  if (vm.sprint) { y -= 0.06; rx -= 0.35; ry = 0.4; x -= 0.04; }
  eng.beginViewmodel();
  const G = RP && RP.rz ? mul(tmp(), trs(tmp(), x, y, z, ry, rx, 1, 1, 1), rotZ(RP.rz)) : trs(tmp(), x, y, z, ry, rx, 1, 1, 1);
  drawGun(eng, G, st, vm.flash, RP);
  if (RP) drawReloadExtras(eng, G, st, RP);
  // الأيدي والأكمام
  const sleeve = TEAM_D[vm.team] || TEAM_D[0], skin = C.s;
  const part = (px, py, pz, sx, sy, sz, c) => eng.cubeAt(mul(tmp(), G, trs(tmp(), px, py, pz, 0, 0, sx, sy, sz)), c[0], c[1], c[2], false);
  const gz = st.kind === 'pistol' || st.kind === 'magnum' ? 0.04 : 0.06;
  part(0.0, -0.075, gz, 0.05, 0.05, 0.06, skin);
  part(0.015, -0.12, gz + 0.12, 0.065, 0.06, 0.28, sleeve);
  if (RP) { // اليد اليسرى تتبع حركة التعبئة
    const h = RP.hand; part(h[0], h[1], h[2], 0.055, 0.05, 0.07, skin); part(h[0] - 0.03, h[1] - 0.05, h[2] + 0.13, 0.06, 0.055, 0.18, sleeve);
  } else if (st.kind !== 'pistol' && st.kind !== 'magnum') {
    part(-0.005, -0.05, g.mz * 0.42, 0.055, 0.05, 0.07, skin);
    part(-0.03, -0.09, g.mz * 0.42 + 0.2, 0.06, 0.055, 0.3, sleeve);
  } else part(-0.03, -0.075, gz + 0.01, 0.05, 0.05, 0.06, skin);
  eng.endViewmodel();
}

// ---- المؤثرات ----
const CAP_LITE = { parts: 36, decals: 36, smokes: 16, tracers: 12, shells: 10 }, CAP_FULL = { parts: 70, decals: 90, smokes: 40, tracers: 24, shells: 30 };
export class Effects {
  constructor() { this.tracers = []; this.parts = []; this.booms = []; this.smokes = []; this.flames = []; this.shells = []; this.decals = []; this.lite = false; }
  get cap() { return this.lite ? CAP_LITE : CAP_FULL; }
  decal(x, y, z, blood) { if (this.decals.length >= this.cap.decals) this.decals.shift(); this.decals.push({ x, y, z, age: 0, blood: !!blood, s: 0.05 + Math.random() * 0.04 }); }
  dust(x, z, k = 1) { if (this.smokes.length < this.cap.smokes) this.smokes.push({ x, y: 0.15, z, vx: (Math.random() - 0.5) * 0.8, vz: (Math.random() - 0.5) * 0.8, age: 0, life: 0.9 + Math.random() * 0.6, col: [0.78, 0.68, 0.5], rise: 0.35, sz: 0.5 + k * 0.6, a: 0.32 }); }
  shell(x, y, z, yaw, big) { if (this.shells.length >= this.cap.shells) this.shells.shift(); const rx = Math.cos(yaw), rz = -Math.sin(yaw), v = 1.8 + Math.random() * 1.2; this.shells.push({ x, y, z, vx: rx * v + (Math.random() - 0.5), vy: 1.6 + Math.random(), vz: rz * v + (Math.random() - 0.5), age: 0, life: 2.5, big, rot: Math.random() * 6 }); }
  smoke(x, y, z, col) { if (this.smokes.length < this.cap.smokes) this.smokes.push({ x, y, z, vx: (Math.random() - 0.5) * 0.5, vz: (Math.random() - 0.5) * 0.5, age: 0, life: 1.8 + Math.random() * 1.4, col }); }
  flame(x, y, z) { if (this.flames.length < 24) this.flames.push({ x, y, z, age: 0, life: 0.35 + Math.random() * 0.25 }); }
  tracer(o, e, muted, hit) { if (this.tracers.length < this.cap.tracers) this.tracers.push({ o, e, age: 0, muted }); }
  spark(x, y, z, col, n = 5, spd = 3) {
    for (let i = 0; i < (this.lite ? Math.ceil(n * 0.6) : n) && this.parts.length < this.cap.parts; i++) {
      this.parts.push({ x, y, z, vx: (Math.random() - 0.5) * spd, vy: Math.random() * spd * 0.8, vz: (Math.random() - 0.5) * spd, life: 0.25 + Math.random() * 0.25, age: 0, col, s: 0.03 + Math.random() * 0.03 });
    }
  }
  boom(x, y, z) { this.booms.push({ x, y, z, age: 0 }); this.spark(x, y, z, [1, 0.6, 0.15], 18, 12); this.spark(x, y, z, [0.3, 0.3, 0.3], 8, 6); }
  update(dt) {
    for (const t of this.tracers) t.age += dt; this.tracers = this.tracers.filter(t => t.age < 0.14);
    for (const p of this.parts) { p.age += dt; p.vy -= 12 * dt; p.x += p.vx * dt; p.y += p.vy * dt; p.z += p.vz * dt; }
    this.parts = this.parts.filter(p => p.age < p.life);
    for (const b of this.booms) b.age += dt; this.booms = this.booms.filter(b => b.age < 0.7);
    for (const q of this.smokes) { q.age += dt; q.y += dt * (q.rise === undefined ? 1.6 : q.rise); q.x += q.vx * dt; q.z += q.vz * dt; } this.smokes = this.smokes.filter(q => q.age < q.life);
    for (const q of this.shells) { q.age += dt; q.rot += dt * 14; if (q.y > 0.03) { q.vy -= 14 * dt; q.x += q.vx * dt; q.y += q.vy * dt; q.z += q.vz * dt; if (q.y <= 0.03) { q.y = 0.03; q.vy = q.vy < -2 ? -q.vy * 0.35 : 0; q.vx *= 0.4; q.vz *= 0.4; if (q.vy === 0) q.vx = q.vz = 0; } } }
    this.shells = this.shells.filter(q => q.age < q.life);
    for (const d of this.decals) d.age += dt; this.decals = this.decals.filter(d => d.age < 40);
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
      if (!this.lite) eng.cubeAt(basis(tmp(), mx, my, mz, dx / L, dy / L, dz / L, 0.06, 0.06, len * 0.9), 1, 0.6, 0.2, true, t.muted ? 0.06 : 0.22, true); // هالة
      const hx = t.o[0] + dx / L * s, hy = t.o[1] + dy / L * s, hz = t.o[2] + dz / L * s; // رأس الرصاصة
      eng.cubeAt(basis(tmp(), hx, hy, hz, dx / L, dy / L, dz / L, 0.045, 0.045, 0.09), 1, 1, 0.85, true, t.muted ? 0.4 : 1, true);
    }
    for (const p of this.parts) {
      const f = 1 - p.age / p.life;
      eng.cubeAt(trs(tmp(), p.x, p.y, p.z, 0, 0, p.s, p.s, p.s), p.col[0], p.col[1], p.col[2], true, f);
    }
    for (const d of this.decals) { const f = d.age > 30 ? 1 - (d.age - 30) / 10 : 1; eng.cubeAt(trs(tmp(), d.x, d.y, d.z, 0, 0, d.s, d.s, d.s), d.blood ? 0.35 * f : 0.07, d.blood ? 0.02 : 0.07, d.blood ? 0.02 : 0.08, false); }
    for (const q of this.shells) { const l = q.big ? 0.09 : 0.055, w = q.big ? 0.03 : 0.02; eng.cubeAt(trs(tmp(), q.x, q.y, q.z, q.rot, 0, w, w, l), 0.85, 0.65, 0.2, false); }
    for (const q of this.smokes) { const k = q.age / q.life, sz = 0.5 + k * (q.sz || 1.8); eng.sphereAt(trs(tmp(), q.x, q.y, q.z, 0, 0, sz, sz, sz), q.col[0], q.col[1], q.col[2], true, (1 - k) * (q.a || 0.5)); }
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
