// اختبار المميزات الإضافية: وضع السيطرة، حزم الإسعاف، سلاسل القتل، إعدادات الوقت
import { Game } from '../shared/game.js';
import { MAP_COUNT } from '../shared/maps.js';
import { blocked, groundAt, PLAYER } from '../shared/physics.js';
let fails = 0;
const ok = (c, m) => { if (!c) { console.log('  ✗ ' + m); fails++; } else console.log('  ✓ ' + m); };
let T = 1000; const g = new Game(() => T);
const mk = (name) => { const c = { log: [], send(s) { this.log.push(JSON.parse(s)); }, close() {} }; const p = g.connect(c); p.name = name; c.p = p; return c; };
const send = (c, o) => g.onMessage(c.p, JSON.stringify(o));
const last = (c, t) => [...c.log].reverse().find(m => m.t === t);
const step = (sec) => { for (let i = 0; i < sec * 20; i++) { T += 50; g.tick(); } };
const A = mk('A'), B = mk('B'), C = mk('C');
send(A, { t: 'team', v: 0 }); send(B, { t: 'team', v: 1 }); send(C, { t: 'team', v: 1 });

console.log('1) الإعدادات');
send(A, { t: 'cfg', mode: 1, tod: 2, streaks: 0 });
ok(g.cfg.mode === 1 && g.cfg.tod === 2 && g.cfg.streaks === 0, 'المضيف يغيّر الوضع والوقت والمكافآت');
send(A, { t: 'cfg', mode: 7, tod: 9, streaks: 5 }); ok(g.cfg.mode === 1 && g.cfg.tod === 2, 'قيم غير صالحة تُهمل');
send(B, { t: 'cfg', mode: 0 }); ok(g.cfg.mode === 1, 'غير المضيف لا يغيّر');
send(A, { t: 'cfg', streaks: 1 });

console.log('2) حزم الإسعاف والمناطق في كل الخرائط');
for (let m = 0; m < MAP_COUNT; m++) {
  g.cfg.map = m; send(A, { t: 'start' });
  const st = last(A, 'start');
  ok(st && st.kits.length >= 2 && st.kits.length % 2 === 0, `خريطة ${m}: ${g.kits.length} حزم إسعاف مرسلة في start`);
  ok(g.kits.every(k => !blocked(g.map, k.x, 0, k.z, PLAYER.h) && groundAt(g.map, k.x, k.z, 0) === 0), `خريطة ${m}: الحزم في أماكن صالحة`);
  ok(g.zones.length === 3 && g.zones.every(z => !blocked(g.map, z[0], 0, z[1], PLAYER.h)), `خريطة ${m}: ثلاث مناطق سيطرة صالحة`);
  send(A, { t: 'back' });
}

console.log('3) حزمة إسعاف');
g.cfg.map = 2; send(A, { t: 'start' }); step(3);
const P = A.p, k = g.kits[0];
P.x = k.x; P.z = k.z; P.y = 0; P.hp = 30; P.prot = 0; step(0.2);
ok(P.hp === 80 && !k.up, 'عالج 50 نقطة واختفت الحزمة');
ok(last(A, 'heal') && last(A, 'kit').up === 0, 'أُرسلت heal وkit');
P.hp = 40; step(1); ok(P.hp === 40 || P.hp > 40 && P.hp < 60, 'لا علاج من حزمة غير جاهزة');
P.x = 0; P.z = 0; step(26); ok(k.up && last(A, 'kit').up === 1, 'عادت الحزمة بعد 25 ثانية');
ok(/^[01]+$/.test(last(A, 'snap').kt) && last(A, 'snap').kt.length === g.kits.length, 'حالة الحزم في اللقطة');

console.log('4) وضع السيطرة');
const z0 = g.zones[g.zoneIdx]; g.score = [0, 0];
for (const q of g.players.values()) { q.hp = 100; q.alive = true; q.prot = T + 99999; q.x = 0; q.z = 0; }
P.x = z0[0]; P.z = z0[1]; P.y = 0; B.p.x = g.map.red[0][0]; B.p.z = g.map.red[0][1]; C.p.x = g.map.red[1][0]; C.p.z = g.map.red[1][1];
step(4); ok(g.score[0] >= 3 && g.zoneState === 0, `الأحمر يحتل المنطقة (${g.score[0]} نقاط)`);
const s0 = g.score[0]; B.p.x = z0[0] + 1; B.p.z = z0[1]; step(3);
ok(g.zoneState === 2 && g.score[0] === s0 && g.score[1] === 0, 'المنطقة متنازع عليها: لا نقاط');
const zn = last(A, 'snap').zn; ok(zn && zn.length === 6 && zn[3] === 7, 'لقطة المنطقة');
const idx0 = g.zoneIdx; step(61); ok(g.zoneIdx !== idx0 && last(A, 'zone'), 'المنطقة تتحرك بعد 60 ثانية');
ok(g.limit() === 150, 'حد النقاط في وضع السيطرة ×5');

console.log('5) سلسلة القتل والدرع');
g.cfg.mode = 0; const E = B.p, K = A.p; K.hp = 100; K.prot = 0;
const kill = () => { E.alive = true; E.hp = 100; E.prot = 0; g.hurt(E, 999, K, false, 'carbine', K.x, K.z); };
kill(); kill(); ok(g.uav[0] <= T, 'لا طائرة قبل 3 قتلات'); kill();
ok(g.uav[0] > T && last(A, 'ks') && last(A, 'ks').k === 'uav', 'طائرة استطلاع عند 3 قتلات');
step(0.1); ok(last(A, 'snap').uv[0] > 0, 'اللقطة تحمل مدة الطائرة');
kill(); kill(); ok(K.shield === 60 && last(A, 'ks').k === 'shield', 'درع عند 5 قتلات');
K.hp = 100; K.prot = 0; E.alive = true; g.hurt(K, 40, E, false, 'carbine', 0, 0);
ok(K.hp === 100 && K.shield === 20, 'الدرع يمتص الضرر أولًا');
g.hurt(K, 50, E, false, 'carbine', 0, 0); ok(K.shield === 0 && K.hp === 70, 'الباقي يخصم من الصحة');
kill(); kill(); kill(); ok(K.hp === 100 && K.grenades === 4 && last(A, 'ks').k === 'supply', 'إمداد عند 8 قتلات');
K.alive = true; g.kill(K, E, 'carbine', false); ok(K.streak === 0 && K.shield === 0, 'الموت يصفّر السلسلة');
g.cfg.streaks = 0; E.streak = 0; K.alive = true; K.hp = 100; K.prot = 0; const u0 = g.uav[1]; for (let i = 0; i < 3; i++) { K.alive = true; K.hp = 100; K.prot = 0; g.hurt(K, 999, E, false, 'carbine', 0, 0); } ok(g.uav[1] === u0, 'المكافآت تتعطل من الإعدادات');

console.log('6) الطائرة المسيّرة');
g.cfg.map = 2; g.cfg.mode = 0; send(A, { t: 'back' }); send(A, { t: 'start' }); step(3);
const PA = A.p, PB = B.p; PA.alive = true; PA.hp = 100; PA.prot = 0; PB.alive = true; PB.hp = 100; PB.prot = T + 99999;
PA.team = 0; PB.team = 1;
send(A, { t: 'drone', on: 1 }); ok(PA.drone && last(A, 'dron').on === 1, 'إطلاق الطائرة');
const d0 = { ...PA.drone };
send(A, { t: 'dp', x: d0.x + 0.5, y: d0.y, z: d0.z, yaw: 1 }); ok(Math.abs(PA.drone.x - (d0.x + 0.5)) < 0.01, 'تحريك مسموح');
step(0.1); send(A, { t: 'dp', x: d0.x + 60, y: d0.y, z: d0.z, yaw: 1 }); ok(last(A, 'dfix') && Math.abs(PA.drone.x - (d0.x + 0.5)) < 0.01, 'القفز المفاجئ يرفض');
send(A, { t: 'dp', x: 9999, y: 5, z: 0, yaw: 0 }); ok(last(A, 'dfix'), 'خارج الحدود يرفض');
send(A, { t: 'drone', on: 1 }); ok(true, 'إطلاق ثانٍ لا يسبب خطأ');
PB.x = PA.drone.x + 8; PB.z = PA.drone.z; PB.y = 0; PB.reveal = 0;
g.map.boxes = g.map.boxes.filter(b => !(b.y0 < 5 && b.x1 > PA.x - 30 && b.x0 < PA.x + 30 && b.z1 > PA.z - 30 && b.z0 < PA.z + 30));
step(1.5); ok(PB.reveal > T, 'الطائرة تكشف العدو القريب');
ok(last(A, 'snap').dn.length === 1 && last(A, 'snap').dn[0][0] === PA.id, 'الطائرة في اللقطة');
// إسقاط الطائرة بالرصاص من العدو
PB.alive = true; PB.prot = 0; PB.st = g.players.get(PB.id).st; PB.credit = [2, 2];
const dx = PA.drone.x - PB.x, dy = PA.drone.y - (PB.y + 1.62), dz = PA.drone.z - PB.z, L = Math.hypot(dx, dy, dz);
for (let i = 0; i < 4; i++) { PB.alive = true; PB.prot = 0; g.processShot(PB, 0, [[dx / L, dy / L, dz / L]]); }
ok(!PA.drone && last(A, 'dron').why === 'shot', 'العدو أسقط الطائرة');
send(A, { t: 'drone', on: 1 }); ok(!PA.drone, 'لا إطلاق أثناء فترة الانتظار');
T += 41000; send(A, { t: 'drone', on: 1 }); ok(PA.drone, 'يمكن الإطلاق بعد الانتظار');
// تفجير انتحاري
PB.alive = true; PB.hp = 100; PB.prot = 0; PB.x = PA.drone.x + 2; PB.z = PA.drone.z; PB.y = 0; PA.drone.y = 1.2;
send(A, { t: 'dboom' }); ok(!PA.drone && PB.hp < 100, `التفجير يؤذي العدو (${PB.hp})`);
T += 41000; send(A, { t: 'drone', on: 1 }); PA.alive = false; step(0.2); ok(!PA.drone, 'موت الطيار يدمّر الطائرة');
console.log(fails ? `فشل: ${fails}` : 'نجحت اختبارات المميزات الإضافية ✓');
process.exit(fails ? 1 : 0);
