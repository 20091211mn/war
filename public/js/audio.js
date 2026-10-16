// صوتيات مولّدة برمجيًا: طبقات (طقطقة + جسم + دويّ منخفض) + صدى غرفة + ترشيح بحسب المسافة
let ctx = null, master = null, dry = null, wet = null, noiseBuf = null, enabled = true;

function makeImpulse(c, seconds, decay) {
  const len = Math.floor(c.sampleRate * seconds), b = c.createBuffer(2, len, c.sampleRate);
  for (let ch = 0; ch < 2; ch++) { const d = b.getChannelData(ch); for (let i = 0; i < len; i++) d[i] = (Math.random() * 2 - 1) * Math.pow(1 - i / len, decay); }
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
      const conv = ctx.createConvolver(); conv.buffer = makeImpulse(ctx, 1.4, 3.2);
      wet.connect(conv); conv.connect(master);
      noiseBuf = ctx.createBuffer(1, ctx.sampleRate * 2, ctx.sampleRate);
      const d = noiseBuf.getChannelData(0); for (let i = 0; i < d.length; i++) d[i] = Math.random() * 2 - 1;
      if (ctx.state === 'suspended') ctx.resume().catch(() => {});
    } catch { ctx = null; }
  },
  setVolume(v) { if (master) master.gain.value = v * 1.1; enabled = v > 0; },
  // عقدة خرج: pan، vol، dist (0..1 ليقلّ الحدّة)، rev (مقدار الصدى)
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
  shot(kind, vol = 1, pan = 0, muted = false, dist = 0) {
    if (!ctx || !enabled || vol <= 0.01) return;
    const t = ctx.currentTime + 0.001;
    // [مدة الذيل, تردد الجسم, جهارة, تردد الدويّ, ذيل صدى, طقطقة]
    const P = {
      pistol: [0.22, 1800, 0.8, 150, 0.25, 1], magnum: [0.45, 1100, 1.2, 90, 0.4, 1.2], rifle: [0.3, 1500, 1.0, 110, 0.3, 1], smg: [0.2, 2200, 0.8, 140, 0.22, 0.9],
      lmg: [0.35, 1100, 1.15, 85, 0.32, 1.1], dmr: [0.5, 1000, 1.25, 75, 0.4, 1.2], sniper: [0.9, 800, 1.5, 60, 0.6, 1.4], shotgun: [0.6, 700, 1.4, 70, 0.4, 1],
    }[kind] || [0.3, 1500, 1, 110, 0.3, 1];
    const [tail, body, g, low, rev, crack] = P;
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
    this._noise(out, t + 0.004, tail * 1.4, 'lowpass', 3500, 400, 0.5, g * 0.7);              // ذيل
    this._tone(out, t, 0.2 + tail * 0.3, low * 2.4 * jitter, low * 0.45, g * 1.3);             // دويّ منخفض
    if (kind === 'shotgun') { this._noise(out, t + 0.012, 0.25, 'lowpass', 1800, 200, 0.6, g * 0.8); }
    if (kind === 'sniper' || kind === 'dmr') this._noise(out, t + 0.05, 0.5, 'bandpass', 600, 150, 0.5, g * 0.35);
  },
  step(vol = 0.3, pan = 0, dist = 0) {
    if (!ctx || !enabled || vol <= 0.01) return;
    const t = ctx.currentTime, out = this._out(pan, vol, dist, 0.08);
    this._tone(out, t, 0.08, 120 + Math.random() * 30, 55, 0.9);
    this._noise(out, t, 0.06, 'bandpass', 900 + Math.random() * 500, 400, 1.3, 0.7, 0.002);
  },
  reload(vol = 0.5, pan = 0) {
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
  boom(vol = 1, pan = 0, dist = 0) {
    if (!ctx || !enabled || vol <= 0.02) return;
    const t = ctx.currentTime, o = this._out(pan, vol * 1.2, dist, 0.6);
    this._noise(o, t, 0.05, 'highpass', 3000, 1500, 0.7, 1.2);
    this._noise(o, t, 1.4, 'lowpass', 1400, 90, 0.5, 1.6, 0.003);
    this._tone(o, t, 1.0, 100, 24, 1.8); this._noise(o, t + 0.15, 0.9, 'bandpass', 300, 80, 0.6, 0.8, 0.08);
  },
  throwG() { if (!ctx || !enabled) return; const t = ctx.currentTime, o = this._out(0, 0.4, 0, 0.05); this._noise(o, t, 0.3, 'bandpass', 500, 1800, 0.9, 0.6, 0.05); this._noise(o, t + 0.02, 0.03, 'bandpass', 2500, 2000, 4, 0.5); },
  click() { if (!ctx || !enabled) return; const t = ctx.currentTime, o = this._out(0, 0.3, 0, 0); this._tone(o, t, 0.03, 900, 500, 0.5, 'square'); },
  spawn() { if (!ctx || !enabled) return; const t = ctx.currentTime, o = this._out(0, 0.4, 0, 0.2); this._tone(o, t, 0.18, 330, 660, 0.5, 'triangle'); this._tone(o, t + 0.1, 0.2, 495, 990, 0.4, 'triangle'); },
};
