// خادم Warzone LAN — HTTP ثابت + WebSocket + منطق اللعبة. بدون أي اعتماديات.
import http from 'node:http';
import fs from 'node:fs';
import path from 'node:path';
import os from 'node:os';
import { fileURLToPath } from 'node:url';
import { attachWS } from './ws.js';
import { Game } from '../shared/game.js';

const ROOT = path.resolve(path.dirname(fileURLToPath(import.meta.url)), '..');
const PORT = Number(process.env.PORT) || 3000;
const MIME = {
  '.html': 'text/html; charset=utf-8', '.js': 'text/javascript; charset=utf-8', '.css': 'text/css; charset=utf-8',
  '.json': 'application/json; charset=utf-8', '.png': 'image/png', '.svg': 'image/svg+xml', '.ico': 'image/x-icon',
  '.webmanifest': 'application/manifest+json',
};
const ROUTES = { '/shared/': path.join(ROOT, 'shared'), '/': path.join(ROOT, 'public') };

export function lanAddresses() {
  const out = [];
  try {
    for (const list of Object.values(os.networkInterfaces())) for (const i of list || []) if (i.family === 'IPv4' && !i.internal) out.push(i.address);
  } catch { /* بعض أجهزة أندرويد تمنع القراءة */ }
  return out;
}

export function createServer(port = PORT, host = '0.0.0.0') {
  const game = new Game();
  const server = http.createServer((req, res) => {
    let url;
    try { url = decodeURIComponent(new URL(req.url, 'http://x').pathname); } catch { res.writeHead(400); res.end('bad request'); return; }
    if (url === '/api/info') {
      res.writeHead(200, { 'Content-Type': 'application/json', 'Cache-Control': 'no-store', 'Access-Control-Allow-Origin': '*', 'Access-Control-Allow-Private-Network': 'true' });
      res.end(JSON.stringify({ ips: lanAddresses(), port, players: game.humans().length, phase: game.phase }));
      return;
    }
    if (url === '/') url = '/index.html';
    let base = ROUTES['/'], rel = url;
    if (url.startsWith('/shared/')) { base = ROUTES['/shared/']; rel = url.slice('/shared/'.length); }
    const file = path.resolve(base, '.' + path.sep + rel.replace(/^\/+/, ''));
    if (!file.startsWith(base + path.sep)) { res.writeHead(403); res.end('forbidden'); return; }
    fs.readFile(file, (err, data) => {
      if (err) { res.writeHead(404, { 'Content-Type': 'text/plain; charset=utf-8' }); res.end('غير موجود'); return; }
      res.writeHead(200, { 'Content-Type': MIME[path.extname(file)] || 'application/octet-stream', 'Cache-Control': 'no-cache' });
      res.end(data);
    });
  });
  const conns = new Set();
  attachWS(server, (conn) => {
    const p = game.connect(conn);
    if (!p) return;
    conns.add(conn);
    conn.on('message', (s) => { try { game.onMessage(p, s); } catch (e) { console.error('خطأ رسالة:', e); } });
    conn.on('close', () => { conns.delete(conn); try { game.disconnect(p); } catch (e) { console.error('خطأ فصل:', e); } });
  });
  const tick = setInterval(() => { try { game.tick(); } catch (e) { console.error('خطأ tick:', e); } }, 50);
  const beat = setInterval(() => {
    for (const c of conns) { if (!c.alive) { c.terminate(); continue; } c.alive = false; c.ping(); }
  }, 10000);
  server.on('close', () => { clearInterval(tick); clearInterval(beat); });
  return new Promise((resolve, reject) => {
    server.once('error', reject);
    server.listen(port, host, () => resolve({ server, game, port: server.address().port, close: () => { for (const c of conns) c.terminate(); server.close(); } }));
  });
}

if (process.argv[1] && path.resolve(process.argv[1]) === fileURLToPath(import.meta.url)) {
  createServer().then(({ port }) => {
    console.log('\n=== Warzone LAN ===');
    console.log(`الخادم يعمل على المنفذ ${port}`);
    console.log(`على هذا الجهاز:       http://localhost:${port}`);
    const ips = lanAddresses();
    if (ips.length) for (const ip of ips) console.log(`للأصدقاء (نفس الشبكة): http://${ip}:${port}`);
    else console.log('لم أستطع قراءة عنوان الشبكة تلقائيًا. نفّذ الأمر:  ifconfig  أو  ip addr  وابحث عن عنوان مثل 192.168.x.x');
    console.log('اضغط Ctrl+C للإيقاف.\n');
  }).catch((e) => {
    console.error(e.code === 'EADDRINUSE' ? 'المنفذ مستخدم، جرّب: PORT=3001 node server/server.js' : e);
    process.exit(1);
  });
  process.on('uncaughtException', (e) => console.error('خطأ غير متوقع:', e));
  process.on('unhandledRejection', (e) => console.error('رفض غير معالج:', e));
}
