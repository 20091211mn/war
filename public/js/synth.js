// تركيب صوت الأسلحة بالمعالجة الرقمية: كل صوت يُولَّد مرة واحدة في مصفوفة عينات (بلا ملفات)
// ثم يُشغَّل كمصدر واحد لكل طلقة (أخف بكثير من بناء عشرات العقد الصوتية في كل طلقة).
// الوصفة (طبقات كما في الطلقة الحقيقية): فرقعة فوق صوتية حادة + انفجار الفوهة (ضجيج بمرشّح يهبط) + دويّ منخفض
// يهبط تردده + ذيل غرفة + أصداء متتابعة تزداد عتمة + ميكانيك السلاح (نقرة الترباس) + رنين الظرف الفارغ.
// نفس الوصفة منفَّذة في C++ لنسخة PS4 (native/src/sfx.h) كي تتطابق النسختان.

export const SHOT_PARAMS = {
  //          مدة   انفجار  قطع   دويّ  عمر    قوة   ذيل   قوة   فرقعة   أصداء [تأخير، قوة]
  pistol:  { dur: 0.75, bt: 0.028, hi: 7000, tf: 190, tt: 0.05, ta: 0.62, tail: 0.3,  tl: 0.18, ck: 1.0, ec: [[0.055, 0.3], [0.13, 0.18], [0.26, 0.09]], mech: 0.5 },
  magnum:  { dur: 1.5,  bt: 0.06,  hi: 6000, tf: 120, tt: 0.12, ta: 0.95, tail: 0.55, tl: 0.3,  ck: 1.2, ec: [[0.07, 0.35], [0.16, 0.25], [0.32, 0.15], [0.6, 0.08]], mech: 0.2 },
  rifle:   { dur: 1.1,  bt: 0.04,  hi: 7500, tf: 150, tt: 0.07, ta: 0.8,  tail: 0.45, tl: 0.24, ck: 1.1, ec: [[0.06, 0.3], [0.14, 0.22], [0.3, 0.12], [0.55, 0.06]], mech: 0.8 },
  smg:     { dur: 0.75, bt: 0.026, hi: 8000, tf: 170, tt: 0.05, ta: 0.6,  tail: 0.28, tl: 0.2,  ck: 1.0, ec: [[0.05, 0.3], [0.12, 0.18], [0.24, 0.08]], mech: 0.8 },
  lmg:     { dur: 1.3,  bt: 0.055, hi: 6500, tf: 120, tt: 0.1,  ta: 0.95, tail: 0.5,  tl: 0.26, ck: 1.1, ec: [[0.06, 0.32], [0.15, 0.24], [0.32, 0.14], [0.6, 0.07]], mech: 0.9 },
  dmr:     { dur: 1.5,  bt: 0.07,  hi: 6500, tf: 105, tt: 0.12, ta: 1.0,  tail: 0.65, tl: 0.3,  ck: 1.2, ec: [[0.07, 0.34], [0.16, 0.25], [0.34, 0.14], [0.65, 0.07]], mech: 0.6 },
  sniper:  { dur: 2.2,  bt: 0.1,   hi: 5500, tf: 80,  tt: 0.18, ta: 1.2,  tail: 0.9,  tl: 0.36, ck: 1.3, ec: [[0.09, 0.4], [0.2, 0.3], [0.4, 0.2], [0.8, 0.12], [1.3, 0.06]], mech: 0.0, bolt: 0.62 },
  shotgun: { dur: 1.5,  bt: 0.09,  hi: 4500, tf: 90,  tt: 0.14, ta: 1.1,  tail: 0.5,  tl: 0.26, ck: 0.9, ec: [[0.07, 0.34], [0.16, 0.24], [0.34, 0.13], [0.6, 0.07]], mech: 0.0, pump: 0.45 },
};

function rng(seed) { let s = (seed >>> 0) || 1; return () => { s ^= s << 13; s >>>= 0; s ^= s >>> 17; s ^= s << 5; s >>>= 0; return (s / 4294967296) * 2 - 1; }; }
const TAU = Math.PI * 2;

function ring(o, sr, t0, f, tau, amp, parts = [1, 2.76, 5.4], gains = [1, 0.5, 0.25]) { // رنين معدني (جزئيات غير توافقية)
  const n = Math.min(o.length - Math.floor(t0 * sr), Math.floor(tau * 7 * sr)); const s0 = Math.floor(t0 * sr);
  for (let k = 0; k < parts.length; k++) {
    const w = TAU * f * parts[k] / sr, g = amp * gains[k], d = Math.exp(-1 / sr / (tau / (1 + k * 0.6)));
    let e = g;
    for (let i = 0; i < n; i++) { o[s0 + i] += Math.sin(w * i) * e; e *= d; }
  }
}
function tick(o, sr, t0, tau, amp, rnd, hp = 0.5) { // نقرة ضجيج قصيرة
  const s0 = Math.floor(t0 * sr), n = Math.min(o.length - s0, Math.floor(tau * 8 * sr)); let p = 0, pw = 0, e = amp; const d = Math.exp(-1 / sr / tau);
  for (let i = 0; i < n; i++) { const w = rnd(); p = hp * (p + w - pw); pw = w; o[s0 + i] += p * e; e *= d; }
}
function thump(o, sr, t0, f0, tau, amp) { // دقة منخفضة بتردد يهبط
  const s0 = Math.floor(t0 * sr), n = Math.min(o.length - s0, Math.floor(tau * 6 * sr)); let ph = 0, fe = f0, e = amp, at = 1; const df = Math.exp(-1 / sr / (tau * 0.8)), de = Math.exp(-1 / sr / tau), da = Math.exp(-1 / sr / 0.002);
  for (let i = 0; i < n; i++) { ph += TAU * (fe + 40) / sr; o[s0 + i] += Math.sin(ph) * e * (1 - at); fe *= df; e *= de; at *= da; }
}
function grate(o, sr, t0, dur, amp, rnd, rate = 85, fc = 2400) { // احتكاك معدني (طقطقة سريعة متتابعة)
  const s0 = Math.floor(t0 * sr), n = Math.min(o.length - s0, Math.floor(dur * sr)); let lp = 0, p = 0;
  const k = Math.min(0.95, 2 * Math.PI * fc / sr);
  for (let i = 0; i < n; i++) { const t = i / sr, w = rnd(); lp += k * (w - lp); const am = 0.35 + 0.65 * Math.max(0, Math.sin(TAU * rate * t)); const env = Math.sin(Math.PI * i / n); o[s0 + i] += (w - lp) * 0.6 * am * env * amp + lp * 0.1 * env * amp; p = w; }
}
function finish(o, sr, drive = 1.4, peak = 0.9) { // قصّ ناعم + تطبيع + خفوت أخير
  let mx = 0; for (let i = 0; i < o.length; i++) { o[i] = Math.tanh(o[i] * drive); const a = Math.abs(o[i]); if (a > mx) mx = a; }
  const g = peak / (mx || 1), fo = Math.floor(0.03 * sr);
  for (let i = 0; i < o.length; i++) { o[i] *= g; if (i > o.length - fo) o[i] *= (o.length - i) / fo; }
  return o;
}

export function renderShot(kind, seed = 1, sr = 44100, supp = false) {
  const P = SHOT_PARAMS[kind] || SHOT_PARAMS.rifle, rnd = rng(seed * 7919 + kind.length * 104729), N = Math.floor(P.dur * sr);
  const out = new Float32Array(N), core = new Float32Array(N);
  const heavy = !(kind === 'pistol' || kind === 'smg' || kind === 'magnum');
  // --- النواة: فرقعة + انفجار + دويّ ---
  let lo = 0, bp = 0, hpP = 0, hpW = 0, ph = 0;
  const bt = supp ? P.bt * 1.25 : P.bt, hi = supp ? 1800 : P.hi;
  const dCk = Math.exp(-1 / sr / 0.0012), dFc = Math.exp(-1 / sr / (bt * 1.3)), dBl = Math.exp(-1 / sr / bt), dBa = Math.exp(-1 / sr / 0.0004), dTf = Math.exp(-1 / sr / 0.05), dTh = Math.exp(-1 / sr / P.tt), dTa = Math.exp(-1 / sr / 0.0015);
  let eCk = 1, eFc = 1, eBl = 1, eBa = 1, eTf = 1, eTh = 1, eTa = 1;
  const bAmp = supp ? 0.9 : 1.7, tAmp = P.ta * (supp ? 0.55 : 1);
  for (let i = 0; i < N; i++) {
    const w = rnd();
    hpP = 0.55 * (hpP + w - hpW); hpW = w;                                   // فرقعة حادة
    const crack = supp ? 0 : hpP * eCk * P.ck * 1.6;
    const fc = 450 + (hi - 450) * eFc, f = Math.min(1.2, TAU * fc / sr);       // انفجار: قطع يهبط
    lo += f * bp; const hp2 = w - lo - 0.8 * bp; bp += f * hp2;
    const blast = lo * eBl * bAmp * (1 - eBa);
    ph += TAU * (P.tf * eTf + 42) / sr;                                        // دويّ منخفض
    const th = Math.sin(ph) * eTh * tAmp * (1 - eTa);
    core[i] = crack + blast + th;
    eCk *= dCk; eFc *= dFc; eBl *= dBl; eBa *= dBa; eTf *= dTf; eTh *= dTh; eTa *= dTa;
  }
  // --- ذيل الغرفة ---
  let tl = 0; const tailTau = supp ? P.tail * 0.5 : P.tail, tailAmp = supp ? P.tl * 0.6 : P.tl;
  const dF = Math.exp(-1 / sr / 0.25), dT = Math.exp(-1 / sr / tailTau), dA = Math.exp(-1 / sr / 0.01); let eF = 1, eT = 1, eA = 1;
  for (let i = 0; i < N; i++) {
    const fc = 300 + 900 * eF; tl += Math.min(0.9, TAU * fc / sr) * (rnd() - tl);
    out[i] = core[i] + tl * eT * tailAmp * 2.2 * (1 - eA); eF *= dF; eT *= dT; eA *= dA;
  }
  // --- أصداء متتابعة (كل واحد أكثر عتمة وأخفت) ---
  for (let e = 0; e < P.ec.length; e++) {
    const d = Math.floor(P.ec[e][0] * sr * (0.95 + 0.1 * Math.abs(rnd()))), g = P.ec[e][1] * (supp ? 0.5 : 1), k = Math.max(0.06, 0.45 - e * 0.09); let lp = 0;
    for (let i = d; i < N; i++) { lp += k * (core[i - d] - lp); out[i] += lp * g * (e % 2 ? -1 : 1); }
  }
  // --- ميكانيك السلاح ---
  const mech = (supp ? Math.max(P.mech, 0.5) * 1.6 : P.mech);
  if (mech > 0) { tick(out, sr, 0.045, 0.004, 0.25 * mech, rnd, 0.7); ring(out, sr, 0.048, 2300, 0.02, 0.1 * mech); }
  if (P.bolt) { const b = P.bolt; grate(out, sr, b, 0.1, 0.22, rnd, 80, 2000); ring(out, sr, b + 0.1, 1300, 0.05, 0.16); tick(out, sr, b + 0.15, 0.004, 0.3, rnd); thump(out, sr, b + 0.15, 130, 0.04, 0.3); }
  if (P.pump) { const b = P.pump; grate(out, sr, b, 0.09, 0.22, rnd, 70, 1800); tick(out, sr, b + 0.16, 0.004, 0.35, rnd); thump(out, sr, b + 0.16, 110, 0.05, 0.4); ring(out, sr, b + 0.16, 1500, 0.04, 0.16); }
  // رنين الظرف الفارغ على الأرض (ارتدادان)
  const sh = (kind === 'pistol' || kind === 'magnum') ? [0.28, 0.4] : [0.34, 0.47, 0.55];
  if (!supp || true) for (let q = 0; q < (heavy ? 2 : 3); q++) ring(out, sr, sh[q] + 0.02 * rnd(), 4800 + 600 * rnd(), 0.018, (0.055 - q * 0.015) * (kind === 'shotgun' ? 1.4 : 1), [1, 1.9], [1, 0.4]);
  return finish(out, sr, supp ? 1.2 : 1.5, 0.9);
}

// ميكانيكا التعبئة. الأسماء: magout, magin, bolt, slam, shell, pump, slide
export const MECH_DUR = { magout: 0.35, magin: 0.4, bolt: 0.3, slam: 0.25, shell: 0.22, pump: 0.5, slide: 0.45 };
export function renderMech(name, seed = 1, sr = 44100) {
  const rnd = rng(seed * 6007 + name.length * 31337), N = Math.floor((MECH_DUR[name] || 0.3) * sr), o = new Float32Array(N);
  switch (name) {
    case 'magout': // زرّ التحرير + انزلاق المخزن + ارتجاج بلاستيكي
      tick(o, sr, 0.0, 0.003, 0.7, rnd, 0.6); ring(o, sr, 0.0, 1900, 0.025, 0.35); grate(o, sr, 0.015, 0.11, 0.3, rnd, 60, 1800);
      ring(o, sr, 0.14, 820, 0.02, 0.2, [1, 2.2], [1, 0.4]); tick(o, sr, 0.15, 0.005, 0.25, rnd, 0.3); break;
    case 'magin': // انزلاق ثم دقّة التثبيت + قفل
      grate(o, sr, 0.0, 0.1, 0.22, rnd, 55, 1600); tick(o, sr, 0.1, 0.005, 0.8, rnd, 0.4); thump(o, sr, 0.1, 170, 0.045, 0.7);
      ring(o, sr, 0.1, 2400, 0.022, 0.3); tick(o, sr, 0.145, 0.002, 0.5, rnd, 0.7); ring(o, sr, 0.145, 3100, 0.012, 0.25); break;
    case 'bolt': // سحب مقبض التلقيم: احتكاك ثم توقف
      grate(o, sr, 0.0, 0.14, 0.34, rnd, 90, 2200); ring(o, sr, 0.0, 1200, 0.06, 0.12); tick(o, sr, 0.15, 0.004, 0.35, rnd, 0.6); thump(o, sr, 0.15, 140, 0.04, 0.35); break;
    case 'slam': // انطلاق الترباس للأمام: ضربة معدنية
      tick(o, sr, 0.0, 0.004, 0.9, rnd, 0.5); ring(o, sr, 0.0, 2800, 0.04, 0.5); thump(o, sr, 0.0, 120, 0.06, 0.65); ring(o, sr, 0.0, 1400, 0.05, 0.25); break;
    case 'shell': // إدخال خرطوشة في السلاح
      tick(o, sr, 0.0, 0.003, 0.5, rnd, 0.5); ring(o, sr, 0.0, 3600, 0.03, 0.3); ring(o, sr, 0.0, 700, 0.025, 0.28, [1, 2.4], [1, 0.4]); tick(o, sr, 0.07, 0.002, 0.25, rnd, 0.6); break;
    case 'pump': // سحب الغطاء ثم دفعه
      grate(o, sr, 0.0, 0.1, 0.3, rnd, 70, 1800); ring(o, sr, 0.0, 1000, 0.06, 0.1); tick(o, sr, 0.17, 0.004, 0.95, rnd, 0.5); thump(o, sr, 0.17, 110, 0.06, 0.7); ring(o, sr, 0.17, 1600, 0.05, 0.3); break;
    case 'slide': // مسدس: سحب الشريحة وإفلاتها
      grate(o, sr, 0.0, 0.09, 0.3, rnd, 80, 2200); tick(o, sr, 0.14, 0.004, 0.85, rnd, 0.5); ring(o, sr, 0.14, 2500, 0.035, 0.4); thump(o, sr, 0.14, 150, 0.04, 0.5); break;
    default: break;
  }
  return finish(o, sr, 1.1, 0.85);
}
