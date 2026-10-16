// أصوات مولّدة برمجيًا (بدون ملفات صوت) — WebAudio
let ctx = null, master = null, noiseBuf = null, enabled = true;

export const Snd = {
  init() {
    if (ctx) { if (ctx.state === 'suspended') ctx.resume().catch(() => {}); return; }
    try {
      const AC = window.AudioContext || window.webkitAudioContext;
      if (!AC) return;
      ctx = new AC();
      master = ctx.createGain(); master.gain.value = 0.7;
      const comp = ctx.createDynamicsCompressor(); master.connect(comp); comp.connect(ctx.destination);
      noiseBuf = ctx.createBuffer(1, ctx.sampleRate, ctx.sampleRate);
      const d = noiseBuf.getChannelData(0); for (let i = 0; i < d.length; i++) d[i] = Math.random() * 2 - 1;
      if (ctx.state === 'suspended') ctx.resume().catch(() => {});
    } catch { ctx = null; }
  },
  setVolume(v) { if (master) master.gain.value = v; enabled = v > 0; },
  _out(pan, vol) {
    const g = ctx.createGain(); g.gain.value = vol;
    let node = g;
    if (ctx.createStereoPanner) { const p = ctx.createStereoPanner(); p.pan.value = Math.max(-1, Math.min(1, pan)); g.connect(p); p.connect(master); }
    else g.connect(master);
    return node;
  },
  _noise(out, t, dur, freq, q, type, gain, sweepTo) {
    const s = ctx.createBufferSource(); s.buffer = noiseBuf; s.loop = true;
    const f = ctx.createBiquadFilter(); f.type = type; f.frequency.setValueAtTime(freq, t); if (sweepTo) f.frequency.exponentialRampToValueAtTime(sweepTo, t + dur); f.Q.value = q;
    const g = ctx.createGain(); g.gain.setValueAtTime(gain, t); g.gain.exponentialRampToValueAtTime(0.001, t + dur);
    s.connect(f); f.connect(g); g.connect(out); s.start(t, Math.random() * 0.5); s.stop(t + dur + 0.02);
  },
  _tone(out, t, dur, f0, f1, gain, type = 'sine') {
    const o = ctx.createOscillator(); o.type = type; o.frequency.setValueAtTime(f0, t); o.frequency.exponentialRampToValueAtTime(Math.max(20, f1), t + dur);
    const g = ctx.createGain(); g.gain.setValueAtTime(gain, t); g.gain.exponentialRampToValueAtTime(0.001, t + dur);
    o.connect(g); g.connect(out); o.start(t); o.stop(t + dur + 0.02);
  },
  shot(kind, vol = 1, pan = 0, muted = false) {
    if (!ctx || !enabled || vol <= 0.01) return;
    const t = ctx.currentTime, out = this._out(pan, vol * (muted ? 0.35 : 1));
    const P = {
      pistol: [0.14, 2600, 0.5, 120], magnum: [0.28, 1800, 1.0, 70], rifle: [0.18, 2200, 0.8, 90], smg: [0.13, 3000, 0.6, 110],
      lmg: [0.22, 1500, 0.9, 70], dmr: [0.3, 1600, 1.0, 60], sniper: [0.6, 1200, 1.2, 45], shotgun: [0.4, 1100, 1.2, 60],
    }[kind] || [0.18, 2200, 0.8, 90];
    const [dur, fr, g, low] = P, d = muted ? dur * 0.5 : dur;
    this._noise(out, t, d, muted ? 1200 : fr, 0.7, 'lowpass', g * (muted ? 0.5 : 1), 300);
    this._tone(out, t, d * 0.8, low * 2.2, low * 0.5, g * 0.9);
    if (!muted) this._noise(out, t, 0.04, 6000, 1, 'highpass', g * 0.6);
  },
  step(vol = 0.3, pan = 0) {
    if (!ctx || !enabled || vol <= 0.01) return;
    const t = ctx.currentTime, out = this._out(pan, vol);
    this._noise(out, t, 0.07, 500 + Math.random() * 200, 1.2, 'bandpass', 0.8);
  },
  reload(vol = 0.5, pan = 0) {
    if (!ctx || !enabled) return;
    const t = ctx.currentTime, out = this._out(pan, vol);
    this._tone(out, t, 0.05, 900, 500, 0.5, 'square'); this._tone(out, t + 0.7, 0.06, 600, 1000, 0.5, 'square'); this._noise(out, t + 0.9, 0.05, 2500, 2, 'bandpass', 0.6);
  },
  empty() { if (!ctx || !enabled) return; this._tone(this._out(0, 0.5), ctx.currentTime, 0.04, 1400, 900, 0.5, 'square'); },
  hit(head) { if (!ctx || !enabled) return; const t = ctx.currentTime, o = this._out(0, 0.6); this._tone(o, t, 0.06, head ? 1800 : 1200, head ? 1400 : 900, 0.6, 'triangle'); },
  hurt() { if (!ctx || !enabled) return; const t = ctx.currentTime, o = this._out(0, 0.7); this._tone(o, t, 0.18, 220, 80, 0.8, 'sawtooth'); this._noise(o, t, 0.12, 800, 1, 'lowpass', 0.4); },
  kill() { if (!ctx || !enabled) return; const t = ctx.currentTime, o = this._out(0, 0.7); this._tone(o, t, 0.1, 700, 700, 0.6, 'triangle'); this._tone(o, t + 0.1, 0.18, 1050, 1050, 0.6, 'triangle'); },
  boom(vol = 1, pan = 0) {
    if (!ctx || !enabled || vol <= 0.02) return;
    const t = ctx.currentTime, o = this._out(pan, vol);
    this._noise(o, t, 0.9, 900, 0.5, 'lowpass', 1.4, 80); this._tone(o, t, 0.7, 90, 28, 1.4);
  },
  throwG() { if (!ctx || !enabled) return; const t = ctx.currentTime, o = this._out(0, 0.35); this._noise(o, t, 0.25, 700, 0.8, 'bandpass', 0.6, 1400); },
  click() { if (!ctx || !enabled) return; this._tone(this._out(0, 0.25), ctx.currentTime, 0.03, 800, 600, 0.5, 'square'); },
  spawn() { if (!ctx || !enabled) return; const t = ctx.currentTime, o = this._out(0, 0.35); this._tone(o, t, 0.15, 400, 800, 0.5, 'triangle'); },
};
