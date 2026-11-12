// يصدّر الخرائط والأسلحة إلى JSON لمشروع Godot التجريبي
import { buildMap, MAP_COUNT, MAP_NAMES } from './data/maps.js';
import { WEAPONS } from './data/weapons.js';
import fs from 'fs';
const r = (v) => Math.round(v * 1000) / 1000;
const maps = [];
for (let i = 0; i < MAP_COUNT; i++) {
  const m = buildMap(i), t = m.theme;
  const bx = (a) => a.map((b) => [r(b.x0), r(b.y0), r(b.z0), r(b.x1), r(b.y1), r(b.z1), b.k | 0]);
  maps.push({ name: MAP_NAMES[i], W: m.W, D: m.D, boxes: bx(m.boxes), deco: bx(m.deco), red: m.red, blue: m.blue,
    vehicles: m.vehicles.map((v) => [r(v.x), r(v.z), r(v.h || 0), v.t | 0]),
    theme: { sky: t.sky, hor: t.hor, ground: t.ground, fog: t.fog, fogN: t.fogN, fogF: t.fogF, sun: t.sun, amb: t.amb, sunI: t.sunI, cols: t.cols } });
}
const weapons = Object.entries(WEAPONS).map(([id, w]) => ({ id, name: w.name, kind: w.kind, dmg: w.dmg, head: w.head, rpm: w.rpm, auto: !!w.auto, pellets: w.pellets, mag: w.mag, mags: w.mags, reload: w.reload, hip: w.hip, ads: w.ads, recoilV: w.recoilV, recoilH: w.recoilH, range: w.range, minMul: w.minMul, move: w.move, zoom: w.zoom }));
fs.writeFileSync('godot/data/game.json', JSON.stringify({ maps, weapons }));
console.log('ok', maps.length, weapons.length, fs.statSync('godot/data/game.json').size);
