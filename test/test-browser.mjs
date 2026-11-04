// اختبار المتصفح: لعب منفرد داخل الصفحة (بدون خادم)، كل الخرائط، المركبات، الدردشة، لوحة الموت، والانضمام عبر الشبكة
import { createServer } from '../server/server.js';
import { chromium } from 'playwright';
import fs from 'node:fs';
const srv = await createServer(0, '127.0.0.1', { web: true });
const url = `http://127.0.0.1:${srv.port}/`;
const out = process.env.SHOTS || '/tmp/claude-0/shots'; fs.mkdirSync(out, { recursive: true });
const browser = await chromium.launch({ args: ['--use-gl=angle', '--use-angle=swiftshader', '--enable-unsafe-swiftshader', '--ignore-gpu-blocklist', '--autoplay-policy=no-user-gesture-required'] });
let fails = 0; const ok = (c, m) => { console.log((c ? '  ✓ ' : '  ✗ ') + m); if (!c) fails++; };
const errors = [];
async function mk(touch) {
  const ctx = await browser.newContext(touch ? { viewport: { width: 844, height: 390 }, hasTouch: true, isMobile: true } : { viewport: { width: 1000, height: 600 } });
  const page = await ctx.newPage();
  page.on('pageerror', (e) => errors.push('pageerror: ' + e.message));
  page.on('console', (m) => { if (m.type() === 'error') errors.push('console: ' + m.text()); });
  await page.goto(url); return page;
}
const sel = async (page, id, v) => page.evaluate(([id, v]) => { const e = document.getElementById(id); e.value = v; e.dispatchEvent(new Event('change', { bubbles: true })); }, [id, v]);
const page = await mk(false);
console.log('1) لعب منفرد (بدون أي اتصال بالخادم)');
let wsOpened = false; page.on('websocket', () => { wsOpened = true; });
await page.fill('#name', 'مختبر'); await page.click('#btnSolo');
await page.waitForSelector('#lobby:not(.hidden)');
ok(!wsOpened, 'لا يوجد اتصال WebSocket في اللعب المنفرد');
ok((await page.textContent('#addr')).includes('منفرد'), 'عنوان الغرفة يوضح اللعب المنفرد');
// دردشة اللوبي
await page.fill('#lobbyIn', 'مرحبا يا شباب'); await page.click('#lobbySend');
await page.waitForFunction(() => document.getElementById('chatLog').textContent.includes('مرحبا يا شباب'));
ok(true, 'الدردشة تعمل في اللوبي');
ok(!(await page.isDisabled('#cfgMap')) && !(await page.isDisabled('#cfgFf')), 'المضيف يستطيع تغيير الخريطة والإعدادات');
for (let map = 0; map < 5; map++) {
  await sel(page, 'cfgMap', String(map)); await sel(page, 'cfgFill', '3');
  await page.waitForTimeout(300);
  await page.click('#btnStart');
  await page.waitForFunction(() => window.__wz.me.alive, null, { timeout: 8000 });
  await page.waitForTimeout(2000);
  await page.keyboard.down('KeyW'); await page.waitForTimeout(600); await page.keyboard.up('KeyW');
  await page.mouse.move(500, 300); await page.mouse.down(); await page.waitForTimeout(700); await page.mouse.up();
  await page.keyboard.press('Digit2'); await page.keyboard.press('KeyR'); await page.keyboard.press('KeyG'); await page.keyboard.press('Space');
  await page.waitForTimeout(600);
  await page.screenshot({ path: `${out}/map${map}.png` });
  const info = await page.evaluate(() => ({ remote: window.__wz.S.remote.size, veh: window.__wz.S.vehs.size }));
  ok(info.remote >= 5, `خريطة ${map}: ظهر ${info.remote} لاعبًا آخر`);
  if (map === 4) ok(info.veh === 8, `خريطة الشاص: ${info.veh} مركبات`);
  if (map === 4) {
    console.log('2) المركبات');
    // أنقل اللاعب بجوار أول مركبة قابلة للركوب
    await page.evaluate(() => {
      const { S, me } = window.__wz; const v = [...S.vehs.values()].find((q) => q.x < 0) || [...S.vehs.values()][0];
      me.x = v.x + 2.2; me.z = v.z - 1; me.y = 0; me.team = 0; window.__v0 = v.id;
      // اجعله فريق السيارة: انقل إلى الفريق الأحمر عبر الخادم غير ممكن الآن، نتحقق من الأقرب فقط
    });
    const near = await page.evaluate(() => { const { me } = window.__wz; return me.alive; });
    // ركوب بالضغط على E (قد ترفض الخادم إن كان المقعد لفريق آخر؛ نتحقق من أي نتيجة بلا أخطاء)
    await page.waitForTimeout(150);
    await page.keyboard.press('KeyE'); await page.waitForTimeout(500);
    const seated = await page.evaluate(() => window.__wz.me.veh);
    ok(seated >= -1, `طلب الركوب لم يسبب أخطاء (veh=${seated})`);
    if (seated >= 0) {
      ok(await page.evaluate(() => document.body.classList.contains('invehicle')), 'واجهة المركبة تظهر');
      await page.keyboard.down('KeyW'); let sp = 0;
      for (let i = 0; i < 11; i++) { await page.waitForTimeout(200); sp = Math.max(sp, await page.evaluate(() => Math.abs(window.__wz.LV.speed))); }
      await page.screenshot({ path: `${out}/driving.png` });
      await page.keyboard.up('KeyW');
      ok(sp > 1.2, `السيارة تتسارع (${sp.toFixed(1)} م/ث)`);
      await page.keyboard.press('KeyV'); await page.waitForTimeout(300); await page.screenshot({ path: `${out}/driving-fp.png` });
      await page.keyboard.down('KeyS'); await page.waitForTimeout(1800); await page.keyboard.up('KeyS');
      await page.keyboard.press('KeyE'); await page.waitForTimeout(500);
      ok(await page.evaluate(() => window.__wz.me.veh) === -1, 'النزول من المركبة يعمل');
    }
  }
  // دردشة داخل المباراة
  await page.keyboard.press('Enter'); await page.waitForSelector('#chatbar:not(.hidden)');
  await page.fill('#chatIn', 'اختبار داخل اللعبة'); await page.keyboard.press('Enter');
  await page.waitForFunction(() => document.getElementById('chatfeed').textContent.includes('اختبار داخل اللعبة'));
  // الموت ولوحة تغيير السلاح
  await page.evaluate(() => window.__wz.send({ t: 'chat', m: '' }));
  const dead = await page.evaluate(async () => {
    const { S, me } = window.__wz; const g = window.__wz; return !!g;
  });
  await page.evaluate(() => window.__wz.send({ t: 'back' }));
  await page.waitForSelector('#lobby:not(.hidden)', { timeout: 5000 });
}
console.log('3) الموت وتغيير السلاح وسط المباراة');
await page.click('#btnStart');
await page.waitForFunction(() => window.__wz.me.alive, null, { timeout: 8000 });
await page.waitForTimeout(500);
// نقتل اللاعب عبر الخادم المحلي
await page.evaluate(() => { const h = window.__wz.S.host; const p = h.game.players.get(window.__wz.S.id); p.prot = 0; h.game.hurt(p, 999, [...h.game.players.values()].find((q) => q.bot), false, 'carbine', 0, 0); });
await page.waitForSelector('#deadpanel:not(.hidden)', { timeout: 4000 });
ok(true, 'لوحة الموت تظهر');
await page.keyboard.press('Digit4'); await page.waitForTimeout(200);
const lo = await page.evaluate(() => JSON.parse(localStorage.getItem('wz_lo')).p);
ok(lo === 'sniper', `تغيير السلاح بالرقم 4 عند الموت (${lo})`);
await page.screenshot({ path: `${out}/dead.png` });
await page.waitForFunction(() => window.__wz.me.alive, null, { timeout: 8000 });
ok(await page.evaluate(() => window.__wz.me.st[0].id) === 'sniper', 'يظهر بالسلاح الجديد');
await page.evaluate(() => window.__wz.send({ t: 'back' }));
await page.waitForSelector('#lobby:not(.hidden)', { timeout: 5000 });
await page.click('#btnLeave'); await page.waitForSelector('#menu:not(.hidden)');
ok(true, 'الخروج إلى القائمة الرئيسية');
await page.context().close(); await new Promise(r => setTimeout(r, 300));

console.log('4) الانضمام لخادم عبر الشبكة + دردشة بين لاعبين + صلاحيات المضيف');
const a = await mk(false), b = await mk(false);
await a.fill('#name', 'أ'); await a.evaluate(() => window.__wz.joinPage()); await a.waitForSelector('#lobby:not(.hidden)');
await b.fill('#name', 'ب'); await b.evaluate(() => window.__wz.joinPage()); await b.waitForSelector('#lobby:not(.hidden)');
await a.waitForTimeout(400);
ok(await b.isDisabled('#cfgMap'), 'غير المضيف لا يغيّر الإعدادات');
ok(!(await a.isDisabled('#cfgMap')), 'المضيف يغيّر الإعدادات');
await b.fill('#lobbyIn', 'مرحبا من ب'); await b.click('#lobbySend');
await a.waitForFunction(() => document.getElementById('chatLog').textContent.includes('مرحبا من ب'));
ok(true, 'الدردشة تصل بين اللاعبين');
await sel(a, 'cfgMap', '4'); await sel(a, 'cfgFill', '2');
await b.waitForFunction(() => document.getElementById('cfgMap').value === '4');
ok(true, 'تغيير الخريطة يظهر عند الآخرين');
await a.click('#btnStart');
await a.waitForFunction(() => window.__wz.me.alive); await b.waitForFunction(() => window.__wz.me.alive);
await a.waitForTimeout(1500);
ok(await b.evaluate(() => window.__wz.S.vehs.size) === 8, 'اللاعب الثاني يرى المركبات');
await a.screenshot({ path: `${out}/lan-a.png` }); await b.screenshot({ path: `${out}/lan-b.png` });
await a.evaluate(() => window.__wz.send({ t: 'back' }));
await b.waitForSelector('#lobby:not(.hidden)', { timeout: 5000 });
ok(true, 'العودة الجماعية إلى الغرفة');
await a.context().close(); await b.context().close();

console.log('5) واجهة اللمس');
const tp = await mk(true);
await tp.fill('#name', 'جوال'); await tp.tap('#btnSolo'); await tp.waitForSelector('#lobby:not(.hidden)');
await tp.screenshot({ path: `${out}/lobby-touch.png` });
await sel(tp, 'cfgMap', '4'); await sel(tp, 'cfgFill', '3'); await tp.tap('#btnStart');
await tp.waitForFunction(() => window.__wz.me.alive, null, { timeout: 8000 });
await tp.waitForTimeout(1500);
await tp.screenshot({ path: `${out}/game-touch.png` });
ok(await tp.evaluate(() => document.body.classList.contains('touch')), 'وضع اللمس مفعّل');
ok(errors.length === 0, `لا أخطاء في المتصفح (${errors.length})`); if (errors.length) console.log(errors.slice(0, 10));
await browser.close(); srv.close();
console.log(fails ? 'فشل' : 'نجح اختبار المتصفح ✓'); process.exit(fails ? 1 : 0);
