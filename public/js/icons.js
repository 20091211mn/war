// أيقونات SVG مرسومة يدويًا (بدون إيموجي)
import { GUNS } from './models.js';
const P = {
  fire: '<path d="M8.5 21V9.5a3.5 3.5 0 0 1 7 0V21z"/><path d="M8.5 15h7M8.5 18h7"/>',
  ads: '<circle cx="12" cy="12" r="9"/><path d="M12 3v18M3 12h18"/><circle cx="12" cy="12" r="2" fill="currentColor"/>',
  jump: '<path d="M5 14l7-7 7 7M5 20l7-7 7 7"/>',
  crouch: '<path d="M5 4l7 7 7-7M5 10l7 7 7-7"/><path d="M5 21h14"/>',
  reload: '<path d="M20 12a8 8 0 1 1-2.4-5.7"/><path d="M20 4v5h-5"/>',
  swap: '<path d="M4 8h14l-3-3M20 16H6l3 3"/>',
  c4: '<rect x="4" y="9" width="16" height="9" rx="1.5"/><path d="M8 9V6.5M12 9V6M16 9V6.5"/><circle cx="12" cy="13.5" r="1.6" fill="currentColor"/>',
  det: '<path d="M13 2L5 14h6l-1 8 9-13h-6z"/>',
  gren: '<circle cx="12" cy="14" r="6.5"/><path d="M9.5 7.5V5h5v2.5M14.5 5l3-2.5"/><path d="M9 14h6" opacity=".5"/>',
  board: '<path d="M5 6h14M5 12h14M5 18h14"/>',
  pause: '<path d="M8 5v14M16 5v14"/>',
  head: '<circle cx="12" cy="12" r="8"/><circle cx="12" cy="12" r="3.5"/><path d="M12 1v4M12 19v4M1 12h4M19 12h4"/>',
  skull: '<path d="M12 3a8 8 0 0 0-5 14v3h10v-3a8 8 0 0 0-5-14z"/><circle cx="9" cy="12" r="1.6" fill="currentColor"/><circle cx="15" cy="12" r="1.6" fill="currentColor"/>',
  bot: '<rect x="5" y="8" width="14" height="11" rx="2"/><path d="M12 8V4M9 13h.01M15 13h.01M9 16h6"/><circle cx="12" cy="3.5" r="1"/>',
  crown: '<path d="M3 8l4.5 4L12 5l4.5 7L21 8l-2 11H5L3 8z"/>',
  scan: '<path d="M3 8V5a2 2 0 0 1 2-2h3M16 3h3a2 2 0 0 1 2 2v3M21 16v3a2 2 0 0 1-2 2h-3M8 21H5a2 2 0 0 1-2-2v-3"/><path d="M4 12h16"/>',
  wifi: '<path d="M2 9a15 15 0 0 1 20 0M5 12.5a10 10 0 0 1 14 0M8.5 16a5 5 0 0 1 7 0"/><circle cx="12" cy="19.5" r="1" fill="currentColor"/>',
  tool: '<path d="M14.5 6.5a4 4 0 0 0-5 5L3 18l3 3 6.5-6.5a4 4 0 0 0 5-5l-2.5 2.5-2.5-.5-.5-2.5z"/>',
  play: '<path d="M7 4l13 8-13 8z" fill="currentColor"/>',
  check: '<path d="M4 12.5l5 5L20 6"/>',
  use: '<path d="M5 17h14v-4l-2-5H7l-2 5v4zM7 17v2M17 17v2M5 13h14"/><circle cx="8.5" cy="15" r=".6" fill="currentColor"/><circle cx="15.5" cy="15" r=".6" fill="currentColor"/>',
  drone: '<circle cx="12" cy="12" r="2.2"/><path d="M9.8 9.8L5.5 5.5M14.2 9.8l4.3-4.3M9.8 14.2l-4.3 4.3M14.2 14.2l4.3 4.3"/><circle cx="4.5" cy="4.5" r="2.3"/><circle cx="19.5" cy="4.5" r="2.3"/><circle cx="4.5" cy="19.5" r="2.3"/><circle cx="19.5" cy="19.5" r="2.3"/>',
  view: '<path d="M2 12s4-7 10-7 10 7 10 7-4 7-10 7S2 12 2 12z"/><circle cx="12" cy="12" r="3"/>',
  chat: '<path d="M4 5h16v11H9l-5 4V5z"/>',
  kick: '<path d="M6 6l12 12M18 6L6 18"/>',
  trophy: '<path d="M7 4h10v5a5 5 0 0 1-10 0V4zM7 6H3v2a4 4 0 0 0 4 4M17 6h4v2a4 4 0 0 1-4 4M12 14v4M8 21h8"/>',
};
export function icon(name, size = 22, extra = '') {
  return `<svg class="ic" width="${size}" height="${size}" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2" stroke-linecap="round" stroke-linejoin="round" ${extra}>${P[name] || ''}</svg>`;
}
// صورة جانبية للسلاح من أجزائه (x=z الأمامية)
export function gunIcon(kind, w = 120, h = 36) {
  const g = GUNS[kind]; if (!g) return '';
  const col = { d: '#9fb0c4', g: '#cdd7e3', b: '#6b7a8c', w: '#c79a5a', t: '#c8b27a' };
  let minz = 1e9, maxz = -1e9, miny = 1e9, maxy = -1e9; const rects = [];
  const add = (x, y, z, sx, sy, sz, c) => { rects.push([z - sz / 2, -(y + sy / 2), sz, sy, c]); minz = Math.min(minz, z - sz / 2); maxz = Math.max(maxz, z + sz / 2); miny = Math.min(miny, -(y + sy / 2)); maxy = Math.max(maxy, -(y - sy / 2)); };
  for (const p of g.parts) add(p[0], p[1], p[2], p[3], p[4], p[5], col[p[6]] || '#9fb0c4');
  if (g.mag) add(g.mag[0], g.mag[1], g.mag[2], g.mag[3], g.mag[4], g.mag[5], '#6b7a8c');
  const bw = maxz - minz, bh = maxy - miny, s = Math.min((w - 6) / bw, (h - 6) / bh);
  // الفوهة إلى اليمين: نعكس محور z
  const out = rects.map(([z, y, dz, dy, c]) => `<rect x="${((maxz - (z + dz)) * s + 3).toFixed(1)}" y="${((y - miny) * s + 3).toFixed(1)}" width="${Math.max(1.5, dz * s).toFixed(1)}" height="${Math.max(1.5, dy * s).toFixed(1)}" rx="1" fill="${c}"/>`).join('');
  return `<svg class="gun" width="${w}" height="${h}" viewBox="0 0 ${w} ${h}">${out}</svg>`;
}
