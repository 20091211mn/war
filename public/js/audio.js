import { renderShot, renderMech, SHOT_PARAMS, MECH_DUR } from './synth.js';
// صوتيات مولّدة برمجيًا: طبقات (طقطقة + جسم + دويّ منخفض) + صدى غرفة + ترشيح بحسب المسافة
let ctx = null, master = null, dry = null, wet = null, noiseBuf = null, enabled = true;

function makeImpulse(c, seconds, decay) {
  const len = Math.floor(c.sampleRate * seconds), b = c.createBuffer(1, len, c.sampleRate);
  for (let ch = 0; ch < 1; ch++) { const d = b.getChannelData(ch); for (let i = 0; i < len; i++) d[i] = (Math.random() * 2 - 1) * Math.pow(1 - i / len, decay); }
  return b;
}

export const Snd = {
  init() {
    if (ctx) { if (ctx.state === 'suspended') ctx.resume().catch(() => {}); return; }
    try {
      const AC = window.AudioContext || window.webkitAudioContext;
      if (!AC) return;
      ctx = new AC();
      master = ctx.createGain(); master.gain.value = 0.8;
      const comp = ctx.createDynamicsCompressor(); comp.threshold.value = -14; comp.ratio.value = 6; comp.attack.value = 0.002; comp.release.value = 0.15;
      master.connect(comp); comp.connect(ctx.destination);
      dry = ctx.createGain(); dry.connect(master);
      wet = ctx.createGain(); wet.gain.value = 0.35;
      const conv = ctx.createConvolver(); conv.buffer = makeImpulse(ctx, 0.7, 3.2);
      wet.connect(conv); conv.connect(master);
      noiseBuf = ctx.createBuffer(1, ctx.sampleRate * 2, ctx.sampleRate);
      const d = noiseBuf.getChannelData(0); for (let i = 0; i < d.length; i++) d[i] = Math.random() * 2 - 1;
      if (ctx.state === 'suspended') ctx.resume().catch(() => {});
      this._warmStart();
    } catch { ctx = null; }
  },
  setVolume(v) { if (master) master.gain.value = v * 1.1; enabled = v > 0; },
  // عقدة خرج: pan، vol، dist (0..1 ليقلّ الحدّة)، rev (مقدار الصدى)
  // ميزانية أصوات: على الأجهزة الضعيفة كثرة العقد الصوتية المتزامنة (إطلاق متعدد + ارتدادات) تسبب لاق؛ نتجاهل الأصوات الضعيفة/البعيدة عند الازدحام
  _budget(dur, vol, max = 7) {
    const t = ctx.currentTime, a = this._ends || (this._ends = []);
    while (a.length && a[0] < t) a.shift();
    if (a.length >= max && vol < 0.8) return false;
    if (a.length >= max * 2) return false;
    a.push(t + dur); a.sort((x, y) => x - y); return true;
  },
  _out(pan = 0, vol = 1, dist = 0, rev = 0.25) {
    const g = ctx.createGain(); g.gain.value = vol;
    const lp = ctx.createBiquadFilter(); lp.type = 'lowpass'; lp.frequency.value = 18000 - dist * 15000; lp.Q.value = 0.4;
    g.connect(lp);
    let tail = lp;
    if (ctx.createStereoPanner) { const p = ctx.createStereoPanner(); p.pan.value = Math.max(-1, Math.min(1, pan)); lp.connect(p); tail = p; }
    tail.connect(dry);
    const s = ctx.createGain(); s.gain.value = rev * (1 + dist); tail.connect(s); s.connect(wet);
    return g;
  },
  _noise(out, t, dur, type, f0, f1, q, gain, attack = 0.001) {
    const s = ctx.createBufferSource(); s.buffer = noiseBuf; s.loop = true;
    const f = ctx.createBiquadFilter(); f.type = type; f.Q.value = q; f.frequency.setValueAtTime(f0, t);
    if (f1) f.frequency.exponentialRampToValueAtTime(Math.max(20, f1), t + dur);
    const g = ctx.createGain(); g.gain.setValueAtTime(0.0001, t); g.gain.linearRampToValueAtTime(gain, t + attack); g.gain.exponentialRampToValueAtTime(0.0001, t + dur);
    s.connect(f); f.connect(g); g.connect(out); s.start(t, Math.random()); s.stop(t + dur + 0.05);
  },
  _tone(out, t, dur, f0, f1, gain, type = 'sine', attack = 0.001) {
    const o = ctx.createOscillator(); o.type = type; o.frequency.setValueAtTime(f0, t); o.frequency.exponentialRampToValueAtTime(Math.max(15, f1), t + dur);
    const g = ctx.createGain(); g.gain.setValueAtTime(0.0001, t); g.gain.linearRampToValueAtTime(gain, t + attack); g.gain.exponentialRampToValueAtTime(0.0001, t + dur);
    o.connect(g); g.connect(out); o.start(t); o.stop(t + dur + 0.05);
  },
  // ---- مكتبة العينات المولَّدة: تُبنى على دفعات صغيرة وقت الخمول كي لا تسبب تقطيعًا ----
  _bufs: {},
  _queue: [],
  _warmStart() {
    if (this._warming) return; this._warming = true;
    const kinds = ['rifle', 'pistol', 'smg', 'shotgun', 'sniper', 'dmr', 'lmg', 'magnum'];
    for (const k of kinds) this._queue.push(['shot', k, 1, false]);
    for (const m of Object.keys(MECH_DUR)) this._queue.push(['mech', m, 1]);
    for (const k of kinds) this._queue.push(['shot', k, 2, false]);
    const tick = () => {
      const j = this._queue.shift(); if (!j) { this._warming = false; return; }
      this._make(j); setTimeout(tick, 90);
    };
    setTimeout(tick, 400);
  },
  _make(j) {
    if (!ctx) return;
    const key = j[0] === 'mech' ? 'm:' + j[1] : 's:' + j[1] + (j[3] ? 'S' : '');
    const arr = this._bufs[key] || (this._bufs[key] = []); if (arr.length >= j[2]) return;
    try {
      const data = j[0] === 'mech' ? renderMech(j[1], j[2], ctx.sampleRate) : renderShot(j[1], j[2], ctx.sampleRate, !!j[3]);
      const b = ctx.createBuffer(1, data.length, ctx.sampleRate); b.getChannelData(0).set(data); arr.push(b);
    } catch { /* */ }
  },
  _get(key, kind, supp) { // يعيد عينة جاهزة، أو يطلب بناءها (أولوية) ويعيد null
    const a = this._bufs[key];
    if (a && a.length) return a[(Math.random() * a.length) | 0];
    if (!this._pend) this._pend = {};
    if (!this._pend[key]) { this._pend[key] = 1; this._queue.unshift(kind === 'mech' ? ['mech', key.slice(2), 1] : ['shot', key.slice(2).replace('S', ''), 1, supp]); if (!this._warming) this._warmStart(); }
    return null;
  },
  _playBuf(buf, when, vol, pan, dist, rev, rate = 1) {
    const out = this._out(pan, vol, dist, rev), src = ctx.createBufferSource();
    src.buffer = buf; src.playbackRate.value = rate; src.connect(out); src.start(when);
  },
  shot(kind, vol = 1, pan = 0, muted = false, dist = 0) {
    if (!ctx || !enabled || vol <= 0.01) return;
    const t = ctx.currentTime + 0.001;
    // [مدة الذيل, تردد الجسم, جهارة, تردد الدويّ, ذيل صدى, طقطقة]
    const P = {
      pistol: [0.22, 1800, 0.8, 150, 0.25, 1], magnum: [0.45, 1100, 1.2, 90, 0.4, 1.2], rifle: [0.3, 1500, 1.0, 110, 0.3, 1], smg: [0.2, 2200, 0.8, 140, 0.22, 0.9],
      lmg: [0.35, 1100, 1.15, 85, 0.32, 1.1], dmr: [0.5, 1000, 1.25, 75, 0.4, 1.2], sniper: [0.9, 800, 1.5, 60, 0.6, 1.4], shotgun: [0.6, 700, 1.4, 70, 0.4, 1],
    }[kind] || [0.3, 1500, 1, 110, 0.3, 1];
    const [tail, body, g, low, rev, crack] = P;
    if (!this._budget(Math.min(tail, 0.35), vol)) return;
    { const key = 's:' + (SHOT_PARAMS[kind] ? kind : 'rifle') + (muted ? 'S' : ''), buf = this._get(key, 'shot', muted);
      if (buf) { this._playBuf(buf, t, vol * (muted ? 0.55 : 1) * 0.95, pan, dist, muted ? 0.08 : 0.12 + rev * 0.3, 0.97 + Math.random() * 0.06); return; } }
    const lite = !!(this._ends && this._ends.length > 4) || vol < 0.35;
    const out = this._out(pan, vol * (muted ? 0.4 : 1) * 0.9, dist, muted ? 0.1 : rev);
    const jitter = 0.92 + Math.random() * 0.16;
    if (muted) {
      this._noise(out, t, 0.09, 'bandpass', 1400 * jitter, 500, 0.8, g * 0.9, 0.002);
      this._tone(out, t, 0.12, low * 1.4, low * 0.5, g * 0.6);
      this._noise(out, t + 0.01, 0.05, 'highpass', 5000, 3000, 0.7, g * 0.25);
      return;
    }
    this._noise(out, t, 0.018, 'highpass', 4500 * jitter, 2500, 0.7, g * crack * 1.1);       // طقطقة حادة
    this._noise(out, t, tail, 'bandpass', body * jitter, body * 0.25, 0.6, g * 1.2);          // جسم الطلقة
    if (!lite) this._noise(out, t + 0.004, tail * 1.4, 'lowpass', 3500, 400, 0.5, g * 0.7);   // ذيل (يُحذف عند الازدحام)
    this._tone(out, t, 0.2 + tail * 0.3, low * 2.4 * jitter, low * 0.45, g * 1.3);             // دويّ منخفض
    if (lite) return;
    if (kind === 'shotgun') { this._noise(out, t + 0.012, 0.25, 'lowpass', 1800, 200, 0.6, g * 0.8); }
    if (kind === 'sniper' || kind === 'dmr') this._noise(out, t + 0.05, 0.5, 'bandpass', 600, 150, 0.5, g * 0.35);
  },
  step(vol = 0.3, pan = 0, dist = 0) {
    if (!ctx || !enabled || vol <= 0.01 || !this._budget(0.1, vol, 5)) return;
    const t = ctx.currentTime, out = this._out(pan, vol, dist, 0.08);
    this._tone(out, t, 0.08, 120 + Math.random() * 30, 55, 0.9);
    this._noise(out, t, 0.06, 'bandpass', 900 + Math.random() * 500, 400, 1.3, 0.7, 0.002);
  },
  // تعبئة بصوت ميكانيكي واقعي متزامن مع حركة التعبئة (نقاط التوقيت نفسها في RELOAD_SYNC بـ models.js)
  reload(vol = 0.5, pan = 0, dur = 2, kind = 'rifle') {
    if (!ctx || !enabled) return;
    if (!this._bufs['m:magout'] || !this._bufs['m:magout'].length) { this._get('m:magout', 'mech'); return this._reloadOld(vol, pan); }
    const t = ctx.currentTime, E = (name, at, v = 1) => { const b = this._get('m:' + name, 'mech'); if (b) this._playBuf(b, t + Math.max(0.01, at * dur), vol * v, pan, 0, 0.08, 0.97 + Math.random() * 0.06); };
    if (kind === 'shotgun') { for (let k = 0; k < 4; k++) E('shell', 0.14 + 0.16 * k + 0.1); E('pump', 0.8); }
    else if (kind === 'pistol' || kind === 'magnum') { E('magout', 0.17); E('magin', 0.5); E('slide', 0.7); }
    else { E('magout', 0.17); E('magin', 0.5); E('bolt', 0.69); E('slam', 0.83); }
  },
  _reloadOld(vol = 0.5, pan = 0) {
    if (!ctx || !enabled) return;
    const t = ctx.currentTime, out = this._out(pan, vol, 0, 0.1);
    const click = (dt, f, g) => { this._noise(out, t + dt, 0.04, 'bandpass', f, f * 0.6, 3, g, 0.001); this._tone(out, t + dt, 0.05, f * 0.5, f * 0.3, g * 0.5, 'square'); };
    click(0.15, 2200, 0.6);   // إخراج المخزن
    click(0.9, 1500, 0.7);    // إدخال المخزن
    click(1.15, 2800, 0.9);   // سحب الترباس
    click(1.25, 1800, 0.8);
  },
  empty() { if (!ctx || !enabled) return; const t = ctx.currentTime, o = this._out(0, 0.5, 0, 0.05); this._noise(o, t, 0.03, 'bandpass', 3000, 2000, 4, 0.8); this._tone(o, t, 0.04, 800, 400, 0.3, 'square'); },
  hit(head) {
    if (!ctx || !enabled) return; const t = ctx.currentTime, o = this._out(0, 0.55, 0, 0.05);
    this._tone(o, t, 0.07, head ? 2400 : 1500, head ? 1800 : 1100, 0.7, 'triangle'); this._noise(o, t, 0.03, 'highpass', 3000, 2000, 1, 0.3);
    if (head) this._tone(o, t + 0.03, 0.08, 3200, 2600, 0.4, 'triangle');
  },
  hurt() {
    if (!ctx || !enabled) return; const t = ctx.currentTime, o = this._out(0, 0.8, 0, 0.1);
    this._tone(o, t, 0.25, 180, 60, 0.9, 'sawtooth'); this._noise(o, t, 0.15, 'lowpass', 900, 200, 0.8, 0.6); this._tone(o, t, 0.12, 90, 40, 1.0);
  },
  kill() {
    if (!ctx || !enabled) return; const t = ctx.currentTime, o = this._out(0, 0.6, 0, 0.15);
    this._tone(o, t, 0.12, 880, 880, 0.55, 'triangle'); this._tone(o, t + 0.09, 0.2, 1320, 1320, 0.55, 'triangle');
  },
  crack(vol = 0.8, pan = 0) { // طلقة تمرّ بجانب الرأس
    if (!ctx || !enabled || !this._budget(0.06, vol, 6)) return; const t = ctx.currentTime, o = this._out(pan, vol, 0, 0.05);
    this._noise(o, t, 0.045, 'highpass', 4500, 2500, 0.8, 1.0, 0.001); this._tone(o, t, 0.06, 2600, 700, 0.35, 'triangle');
  },
  ring(vol = 0.6, sec = 2.5) { // طنين الأذن بعد انفجار قريب + كتم مؤقت للصوت
    if (!ctx || !enabled) return; const t = ctx.currentTime, o = this._out(0, vol * 0.18, 0, 0.0);
    this._tone(o, t, sec, 5200, 5000, 1.0, 'sine');
    if (master) { const g = master.gain, v0 = g.value; try { g.cancelScheduledValues(t); g.setValueAtTime(v0 * 0.25, t); g.linearRampToValueAtTime(v0, t + sec); } catch { /* */ } }
  },
  heart() { // نبض القلب عند الصحة المنخفضة
    if (!ctx || !enabled) return; const t = ctx.currentTime, o = this._out(0, 0.7, 0, 0.05);
    this._tone(o, t, 0.12, 62, 40, 1.0); this._tone(o, t + 0.2, 0.1, 55, 38, 0.7);
  },
  boom(vol = 1, pan = 0, dist = 0) {
    if (!ctx || !enabled || vol <= 0.02) return;
    const t = ctx.currentTime, o = this._out(pan, vol * 1.2, dist, 0.6);
    this._noise(o, t, 0.05, 'highpass', 3000, 1500, 0.7, 1.2);
    this._noise(o, t, 1.4, 'lowpass', 1400, 90, 0.5, 1.6, 0.003);
    this._tone(o, t, 1.0, 100, 24, 1.8); this._noise(o, t + 0.15, 0.9, 'bandpass', 300, 80, 0.6, 0.8, 0.08);
  },
  throwG() { if (!ctx || !enabled) return; const t = ctx.currentTime, o = this._out(0, 0.4, 0, 0.05); this._noise(o, t, 0.3, 'bandpass', 500, 1800, 0.9, 0.6, 0.05); this._noise(o, t + 0.02, 0.03, 'bandpass', 2500, 2000, 4, 0.5); },
  // محرك المركبة: صوت مستمر واحد (أقرب مركبة). rpm 0..1
  engine(rpm, vol, pan = 0, heavy = false) {
    if (!ctx || !enabled) return;
    if (!this._eng) {
      const g = ctx.createGain(); g.gain.value = 0;
      const lp = ctx.createBiquadFilter(); lp.type = 'lowpass'; lp.frequency.value = 600; lp.Q.value = 2;
      const o1 = ctx.createOscillator(); o1.type = 'sawtooth'; const o2 = ctx.createOscillator(); o2.type = 'square';
      const lfo = ctx.createOscillator(); lfo.frequency.value = 9; const lg = ctx.createGain(); lg.gain.value = 6; lfo.connect(lg); lg.connect(o1.frequency);
      const n = ctx.createBufferSource(); n.buffer = noiseBuf; n.loop = true; const ng = ctx.createGain(); ng.gain.value = 0.05; const nf = ctx.createBiquadFilter(); nf.type = 'lowpass'; nf.frequency.value = 500; n.connect(nf); nf.connect(ng); ng.connect(lp);
      const g1 = ctx.createGain(); g1.gain.value = 0.5; const g2 = ctx.createGain(); g2.gain.value = 0.22;
      o1.connect(g1); o2.connect(g2); g1.connect(lp); g2.connect(lp); lp.connect(g);
      let tail = g; if (ctx.createStereoPanner) { const p = ctx.createStereoPanner(); g.connect(p); tail = p; this._engPan = p; }
      tail.connect(dry);
      o1.start(); o2.start(); lfo.start(); n.start();
      this._eng = { g, lp, o1, o2, ng, lfo };
    }
    const e = this._eng, t = ctx.currentTime, f = heavy ? 26 + rpm * 38 : 38 + rpm * 70;
    e.o1.frequency.setTargetAtTime(f, t, 0.08); e.o2.frequency.setTargetAtTime(f * 0.5, t, 0.08);
    e.lfo.frequency.setTargetAtTime(7 + rpm * 20, t, 0.1);
    e.lp.frequency.setTargetAtTime(heavy ? 260 + rpm * 700 : 380 + rpm * 1300, t, 0.1);
    e.ng.gain.setTargetAtTime(heavy ? 0.07 + rpm * 0.14 : 0.03 + rpm * 0.08, t, 0.1); // جنازير: ضجيج أعلى
    e.g.gain.setTargetAtTime(vol * (0.28 + rpm * 0.3), t, 0.1);
    if (this._engPan) this._engPan.pan.setTargetAtTime(Math.max(-1, Math.min(1, pan)), t, 0.1);
  },
  // مدفع الدبابة: فرقعة حادة + دويّ منخفض طويل + ارتداد
  cannon(vol = 1, pan = 0, dist = 0) {
    if (!ctx || !enabled || vol <= 0.02) return; const t = ctx.currentTime, o = this._out(pan, vol * 1.3, dist, 0.7);
    this._noise(o, t, 0.06, 'highpass', 2500, 1200, 0.9, 1.4);
    this._noise(o, t, 0.9, 'lowpass', 900, 70, 0.6, 1.5, 0.002);
    this._tone(o, t, 0.8, 80, 28, 1.9); this._tone(o, t + 0.05, 0.5, 52, 22, 1.2);
    this._noise(o, t + 0.12, 1.2, 'bandpass', 260, 60, 0.5, 0.7, 0.1);
  },
  drone(vol, pan = 0) {
    if (!ctx || !enabled) return;
    if (!this._dr) {
      const g = ctx.createGain(); g.gain.value = 0; const o = ctx.createOscillator(); o.type = 'sawtooth'; o.frequency.value = 170;
      const o2 = ctx.createOscillator(); o2.type = 'triangle'; o2.frequency.value = 343; const g2 = ctx.createGain(); g2.gain.value = 0.5;
      const lp = ctx.createBiquadFilter(); lp.type = 'lowpass'; lp.frequency.value = 1400;
      const lfo = ctx.createOscillator(); lfo.frequency.value = 23; const lg = ctx.createGain(); lg.gain.value = 9; lfo.connect(lg); lg.connect(o.frequency);
      o.connect(lp); o2.connect(g2); g2.connect(lp); lp.connect(g); let tail = g;
      if (ctx.createStereoPanner) { const p = ctx.createStereoPanner(); g.connect(p); tail = p; this._drPan = p; }
      tail.connect(dry); o.start(); o2.start(); lfo.start(); this._dr = { g };
    }
    this._dr.g.gain.setTargetAtTime(Math.max(0, Math.min(1, vol)) * 0.09, ctx.currentTime, 0.1);
    if (this._drPan) this._drPan.pan.value = pan;
  },
  engineOff() { if (this._eng && ctx) this._eng.g.gain.setTargetAtTime(0, ctx.currentTime, 0.08); },
  tire(vol = 0.8, pan = 0, dist = 0) {
    if (!ctx || !enabled || vol <= 0.02) return; const t = ctx.currentTime, o = this._out(pan, vol, dist, 0.15);
    this._noise(o, t, 0.5, 'bandpass', 1800, 500, 0.7, 1.0, 0.002); this._tone(o, t, 0.12, 200, 70, 0.9);
  },
  crash(vol = 0.8, pan = 0) {
    if (!ctx || !enabled || vol <= 0.02) return; const t = ctx.currentTime, o = this._out(pan, vol, 0, 0.2);
    this._noise(o, t, 0.35, 'lowpass', 1800, 150, 0.6, 1.2, 0.002); this._tone(o, t, 0.3, 110, 40, 1.2); this._noise(o, t, 0.12, 'bandpass', 3000, 1500, 2, 0.6);
  },
  skid(vol = 0.3, pan = 0) {
    if (!ctx || !enabled || vol <= 0.02) return; const t = ctx.currentTime, o = this._out(pan, vol, 0, 0.05);
    this._noise(o, t, 0.25, 'bandpass', 2200, 1800, 3, 0.5, 0.03);
  },
  door() { if (!ctx || !enabled) return; const t = ctx.currentTime, o = this._out(0, 0.5, 0, 0.1); this._tone(o, t, 0.08, 220, 90, 0.9); this._noise(o, t, 0.06, 'bandpass', 1200, 600, 2, 0.5); },
  pickup() { if (!ctx || !enabled) return; const t = ctx.currentTime, o = this._out(0, 0.5, 0, 0.1); this._noise(o, t, 0.05, 'bandpass', 2500, 1500, 3, 0.7); this._tone(o, t + 0.05, 0.1, 700, 1100, 0.4, 'triangle'); },
  msg() { if (!ctx || !enabled) return; const t = ctx.currentTime, o = this._out(0, 0.3, 0, 0); this._tone(o, t, 0.08, 1200, 1500, 0.4, 'sine'); },
  click() { if (!ctx || !enabled) return; const t = ctx.currentTime, o = this._out(0, 0.3, 0, 0); this._tone(o, t, 0.03, 900, 500, 0.5, 'square'); },
  spawn() { if (!ctx || !enabled) return; const t = ctx.currentTime, o = this._out(0, 0.4, 0, 0.2); this._tone(o, t, 0.18, 330, 660, 0.5, 'triangle'); this._tone(o, t + 0.1, 0.2, 495, 990, 0.4, 'triangle'); },
};
