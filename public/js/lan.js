// اكتشاف خوادم اللعبة على الشبكة المحلية (فحص عناوين الشبكة الفرعية)
const COMMON = ['192.168.1', '192.168.0', '192.168.43', '192.168.8', '192.168.100', '192.168.2', '192.168.137', '10.0.0', '10.0.1', '172.20.10', '192.168.3', '192.168.10'];

export function detectLocalSubnet() {
  return new Promise((resolve) => {
    let done = false; const fin = (v) => { if (!done) { done = true; resolve(v); } };
    try {
      const pc = new RTCPeerConnection({ iceServers: [] });
      pc.createDataChannel('x');
      pc.onicecandidate = (e) => {
        if (!e.candidate) return;
        const m = /(\d+\.\d+\.\d+)\.\d+/.exec(e.candidate.candidate);
        if (m && !m[1].startsWith('0.') && /^(10|192\.168|172\.(1[6-9]|2\d|3[01]))\./.test(m[1])) { fin(m[1]); try { pc.close(); } catch { /* */ } }
      };
      pc.createOffer().then((o) => pc.setLocalDescription(o)).catch(() => fin(null));
      setTimeout(() => { fin(null); try { pc.close(); } catch { /* */ } }, 1500);
    } catch { fin(null); }
  });
}

export async function probe(host, port = 3000, timeout = 900) {
  const ac = new AbortController(); const t = setTimeout(() => ac.abort(), timeout);
  try {
    const r = await fetch(`http://${host}:${port}/api/info`, { signal: ac.signal, cache: 'no-store' });
    const j = await r.json();
    if (typeof j.port !== 'number') return null;
    return { host, port, players: j.players, phase: j.phase };
  } catch { return null; } finally { clearTimeout(t); }
}

// onFound(server) لكل خادم، onProgress(0..1). cancel عبر obj.stop = true
export async function scan(onFound, onProgress, ctl = {}) {
  const found = new Set();
  const hit = (s) => { const k = s.host + ':' + s.port; if (!found.has(k)) { found.add(k); onFound(s); } };
  const first = await probe('127.0.0.1'); if (first) hit({ ...first, host: '127.0.0.1', local: true });
  const sub = await detectLocalSubnet();
  const subnets = [...new Set([sub, ...COMMON].filter(Boolean))];
  const targets = []; for (const s of subnets) for (let i = 1; i < 255; i++) targets.push(`${s}.${i}`);
  let idx = 0, done = 0;
  const worker = async () => {
    while (!ctl.stop && idx < targets.length) {
      const h = targets[idx++]; const r = await probe(h, 3000, 700); if (r) hit(r);
      if (++done % 20 === 0) onProgress(done / targets.length);
    }
  };
  await Promise.all(Array.from({ length: 64 }, worker));
  onProgress(1);
  return [...found];
}
