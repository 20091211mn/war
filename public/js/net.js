// طبقات النقل: لعب منفرد داخل التطبيق، استضافة LAN عبر الإضافة الأصلية، أو اتصال WebSocket بمضيف آخر.
// كل طبقة تعطي كائنًا بنفس الواجهة: send(str) / close() وأحداث onopen / onmessage / onclose
import { Game } from '/shared/game.js';

// ---------- لعبة محلية داخل الصفحة (بدون شبكة) ----------
export class LocalHost {
  constructor() {
    this.game = new Game();
    this.game.cfg.fill = 5;               // بوتات افتراضيًا للعب المنفرد
    this.timer = setInterval(() => { try { this.game.tick(); } catch (e) { console.error(e); } }, 50);
    this.remotes = new Map();              // cid → {p, conn} لاعبو الشبكة (عند الاستضافة)
    this.lan = null;
  }
  // اتصال اللاعب المحلي
  link() {
    const game = this.game, link = { onopen: null, onmessage: null, onclose: null, p: null, closed: false };
    const conn = {
      send: (s) => { if (!link.closed) queueMicrotask(() => { if (!link.closed && link.onmessage) link.onmessage({ data: s }); }); },
      close: () => { link.close(); },
    };
    link.send = (s) => { if (!link.closed && link.p) game.onMessage(link.p, s); };
    link.close = () => { if (link.closed) return; link.closed = true; if (link.p) game.disconnect(link.p); if (link.onclose) link.onclose(); };
    link.readyState = 1;
    queueMicrotask(() => { link.p = game.connect(conn); if (link.onopen) link.onopen(); });
    return link;
  }
  // استضافة LAN عبر الإضافة الأصلية (أندرويد)
  async startLan(plugin, port = 3000) {
    this.lan = plugin;
    const info = await plugin.start({ port });
    const game = this.game, outq = new Map();
    let flushing = false;
    const flush = () => {
      flushing = false;
      if (!outq.size) return;
      const items = []; for (const [c, arr] of outq) for (const d of arr) items.push({ c, d });
      outq.clear();
      plugin.sendBatch({ items }).catch(() => {});
    };
    const queue = (cid, s) => { let a = outq.get(cid); if (!a) outq.set(cid, a = []); a.push(s); if (!flushing) { flushing = true; setTimeout(flush, 0); } };
    this.handles = [];
    this.handles.push(await plugin.addListener('open', (e) => {
      const cid = e.cid;
      const conn = { send: (s) => queue(cid, s), close: () => plugin.closeClient({ cid }).catch(() => {}) };
      const p = game.connect(conn);
      if (p) this.remotes.set(cid, { p, conn });
    }));
    this.handles.push(await plugin.addListener('message', (e) => { const r = this.remotes.get(e.cid); if (r) game.onMessage(r.p, e.data); }));
    this.handles.push(await plugin.addListener('close', (e) => { const r = this.remotes.get(e.cid); if (r) { this.remotes.delete(e.cid); game.disconnect(r.p); } }));
    this.infoTimer = setInterval(() => {
      plugin.setInfo({ info: JSON.stringify({ app: 'warzone', players: game.humans().length, phase: game.phase, max: 16 }) }).catch(() => {});
    }, 1000);
    return info; // {port, ips}
  }
  async stop() {
    clearInterval(this.timer); clearInterval(this.infoTimer);
    if (this.lan) { for (const h of this.handles || []) { try { h.remove(); } catch { /* */ } } try { await this.lan.stop(); } catch { /* */ } }
    for (const r of this.remotes.values()) { try { r.conn.close(); } catch { /* */ } }
    this.remotes.clear();
  }
}

// ---------- WebSocket عادي ----------
export function remoteLink(addr, secure) {
  const ws = new WebSocket(`${secure ? 'wss' : 'ws'}://${addr}`);
  const link = { onopen: null, onmessage: null, onclose: null, ws, closed: false };
  Object.defineProperty(link, 'readyState', { get: () => ws.readyState });
  link.send = (s) => { if (ws.readyState === 1) ws.send(s); };
  link.close = () => { link.closed = true; try { ws.close(); } catch { /* */ } };
  const timer = setTimeout(() => { if (ws.readyState !== 1) { try { ws.close(); } catch { /* */ } } }, 5000);
  ws.onopen = () => { clearTimeout(timer); link.onopen && link.onopen(); };
  ws.onmessage = (e) => link.onmessage && link.onmessage(e);
  ws.onclose = () => { clearTimeout(timer); link.onclose && link.onclose(); };
  ws.onerror = () => {};
  return link;
}
