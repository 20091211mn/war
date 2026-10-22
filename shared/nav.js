// شبكة ملاحة بسيطة للبوتات + اختبار الاتصال
import { PLAYER } from './physics.js';

export function buildNav(map) {
  const cell = 1;
  const cols = Math.ceil(map.W / cell), rows = Math.ceil(map.D / cell);
  const grid = new Uint8Array(cols * rows); // 1 = محجوب
  const ox = -map.W / 2, oz = -map.D / 2, e = PLAYER.r + 0.1;
  for (let j = 0; j < rows; j++) for (let i = 0; i < cols; i++) {
    const cx = ox + (i + 0.5) * cell, cz = oz + (j + 0.5) * cell;
    let bl = 0;
    for (const b of map.boxes) {
      if (b.y1 > PLAYER.step && b.y0 < PLAYER.h && cx > b.x0 - e && cx < b.x1 + e && cz > b.z0 - e && cz < b.z1 + e) { bl = 1; break; }
    }
    if (cx < ox + e || cx > -ox - e || cz < oz + e || cz > -oz - e) bl = 1;
    grid[j * cols + i] = bl;
  }
  return { cell, cols, rows, grid, ox, oz };
}

export function cellOf(nav, x, z) {
  const i = Math.max(0, Math.min(nav.cols - 1, Math.floor((x - nav.ox) / nav.cell)));
  const j = Math.max(0, Math.min(nav.rows - 1, Math.floor((z - nav.oz) / nav.cell)));
  return j * nav.cols + i;
}
export function cellCenter(nav, idx) {
  return [nav.ox + ((idx % nav.cols) + 0.5) * nav.cell, nav.oz + (Math.floor(idx / nav.cols) + 0.5) * nav.cell];
}

export function nearestFree(nav, idx) {
  if (!nav.grid[idx]) return idx;
  const ci = idx % nav.cols, cj = Math.floor(idx / nav.cols);
  for (let r = 1; r < 8; r++) {
    for (let dj = -r; dj <= r; dj++) for (let di = -r; di <= r; di++) {
      if (Math.max(Math.abs(di), Math.abs(dj)) !== r) continue;
      const i = ci + di, j = cj + dj;
      if (i < 0 || j < 0 || i >= nav.cols || j >= nav.rows) continue;
      if (!nav.grid[j * nav.cols + i]) return j * nav.cols + i;
    }
  }
  return -1;
}

class Heap {
  constructor() { this.a = []; }
  push(k, v) { const a = this.a; a.push([k, v]); let i = a.length - 1; while (i > 0) { const p = (i - 1) >> 1; if (a[p][0] <= a[i][0]) break; [a[p], a[i]] = [a[i], a[p]]; i = p; } }
  pop() { const a = this.a; const top = a[0]; const last = a.pop(); if (a.length) { a[0] = last; let i = 0; for (;;) { let l = 2 * i + 1, r = l + 1, m = i; if (l < a.length && a[l][0] < a[m][0]) m = l; if (r < a.length && a[r][0] < a[m][0]) m = r; if (m === i) break; [a[m], a[i]] = [a[i], a[m]]; i = m; } } return top; }
  get size() { return this.a.length; }
}

// يرجع مصفوفة نقاط [x,z] أو null
export function findPath(nav, sx, sz, ex, ez) {
  let s = nearestFree(nav, cellOf(nav, sx, sz)), g = nearestFree(nav, cellOf(nav, ex, ez));
  if (s < 0 || g < 0) return null;
  if (s === g) return [cellCenter(nav, g)];
  const n = nav.cols * nav.rows, cols = nav.cols;
  // مخازن مؤقتة قابلة لإعادة الاستخدام (تفادي تخصيص ذاكرة كل استدعاء في الخرائط الكبيرة)
  let sc = nav._sc; if (!sc) sc = nav._sc = { gs: new Float32Array(n), from: new Int32Array(n), closed: new Uint8Array(n), ver: new Uint32Array(n), v: 0 };
  const gs = sc.gs, from = sc.from, closed = sc.closed, ver = sc.ver; const V = ++sc.v;
  const touch = (i) => { if (ver[i] !== V) { ver[i] = V; gs[i] = Infinity; from[i] = -1; closed[i] = 0; } };
  let budget = 14000;
  const gi = g % cols, gj = Math.floor(g / cols);
  const h = (i) => { const dx = Math.abs((i % cols) - gi), dz = Math.abs(Math.floor(i / cols) - gj); return (dx + dz) + (Math.SQRT2 - 2) * Math.min(dx, dz); };
  let g2 = -1; const open = new Heap(); touch(s); gs[s] = 0; open.push(h(s), s);
  const grid = nav.grid, rows = nav.rows;
  while (open.size) {
    const [, cur] = open.pop();
    if (closed[cur]) continue; closed[cur] = 1;
    if (cur === g || --budget < 0) { if (cur !== g) { g2 = cur; } break; }
    const ci = cur % cols, cj = Math.floor(cur / cols);
    for (let dj = -1; dj <= 1; dj++) for (let di = -1; di <= 1; di++) {
      if (!di && !dj) continue;
      const i = ci + di, j = cj + dj;
      if (i < 0 || j < 0 || i >= cols || j >= rows) continue;
      const ni = j * cols + i;
      if (grid[ni]) continue; touch(ni); if (closed[ni]) continue;
      if (di && dj && (grid[cj * cols + i] || grid[j * cols + ci])) continue;
      const ng = gs[cur] + (di && dj ? Math.SQRT2 : 1);
      if (ng < gs[ni]) { gs[ni] = ng; from[ni] = cur; open.push(ng + h(ni), ni); }
    }
  }
  touch(g); if (g2 >= 0) g = g2; if (from[g] < 0) return null;
  const path = []; let c = g;
  while (c !== s && c >= 0) { path.push(cellCenter(nav, c)); c = from[c]; }
  path.reverse();
  return path;
}

export function reachableFrom(nav, startIdx) {
  const seen = new Uint8Array(nav.cols * nav.rows);
  const st = nearestFree(nav, startIdx); if (st < 0) return seen;
  const q = [st]; seen[st] = 1;
  while (q.length) {
    const c = q.pop(); const ci = c % nav.cols, cj = Math.floor(c / nav.cols);
    for (const [di, dj] of [[1, 0], [-1, 0], [0, 1], [0, -1]]) {
      const i = ci + di, j = cj + dj;
      if (i < 0 || j < 0 || i >= nav.cols || j >= nav.rows) continue;
      const ni = j * nav.cols + i;
      if (!seen[ni] && !nav.grid[ni]) { seen[ni] = 1; q.push(ni); }
    }
  }
  return seen;
}
