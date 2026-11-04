// الخرائط الأربع — مبنية من صناديق (AABB) بتماثل دوراني 180° لضمان توازن الفريقين
// k: 0 جدار، 1 صندوق/ساتر، 2 مميّز/منصة، 3 حاوية/مركبة

export const MAP_NAMES = ['المستودع', 'أطلال الصحراء', 'شارع المدينة', 'قاعدة القطب', 'صحراء الشاص (مركبات)'];

const SWAP = { n: 's', s: 'n', e: 'w', w: 'e' };

function builder(W, D) {
  const boxes = [], decos = [], climbs = [];
  const deco = (cx, cz, w, d, h, y0 = 0, k = 1) => { decos.push({ x0: cx - w / 2, x1: cx + w / 2, z0: cz - d / 2, z1: cz + d / 2, y0, y1: y0 + h, k }); };
  const decoSym = (cx, cz, w, d, h, y0 = 0, k = 1) => { deco(cx, cz, w, d, h, y0, k); deco(-cx, -cz, w, d, h, y0, k); };
  const add = (cx, cz, w, d, h, y0 = 0, k = 1) => {
    boxes.push({ x0: cx - w / 2, x1: cx + w / 2, z0: cz - d / 2, z1: cz + d / 2, y0, y1: y0 + h, k });
  };
  const sym = (cx, cz, w, d, h, y0 = 0, k = 1) => {
    add(cx, cz, w, d, h, y0, k);
    if (Math.abs(cx) > 0.01 || Math.abs(cz) > 0.01) add(-cx, -cz, w, d, h, y0, k);
  };
  const T = 0.5;
  // غرفة بجدران وأبواب (n: شمال z-, s: جنوب z+, w: غرب x-, e: شرق x+)
  const room = (cx, cz, w, d, h, doors = '', roof = true) => {
    const seg = (x, z, len, horiz, door) => {
      if (!door) { horiz ? add(x, z, len, T, h, 0, 0) : add(x, z, T, len, h, 0, 0); return; }
      const dw = 3, sg = (len - dw) / 2, off = (dw + sg) / 2;
      if (horiz) { add(x - off, z, sg, T, h, 0, 0); add(x + off, z, sg, T, h, 0, 0); add(x, z, dw, T, h - 2.6, 2.6, 0); }
      else { add(x, z - off, T, sg, h, 0, 0); add(x, z + off, T, sg, h, 0, 0); add(x, z, T, dw, h - 2.6, 2.6, 0); }
    };
    seg(cx, cz - d / 2, w, true, doors.includes('n'));
    seg(cx, cz + d / 2, w, true, doors.includes('s'));
    seg(cx - w / 2, cz, d, false, doors.includes('w'));
    seg(cx + w / 2, cz, d, false, doors.includes('e'));
    if (roof) add(cx, cz, w + T, d + T, 0.4, h, 2);
  };
  const roomS = (cx, cz, w, d, h, doors = '', roof = true) => {
    room(cx, cz, w, d, h, doors, roof);
    room(-cx, -cz, w, d, h, doors.split('').map(c => SWAP[c]).join(''), roof);
  };
  // درج: خطوات ارتفاع 0.4 تصعد نحو المنصة. dir: 'x+' يعني الدرج يمتد نحو +x بعيدًا عن المنصة
  const stairs = (x, z, n, tread, width, dir) => {
    const L = n * tread + 1.4; // نقطة بداية اختبار الصعود
    climbs.push(dir === 'x+' ? { x: x + L, z, vx: -1, vz: 0, h: 0.4 * n } : dir === 'x-' ? { x: x - L, z, vx: 1, vz: 0, h: 0.4 * n } : dir === 'z+' ? { x, z: z + L, vx: 0, vz: -1, h: 0.4 * n } : { x, z: z - L, vx: 0, vz: 1, h: 0.4 * n });
    for (let i = 0; i < n; i++) {
      const top = 0.4 * (i + 1), off = (n - 1 - i) * tread + tread / 2;
      if (dir === 'x+') add(x + off, z, tread, width, top, 0, 2);
      else if (dir === 'x-') add(x - off, z, tread, width, top, 0, 2);
      else if (dir === 'z+') add(x, z + off, width, tread, top, 0, 2);
      else add(x, z - off, width, tread, top, 0, 2);
    }
  };

  // جدار بفتحات: ops = [{c: إزاحة المركز عن منتصف الجدار, w: العرض, type: 'door'|'win'}]
  const wall = (x, z, len, horiz, h, ops = [], y0 = 0, k = 0) => {
    const list = ops.slice().sort((a, b) => a.c - b.c); let cur = -len / 2;
    const seg = (a, b, ya, yb) => {
      if (b - a < 0.05 || yb - ya < 0.05) return; const mid = (a + b) / 2, L = b - a;
      if (horiz) add(x + mid, z, L, T, yb - ya, y0 + ya, k); else add(x, z + mid, T, L, yb - ya, y0 + ya, k);
    };
    for (const o of list) {
      const a = o.c - o.w / 2, c2 = o.c + o.w / 2; seg(cur, a, 0, h);
      if (o.type === 'door') seg(a, c2, 2.6, h); else { seg(a, c2, 0, 0.9); seg(a, c2, 2.3, h); }
      cur = c2;
    }
    seg(cur, len / 2, 0, h);
  };
  // مبنى بأبواب ونوافذ. o: { doors: 'nsew', at: {n: إزاحة}, win: عدد النوافذ لكل جدار, roof, ruin: احتمال انهيار أجزاء }
  const building = (cx, cz, w, d, h, o = {}) => {
    const doors = o.doors || '', win = o.win ?? 2, sides = [['n', cx, cz - d / 2, w, true], ['s', cx, cz + d / 2, w, true], ['w', cx - w / 2, cz, d, false], ['e', cx + w / 2, cz, d, false]];
    for (const [sd, x, z, len, horiz] of sides) {
      const ops = []; const dc = doors.includes(sd) ? ((o.at && o.at[sd]) || 0) : null;
      if (dc !== null) ops.push({ c: dc, w: 3, type: 'door' });
      for (let i = 0; i < win; i++) {
        const c = -len / 2 + (i + 1) * len / (win + 1);
        if (dc !== null && Math.abs(c - dc) < 3.4) continue; if (len < 6) continue;
        ops.push({ c, w: 1.8, type: 'win' });
      }
      wall(x, z, len, horiz, h, ops);
    }
    if (o.roof !== false) add(cx, cz, w + T, d + T, 0.4, h, 2);
    if (o.para) { const y = h + 0.4, op = o.open || '';
      if (!op.includes('n')) add(cx, cz - d / 2, w, 0.3, 0.8, y, 0); if (!op.includes('s')) add(cx, cz + d / 2, w, 0.3, 0.8, y, 0);
      if (!op.includes('w')) add(cx - w / 2, cz, 0.3, d, 0.8, y, 0); if (!op.includes('e')) add(cx + w / 2, cz, 0.3, d, 0.8, y, 0); }
  };
  const swp = (s) => (s || '').split('').map((c) => SWAP[c]).join('');
  const buildingS = (cx, cz, w, d, h, o = {}) => {
    building(cx, cz, w, d, h, o); const at = {}; if (o.at) for (const k in o.at) at[SWAP[k]] = -o.at[k];
    building(-cx, -cz, w, d, h, { ...o, doors: swp(o.doors), open: swp(o.open), at });
  };
  const OPP = { 'x+': 'x-', 'x-': 'x+', 'z+': 'z-', 'z-': 'z+' };
  const stairsS = (x, z, n, t, wd, dir) => { stairs(x, z, n, t, wd, dir); stairs(-x, -z, n, t, wd, OPP[dir]); };
  // جدار داخلي بباب
  const part = (x0, z0, x1, z1, h, doorAt = null) => {
    const horiz = Math.abs(z1 - z0) < 0.01, len = horiz ? Math.abs(x1 - x0) : Math.abs(z1 - z0);
    wall((x0 + x1) / 2, (z0 + z1) / 2, len, horiz, h, doorAt === null ? [] : [{ c: doorAt, w: 3, type: 'door' }]);
  };
  // مستطيل زخرفي على الأرض (طرق، أرصفة، علامات)
  const patch = (cx, cz, w, d, k, h = 0.04) => deco(cx, cz, w, d, h, 0, k);
  const patchSym = (cx, cz, w, d, k, h = 0.04) => decoSym(cx, cz, w, d, h, 0, k);
  // خط متقطع على الطريق (أفقي أو رأسي)
  const dashes = (x0, z0, x1, z1, k = 8) => {
    const L = Math.hypot(x1 - x0, z1 - z0), n = Math.floor(L / 6);
    for (let i = 0; i < n; i++) { const t = (i + 0.5) / n, x = x0 + (x1 - x0) * t, z = z0 + (z1 - z0) * t; Math.abs(x1 - x0) > Math.abs(z1 - z0) ? deco(x, z, 3, 0.25, 0.07, 0, k) : deco(x, z, 0.25, 3, 0.07, 0, k); }
  };
  // حوض/فناء مسوّر بأكياس رمل (ساتر منخفض) مع فتحات
  const sandbags = (cx, cz, w, d, gaps = '') => {
    const hh = 1.1, t = 0.9;
    const side = (x, z, len, horiz, gap) => {
      if (!gap) { horiz ? add(x, z, len, t, hh, 0, 1) : add(x, z, t, len, hh, 0, 1); return; }
      const g = 5, s2 = (len - g) / 2, off = (g + s2) / 2;
      if (horiz) { add(x - off, z, s2, t, hh, 0, 1); add(x + off, z, s2, t, hh, 0, 1); } else { add(x, z - off, t, s2, hh, 0, 1); add(x, z + off, t, s2, hh, 0, 1); }
    };
    side(cx, cz - d / 2, w, true, gaps.includes('n')); side(cx, cz + d / 2, w, true, gaps.includes('s')); side(cx - w / 2, cz, d, false, gaps.includes('w')); side(cx + w / 2, cz, d, false, gaps.includes('e'));
  };
  const sandbagsS = (cx, cz, w, d, gaps = '') => { sandbags(cx, cz, w, d, gaps); sandbags(-cx, -cz, w, d, swp(gaps)); };
  const border = (h = 7) => {
    const t = 1;
    add(0, -D / 2 - t / 2, W + 2 * t, t, h, 0, 0);
    add(0, D / 2 + t / 2, W + 2 * t, t, h, 0, 0);
    add(-W / 2 - t / 2, 0, t, D, h, 0, 0);
    add(W / 2 + t / 2, 0, t, D, h, 0, 0);
  };
  return { boxes, decos, climbs, add, sym, deco, decoSym, room, roomS, stairs, border, wall, building, buildingS, stairsS, part, patch, patchSym, dashes, sandbags, sandbagsS };
}

function spawnSet(W, D, margin, zSpan) {
  const red = [], blue = [];
  for (let i = 0; i < 8; i++) {
    const x = -(W / 2 - margin) + (i % 2) * 2.5;
    const z = -zSpan + i * (2 * zSpan / 7);
    red.push([x, z]);
    blue.push([-x, -z]);
  }
  return { red, blue };
}


// ---------- أدوات توليد ثابتة (بذرة ثابتة => نفس الخريطة عند الجميع) ----------
function rngf(seed) {
  let a = seed >>> 0;
  return () => { a = (a + 0x6D2B79F5) >>> 0; let t = a; t = Math.imul(t ^ (t >>> 15), t | 1); t ^= t + Math.imul(t ^ (t >>> 7), t | 61); return ((t ^ (t >>> 14)) >>> 0) / 4294967296; };
}
const rg = (r, [a, c]) => a + r() * (c - a);
// أنواع الدعائم: كل نوع يبني نفسه بتماثل دوراني
const PROP = {
  box: (b, x, z, w, d, h, k) => b.sym(x, z, w, d, h, 0, k),
  dune: (b, x, z, w, d, h) => { // كثيب متدرّج يمكن تسلّقه (درجات 0.5م)
    const n = Math.max(2, Math.min(4, Math.round(h / 0.5)));
    for (let i = 0; i < n; i++) { const f = 1 - i * 0.26; b.sym(x, z, w * f, d * f, 0.5 * (i + 1), 0, 2); }
  },
  palm: (b, x, z, w, d, h) => { // جذع يحجب + سعف زخرفي
    b.sym(x, z, 0.7, 0.7, h, 0, 5);
    b.decoSym(x, z, 4.6, 0.5, 0.35, h, 4); b.decoSym(x, z, 0.5, 4.6, 0.35, h, 4); b.decoSym(x, z, 1.4, 1.4, 0.5, h + 0.2, 4);
  },
  pine: (b, x, z, w, d, h) => {
    b.sym(x, z, 0.6, 0.6, 1.4, 0, 5);
    b.decoSym(x, z, 3.4, 3.4, 0.8, 1.2, 4); b.decoSym(x, z, 2.4, 2.4, 0.8, 2.0, 4); b.decoSym(x, z, 1.3, 1.3, 0.8, 2.8, 4);
  },
  lamp: (b, x, z, w, d, h) => { b.sym(x, z, 0.4, 0.4, h, 0, 0); b.decoSym(x, z, 1.4, 0.3, 0.25, h, 2); },
};
// يوزّع دعائم بدون تداخل (مع مسافة pad) وبعيدًا عن نقاط avoid=[x,z,r]
function scatter(b, rng, avoid, o) {
  let placed = 0, tries = 0;
  const pad = o.pad ?? 1.4, tot = o.types.reduce((a, t) => a + (t.wt || 1), 0);
  const pick = () => { let r0 = rng() * tot; for (const t of o.types) { r0 -= t.wt || 1; if (r0 <= 0) return t; } return o.types[0]; };
  const hit = (x0, x1, z0, z1) => {
    for (const q of b.boxes) if (q.y0 < 3 && q.y1 > 0.05 && x0 < q.x1 + pad && x1 > q.x0 - pad && z0 < q.z1 + pad && z1 > q.z0 - pad) return true;
    return false;
  };
  while (placed < o.n && tries < o.n * 80) {
    tries++;
    const t = pick(), w = rg(rng, t.w), d = rg(rng, t.d), h = rg(rng, t.h);
    const x = rg(rng, o.x), z = rg(rng, o.z), hw = w / 2, hd = d / 2;
    if (hit(x - hw, x + hw, z - hd, z + hd)) continue;
    if (x + hw + pad > -x - hw && x - hw - pad < -x + hw && z + hd + pad > -z - hd && z - hd - pad < -z + hd) continue; // يتداخل مع صورته
    let bad = false;
    for (const [ax, az, ar] of avoid) { const dx = Math.max(Math.abs(x - ax) - hw, 0), dz = Math.max(Math.abs(z - az) - hd, 0); if (dx * dx + dz * dz < ar * ar) { bad = true; break; } }
    if (bad) continue;
    (PROP[t.p || 'box'])(b, x, z, w, d, h, t.k ?? 1);
    placed++;
  }
  return placed;
}
// حلقة تلال/جبال زخرفية خارج الحدود (غير قابلة للاصطدام) لتكسر الأفق الفارغ
function skyline(b, rng, W, D, o) {
  const n = o.n;
  for (let i = 0; i < n; i++) {
    const t = i / n * 4, side = Math.floor(t), u = t - side;
    const off = o.near + rng() * (o.far - o.near);
    let x, z;
    if (side === 0) { x = -W / 2 - 8 + u * (W + 16); z = -D / 2 - off; }
    else if (side === 1) { x = W / 2 + off; z = -D / 2 - 8 + u * (D + 16); }
    else if (side === 2) { x = W / 2 + 8 - u * (W + 16); z = D / 2 + off; }
    else { x = -W / 2 - off; z = D / 2 + 8 - u * (D + 16); }
    const w = rg(rng, o.w), d = rg(rng, o.w), h = rg(rng, o.h), tiers = o.tiers || 3;
    for (let j = 0; j < tiers; j++) { const f = 1 - j * 0.28; b.deco(x, z, w * f, d * f, h * (j + 1) / tiers, 0, o.k); }
  }
}

const av = (sp, extra = []) => [...sp.red, ...sp.blue].map(([x, z]) => [x, z, 4.5]).concat(extra);
const MAPS = [
  // 0 — المستودع (داخلي، رفوف ومكاتب وممر علوي)
  () => {
    const W = 96, D = 60, b = builder(W, D);
    b.border(8); b.add(0, 0, W + 2, D + 2, 0.5, 8, 0);
    // أرضية: ممرات ومناطق مرسومة
    b.patch(0, 0, W - 6, 10, 7); b.dashes(-W / 2 + 3, 0, W / 2 - 3, 0, 8);
    b.patchSym(-42.5, 0, 8, 50, 6); b.patchSym(-24, -26, 30, 4, 7); b.patch(0, 0, 8, D - 4, 7);
    for (const x of [-36, -24, -12, 0, 12, 24, 36]) for (const z of [-20, 0, 20]) b.deco(x, z, 6, 0.5, 0.15, 7.85, 8);
    // رفوف التخزين (صفوف طويلة بممرات عرضية)
    const rack = (x, z, len) => { b.sym(x, z, len, 1.2, 3.4, 0, 6); b.decoSym(x, z, len - 0.6, 1.0, 0.9, 3.4, 1); b.decoSym(x, z - 0.2, len * 0.4, 0.9, 0.7, 4.3, 5); };
    for (const z of [-24.5, -17.5, -10.5]) { rack(-30, z, 10); rack(-16.5, z, 8); }
    // مكاتب إدارة بأبواب ونوافذ + سطح مع درج
    b.buildingS(-21, 0, 11, 8, 3.4, { doors: 'ew', win: 2, para: true, open: 's' });
    b.stairsS(-21, 7.4, 9, 0.8, 2.4, 'z+'); // يصعد إلى سطح المكتب
    // الممر العلوي الأوسط
    b.add(0, 0, 4, 36, 0.4, 2.4, 2);
    for (const z of [-15, -5, 5, 15]) b.add(0, z, 0.6, 0.6, 2.4, 0, 0);
    b.add(1.85, 0, 0.2, 36, 0.9, 2.8, 1); b.add(-1.85, 0, 0.2, 36, 0.9, 2.8, 1);
    b.stairsS(0, -18, 7, 1.0, 2.4, 'z-');
    // غرفة تحكم صغيرة تحت الممر العلوي مع نوافذ
    b.sym(7.5, -3, 3, 3, 2.2, 0, 3);
    // حاويات على أرصفة التحميل
    for (const [x, z] of [[-30, -27.2], [-18, -27.2], [-6, -27.2]]) b.sym(x, z, 6, 2.6, 2.6, 0, 3);
    b.sym(-26, -27.2, 0.1, 0.1, 0.1, 0, 3);
    // صناديق وبراميل ومنصات نقل
    b.sym(-9, -9, 2, 2, 1.2, 0, 1); b.sym(-9, 12, 1.4, 1.4, 1.1, 0, 3); b.sym(-34, 0.0, 1.5, 1.5, 1.2, 0, 1);
    b.sym(-6, 22, 3.5, 1.2, 1.2, 0, 1); b.sym(-12, 6, 2.2, 2.2, 1.6, 0, 2);
    // v2: منصات علوية جانبية بدرج + رافعات شوكية + إضاءة سقفية وخطوط أرضية
    b.sym(-30, 20, 8, 4, 0.4, 2.4, 2); b.sym(-30, 20 - 1.85, 8, 0.2, 0.9, 2.8, 1); b.stairsS(-34.4, 20, 6, 0.9, 2.2, 'x-'); b.sym(-33.8, 21.7, 0.6, 0.6, 2.4, 0, 0); b.sym(-26.2, 21.7, 0.6, 0.6, 2.4, 0, 0);
    b.sym(-7, 17, 3.4, 1.6, 1.4, 0, 3); b.sym(-24, 14, 3.4, 1.6, 1.4, 0, 3); b.sym(-3.5, -13, 1.6, 3.4, 1.3, 0, 3);
    for (const x of [-36, -24, -12, 12, 24, 36]) for (const z of [-24, -8, 8, 24]) b.deco(x, z, 3, 0.6, 0.12, 7.7, 9);
    for (const z of [-20, -10, 10, 20]) b.patch(0, z, W - 12, 0.3, 8);
    // سواتر قرب نقاط الظهور
    b.sym(-37, -12, 1, 5, 1.2, 0, 1); b.sym(-37, 12, 1, 5, 1.2, 0, 1); b.sym(-37, 0, 1, 3, 1.2, 0, 1);
    const sp = spawnSet(W, D, 4, 22), rng = rngf(101);
    scatter(b, rng, av(sp), { n: 22, x: [-36, -3], z: [-26, 26], pad: 1.6, types: [
      { w: [1.4, 2.2], d: [1.4, 2.2], h: [1.1, 1.7], k: 1, wt: 3 }, { w: [1.2, 1.3], d: [1.2, 1.3], h: [1.1, 1.1], k: 3, wt: 2 }, { w: [2.2, 3.4], d: [1.2, 1.4], h: [1.0, 1.2], k: 5, wt: 2 }] });
    return {
      W, D, boxes: b.boxes, ...sp, deco: b.decos, climbs: b.climbs, indoor: true,
      theme: { sky: [0.16, 0.17, 0.2], hor: [0.22, 0.23, 0.26], ground: [0.42, 0.42, 0.44], fog: [0.2, 0.21, 0.24], fogN: 30, fogF: 100,
        cols: [[0.55, 0.57, 0.6], [0.72, 0.52, 0.26], [0.3, 0.45, 0.62], [0.35, 0.52, 0.4], [0.3, 0.5, 0.24], [0.45, 0.32, 0.18], [0.4, 0.44, 0.5], [0.3, 0.3, 0.33], [0.9, 0.78, 0.2]], sun: [0.4, 0.8, 0.3], amb: 0.66, sunI: 0.5 },
    };
  },
  // 1 — أطلال الصحراء (بلدة قديمة بساحة وبرج وأسواق)
  () => {
    const W = 130, D = 96, b = builder(W, D);
    b.border(3.4);
    // ساحة مركزية مرصوفة وبرج بدرجين
    b.patch(0, 0, 30, 30, 6); b.patch(0, 0, W - 16, 6, 7); b.patch(0, 0, 6, D - 16, 7);
    b.add(0, 0, 8, 8, 3.2, 0, 2);
    b.sym(-2.6, -3.85, 2.8, 0.3, 0.9, 3.2, 1); b.sym(2.6, -3.85, 2.8, 0.3, 0.9, 3.2, 1); b.sym(3.85, 0, 0.3, 8, 0.9, 3.2, 1);
    b.stairs(0, -4, 8, 1.2, 2.4, 'z-'); b.stairs(0, 4, 8, 1.2, 2.4, 'z+');
    for (const [x, z] of [[-9, -9], [-9, 9]]) { b.sym(x, z, 1.4, 1.4, 3.4, 0, 0); b.sym(x + 5, z, 1.2, 1.2, 1.6, 0, 0); }
    // مبانٍ سليمة بأبواب ونوافذ وأسطح قابلة للصعود
    b.buildingS(-25, -22, 14, 10, 3.6, { doors: 'se', win: 2, para: true, open: 'e', at: { s: -3 } });
    b.stairsS(-17.8, -26.5, 9, 0.8, 2.4, 'x+');
    b.buildingS(-28, 20, 10, 12, 3.4, { doors: 'ne', win: 2, at: { e: 2 } });
    b.buildingS(-8, -34, 12, 9, 3.4, { doors: 'sw', win: 2 });
    b.buildingS(-12, 28, 9, 9, 3.2, { doors: 'n', win: 1 });
    b.buildingS(-46, -6, 8, 10, 3.2, { doors: 'ew', win: 1 });
    // أطلال: جدران مهدّمة متفاوتة الارتفاع
    const rr = rngf(77);
    const ruin = (cx, cz, w, d) => {
      const wallR = (x, z, len, horiz) => { let c = -len / 2; while (c < len / 2 - 1.2) { const sg = 1.5 + rr() * 3.2, gap = rr() < 0.45 ? 1.6 + rr() * 1.4 : 0, h = 1.0 + rr() * 2.0; const l = Math.min(sg, len / 2 - c); const mid = c + l / 2; horiz ? b.sym(x + mid, z, l, 0.5, h, 0, 0) : b.sym(x, z + mid, 0.5, l, h, 0, 0); c += l + gap; } };
      wallR(cx, cz - d / 2, w, true); wallR(cx, cz + d / 2, w, true); wallR(cx - w / 2, cz, d, false); wallR(cx + w / 2, cz, d, false);
    };
    ruin(-44, -30, 12, 9); ruin(-40, 32, 10, 10); ruin(-20, -8, 8, 8); ruin(-18, 8, 8, 6);
    // سوق: بسطات منخفضة بمظلات زخرفية
    for (let i = 0; i < 4; i++) { b.sym(-18 + i * 3.2, -16, 2.2, 1.2, 1.1, 0, 5); b.decoSym(-18 + i * 3.2, -16, 2.6, 1.8, 0.15, 2.3, i % 2 ? 2 : 1); }
    // v2: بئر مركزي، مظلات الساحة، لافتات ومصابيح
    b.sym(-14, 0, 1.2, 4.4, 1.1, 0, 2); b.sym(-17.5, 3.2, 3.6, 1, 1.1, 0, 1); b.sym(-24, 12, 1, 6, 1.4, 0, 1);
    b.decoSym(-12, 0, 3, 5, 0.12, 0, 8); b.decoSym(-12, -3, 2, 0.4, 3.0, 0, 5); b.decoSym(-12, 3, 2, 0.4, 3.0, 0, 5);
    for (const [x, z] of [[-6, -14], [-6, 14], [-22, -14], [-22, 14]]) b.decoSym(x, z, 0.3, 0.3, 3.6, 0, 5);
    // أسوار بأقواس وحدائق صغيرة
    b.sym(-34, 0, 1, 10, 2.6, 0, 0); b.sym(-34, 11, 1, 6, 1.2, 0, 0);
    // مجمعات الأفرقة: أكياس رمل
    b.sandbagsS(-44, -34, 8, 6, "e"); b.sandbagsS(-44, 36, 8, 6, "e"); b.sym(-50, 0, 1, 9, 1.2, 0, 1);
    const sp = spawnSet(W, D, 5, 34), rng = rngf(202);
    scatter(b, rng, av(sp), { n: 44, x: [-62, -3], z: [-45, 45], pad: 1.8, types: [
      { w: [1, 1], d: [1, 1], h: [2.2, 4], k: 0, wt: 2 }, { w: [3, 6], d: [0.7, 0.7], h: [1.2, 2.4], k: 0, wt: 2 }, { w: [2, 4], d: [2, 4], h: [0.8, 1.4], k: 2, wt: 3 },
      { w: [1.5, 2], d: [1.5, 2], h: [1.2, 1.6], k: 1, wt: 3 }, { p: 'palm', w: [1, 1], d: [1, 1], h: [3.6, 4.4], wt: 3 }, { p: 'dune', w: [8, 14], d: [5, 9], h: [1.0, 1.5], wt: 2 }, { w: [1.3, 1.3], d: [1.3, 1.3], h: [1.1, 1.1], k: 3, wt: 2 }] });
    skyline(b, rng, W, D, { n: 30, near: 6, far: 40, w: [10, 24], h: [4, 14], k: 2 });
    return {
      W, D, boxes: b.boxes, ...sp, deco: b.decos, climbs: b.climbs, indoor: false,
      theme: { sky: [0.38, 0.6, 0.88], hor: [0.93, 0.85, 0.7], ground: [0.82, 0.7, 0.48], fog: [0.9, 0.82, 0.68], fogN: 50, fogF: 190,
        cols: [[0.78, 0.66, 0.46], [0.62, 0.5, 0.32], [0.7, 0.58, 0.4], [0.5, 0.5, 0.45], [0.42, 0.52, 0.28], [0.4, 0.28, 0.16], [0.76, 0.68, 0.52], [0.68, 0.56, 0.38]], sun: [0.5, 0.8, 0.3], amb: 0.6, sunI: 0.62 },
    };
  },
  // 2 — شارع المدينة (جادة رئيسية، صفّا مبانٍ، أزقة، جسر مشاة)
  () => {
    const W = 150, D = 84, b = builder(W, D);
    b.border(5);
    // الطريق والأرصفة وعلامات المرور
    b.patch(0, 0, W, 16, 7); b.dashes(-W / 2 + 4, 0, W / 2 - 4, 0, 8); b.patch(0, -10, W, 4, 6); b.patch(0, 10, W, 4, 6);
    for (const x of [-60, -30, 30, 60]) { b.deco(x, 4, 0.3, 6, 0.08, 0, 8); b.deco(x, -4, 0.3, 6, 0.08, 0, 8); }
    // صف شمالي (الجنوبي يتولّد بالتماثل)
    b.buildingS(-48, -24, 12, 12, 3.8, { doors: 'se', win: 2, para: true, open: 'w', at: { s: 2 } });
    b.stairsS(-54.4, -30, 10, 0.8, 2.4, 'x-');
    b.buildingS(-30, -23, 16, 12, 3.6, { doors: 'sn', win: 3 });
    b.buildingS(-10, -23, 12, 12, 3.6, { doors: 's', win: 2, at: { s: -2 } });
    b.buildingS(-44, 24, 14, 12, 3.6, { doors: 'nw', win: 2 });
    b.buildingS(-24, 24, 12, 12, 3.4, { doors: 'ne', win: 2 });
    b.buildingS(-6, 25, 10, 10, 3.4, { doors: 'n', win: 1 });
    // جسر المشاة فوق الجادة
    b.add(0, 0, 4, 30, 0.4, 3.4, 2); b.add(1.85, 0, 0.2, 30, 0.9, 3.8, 1); b.add(-1.85, 0, 0.2, 30, 0.9, 3.8, 1);
    for (const z of [-9, 9]) b.add(0, z, 0.7, 0.7, 3.4, 0, 0);
    b.stairsS(0, -15, 9, 0.9, 2.4, 'z-');
    // حواجز وسيارات ومركبات متوقفة على الجادة
    b.sym(-20, -4, 4.4, 1.9, 1.4, 0, 3); b.sym(14, 3, 4.4, 1.9, 1.4, 0, 3); b.sym(-36, 3.5, 1.9, 4.4, 1.4, 0, 3); b.sym(26, -4, 4.4, 1.9, 1.4, 0, 3);
    b.sym(-58, -8, 1, 6, 1.2, 0, 1); b.sym(-58, 8, 1, 6, 1.2, 0, 1); b.sym(-5, 0, 3, 1, 1, 0, 1);
    // v2: حواجز شيكان على الجادة، مواقف باص، أعمدة إنارة على الأرصفة
    b.sym(-14, -6, 3, 0.8, 1.2, 0, 1); b.sym(-8, 6, 3, 0.8, 1.2, 0, 1); b.sym(-45, 5.5, 5, 0.9, 1.2, 0, 5);
    b.sym(-26, 14, 5, 0.3, 2.6, 0, 8); b.sym(-26, 14, 5.4, 1.2, 0.2, 2.6, 8);
    for (const x of [-66, -52, -38, -24, -10, 10, 24, 38, 52, 66]) { b.deco(x, 11.8, 0.3, 0.3, 4.4, 0, 7); b.deco(x, -11.8, 0.3, 0.3, 4.4, 0, 7); b.deco(x, 11.8, 1.6, 0.3, 0.2, 4.2, 9); }
    for (const x of [-52, 52]) b.patch(x, 0, 6, 16, 8, 0.06);
    const sp = spawnSet(W, D, 4, 28), rng = rngf(303);
    // الأفنية الخلفية: حاويات وحاويات قمامة وسيارات
    scatter(b, rng, av(sp), { n: 40, x: [-70, -2], z: [-39, 39], pad: 2.2, types: [
      { w: [5.5, 6.5], d: [2.3, 2.5], h: [2.5, 2.7], k: 3, wt: 2 }, { w: [1.5, 2.5], d: [1.5, 2.5], h: [1.1, 1.6], k: 1, wt: 3 }, { w: [4.4, 4.4], d: [1.9, 1.9], h: [1.4, 1.4], k: 3, wt: 2 },
      { p: 'lamp', w: [0.4, 0.4], d: [0.4, 0.4], h: [4.2, 4.6], wt: 2 }, { w: [1.3, 1.3], d: [1.3, 1.3], h: [1.1, 1.1], k: 2, wt: 2 }] });
    skyline(b, rng, W, D, { n: 34, near: 4, far: 30, w: [12, 26], h: [8, 28], k: 6, tiers: 3 });
    return {
      W, D, boxes: b.boxes, ...sp, deco: b.decos, climbs: b.climbs, indoor: false,
      theme: { sky: [0.45, 0.58, 0.78], hor: [0.78, 0.8, 0.85], ground: [0.52, 0.52, 0.5], fog: [0.7, 0.74, 0.8], fogN: 45, fogF: 170,
        cols: [[0.62, 0.6, 0.58], [0.7, 0.45, 0.3], [0.45, 0.5, 0.58], [0.38, 0.42, 0.5], [0.4, 0.52, 0.34], [0.4, 0.3, 0.2], [0.55, 0.58, 0.64], [0.2, 0.2, 0.22], [0.92, 0.86, 0.5]], sun: [0.4, 0.85, 0.3], amb: 0.62, sunI: 0.6 },
    };
  },
  // 3 — قاعدة القطب (حظيرة، ثكنات، رادار، خزانات وقود)
  () => {
    const W = 116, D = 92, b = builder(W, D);
    b.border(4);
    b.patch(0, 0, W - 10, 8, 7); b.patch(0, 0, 8, D - 10, 7); b.patch(0, 0, 22, 22, 6);
    // حظيرة كبيرة في الوسط (طائرات) ببابين واسعين
    b.building(0, 0, 26, 16, 6.5, { doors: 'ns', win: 0, roof: true });
    b.sym(-6, -3, 5, 3, 2.4, 0, 3); b.sym(5, 2, 2, 2, 1.2, 0, 1);
    // ثكنات
    b.buildingS(-26, -22, 14, 9, 3.4, { doors: 'se', win: 3, para: true, open: 'w', at: { s: 3 } });
    b.stairsS(-33.4, -26.6, 9, 0.8, 2.4, 'x-');
    b.buildingS(-26, -4, 12, 8, 3.2, { doors: 'ew', win: 2 });
    b.buildingS(-10, 24, 12, 9, 3.2, { doors: 'ne', win: 2 });
    b.buildingS(-30, 26, 10, 8, 3.2, { doors: 'n', win: 2 });
    // برج رادار بمنصة عالية ودرج
    b.add(-14, -30, 5, 5, 4.4, 0, 2); b.add(-14, -30, 1.4, 1.4, 3.0, 4.4, 0); b.decoSym(-14, -30, 6, 6, 0.4, 7.4, 8);
    b.sym(-14, -30, 0.1, 0.1, 0.1, 0, 2); b.stairsS(-14, -33, 11, 0.7, 2.4, 'z-');
    // خزانات وقود وأنابيب
    for (const [x, z] of [[-6, 14], [-6, 22]]) b.sym(x, z, 6, 6, 4, 0, 3);
    b.sym(-12, 14, 1, 14, 0.9, 0, 1); b.sym(0, 30, 14, 0.8, 0.9, 0, 1);
    // v2: برج مراقبة صغير ثانٍ + جدران جليدية + كشافات
    b.add(-34, 8, 3.6, 3.6, 3.2, 0, 2); b.add(-34, 8, 2.2, 2.2, 1.0, 3.2, 0); b.stairsS(-34, 12.6, 8, 0.7, 2.2, 'z+');
    b.sym(-20, -14, 6, 1, 1.5, 0, 7); b.sym(-18, 4, 1, 6, 1.5, 0, 7); b.sym(-4, -22, 5, 1, 1.4, 0, 7);
    for (const [x, z] of [[-16, -12], [-16, 12], [-40, -8], [-40, 8]]) b.decoSym(x, z, 0.3, 0.3, 5.2, 0, 6), b.decoSym(x, z, 1.6, 0.6, 0.4, 5.2, 9);
    // مجمعات الفرق
    b.sandbagsS(-44, -18, 8, 8, 'e'); b.sandbagsS(-44, 18, 8, 8, 'e');
    b.sym(-40, 0, 1, 8, 1.2, 0, 1);
    const sp = spawnSet(W, D, 4, 30), rng = rngf(404);
    scatter(b, rng, av(sp), { n: 36, x: [-54, -3], z: [-42, 42], pad: 2, types: [
      { p: 'pine', w: [1, 1], d: [1, 1], h: [4, 5], wt: 3 }, { w: [2.5, 4.5], d: [2.5, 4.5], h: [1.4, 2.6], k: 2, wt: 3 }, { p: 'dune', w: [8, 14], d: [5, 9], h: [1.0, 1.5], wt: 2 },
      { w: [1.5, 2.2], d: [1.5, 2.2], h: [1.1, 1.5], k: 1, wt: 3 }, { w: [5.5, 6.5], d: [2.3, 2.5], h: [2.5, 2.7], k: 3, wt: 1 }] });
    skyline(b, rng, W, D, { n: 30, near: 5, far: 40, w: [10, 24], h: [6, 20], k: 6, tiers: 3 });
    return {
      W, D, boxes: b.boxes, ...sp, deco: b.decos, climbs: b.climbs, indoor: false,
      theme: { sky: [0.55, 0.7, 0.88], hor: [0.88, 0.93, 0.98], ground: [0.9, 0.93, 0.97], fog: [0.85, 0.9, 0.96], fogN: 40, fogF: 150,
        cols: [[0.6, 0.66, 0.72], [0.8, 0.5, 0.2], [0.35, 0.5, 0.65], [0.4, 0.48, 0.45], [0.2, 0.42, 0.3], [0.4, 0.3, 0.2], [0.7, 0.78, 0.86], [0.7, 0.74, 0.8], [0.9, 0.3, 0.2]], sun: [0.4, 0.75, 0.3], amb: 0.68, sunI: 0.6 },
    };
  },
  // 4 — صحراء الشاص الكبرى (مركبات، واحة، حقل نفط، وادٍ)
  () => {
    const W = 200, D = 140, b = builder(W, D);
    b.border(2.6);
    const vehicles = [];
    const vsym = (x, z, h, t = 0) => { vehicles.push({ x, z, h, t }); vehicles.push({ x: -x, z: -z, h: h + Math.PI, t }); };
    vsym(-88, -14, -Math.PI / 2); vsym(-88, 14, -Math.PI / 2); vsym(-76, 40, -Math.PI / 2); vsym(-76, -40, -Math.PI / 2, 1);
    // طرق مرسومة: حلقة وطريق رئيسي
    b.patch(0, 0, W - 20, 8, 7); b.patchSym(-60, 25, 8, 60, 7); b.patch(0, 0, 40, 40, 6);
    // قاعدة: مرآب وأكياس رمل
    b.buildingS(-92, 0, 8, 10, 3.2, { doors: 'ew', win: 1 });
    b.sandbagsS(-80, -26, 8, 6, 'e'); b.sandbagsS(-80, 26, 8, 6, 'e'); b.sym(-70, -2, 1, 10, 1.2, 0, 1);
    // قريتان
    b.buildingS(-48, -36, 12, 9, 3.2, { doors: 'es', win: 2, para: true, open: 'w' });
    b.stairsS(-54.4, -39, 9, 0.8, 2.4, 'x-');
    b.buildingS(-34, -30, 8, 8, 3.2, { doors: 'ws', win: 1 });
    b.buildingS(-46, 38, 10, 10, 3.2, { doors: 'ne', win: 2 });
    b.buildingS(-32, 30, 9, 8, 3.2, { doors: 'n', win: 1 });
    // حقل نفط في الوسط: خزانات وأبراج
    for (const [x, z] of [[-14, -14], [-14, 14]]) { b.sym(x, z, 7, 7, 4.5, 0, 3); }
    b.add(0, 0, 4, 4, 1.3, 0, 1);
    for (const [x, z] of [[-24, 0], [-8, 26]]) for (const [dx, dz] of [[-1.6, -1.6], [1.6, -1.6], [-1.6, 1.6], [1.6, 1.6]]) b.sym(x + dx, z + dz, 0.5, 0.5, 8, 0, 6);
    // الواحة: بركة زخرفية ونخيل
    b.deco(0, -48, 22, 14, 0.06, 0, 8); b.deco(0, 48, 22, 14, 0.06, 0, 8);
    // الوادي: جدران صخرية طويلة مع ممرات واسعة للمركبات
    b.sym(-60, -56, 6, 24, 5, 0, 2); b.sym(-60, 56, 6, 24, 5, 0, 2); b.sym(-20, -62, 30, 5, 4, 0, 2); b.sym(-8, 62, 24, 5, 3.6, 0, 2);
    b.sym(-24, 52, 5, 18, 3, 0, 2);
    // v2 (زخرفة فقط حتى لا تُسدّ مسارات المركبات): علامات طرق، آثار إطارات، مشاعل الحقل
    for (let i = 0; i < 9; i++) b.decoSym(-90 + i * 20, 3.2, 3, 0.35, 0.05, 0, 9);
    for (const [x, z] of [[-24, 0], [-8, 26]]) b.decoSym(x, z, 0.5, 0.5, 1.2, 8, 9);
    b.decoSym(-50, 20, 14, 10, 0.05, 0, 6); b.decoSym(-12, -44, 10, 6, 0.05, 0, 6);
    // ثقوب تحصين ومواضع قناصة
    b.sym(-40, -2, 3, 3, 2.2, 0, 2); b.sym(-54, 8, 2.5, 2.5, 1.7, 0, 2); b.sym(-4, -22, 4, 3, 1.8, 0, 2);
    b.sym(-30, -12, 8, 1, 1.1, 0, 1); b.sym(-30, -15, 8, 1, 1.1, 0, 1); b.sym(-16, 8, 8, 1, 1.1, 0, 1); b.sym(8, -36, 10, 1, 1.1, 0, 1);
    b.sym(-30, 8, 1.4, 1.4, 1.1, 0, 3); b.sym(-60, 14, 1.4, 1.4, 1.1, 0, 3); b.sym(-40, -26, 1.4, 1.4, 1.1, 0, 3);
    const sp = spawnSet(W, D, 6, 52), rng = rngf(505);
    const lanes = []; for (const v of vehicles) for (let k = 0; k < 6; k++) lanes.push([v.x + Math.sign(-v.x) * k * 7, v.z, k ? 6 : 9]);
    const avoid = av(sp, lanes);
    scatter(b, rng, avoid, { n: 16, x: [-96, -4], z: [-66, 66], pad: 4, types: [{ p: 'dune', w: [10, 18], d: [7, 12], h: [1.5, 2.0] }] });
    scatter(b, rng, avoid, { n: 80, x: [-97, -3], z: [-67, 67], pad: 3.6, types: [
      { w: [3, 7], d: [3, 6], h: [1.8, 3.8], k: 2, wt: 3 }, { w: [4, 8], d: [0.8, 0.8], h: [1.0, 2.2], k: 0, wt: 2 }, { p: 'palm', w: [1, 1], d: [1, 1], h: [3.4, 4.6], wt: 4 },
      { w: [5.5, 6.5], d: [2.3, 2.5], h: [2.5, 2.7], k: 3, wt: 1 }, { w: [1.2, 1.4], d: [1.2, 1.4], h: [1.1, 1.1], k: 3, wt: 3 }, { w: [5, 7], d: [1, 1], h: [1.1, 1.1], k: 1, wt: 3 }, { w: [4.4, 4.4], d: [1.9, 1.9], h: [1.4, 1.4], k: 3, wt: 1 }] });
    skyline(b, rng, W, D, { n: 44, near: 5, far: 55, w: [14, 34], h: [5, 22], k: 2, tiers: 4 });
    skyline(b, rng, W, D, { n: 24, near: 55, far: 70, w: [20, 40], h: [14, 30], k: 6, tiers: 3 });
    return {
      W, D, boxes: b.boxes, deco: b.decos, climbs: b.climbs, vehicles, ...sp, indoor: false,
      theme: { sky: [0.3, 0.55, 0.9], hor: [0.95, 0.86, 0.68], ground: [0.84, 0.7, 0.46], fog: [0.92, 0.83, 0.66], fogN: 70, fogF: 260,
        cols: [[0.76, 0.64, 0.45], [0.6, 0.48, 0.3], [0.68, 0.55, 0.38], [0.45, 0.44, 0.4], [0.36, 0.5, 0.24], [0.4, 0.28, 0.16], [0.55, 0.46, 0.4], [0.62, 0.52, 0.36], [0.25, 0.5, 0.6]], sun: [0.45, 0.85, 0.25], amb: 0.62, sunI: 0.66 },
    };
  },
];

export function buildMap(idx) {
  const i = Math.max(0, Math.min(MAPS.length - 1, idx | 0));
  const m = MAPS[i]();
  m.id = i; m.name = MAP_NAMES[i]; if (!m.vehicles) m.vehicles = []; if (!m.climbs) m.climbs = [];
  return m;
}
export const MAP_COUNT = MAPS.length;
