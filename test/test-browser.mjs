// اختبار المتصفح: يحمّل اللعبة في Chromium (WebGL برمجي)، يدخل، يبدأ مباراة بوتات في كل خريطة، ويتحقق من غياب الأخطاء
import { createServer } from '../server/server.js';
import { chromium } from 'playwright';
import fs from 'node:fs';
const srv = await createServer(0, '127.0.0.1');
const url = `http://127.0.0.1:${srv.port}/`;
const out = process.env.SHOTS || '/tmp/claude-0/shots'; fs.mkdirSync(out, { recursive: true });
const browser = await chromium.launch({ args: ['--use-gl=angle', '--use-angle=swiftshader', '--enable-unsafe-swiftshader', '--ignore-gpu-blocklist'] });
let fails = 0; const ok = (c, m) => { console.log((c ? '  ✓ ' : '  ✗ ') + m); if (!c) fails++; };
const errors = [];
async function mk(touch) {
  const ctx = await browser.newContext(touch ? { viewport: { width: 844, height: 390 }, hasTouch: true, isMobile: true } : { viewport: { width: 1000, height: 600 } });
  const page = await ctx.newPage();
  page.on('pageerror', (e) => errors.push('pageerror: ' + e.message));
  page.on('console', (m) => { if (m.type() === 'error') errors.push('console: ' + m.text()); });
  await page.goto(url); return page;
}
const page = await mk(false);
await page.fill('#name', 'مختبر'); await page.click('#btnEnter');
await page.waitForSelector('#lobby:not(.hidden)');
ok(true, 'دخول اللوبي');
for (let map = 0; map < 4; map++) {
  await page.selectOption('#cfgMap', String(map)); await page.selectOption('#cfgFill', '3');
  await page.waitForTimeout(300);
  await page.click('#btnStart');
  await page.waitForFunction(() => window.__wz.me.alive, null, { timeout: 8000 });
  await page.waitForTimeout(2500);
  // محاكاة: تحريك ونظر وإطلاق نار
  await page.evaluate(() => { const i = window.__wz; });
  await page.keyboard.down('KeyW'); await page.waitForTimeout(600); await page.keyboard.up('KeyW');
  await page.evaluate(() => { document.dispatchEvent(new Event('x')); });
  await page.mouse.move(500, 300); await page.mouse.down(); await page.waitForTimeout(700); await page.mouse.up();
  await page.keyboard.press('Digit2'); await page.keyboard.press('KeyR'); await page.keyboard.press('KeyG'); await page.keyboard.press('Space');
  await page.waitForTimeout(800);
  await page.screenshot({ path: `${out}/map${map}.png` });
  const info = await page.evaluate(() => ({ alive: window.__wz.me.alive, x: window.__wz.me.x, remote: window.__wz.S.remote.size }));
  ok(info.remote >= 5, `خريطة ${map}: ظهر ${info.remote} لاعبًا آخر`);
  await page.click('#hud', { force: true }).catch(() => {});
  await page.evaluate(() => window.__wz.S.ws.send(JSON.stringify({ t: 'back' })));
  await page.waitForSelector('#lobby:not(.hidden)', { timeout: 5000 });
}
await page.context().close(); await new Promise(r => setTimeout(r, 500));
const tp = await mk(true);
await tp.fill('#name', 'جوال'); await tp.tap('#btnEnter'); await tp.waitForSelector('#lobby:not(.hidden)');
await tp.screenshot({ path: `${out}/lobby-touch.png` });
await tp.tap('#btnLoadout'); await tp.waitForTimeout(300); await tp.screenshot({ path: `${out}/loadout-touch.png` }); await tp.tap('#btnLoDone');
await tp.selectOption('#cfgFill', '3'); await tp.tap('#btnStart');
await tp.waitForFunction(() => window.__wz.me.alive, null, { timeout: 8000 });
await tp.waitForTimeout(1500);
await tp.screenshot({ path: `${out}/game-touch.png` });
ok(await tp.evaluate(() => document.body.classList.contains('touch')), 'وضع اللمس مفعّل');
await tp.waitForTimeout(1000);
ok(errors.length === 0, `لا أخطاء في المتصفح (${errors.length})`); if (errors.length) console.log(errors.slice(0, 10));
await browser.close(); srv.close();
console.log(fails ? 'فشل' : 'نجح اختبار المتصفح ✓'); process.exit(fails ? 1 : 0);
