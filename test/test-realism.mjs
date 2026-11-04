// واقعية: نبض القلب، قمع النيران، اهتزاز التصويب، ثقوب الرصاص — بلا أخطاء
import { createServer } from '../server/server.js';
import { chromium } from 'playwright';
const srv = await createServer(0, '127.0.0.1', { web: true });
const browser = await chromium.launch({ args: ['--use-gl=angle', '--use-angle=swiftshader', '--enable-unsafe-swiftshader', '--ignore-gpu-blocklist', '--autoplay-policy=no-user-gesture-required'] });
const page = await (await browser.newContext({ viewport: { width: 900, height: 500 } })).newPage(); const errs = []; page.on('pageerror', e => errs.push(e.message));
await page.goto(`http://127.0.0.1:${srv.port}/`);
await page.fill('#name', 'r'); await page.click('#btnSolo'); await page.waitForSelector('#lobby:not(.hidden)');
await page.selectOption('#cfgFill', '5'); await page.click('#btnStart'); await page.waitForFunction(() => window.__wz.me.alive);
await page.evaluate(() => { window.__wz.me.hp = 20; localStorage.setItem('wz_lo', JSON.stringify({ p: 'sniper', s: 'pistol', a: [{}, {}] })); });
await page.keyboard.press('Digit1');
await page.mouse.move(450, 250); await page.mouse.down({ button: 'right' }); await page.keyboard.down('Shift'); await page.waitForTimeout(1500);
for (let i = 0; i < 4; i++) { await page.mouse.down(); await page.mouse.up(); await page.waitForTimeout(250); }
await page.keyboard.up('Shift'); await page.mouse.up({ button: 'right' });
await page.evaluate(() => { window.__wz.me.supp = 1; });
await page.waitForTimeout(800);
const op = await page.evaluate(() => Number(document.getElementById('supp').style.opacity));
console.log('supp opacity', op, 'breath', await page.evaluate(() => window.__wz.me.breath));
const ok = !errs.length && op > 0; console.log(ok ? 'نجح اختبار الواقعية ✓' : 'فشل: ' + errs.join('|')); await browser.close(); srv.close(); process.exit(ok ? 0 : 1);
