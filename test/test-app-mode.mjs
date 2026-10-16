// يحاكي وضع التطبيق (Capacitor): شاشة الاتصال، البحث في الشبكة، الدخول
import { createServer } from '../server/server.js';
import { chromium } from 'playwright';
import fs from 'node:fs';
const srv = await createServer(3000, '0.0.0.0');
const out = process.env.SHOTS || '/tmp/claude-0/shots'; fs.mkdirSync(out, { recursive: true });
const browser = await chromium.launch({ args: ['--use-gl=angle', '--use-angle=swiftshader', '--enable-unsafe-swiftshader'] });
const ctx = await browser.newContext({ viewport: { width: 844, height: 390 }, hasTouch: true, isMobile: true });
await ctx.addInitScript(() => { window.Capacitor = { isNativePlatform: () => true }; });
const page = await ctx.newPage(); const errs = [];
page.on('pageerror', (e) => errs.push(e.message)); page.on('console', (m) => m.type() === 'error' && errs.push(m.text()));
let fails = 0; const ok = (c, m) => { console.log((c ? '  ✓ ' : '  ✗ ') + m); if (!c) fails++; };
await page.goto('http://127.0.0.1:3000/');
await page.waitForSelector('#join:not(.hidden)'); ok(true, 'شاشة الاتصال تظهر أولًا');
await page.waitForSelector('#srvlist .srv', { timeout: 15000 }); ok(true, 'البحث وجد الخادم');
await page.screenshot({ path: `${out}/join.png` });
await page.click('#srvlist .srv'); await page.waitForSelector('#menu:not(.hidden)', { timeout: 8000 }); ok(true, 'الاتصال ثم القائمة');
await page.tap('#btnEnter'); await page.waitForSelector('#lobby:not(.hidden)'); ok(true, 'اللوبي');
await page.screenshot({ path: `${out}/lobby2.png` });
await page.tap('#btnLoadout'); await page.waitForTimeout(300); await page.screenshot({ path: `${out}/loadout2.png` }); await page.tap('#btnLoDone');
await page.selectOption('#cfgFill', '3'); await page.tap('#btnStart'); await page.waitForFunction(() => window.__wz.me.alive, null, { timeout: 8000 });
await page.waitForTimeout(1500); await page.screenshot({ path: `${out}/game2.png` });
// عنوان يدوي خاطئ
await page.reload(); await page.waitForSelector('#join:not(.hidden)'); await page.fill('#manual', '127.0.0.1:3999'); await page.click('#btnManual');
await page.waitForFunction(() => /تعذّر/.test(document.getElementById('joinMsg').textContent), null, { timeout: 9000 }); ok(true, 'رسالة خطأ عند عنوان خاطئ');
ok(errs.length === 0, 'لا أخطاء ' + errs.join('|'));
await browser.close(); srv.close(); process.exit(fails ? 1 : 0);
