import { buildMap } from '../shared/maps.js';
import { moveBody } from '../shared/physics.js';
// الصعود على الدرج والمنصة في المستودع (خريطة 0): الدرج عند x من 7 إلى 14.2 و z=-16
const m = buildMap(0);
const p = { x: 15, y: 0, z: -16, vy: 0, onGround: true };
let mx = 0; for (let i = 0; i < 200; i++) { moveBody(m, p, -4.6, 0, 1 / 60, false, false); mx = Math.max(mx, p.y); }
console.log('أقصى ارتفاع على الدرج:', mx.toFixed(2));
if (mx < 2.3) { console.log('✗ لم يصعد المنصة'); process.exit(1); }
// الصعود لبرج الصحراء
const d = buildMap(1); const q = { x: 0, y: 0, z: -15, vy: 0, onGround: true };
let mq = 0; for (let i = 0; i < 200; i++) { moveBody(d, q, 0, 4.6, 1 / 60, false, false); mq = Math.max(mq, q.y); }
console.log('برج الصحراء:', mq.toFixed(2)); if (mq < 3.1) { console.log('✗ لم يصعد البرج'); process.exit(1); }
// القفز فوق صندوق ارتفاعه 1.2
const r = { x: -6, y: 0, z: 0, vy: 0, onGround: true };
for (let i = 0; i < 200; i++) moveBody(m, r, 4.6, 0, 1 / 60, i === 10 || i === 60, false);
console.log('✓ الفيزياء سليمة');

// الدرج المقابل (النسخ المتماثلة) في خرائط 0 و3
{ const a = { x: -15, y: 0, z: 16, vy: 0, onGround: true }; let h = 0; for (let i = 0; i < 200; i++) { moveBody(m, a, 4.6, 0, 1 / 60, false, false); h = Math.max(h, a.y); } if (h < 2.3) { console.log('✗ الدرج الجنوبي'); process.exit(1); } }
{ const a3 = buildMap(3); const a = { x: -15, y: 0, z: -26, vy: 0, onGround: true }; let h = 0; for (let i = 0; i < 200; i++) { moveBody(a3, a, -4.6, 0, 1 / 60, false, false); h = Math.max(h, a.y); } console.log('منصة القطب:', h.toFixed(2)); if (h < 1.9) { console.log('✗ منصة القطب'); process.exit(1); } }
{ const a3 = buildMap(3); const a = { x: 15, y: 0, z: 26, vy: 0, onGround: true }; let h = 0; for (let i = 0; i < 200; i++) { moveBody(a3, a, 4.6, 0, 1 / 60, false, false); h = Math.max(h, a.y); } if (h < 1.9) { console.log('✗ منصة القطب المقابلة'); process.exit(1); } }
