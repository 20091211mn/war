// الخرائط الأربع — مبنية من صناديق (AABB) بتماثل دوراني 180° لضمان توازن الفريقين
// k: 0 جدار، 1 صندوق/ساتر، 2 مميّز/منصة، 3 حاوية/مركبة

export const MAP_NAMES = ['المستودع', 'أطلال الصحراء', 'شارع المدينة', 'قاعدة القطب'];

const SWAP = { n: 's', s: 'n', e: 'w', w: 'e' };

function builder(W, D) {
  const boxes = [];
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
    for (let i = 0; i < n; i++) {
      const top = 0.4 * (i + 1), off = (n - 1 - i) * tread + tread / 2;
      if (dir === 'x+') add(x + off, z, tread, width, top, 0, 2);
      else if (dir === 'x-') add(x - off, z, tread, width, top, 0, 2);
      else if (dir === 'z+') add(x, z + off, width, tread, top, 0, 2);
      else add(x, z - off, width, tread, top, 0, 2);
    }
  };
  const border = (h = 7) => {
    const t = 1;
    add(0, -D / 2 - t / 2, W + 2 * t, t, h, 0, 0);
    add(0, D / 2 + t / 2, W + 2 * t, t, h, 0, 0);
    add(-W / 2 - t / 2, 0, t, D, h, 0, 0);
    add(W / 2 + t / 2, 0, t, D, h, 0, 0);
  };
  return { boxes, add, sym, room, roomS, stairs, border };
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

const MAPS = [
  // 0 — المستودع (داخلي)
  () => {
    const W = 60, D = 40, b = builder(W, D);
    b.border(7);
    b.add(0, 0, W + 2, D + 2, 0.5, 7, 0); // سقف
    b.sym(-18, -9, 1, 6, 1.3, 0, 1); b.sym(-18, 9, 1, 6, 1.3, 0, 1);
    b.sym(-9, -8, 12, 2.8, 2.8, 0, 3); b.sym(-9, 8, 12, 2.8, 2.8, 0, 3);
    b.sym(-4, 0, 2, 2, 1.2, 0, 1); b.sym(-4, 0, 1.2, 1.2, 1.2, 1.2, 1);
    b.sym(0, -4, 1.5, 1.5, 1.2, 0, 1);
    b.sym(-23, 0, 1.5, 1.5, 1.2, 0, 1);
    // الطابق العلوي (مع درج وحاجز)
    b.sym(0, -16, 14, 6, 0.4, 2.0, 2);
    for (const px of [-6.6, 6.6]) for (const pz of [-13.4, -18.6]) b.sym(px, pz, 0.6, 0.6, 2.0, 0, 0);
    b.stairs(7, -16, 6, 1.2, 2.4, 'x+'); b.stairs(-7, 16, 6, 1.2, 2.4, 'x-');
    b.sym(0, -13.15, 14, 0.3, 0.9, 2.4, 1);
    b.sym(-12, -15, 0.8, 6, 2.2, 0, 0);
    b.sym(-24, -14, 2, 2, 2.4, 0, 3);
    return {
      W, D, boxes: b.boxes, ...spawnSet(W, D, 3, 14), indoor: true,
      theme: { sky: [0.16, 0.17, 0.2], hor: [0.22, 0.23, 0.26], ground: [0.38, 0.38, 0.4], fog: [0.2, 0.21, 0.24], fogN: 25, fogF: 85,
        cols: [[0.5, 0.52, 0.56], [0.7, 0.5, 0.24], [0.28, 0.42, 0.58], [0.35, 0.5, 0.36]], sun: [0.4, 0.8, 0.3], amb: 0.62, sunI: 0.5 },
    };
  },
  // 1 — أطلال الصحراء
  () => {
    const W = 84, D = 64, b = builder(W, D);
    b.border(5);
    b.add(0, 0, 8, 8, 3.2, 0, 2);
    b.sym(-2.6, -3.85, 2.8, 0.3, 0.9, 3.2, 1); b.sym(2.6, -3.85, 2.8, 0.3, 0.9, 3.2, 1);
    b.sym(3.85, 0, 0.3, 8, 0.9, 3.2, 1);
    b.stairs(0, -4, 8, 1.2, 2.4, 'z-'); b.stairs(0, 4, 8, 1.2, 2.4, 'z+');
    b.sym(-14, -12, 10, 1, 3, 0, 0); b.sym(-14, 12, 1, 9, 3, 0, 0);
    b.sym(-24, -6, 4, 1, 1.6, 0, 0); b.sym(-24, 6, 4, 1, 2.2, 0, 0);
    b.sym(-30, -12, 1, 5, 1.0, 0, 1); b.sym(-30, 12, 1, 5, 1.0, 0, 1);
    b.sym(-8, -20, 1.2, 1.2, 3.4, 0, 0); b.sym(-8, -16.5, 1.2, 1.2, 3.4, 0, 0); b.sym(-8, -18.25, 1.2, 4.6, 0.8, 3.4, 0);
    b.sym(-5, -9, 2.6, 2.2, 1.4, 0, 2); b.sym(-6, 18, 3, 3, 2, 0, 2); b.sym(18, -20, 3, 2.5, 1.7, 0, 2);
    b.sym(-10, 0, 1, 6, 1.1, 0, 1);
    b.sym(-22, -22, 3, 3, 2.2, 0, 2); b.sym(-22, 24, 4, 2, 1.5, 0, 2);
    return {
      W, D, boxes: b.boxes, ...spawnSet(W, D, 4, 24), indoor: false,
      theme: { sky: [0.38, 0.6, 0.88], hor: [0.93, 0.85, 0.7], ground: [0.82, 0.7, 0.48], fog: [0.9, 0.82, 0.68], fogN: 40, fogF: 150,
        cols: [[0.78, 0.66, 0.46], [0.62, 0.5, 0.32], [0.7, 0.58, 0.4], [0.5, 0.5, 0.45]], sun: [0.5, 0.8, 0.3], amb: 0.6, sunI: 0.62 },
    };
  },
  // 2 — شارع المدينة
  () => {
    const W = 96, D = 48, b = builder(W, D);
    b.border(8);
    b.roomS(-26, -14, 16, 10, 3.4, 'sw');
    b.roomS(-5, -15, 12, 8, 3.4, 'se');
    b.roomS(14, -14, 10, 10, 3.4, 'sw');
    b.sym(-14, 0.5, 4.4, 1.9, 1.4, 0, 3); b.sym(8, -1.5, 4.4, 1.9, 1.4, 0, 3); b.sym(-34, 2, 1.9, 4.4, 1.4, 0, 3);
    b.sym(-38, -2, 1, 5, 1.1, 0, 1); b.sym(-38, 8, 1, 5, 1.1, 0, 1);
    b.sym(-12, -21, 2, 2, 1.2, 0, 1); b.sym(30, -21, 3, 2, 1.5, 0, 3);
    b.sym(0, -2.5, 3, 1, 1.2, 0, 1);
    b.sym(22, 4, 2, 2, 1.2, 0, 1);
    return {
      W, D, boxes: b.boxes, ...spawnSet(W, D, 4, 17.5), indoor: false,
      theme: { sky: [0.3, 0.45, 0.7], hor: [0.7, 0.76, 0.82], ground: [0.33, 0.34, 0.36], fog: [0.66, 0.72, 0.78], fogN: 35, fogF: 130,
        cols: [[0.62, 0.55, 0.5], [0.55, 0.4, 0.28], [0.4, 0.45, 0.5], [0.7, 0.25, 0.22]], sun: [0.3, 0.85, 0.4], amb: 0.62, sunI: 0.55 },
    };
  },
  // 3 — قاعدة القطب
  () => {
    const W = 72, D = 64, b = builder(W, D);
    b.border(5);
    b.room(0, 0, 18, 12, 3.6, 'nsew');
    b.add(0, 0, 3, 3, 1.2, 0, 1); b.sym(-4, -2, 1, 3, 1.2, 0, 1);
    b.roomS(-16, -12, 7, 7, 3.0, 'es'); b.roomS(-16, 12, 7, 7, 3.0, 'en');
    b.sym(-8, -24, 10, 2.6, 2.6, 0, 3); b.sym(-10, -24, 6, 2.6, 2.6, 2.6, 3);
    b.sym(-24, 0, 1.2, 8, 1.1, 0, 1);
    b.sym(-22, -6, 3, 3, 2.4, 0, 2); b.sym(-22, 6, 3, 3, 2.4, 0, 2);
    b.sym(-26, -26, 8, 6, 2.0, 0, 2); b.stairs(-22, -26, 5, 1.2, 2.4, 'x+'); b.stairs(22, 26, 5, 1.2, 2.4, 'x-');
    b.sym(-28, -28.5, 6, 0.3, 0.9, 2.0, 1);
    return {
      W, D, boxes: b.boxes, ...spawnSet(W, D, 3, 21), indoor: false,
      theme: { sky: [0.45, 0.62, 0.85], hor: [0.88, 0.93, 0.98], ground: [0.9, 0.93, 0.96], fog: [0.85, 0.9, 0.96], fogN: 40, fogF: 140,
        cols: [[0.5, 0.56, 0.62], [0.35, 0.42, 0.5], [0.8, 0.45, 0.2], [0.3, 0.55, 0.45]], sun: [0.35, 0.8, 0.45], amb: 0.7, sunI: 0.55 },
    };
  },
];

export function buildMap(idx) {
  const i = Math.max(0, Math.min(MAPS.length - 1, idx | 0));
  const m = MAPS[i]();
  m.id = i; m.name = MAP_NAMES[i];
  return m;
}
export const MAP_COUNT = MAPS.length;
