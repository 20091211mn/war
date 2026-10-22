// يحاكي وضع التطبيق (Capacitor): قائمة بلا أي موقع/عنوان، لعب منفرد، استضافة LAN عبر إضافة وهمية، بحث وانضمام
import { createServer } from '../server/server.js';
import { chromium } from 'playwright';
import fs from 'node:fs';
const srv = await createServer(3000, '0.0.0.0');
const out = process.env.SHOTS || '/tmp/claude-0/shots'; fs.mkdirSync(out, { recursive: true });
const browser = await chromium.launch({ args: ['--use-gl=angle', '--use-angle=swiftshader', '--enable-unsafe-swiftshader'] });
const ctx = await browser.newContext({ viewport: { width: 844, height: 390 }, hasTouch: true, isMobile: true });
await ctx.addInitScript(() => {
  // إضافة LanHost وهمية تسجّل ما يمرّ عبرها
  const listeners = {}; window.__sent = []; window.__calls = [];
  const plugin = {
    start: async (o) => { window.__calls.push('start'); return { port: o.port, ips: ['192.168.1.50'] }; },
    stop: async () => { window.__calls.push('stop'); },
    addListener: async (n, fn) => { listeners[n] = fn; return { remove() { delete listeners[n]; } }; },
    sendBatch: async (o) => { window.__sent.push(...o.items); },
    send: async () => {}, closeClient: async () => {}, setInfo: async () => {},
  };
  window.__fire = (n, e) => listeners[n] && listeners[n](e);
  window.Capacitor = { isNativePlatform: () => true, registerPlugin: () => plugin };
});
const page = await ctx.newPage(); const errs = [];
page.on('pageerror', (e) => errs.push(e.message)); page.on('console', (m) => m.type() === 'error' && errs.push(m.text()));
let fails = 0; const ok = (c, m) => { console.log((c ? '  ✓ ' : '  ✗ ') + m); if (!c) fails++; };
await page.goto('http://127.0.0.1:3000/');
await page.waitForSelector('#menu:not(.hidden)'); ok(true, 'القائمة الرئيسية تظهر أولًا (بدون شاشة إدخال موقع)');
ok(!(await page.isVisible('#btnPage')), 'لا يوجد زر «خادم الصفحة» داخل التطبيق');
ok(await page.isVisible('#btnSolo') && await page.isVisible('#btnHost') && await page.isVisible('#btnJoin'), 'خيارات: منفرد / استضافة / انضمام');
await page.screenshot({ path: `${out}/menu-app.png` });

console.log('— استضافة LAN عبر الإضافة');
await page.tap('#btnHost'); await page.waitForSelector('#lobby:not(.hidden)');
ok((await page.textContent('#addr')).includes('192.168.1.50:3000'), 'يعرض عنوان الشبكة للأصدقاء');
// لاعب بعيد يدخل عبر الإضافة
await page.evaluate(() => { window.__fire('open', { cid: 7 }); window.__fire('message', { cid: 7, data: JSON.stringify({ t: 'hello', n: 'صديق' }) }); window.__fire('message', { cid: 7, data: JSON.stringify({ t: 'chat', m: 'أهلا بالمضيف' }) }); });
await page.waitForTimeout(400);
const sent = await page.evaluate(() => window.__sent);
ok(sent.some((i) => i.c === 7 && i.d.includes('"t":"welcome"')), 'اللاعب البعيد استلم welcome عبر الإضافة');
ok(sent.some((i) => i.c === 7 && i.d.includes('أهلا بالمضيف')), 'الدردشة تُبثّ للاعب البعيد');
await page.waitForFunction(() => document.getElementById('chatLog').textContent.includes('أهلا بالمضيف'));
ok((await page.textContent('#t0')).includes('صديق') || (await page.textContent('#t1')).includes('صديق'), 'اللاعب البعيد يظهر في الفريق عند المضيف');
await page.screenshot({ path: `${out}/lobby-host.png` });
await page.tap('#btnStart'); await page.waitForFunction(() => window.__wz.me.alive, null, { timeout: 8000 });
await page.waitForTimeout(800);
const snaps = await page.evaluate(() => window.__sent.filter((i) => i.c === 7 && i.d.includes('"t":"snap"')).length);
ok(snaps > 3, `اللقطات تُرسل للاعب البعيد (${snaps})`);
await page.evaluate(() => window.__fire('close', { cid: 7 })); await page.waitForTimeout(300);
await page.evaluate(() => window.__wz.send({ t: 'back' })); await page.waitForSelector('#lobby:not(.hidden)');
await page.tap('#btnLeave'); await page.waitForSelector('#menu:not(.hidden)');
ok((await page.evaluate(() => window.__calls)).includes('stop'), 'الخروج يوقف خادم LAN');

console.log('— لعب منفرد');
await page.tap('#btnSolo'); await page.waitForSelector('#lobby:not(.hidden)');
await page.selectOption('#cfgMap', '4'); await page.selectOption('#cfgFill', '3'); await page.tap('#btnStart');
await page.waitForFunction(() => window.__wz.me.alive, null, { timeout: 8000 }); await page.waitForTimeout(1500);
await page.screenshot({ path: `${out}/game-app.png` });
ok(true, 'مباراة منفردة تعمل');
await page.evaluate(() => window.__wz.send({ t: 'back' })); await page.waitForSelector('#lobby:not(.hidden)'); await page.tap('#btnLeave'); await page.waitForSelector('#menu:not(.hidden)');

console.log('— الانضمام عبر البحث');
await page.tap('#btnJoin'); await page.waitForSelector('#join:not(.hidden)');
await page.waitForSelector('#srvlist .srv', { timeout: 15000 }); ok(true, 'البحث وجد الخادم');
await page.screenshot({ path: `${out}/join.png` });
await page.click('#srvlist .srv:last-child'); await page.waitForSelector('#lobby:not(.hidden)', { timeout: 8000 }); ok(true, 'الانضمام ثم اللوبي');
await page.tap('#btnLeave'); await page.waitForSelector('#menu:not(.hidden)');
await page.tap('#btnJoin'); await page.waitForSelector('#join:not(.hidden)'); await page.fill('#manual', '127.0.0.1:3999'); await page.click('#btnManual');
await page.waitForFunction(() => /تعذّر/.test(document.getElementById('joinMsg').textContent), null, { timeout: 9000 }); ok(true, 'رسالة خطأ عند عنوان خاطئ');
const real = errs.filter((e) => !/ERR_CONNECTION_REFUSED|WebSocket connection/.test(e));
ok(real.length === 0, 'لا أخطاء ' + real.join('|'));
await browser.close(); srv.close(); process.exit(fails ? 1 : 0);
