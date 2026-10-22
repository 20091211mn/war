// يتحقق أن صفحة PS4 المستقلة (بدون خادم) تعمل: لعب منفرد وتشغيل الحلقة بلا أخطاء
import { chromium } from 'playwright';
import path from 'node:path';
const browser = await chromium.launch({ args: ['--use-gl=angle', '--use-angle=swiftshader', '--enable-unsafe-swiftshader', '--ignore-gpu-blocklist'] });
const page = await (await browser.newContext({ viewport: { width: 960, height: 540 } })).newPage();
const errs = []; page.on('pageerror', (e) => errs.push(e.message)); page.on('console', (m) => { if (m.type() === 'error') errs.push(m.text()); });
await page.goto('file://' + path.resolve('ps4/out/page.html'));
await page.fill('#name', 'ps4'); await page.click('#btnSolo'); await page.waitForSelector('#lobby:not(.hidden)');
await page.selectOption('#cfgMap', '2'); await page.click('#btnStart');
await page.waitForFunction(() => window.__wz && window.__wz.me.alive, null, { timeout: 15000 });
await page.waitForTimeout(1500);
console.log(errs.length ? 'أخطاء: ' + errs.slice(0, 5).join(' | ') : 'نجح اختبار صفحة PS4 ✓');
await browser.close(); process.exit(errs.length ? 1 : 0);
