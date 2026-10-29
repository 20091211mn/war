// فيزياء مشتركة: الحركة، التصادم، الأشعة
export const PLAYER = {
  r: 0.35, h: 1.75, hc: 1.2, eye: 1.62, eyec: 1.1, step: 0.6, headH: 0.3,
  walk: 4.6, sprint: 7.0, crouch: 2.4, jump: 6.2, grav: 18,
};

// ---- شبكة تسريع (broadphase): كل خلية 8م تحمل الصناديق التي تلامسها ----
const CS = 8, MARGIN = 0.6;
export function gridOf(map) {
  const bs = map.boxes, g = map._g;
  if (g && g.src === bs && g.n === bs.length) return g;
  const ox = -map.W / 2 - 3, oz = -map.D / 2 - 3, nx = Math.ceil((map.W + 6) / CS) + 1, nz = Math.ceil((map.D + 6) / CS) + 1;
  const cells = new Array(nx * nz); for (let i = 0; i < cells.length; i++) cells[i] = [];
  const ci = (v, o, n) => Math.max(0, Math.min(n - 1, Math.floor((v - o) / CS)));
  for (let k = 0; k < bs.length; k++) {
    const b = bs[k], i0 = ci(b.x0 - MARGIN, ox, nx), i1 = ci(b.x1 + MARGIN, ox, nx), j0 = ci(b.z0 - MARGIN, oz, nz), j1 = ci(b.z1 + MARGIN, oz, nz);
    for (let j = j0; j <= j1; j++) for (let i = i0; i <= i1; i++) cells[j * nx + i].push(b);
  }
  const ng = { src: bs, n: bs.length, ox, oz, nx, nz, cells, mark: new Map() };
  map._g = ng; return ng;
}
function cellAt(g, x, z) {
  const i = Math.max(0, Math.min(g.nx - 1, Math.floor((x - g.ox) / CS))), j = Math.max(0, Math.min(g.nz - 1, Math.floor((z - g.oz) / CS)));
  return g.cells[j * g.nx + i];
}
export function boxesNear(map, x, z) { return cellAt(gridOf(map), x, z); }

export function rayBox(ox, oy, oz, dx, dy, dz, x0, y0, z0, x1, y1, z1) {
  let tmin = 0, tmax = Infinity;
  if (Math.abs(dx) < 1e-9) { if (ox < x0 || ox > x1) return Infinity; }
  else {
    let t1 = (x0 - ox) / dx, t2 = (x1 - ox) / dx;
    if (t1 > t2) { const t = t1; t1 = t2; t2 = t; }
    if (t1 > tmin) tmin = t1; if (t2 < tmax) tmax = t2;
    if (tmin > tmax) return Infinity;
  }
  if (Math.abs(dy) < 1e-9) { if (oy < y0 || oy > y1) return Infinity; }
  else {
    let t1 = (y0 - oy) / dy, t2 = (y1 - oy) / dy;
    if (t1 > t2) { const t = t1; t1 = t2; t2 = t; }
    if (t1 > tmin) tmin = t1; if (t2 < tmax) tmax = t2;
    if (tmin > tmax) return Infinity;
  }
  if (Math.abs(dz) < 1e-9) { if (oz < z0 || oz > z1) return Infinity; }
  else {
    let t1 = (z0 - oz) / dz, t2 = (z1 - oz) / dz;
    if (t1 > t2) { const t = t1; t1 = t2; t2 = t; }
    if (t1 > tmin) tmin = t1; if (t2 < tmax) tmax = t2;
    if (tmin > tmax) return Infinity;
  }
  return tmin;
}

// أقرب تقاطع مع الخريطة (Infinity إن لم يوجد ضمن maxT)
let stampId = 0;
export function raycastMap(map, ox, oy, oz, dx, dy, dz, maxT) {
  const g = gridOf(map);
  let best = maxT;
  // اجتياز الخلايا بخوارزمية DDA ثنائية الأبعاد
  let cx = Math.max(0, Math.min(g.nx - 1, Math.floor((ox - g.ox) / CS))), cz = Math.max(0, Math.min(g.nz - 1, Math.floor((oz - g.oz) / CS)));
  const sx = dx > 0 ? 1 : -1, sz = dz > 0 ? 1 : -1;
  const tdx = Math.abs(dx) > 1e-9 ? CS / Math.abs(dx) : Infinity, tdz = Math.abs(dz) > 1e-9 ? CS / Math.abs(dz) : Infinity;
  let tmx = Math.abs(dx) > 1e-9 ? ((dx > 0 ? (cx + 1) * CS + g.ox : cx * CS + g.ox) - ox) / dx : Infinity;
  let tmz = Math.abs(dz) > 1e-9 ? ((dz > 0 ? (cz + 1) * CS + g.oz : cz * CS + g.oz) - oz) / dz : Infinity;
  const id = ++stampId; const seen = g.mark;
  for (let guard = 0; guard < 400; guard++) {
    const list = g.cells[cz * g.nx + cx];
    for (let i = 0; i < list.length; i++) {
      const b = list[i];
      if (seen.get(b) === id) continue; seen.set(b, id);
      const t = rayBox(ox, oy, oz, dx, dy, dz, b.x0, b.y0, b.z0, b.x1, b.y1, b.z1);
      if (t < best) best = t;
    }
    const tn = Math.min(tmx, tmz);
    if (best <= tn || tn >= maxT) break;
    if (tmx < tmz) { cx += sx; tmx += tdx; if (cx < 0 || cx >= g.nx) break; } else { cz += sz; tmz += tdz; if (cz < 0 || cz >= g.nz) break; }
  }
  return best >= maxT ? Infinity : best;
}

// صناديق إصابة اللاعب: الجسم والرأس
export function rayPlayer(p, ox, oy, oz, dx, dy, dz, maxT) {
  const hh = p.crouch ? PLAYER.hc : PLAYER.h;
  const yb = p.y + hh - PLAYER.headH;
  const tb = rayBox(ox, oy, oz, dx, dy, dz, p.x - 0.34, p.y, p.z - 0.34, p.x + 0.34, yb, p.z + 0.34);
  const th = rayBox(ox, oy, oz, dx, dy, dz, p.x - 0.2, yb, p.z - 0.2, p.x + 0.2, p.y + hh, p.z + 0.2);
  if (th <= tb && th < maxT) return { t: th, head: true };
  if (tb < maxT) return { t: tb, head: false };
  return null;
}

export function blocked(map, x, y, z, hh) {
  const r = PLAYER.r, st = PLAYER.step, bs = cellAt(gridOf(map), x, z);
  for (let i = 0; i < bs.length; i++) {
    const b = bs[i];
    if (x + r > b.x0 && x - r < b.x1 && z + r > b.z0 && z - r < b.z1 && y + hh > b.y0 && y + st < b.y1) return true;
  }
  return false;
}

export function groundAt(map, x, z, y) {
  const r = PLAYER.r, lim = y + PLAYER.step + 1e-4, bs = cellAt(gridOf(map), x, z);
  let g = 0;
  for (let i = 0; i < bs.length; i++) {
    const b = bs[i];
    if (b.y1 <= lim && b.y1 > g && x + r > b.x0 && x - r < b.x1 && z + r > b.z0 && z - r < b.z1) g = b.y1;
  }
  return g;
}

export function canStand(map, p) {
  return !blocked(map, p.x, p.y, p.z, PLAYER.h);
}

// p = {x,y,z,vy,onGround}
export function moveBody(map, p, vx, vz, dt, jump, crouched) {
  const hh = crouched ? PLAYER.hc : PLAYER.h;
  const limx = map.W / 2 - PLAYER.r - 0.05, limz = map.D / 2 - PLAYER.r - 0.05;
  let nx = p.x + vx * dt;
  if (nx > limx) nx = limx; else if (nx < -limx) nx = -limx;
  if (!blocked(map, nx, p.y, p.z, hh)) p.x = nx;
  let nz = p.z + vz * dt;
  if (nz > limz) nz = limz; else if (nz < -limz) nz = -limz;
  if (!blocked(map, p.x, p.y, nz, hh)) p.z = nz;
  if (jump && p.onGround) { p.vy = PLAYER.jump; p.onGround = false; }
  p.vy -= PLAYER.grav * dt;
  if (p.vy < -40) p.vy = -40;
  let ny = p.y + p.vy * dt;
  if (p.vy > 0) {
    const r = PLAYER.r;
    const near = cellAt(gridOf(map), p.x, p.z);
    for (let i = 0; i < near.length; i++) {
      const b = near[i];
      if (b.y0 >= p.y + hh - 0.01 && b.y0 < ny + hh && p.x + r > b.x0 && p.x - r < b.x1 && p.z + r > b.z0 && p.z - r < b.z1) {
        ny = b.y0 - hh; p.vy = 0;
      }
    }
    p.onGround = false;
  } else {
    const g = groundAt(map, p.x, p.z, p.y);
    if (ny <= g) { ny = g; p.vy = 0; p.onGround = true; } else p.onGround = false;
  }
  p.y = ny;
}
