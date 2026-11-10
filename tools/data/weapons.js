// تعريف الأسلحة الثمانية والملحقات — مشترك بين الخادم والعميل
export const OPTICS = {
  iron: { name: 'مصوّب حديدي', zoom: 0 },
  red:  { name: 'نقطة حمراء', zoom: 1.5 },
  x2:   { name: 'تكبير 2x', zoom: 2.2 },
  x4:   { name: 'تكبير 4x', zoom: 3.6 },
  x8:   { name: 'قناص 8x', zoom: 6 },
};
export const MUZZLES = {
  none: { name: 'بدون' },
  supp: { name: 'كاتم صوت' },
  comp: { name: 'مخمّد ارتداد' },
};
export const UNDERS = {
  none:  { name: 'بدون' },
  grip:  { name: 'مقبض عمودي' },
  laser: { name: 'ليزر' },
};
export const MAGS = {
  std:  { name: 'مخزن عادي' },
  ext:  { name: 'مخزن ممتد' },
  fast: { name: 'تعبئة سريعة' },
};

// range = [مدى الضرر الكامل, المدى الأقصى للضرر الأدنى] , minMul = نسبة الضرر عند أقصى مدى
export const WEAPONS = {
  carbine: { name: 'كاربين M4', type: 'primary', kind: 'rifle', dmg: 27, head: 2.0, rpm: 750, auto: true, pellets: 1,
    mag: 30, mags: 4, reload: 2.2, hip: 0.032, ads: 0.0035, recoilV: 0.0095, recoilH: 0.0040,
    range: [30, 75], minMul: 0.55, move: 1.0, zoom: 1.3, optics: ['iron', 'red', 'x2', 'x4'] },
  smg: { name: 'رشاش MP7', type: 'primary', kind: 'smg', dmg: 20, head: 1.8, rpm: 900, auto: true, pellets: 1,
    mag: 32, mags: 4, reload: 1.8, hip: 0.026, ads: 0.0060, recoilV: 0.0075, recoilH: 0.0050,
    range: [16, 42], minMul: 0.5, move: 1.08, zoom: 1.2, optics: ['iron', 'red', 'x2'] },
  shotgun: { name: 'شوتغن 12', type: 'primary', kind: 'shotgun', dmg: 12, head: 1.5, rpm: 70, auto: false, pellets: 9,
    mag: 6, mags: 5, reload: 2.8, hip: 0.060, ads: 0.038, recoilV: 0.045, recoilH: 0.010,
    range: [7, 22], minMul: 0.15, move: 0.97, zoom: 1.15, optics: ['iron', 'red'] },
  sniper: { name: 'قناصة AWM', type: 'primary', kind: 'sniper', dmg: 100, head: 2.0, rpm: 42, auto: false, pellets: 1,
    mag: 5, mags: 4, reload: 3.4, hip: 0.120, ads: 0.0004, recoilV: 0.030, recoilH: 0.004,
    range: [120, 220], minMul: 0.8, move: 0.88, zoom: 3.6, optics: ['x4', 'x8'] },
  lmg: { name: 'رشاش ثقيل M249', type: 'primary', kind: 'lmg', dmg: 23, head: 1.7, rpm: 640, auto: true, pellets: 1,
    mag: 100, mags: 2, reload: 5.0, hip: 0.045, ads: 0.0070, recoilV: 0.0085, recoilH: 0.0070,
    range: [32, 80], minMul: 0.55, move: 0.86, zoom: 1.25, optics: ['iron', 'red', 'x2', 'x4'] },
  dmr: { name: 'بندقية دقيقة SVD', type: 'primary', kind: 'dmr', dmg: 55, head: 2.0, rpm: 270, auto: false, pellets: 1,
    mag: 12, mags: 4, reload: 2.5, hip: 0.050, ads: 0.0010, recoilV: 0.020, recoilH: 0.005,
    range: [55, 120], minMul: 0.7, move: 0.95, zoom: 2.2, optics: ['red', 'x2', 'x4', 'x8'] },
  pistol: { name: 'مسدس G17', type: 'secondary', kind: 'pistol', dmg: 22, head: 2.0, rpm: 420, auto: false, pellets: 1,
    mag: 15, mags: 4, reload: 1.5, hip: 0.028, ads: 0.0070, recoilV: 0.0120, recoilH: 0.0030,
    range: [18, 45], minMul: 0.55, move: 1.1, zoom: 1.15, optics: ['iron', 'red'] },
  magnum: { name: 'ماغنوم .357', type: 'secondary', kind: 'magnum', dmg: 58, head: 2.0, rpm: 150, auto: false, pellets: 1,
    mag: 6, mags: 5, reload: 2.7, hip: 0.040, ads: 0.0040, recoilV: 0.0300, recoilH: 0.0050,
    range: [25, 65], minMul: 0.6, move: 1.05, zoom: 1.2, optics: ['iron', 'red'] },
  rpg: { name: 'قاذف RPG-7', type: 'primary', kind: 'launcher', explosive: true, dmg: 140, head: 1.0, rpm: 20, auto: false, pellets: 1,
    mag: 1, mags: 4, reload: 3.8, hip: 0.030, ads: 0.0030, recoilV: 0.050, recoilH: 0.010,
    range: [250, 300], minMul: 1.0, move: 0.84, zoom: 1.5, optics: ['iron'] },
};
export const PRIMARIES = Object.keys(WEAPONS).filter(k => WEAPONS[k].type === 'primary');
export const SECONDARIES = Object.keys(WEAPONS).filter(k => WEAPONS[k].type === 'secondary');

export function sanitizeAtt(id, att) {
  const w = WEAPONS[id];
  const a = att && typeof att === 'object' ? att : {};
  const optic = w.optics.includes(a.optic) ? a.optic : w.optics[0];
  return {
    optic,
    muzzle: MUZZLES[a.muzzle] ? a.muzzle : 'none',
    under: UNDERS[a.under] ? a.under : 'none',
    mag: MAGS[a.mag] ? a.mag : 'std',
  };
}

export function computeStats(id, att) {
  const w = WEAPONS[id];
  if (!w) return null;
  const a = sanitizeAtt(id, att);
  const s = {
    id, name: w.name, type: w.type, kind: w.kind, dmg: w.dmg, head: w.head, rpm: w.rpm, auto: w.auto,
    pellets: w.pellets, mag: w.mag, reload: w.reload, hip: w.hip, ads: w.ads,
    recoilV: w.recoilV, recoilH: w.recoilH, range: [w.range[0], w.range[1]], minMul: w.minMul,
    move: w.move, zoom: w.zoom, explosive: !!w.explosive, muted: false, comp: false, att: a,
  };
  const oz = OPTICS[a.optic].zoom;
  s.zoom = oz > 0 ? oz : w.zoom;
  if (a.muzzle === 'supp') {
    s.muted = true; s.dmg *= 0.95; s.recoilV *= 0.92; s.range[0] *= 0.9; s.range[1] *= 0.9;
  } else if (a.muzzle === 'comp') {
    s.comp = true; s.recoilH *= 0.55; s.recoilV *= 0.88;
  }
  if (a.under === 'grip') { s.recoilV *= 0.82; s.recoilH *= 0.85; s.hip *= 0.9; }
  else if (a.under === 'laser') { s.hip *= 0.6; }
  if (a.mag === 'ext') { s.mag = Math.round(w.mag * 1.5); s.reload *= 1.12; }
  else if (a.mag === 'fast') { s.reload *= 0.72; }
  s.reserve = s.mag * w.mags;
  s.interval = 60 / w.rpm;
  return s;
}

export function defaultLoadout() {
  return {
    p: 'carbine', s: 'pistol',
    a: [sanitizeAtt('carbine', {}), sanitizeAtt('pistol', {})],
  };
}

export function sanitizeLoadout(lo) {
  const d = defaultLoadout();
  if (!lo || typeof lo !== 'object') return d;
  const p = PRIMARIES.includes(lo.p) ? lo.p : d.p;
  const s = SECONDARIES.includes(lo.s) ? lo.s : d.s;
  const a = Array.isArray(lo.a) ? lo.a : [];
  return { p, s, a: [sanitizeAtt(p, a[0]), sanitizeAtt(s, a[1])] };
}

// الضرر بحسب المسافة (تناقص خطي بين المدى الكامل والمدى الأقصى)
export function damageAt(stats, dist) {
  const [r0, r1] = stats.range;
  if (dist <= r0) return stats.dmg;
  if (dist >= r1) return stats.dmg * stats.minMul;
  return stats.dmg * (1 - (1 - stats.minMul) * ((dist - r0) / (r1 - r0)));
}

// رشاش الشاص المثبّت (مقعد الصندوق الخلفي رقم 4): ذخيرة لا نهائية، معدل رمي عالٍ، دقة جيدة
export const MOUNT_SEAT = 4;
export const MOUNT_MG = (() => {
  const s = computeStats('lmg', {});
  s.name = 'رشاش الشاص'; s.dmg = 30; s.head = 1.5; s.rpm = 720; s.interval = 60 / 720; s.hip = 0.014; s.ads = 0.014; s.recoilV = 0.0016; s.recoilH = 0.0016;
  s.range = [48, 115]; s.minMul = 0.6; s.mag = 9999; s.reserve = 0; s.move = 1; s.zoom = 1; s.muted = false; s.mount = true;
  return s;
})();
