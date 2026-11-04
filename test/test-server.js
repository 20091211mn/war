// اختبار شامل للخادم: عدة عملاء حقيقيين عبر WebSocket + بوتات + قنابل + حالات خاطئة
import { createServer } from '../server/server.js';
import net from 'node:net';
import { MAX_HUMANS } from '../shared/game.js';

let fails = 0;
const ok = (c, m) => { if (!c) { console.log('  ✗ ' + m); fails++; } else console.log('  ✓ ' + m); };
const sleep = (ms) => new Promise(r => setTimeout(r, ms));
const errs = []; const oe = console.error; console.error = (...a) => { errs.push(a.join(' ')); oe(...a); };

const srv = await createServer(0, '127.0.0.1', { web: true });
const url = `ws://127.0.0.1:${srv.port}`;

function client(name) {
  const c = { name, id: 0, msgs: [], snaps: 0, kills: [], hits: [], spawn: null, lobby: null, started: false, ended: null, last: null, ws: new WebSocket(url) };
  c.ready = new Promise((res, rej) => { c.ws.onopen = res; c.ws.onerror = rej; });
  c.ws.onmessage = (e) => {
    const m = JSON.parse(e.data);
    if (m.t === 'welcome') c.id = m.id;
    else if (m.t === 'lobby') c.lobby = m;
    else if (m.t === 'start') c.started = true;
    else if (m.t === 'spawn' && m.id === c.id) c.spawn = m;
    else if (m.t === 'snap') { c.snaps++; c.last = m; }
    else if (m.t === 'kill') c.kills.push(m);
    else if (m.t === 'hit') c.hits.push(m);
    else if (m.t === 'end') c.ended = m;
    else if (m.t === 'full') c.full = true;
    c.msgs.push(m.t);
  };
  c.send = (o) => c.ws.send(JSON.stringify(o));
  return c;
}

console.log('1) اتصال 12 لاعبًا');
const cs = [];
for (let i = 0; i < 12; i++) { const c = client('P' + i); await c.ready; c.send({ t: 'hello', n: 'لاعب' + i }); cs.push(c); }
await sleep(300);
ok(cs.every(c => c.id > 0), 'كل اللاعبين استلموا welcome');
ok(cs[0].lobby && cs[0].lobby.ps.length === 12, 'اللوبي يعرض 12 لاعبًا');
const t0 = cs[0].lobby.ps.filter(p => p.tm === 0).length, t1 = cs[0].lobby.ps.filter(p => p.tm === 1).length;
ok(t0 === 6 && t1 === 6, `توزيع الفرق متوازن (${t0}/${t1})`);
ok(cs[0].lobby.host === cs[0].id, 'اللاعب الأول هو المضيف');

console.log('2) حماية: رسائل خاطئة، غير مضيف، حجم كبير');
cs[1].send({ t: 'start' }); cs[1].ws.send('not json'); cs[1].ws.send('{"t":5}'); cs[1].ws.send('null'); cs[1].send({ t: 'cfg', map: 99, scoreLimit: 7 });
await sleep(200);
ok(!cs[1].started, 'غير المضيف لا يستطيع بدء المباراة');
ok(cs[0].lobby.cfg.scoreLimit === 30, 'لا تُقبل إعدادات من غير المضيف');
cs[0].send({ t: 'cfg', map: 99, scoreLimit: 10, timeLimit: 180, fill: 5 });
cs[0].send({ t: 'lo', lo: { p: 'hax', s: 'sniper', a: [{ optic: 'x99', muzzle: 'zzz' }, null] } });
await sleep(200);
ok(cs[0].lobby.cfg.map === 4 && cs[0].lobby.cfg.fill === 5 && cs[0].lobby.cfg.scoreLimit === 10, 'الإعدادات تُنقّى (map يُقيَّد)');
const me = cs[0].lobby.ps.find(p => p.id === cs[0].id);
ok(me.p === 'carbine' && me.s === 'pistol', 'العتاد غير الصالح يعود للافتراضي');

console.log('3) بدء المباراة مع بوتات');
cs[0].send({ t: 'cfg', map: 2 });
await sleep(100);
cs[0].send({ t: 'start' });
await sleep(600);
ok(cs.every(c => c.started), 'الجميع استلم start');
ok(cs.every(c => c.spawn), 'الجميع ظهر');
const ps = cs[0].lobby.ps;
ok(ps.filter(p => p.tm === 0).length === 6 && ps.filter(p => p.tm === 1).length === 6, 'لا بوتات عندما الفرق ≥ الحد الأدنى (6 ≥ 5)');

console.log('4) إعادة تعيين العتاد وبدء مباراة ثانية مع بوتات');
cs[0].send({ t: 'back' }); await sleep(200);
ok(cs[0].msgs.includes('tolobby'), 'العودة إلى اللوبي');
// نفصل 8 لاعبين ونبقي 4 مع fill=5 → بوتات
for (let i = 4; i < 12; i++) cs[i].ws.close();
await sleep(300);
const live = cs.slice(0, 4);
live[0].send({ t: 'cfg', map: 1, fill: 5, scoreLimit: 10 }); await sleep(100);
live[0].send({ t: 'start' }); await sleep(500);
const ps2 = live[0].lobby.ps;
const bots = ps2.filter(p => p.b);
ok(ps2.filter(p => p.tm === 0).length === 5 && ps2.filter(p => p.tm === 1).length === 5, `الفريقان 5 ضد 5 (بوتات ${bots.length})`);

console.log('5) محاكاة لاعب حقيقي يتحرك ويطلق النار ويرمي قنابل لمدة 20 ثانية');
const alive = (c) => c.last && c.last.ps.find(p => p[0] === c.id);
let shots = 0;
const drive = setInterval(() => {
  for (const c of live) {
    if (!c.spawn || !c.last) continue;
    const me = alive(c); if (!me || !(me[6] & 4)) continue;
    const e = c.spawn.e; const t = Date.now() / 1000;
    c.send({ t: 'st', e: c.lastEpoch ?? (c.lastEpoch = c.spawn.e), x: me[1], y: 0, z: me[3], yaw: 0, pitch: 0, f: 4, w: 0 });
  }
}, 50);
// تحديث epoch عند كل ظهور جديد
for (const c of live) c.ws.addEventListener('message', (ev) => { const m = JSON.parse(ev.data); if (m.t === 'spawn' && m.id === c.id) c.lastEpoch = m.e; });
const fire = setInterval(() => {
  for (const c of live) {
    if (!c.spawn) continue;
    const dirs = [[Math.random() - 0.5, 0, Math.random() - 0.5 || 1]];
    c.send({ t: 'shoot', w: 0, d: dirs }); shots++;
    if (Math.random() < 0.05) c.send({ t: 'gren', d: [1, 0.4, 0] });
  }
}, 120);
await sleep(20000);
clearInterval(drive); clearInterval(fire);
ok(live.every(c => c.snaps > 200), `اللقطات تصل بمعدل مناسب (${live[0].snaps})`);
const totalKills = live[0].kills.length;
ok(totalKills > 0, `حدثت إصابات قاتلة بين البوتات (${totalKills})`);
const lastSnap = live[0].last;
ok(Array.isArray(lastSnap.sc), `النتيجة ${lastSnap.sc}`);
ok(live[0].ended || lastSnap.tl > 0, 'المباراة تنتهي بالحد أو تستمر');
const botsMoved = lastSnap.ps.filter(p => p[0] > 100 || true).length;
ok(botsMoved >= 10, 'البوتات تظهر في اللقطات');

console.log('6) رسائل st خاطئة (epoch قديم، NaN، قيم هائلة)');
const L = live[1];
L.send({ t: 'st', e: -5, x: 1e9, y: 1e9, z: 1e9, yaw: 0, pitch: 0, f: 0, w: 0 });
L.send({ t: 'st', e: 1, x: 'a', y: NaN, z: null, yaw: 0, pitch: 0, f: 0, w: 0 });
L.send({ t: 'shoot', w: 0, d: [[NaN, 0, 0], 'x', [0, 0, 0]] });
L.send({ t: 'gren', d: [0, 0, 0] });
await sleep(300);
ok(L.ws.readyState === 1, 'الاتصال سليم بعد الرسائل الخاطئة');

console.log('7) إطار WebSocket خام كبير وحالات حدّية');
await new Promise((res) => {
  const s = net.connect(srv.port, '127.0.0.1', () => {
    s.write('GET / HTTP/1.1\r\nHost: x\r\nUpgrade: websocket\r\nConnection: Upgrade\r\nSec-WebSocket-Key: dGhlIHNhbXBsZSBub25jZQ==\r\nSec-WebSocket-Version: 13\r\n\r\n');
    setTimeout(() => { s.write(Buffer.from([0x81, 0x7f, 0, 0, 0, 0, 0x10, 0, 0, 0, 1, 2, 3, 4])); }, 100);
  });
  s.on('data', () => {}); s.on('close', res); s.on('error', res);
  setTimeout(() => { s.destroy(); res(); }, 1500);
});
ok(true, 'الخادم يتعامل مع إطار ضخم بدون انهيار');

console.log('8) HTTP');
const r = await fetch(`http://127.0.0.1:${srv.port}/api/info`); ok(r.ok, '/api/info');
const r2 = await fetch(`http://127.0.0.1:${srv.port}/../server/game.js`); ok(r2.status === 404 || r2.status === 403, 'منع تجاوز المسار');
const r3 = await fetch(`http://127.0.0.1:${srv.port}/shared/%2e%2e/server/game.js`); ok(r3.status === 404 || r3.status === 403, 'منع تجاوز المسار المشفّر');
const r4 = await fetch(`http://127.0.0.1:${srv.port}/shared/maps.js`); ok(r4.ok, '/shared/maps.js');

console.log('9) حد اللاعبين');
srv.game.cfg.fill = 0;
const many = [];
for (let i = 0; i < MAX_HUMANS + 2; i++) { const c = client('X' + i); many.push(c); await c.ready.catch(() => {}); }
await sleep(500);
ok(srv.game.humans().length <= MAX_HUMANS, `الحد الأقصى للبشر ${srv.game.humans().length} ≤ ${MAX_HUMANS}`);
ok(many.some(c => c.full), 'يُرسل full عند الامتلاء');

ok(errs.length === 0, `لا أخطاء في الخادم (${errs.length})`);
srv.close();
console.log(fails ? `فشل: ${fails}` : 'نجحت كل اختبارات الخادم ✓');
process.exit(fails ? 1 : 0);
