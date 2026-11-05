// اختبار منطق المركبات والدردشة وصلاحيات المضيف والذخيرة — مباشرة على Game بزمن وهمي
import { Game } from '../shared/game.js';
import { VEH, seatWorld, vehStep, l2w } from '../shared/vehicle.js';
import { buildMap } from '../shared/maps.js';

let fails = 0;
const ok = (c, m) => { if (!c) { console.log('  ✗ ' + m); fails++; } else console.log('  ✓ ' + m); };
let T = 1000; const g = new Game(() => T);
const mk = (name) => { const c = { log: [], closed: false, send(s) { this.log.push(JSON.parse(s)); }, close() { this.closed = true; } }; const p = g.connect(c); p.name = name; c.p = p; return c; };
const send = (c, o) => g.onMessage(c.p, JSON.stringify(o));
const last = (c, t) => [...c.log].reverse().find(m => m.t === t);
const step = (sec) => { for (let i = 0; i < sec * 20; i++) { T += 50; g.tick(); } };

const A = mk('A'), B = mk('B'), C = mk('C');
send(A, { t: 'team', v: 0 }); send(B, { t: 'team', v: 1 }); send(C, { t: 'team', v: 0 });
console.log('1) الدردشة وصلاحيات المضيف');
send(B, { t: 'chat', m: 'مرحبا' });
ok(last(A, 'chat') && last(A, 'chat').m === 'مرحبا', 'الدردشة تصل للجميع');
T += 700; send(A, { t: 'chat', m: 'فريق', to: 1 });
ok(last(C, 'chat').m === 'فريق' && last(B, 'chat').m !== 'فريق', 'دردشة الفريق لا تصل للخصم');
send(B, { t: 'chat', m: 'x' }); send(B, { t: 'chat', m: 'y' });
ok(g.cfg.ff === 0, 'ff مطفأ افتراضيًا');
send(B, { t: 'cfg', ff: 1 }); ok(g.cfg.ff === 0, 'غير المضيف لا يغيّر الإعدادات');
send(A, { t: 'cfg', map: 4, ff: 1, veh: 1, diff: 2, resp: 5, regen: 0, resp2: 9 });
ok(g.cfg.map === 4 && g.cfg.ff === 1 && g.cfg.diff === 2 && g.cfg.resp === 5 && g.cfg.regen === 0, 'المضيف يغيّر الخريطة والإعدادات');
send(A, { t: 'cfg', ff: 0, regen: 1, diff: 99, resp: 7 });
ok(g.cfg.diff === 2 && g.cfg.resp === 5, 'قيم غير صالحة تُهمل');
send(A, { t: 'shuffle' }); ok(g.humans().filter(p => p.team === 0).length >= 1, 'خلط الفرق');
send(A, { t: 'team', v: 0 }); send(B, { t: 'team', v: 1 }); send(C, { t: 'team', v: 0 });
send(A, { t: 'team', id: C.p.id, v: 1 }); ok(C.p.team === 1, 'المضيف ينقل لاعبًا');
send(A, { t: 'team', id: C.p.id, v: 0 });
send(A, { t: 'start' });
ok(g.phase === 'playing' && g.vehs.length === 8, "بدأت المباراة بثماني مركبات");

console.log('2) الركوب والقيادة');
step(3);
const v = g.vehs[0];
const P = A.p;
P.x = v.x + 2.2; P.z = v.z - 1; P.y = 0; P.prot = 0;
send(A, { t: 'use' });
ok(P.veh === v.id && P.seat === 0, 'اللاعب ركب مقعد السائق');
ok(last(A, 'seat') && last(A, 'seat').s === 0, 'وصلت رسالة seat');
// الراكب الثاني من نفس الفريق
const Q = C.p; Q.x = v.x + 2.2; Q.z = v.z + 1.5; Q.y = 0; send(C, { t: 'use' });
ok(Q.veh === v.id && Q.seat >= 1, `زميل ركب (مقعد ${Q.seat})`);
// الراكب يطلق النار من مقعده (رشاش الصندوق)
{ const before = A.log.filter(m => m.t === 'shot').length; Q.credit = [2, 2]; send(C, { t: 'shoot', w: 0, d: [[0, 0.05, -1]] }); ok(A.log.filter(m => m.t === 'shot').length > before, 'راكب الصندوق يُطلق النار ويراها الآخرون'); }
// خصم لا يستطيع الركوب
const E = B.p; E.x = v.x - 2.2; E.z = v.z - 1; E.y = 0; send(B, { t: 'use' });
ok(E.veh === -1, 'الخصم لا يركب مركبة فريق آخر');
// سياقة: نحاكي عميل السائق
let cl = { x: v.x, z: v.z, h: v.h, vx: 0, vz: 0, steer: 0, tires: 0, dyn: null }; const map = g.map;
let crashSent = 0;
for (let i = 0; i < 60; i++) {
  T += 50; const cr = vehStep(map, cl, { thr: 1, steer: 0, hb: false }, 0.05); crashSent = Math.max(crashSent, cr);
  send(A, { t: 'v', e: P.epoch, x: cl.x, z: cl.z, h: cl.h, vx: cl.vx, vz: cl.vz, s: cl.steer, sp: cl.speed, cr });
}
ok(Math.hypot(v.x - cl.x, v.z - cl.z) < 0.01 && Math.abs(v.speed) > 5, `الخادم يتبع السائق (سرعة ${v.speed.toFixed(1)})`);
const [sx, , sz] = seatWorld(v, Q.seat);
ok(Math.hypot(Q.x - sx, Q.z - sz) < 0.01, 'الراكب يتحرك مع المركبة');
// قفزة غير منطقية ترفض
const bx = v.x; send(A, { t: 'v', e: P.epoch, x: v.x + 80, z: v.z, h: v.h, vx: 0, vz: 0, s: 0, sp: 0 });
ok(v.x === bx && last(A, 'vfix'), 'الانتقال المفاجئ يرفض ويُصحَّح');
// الإصابة بنيران عدو: الراكب في الصندوق قابل للإصابة
console.log('3) الإصابات والإطارات والانفجار');
const hp0 = Q.hp; Q.prot = 0;
g.hurt(Q, 30, E, false, 'carbine', E.x, E.z); ok(Q.hp === hp0 - 30, 'راكب المركبة يُصاب');
// رصاص على الإطار
E.x = v.x; E.z = v.z - 20; E.y = 0; E.alive = true; E.prot = 0; E.crouch = false;
let tiresBefore = v.tires;
g.map.boxes = g.map.boxes.filter(b => !(b.y0 < 3 && b.x1 > v.x - 20 && b.x0 < v.x + 20 && b.z1 > v.z - 20 && b.z0 < v.z + 20 && b.y1 < 5)); // أخلِ الساحة من الدعائم لضمان خط الرؤية
for (let i = 0; i < 8; i++) {
  const [wx, , wz] = l2w(v, -0.95, 0, -1.55), [ex, , ez] = l2w(v, -7, 0, -1.55);
  E.x = ex; E.z = ez;
  const dx = wx - E.x, dz = wz - E.z, l = Math.hypot(dx, dz);
  E.st = g.players.get(E.id).st; E.credit = [2, 2];
  g.processShot(E, 0, [[dx / l, (0.35 - 1.62) / l, dz / l]]);
}
ok(v.tires !== tiresBefore, `الإطار ينفجر (قناع ${v.tires})`);
ok(g.vehs[1].hp === VEH.hp, 'مركبة أخرى لم تتأثر');
// انفجار كامل
const hpQ = Q.hp; v.lastAtk = E.id;
g.hurtVeh(v, 9999, E);
ok(v.dead, 'المركبة انفجرت'); ok(!P.alive && !Q.alive, 'ركاب المركبة ماتوا');
ok(E.kills >= 1, 'القاتل حصل على نقاط');
ok(last(A, 'vboom'), 'أُرسل حدث الانفجار');
step(1);
const vs = last(A, 'snap').vs[0]; ok((vs[8] & 2) === 2, 'اللقطة تُظهر الحطام');
console.log('4) إعادة ظهور ذخيرة وعودة اللاعب');
ok(g.drops.length >= 2, `سقطت ذخيرة (${g.drops.length})`);
step(6);
ok(P.alive && P.veh === -1, 'اللاعب عاد للحياة عند نقطة التظهير بعد المهلة');
const d = g.drops.find(x => x.w === E.lo.p) || null;
// E يموت ويسقط ذخيرة، ولاعب بنفس السلاح يلتقطها
const F = C.p; F.x = 5; F.z = 5; F.y = 0; F.am = [10, 0, 5, 0]; const D = g.drops.length;
g.drops.push({ id: 999, x: 5, y: 0, z: 5, w: F.lo.p, n: 40, exp: T + 10000 });
g.drops.push({ id: 998, x: 5.2, y: 0, z: 5, w: 'sniper', n: 5, exp: T + 10000 });
step(1);
ok(!g.drops.find(x => x.id === 999), 'التقط الذخيرة من نفس السلاح');
ok(g.drops.find(x => x.id === 998), 'لا يلتقط ذخيرة سلاح مختلف');
ok(last(C, 'ammo') && last(C, 'ammo').n === 40, 'وصلت رسالة ammo');
console.log('5) نظام الدعم: طرد وتحويل المضيف وإعادة التشغيل');
send(A, { t: 'give', id: B.p.id }); ok(g.hostId === B.p.id, 'نقل المضيف');
send(A, { t: 'kick', id: C.p.id }); ok(!C.closed, 'غير المضيف لا يطرد');
send(B, { t: 'kick', id: C.p.id }); ok(C.closed && !g.players.has(C.p.id), 'المضيف يطرد');
send(B, { t: 'restart', map: 1 }); ok(g.phase === 'playing' && g.cfg.map === 1 && g.vehs.length === 0, 'إعادة التشغيل بخريطة جديدة بلا مركبات');
g.cfg.map = 4; send(B, { t: 'back' }); ok(g.phase === 'lobby', 'العودة للوبي');
for (let i = 0; i < 40; i++) step(1);
console.log(fails ? `فشل: ${fails}` : 'نجحت اختبارات المركبات ✓');
process.exit(fails ? 1 : 0);
