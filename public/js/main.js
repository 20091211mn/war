// Warzone — العميل: الشبكة (منفرد/استضافة/انضمام)، الواجهة، اللعب، المركبات
import { BUILD } from './version.js';
import { Engine, tmp } from './gl.js';
import { WEAPONS, PRIMARIES, SECONDARIES, OPTICS, MUZZLES, UNDERS, MAGS, computeStats, sanitizeLoadout } from '/shared/weapons.js';
import { buildMap } from '/shared/maps.js';
import { PLAYER, moveBody, canStand, raycastMap, rayPlayer } from '/shared/physics.js';
import { VEH, SEATS, seatWorld, rayVehicle, vehStep, pushOutVeh, w2l } from '/shared/vehicle.js';
import { scan } from './lan.js';
import { LocalHost, remoteLink } from './net.js';
import { Snd } from './audio.js';
import { input, consume, takeLook, initInput, resetInput, isTouchDevice, pollGamepad } from './input.js';
import { icon, gunIcon } from './icons.js';
import { drawAvatar, drawViewmodel, Effects, drawGrenade, GUNS } from './models.js';
import { drawVehicle, vehicleFx, drawDrop } from './vehview.js';
import { drawKit, drawZone, drawDrone } from './vis.js';
import { enhanceSelects } from './ui-select.js';
import { droneHit, DRONE } from '/shared/extras.js';

const $ = (id) => document.getElementById(id);
const clamp = (v, a, b) => (v < a ? a : v > b ? b : v);
const lerp = (a, b, t) => a + (b - a) * t;
const angDiff = (a, b) => { let d = (b - a) % (Math.PI * 2); if (d > Math.PI) d -= Math.PI * 2; if (d < -Math.PI) d += Math.PI * 2; return d; };
const nowMs = () => performance.now();
const esc = (s) => String(s).replace(/[&<>"']/g, (c) => ({ '&': '&amp;', '<': '&lt;', '>': '&gt;', '"': '&quot;', "'": '&#39;' }[c]));

// ---------- الإعدادات المحفوظة ----------
const store = {
  get(k, d) { try { const v = localStorage.getItem('wz_' + k); return v === null ? d : JSON.parse(v); } catch { return d; } },
  set(k, v) { try { localStorage.setItem('wz_' + k, JSON.stringify(v)); } catch { /* */ } },
};
const settings = { name: store.get('name', ''), quality: store.get('quality', (navigator.hardwareConcurrency || 4) <= 4 || /Android|iPhone|iPad/i.test(navigator.userAgent) ? 0.6 : 0.8), sens: store.get('sens', 5), vol: store.get('vol', 0.7), fov: store.get('fov', 76), xcol: store.get('xcol', '#ffffff'), showfps: store.get('showfps', 1), vib: store.get('vib', 1), lo: sanitizeLoadout(store.get('lo', null)) };

// ---------- الحالة ----------
const prof = Object.assign({ xp: 0, kills: 0, deaths: 0, heads: 0, matches: 0, wins: 0 }, store.get('prof', {}));
const lvlOf = (xp) => Math.floor(Math.sqrt(Math.max(0, xp) / 150)) + 1;
const saveProf = () => store.set('prof', prof);
function renderProf() {
  const l = lvlOf(prof.xp), lo = 150 * (l - 1) * (l - 1), hi = 150 * l * l;
  $('plvl').textContent = 'مستوى ' + l; $('pbar').style.width = Math.round((prof.xp - lo) / (hi - lo) * 100) + '%';
  $('pstat').textContent = `${prof.kills} قتلة · ${prof.deaths} موت · ${prof.wins}/${prof.matches} فوز`;
}
function vib(ms) { if (settings.vib && navigator.vibrate) try { navigator.vibrate(ms); } catch { /* */ } }
const APP = !!(window.Capacitor && (window.Capacitor.isNativePlatform ? window.Capacitor.isNativePlatform() : true));
const S = {
  link: null, host: null, mode: '', lanInfo: null, id: 0, hostId: 0, connected: false, phase: 'lobby', cfg: null, players: new Map(), remote: new Map(), maps: [],
  map: null, inGame: false, paused: false, timeLeft: 0, score: [0, 0], grenades: new Map(), ping: 0, ended: false, entered: false, leaving: false,
  vehs: new Map(), drops: new Map(), chat: [], typing: false, joinAddr: '',
  drones: new Map(), kits: new Map(), zones: [], zone: null, uavUntil: [0, 0], match: null,
};
const me = {
  x: 0, y: 0, z: 0, vy: 0, vx: 0, vz: 0, onGround: true, yaw: 0, pitch: 0, crouch: false, eyeH: PLAYER.eye, camY: 0,
  hp: 100, alive: false, w: 0, epoch: 0, team: 0, st: [null, null], ammo: [{ mag: 0, res: 0 }, { mag: 0, res: 0 }],
  reloading: -1, reloadStart: 0, reloadEnd: 0, swapAt: 0, nextShot: 0, noFireUntil: 0, shots: 0, lastShot: 0, recAcc: 0, bloom: 0,
  adsT: 0, grenades: 2, nextGren: 0, stepAcc: 0, bob: 0, deadAt: 0, sprinting: false, sprintPrev: false, kick: 0, flash: 0,
  swayX: 0, swayY: 0, emptyClick: false, autoReloadAt: 0, protUntil: 0, fell: 0,
  veh: -1, seat: 0, vview: 'fp', seatAt: 0, shake: 0, killer: '', killW: '', shield: 0, killerId: 0, drone: null, droneCd: 0,
};
// حالة المركبة التي أقودها (المحاكاة محلية، والخادم يتحقق)
const LV = { x: 0, z: 0, h: 0, vx: 0, vz: 0, steer: 0, tires: 0, speed: 0, dyn: null, cr: 0, lastSend: 0, skidAt: 0 };
let eng = null, fx = new Effects(), lastT = 0, lastSend = 0, lastPing = 0, lastRadar = 0, emaDt = 0.016, slowCount = 0;

// ---------- الشاشات ----------
const SCREENS = ['join', 'menu', 'lobby', 'loadout', 'pause', 'settings', 'end', 'err'];
function show(name) { if (name === 'menu') { try { renderProf(); } catch { /* */ } } for (const s of SCREENS) $(s).classList.toggle('hidden', s !== name); }
function hideScreens() { for (const s of SCREENS) $(s).classList.add('hidden'); }
function showError(title, msg, btn = 'العودة للقائمة') { try { window.__dbg && window.__dbg(title + ' | ' + msg); } catch { /* */ } $('errTitle').textContent = title; $('errMsg').textContent = msg; $('btnReload').textContent = btn; show('err'); }
function toast(msg, ms = 2200) { const t = $('toast'); t.textContent = msg; t.classList.add('on'); clearTimeout(toast.h); toast.h = setTimeout(() => t.classList.remove('on'), ms); }

// ---------- الشبكة / الجلسة ----------
function send(o) { if (S.link && S.link.readyState === 1) S.link.send(JSON.stringify(o)); }
function attach(link, mode) {
  S.link = link; S.mode = mode; S.connected = false;
  link.onopen = () => { S.connected = true; send({ t: 'hello', n: settings.name, lo: settings.lo }); };
  link.onmessage = (e) => { let m; try { m = JSON.parse(e.data); } catch { return; } try { onMsg(m); } catch (err) { console.error(err); } };
  link.onclose = () => { if (S.link !== link) return; S.connected = false; S.link = null; onLinkLost(); };
}
function onLinkLost() {
  if (S.leaving) return;
  if (!S.entered) { $('joinMsg').textContent = 'تعذّر الاتصال بـ ' + S.joinAddr + ' — تأكد أن المضيف يعمل وأنكما على نفس الشبكة.'; stopHost(); show('join'); return; }
  cleanupGameUI(); stopHost(); S.entered = false;
  showError('انقطع الاتصال', 'تم قطع الاتصال بالمضيف.');
}
function stopHost() { if (S.host) { const h = S.host; S.host = null; h.stop().catch(() => {}); } S.lanInfo = null; }
function cleanupGameUI() {
  S.inGame = false; S.ended = false; S.paused = false; me.alive = false; me.veh = -1; S.vehs.clear(); S.drops.clear();
  for (const id of ['hud', 'touch', 'scope', 'board', 'deadmsg', 'deadpanel', 'vhud', 'chatbar']) $(id).classList.add('hidden');
  document.body.classList.remove('invehicle', 'driving', 'canuse');
  Snd.engineOff();
  try { document.exitPointerLock && document.exitPointerLock(); } catch { /* */ }
  resetInput();
}
function leaveSession() {
  S.leaving = true;
  if (S.link) { const l = S.link; S.link = null; try { l.close(); } catch { /* */ } }
  stopHost(); cleanupGameUI();
  S.entered = false; S.players.clear(); S.remote.clear(); S.chat = []; $('chatLog').innerHTML = ''; $('chatfeed').innerHTML = '';
  S.leaving = false; show('menu');
}
function beginSession() {
  settings.name = $('name').value.trim().slice(0, 14); settings.quality = Number($('quality').value);
  store.set('name', settings.name); store.set('quality', settings.quality);
  Snd.init(); applySettings();
  try { if (document.documentElement.requestFullscreen && isTouchDevice() && !APP) document.documentElement.requestFullscreen().then(() => { try { screen.orientation.lock('landscape').catch(() => {}); } catch { /* */ } }).catch(() => {}); } catch { /* */ }
}
function startSolo() {
  beginSession(); S.leaving = true; stopHost(); S.leaving = false;
  S.host = new LocalHost(); attach(S.host.link(), 'solo');
}
function envReport() {
  const C = window.Capacitor; let plat = 'لا Capacitor', avail = '-';
  try { if (C) { plat = (C.getPlatform ? C.getPlatform() : '?') + (C.isNativePlatform && C.isNativePlatform() ? ' (أصلي)' : ' (ويب)'); avail = C.isPluginAvailable ? String(C.isPluginAvailable('LanHost')) : '?'; } } catch { /* */ }
  return `إصدار ${BUILD} · ${plat} · إضافة الاستضافة: ${avail}`;
}
try { setTimeout(() => { const e = document.getElementById('envInfo'); if (e) e.textContent = envReport(); }, 0); } catch { /* */ }
let pluginErr = '';
function getPlugin() {
  const C = window.Capacitor; if (!C) { pluginErr = 'no Capacitor'; return null; }
  try { if (C.Plugins && C.Plugins.LanHost) return C.Plugins.LanHost; } catch (e) { pluginErr = String(e); }
  try { if (C.registerPlugin) return (window.__lanHost ||= C.registerPlugin('LanHost')); pluginErr = 'no registerPlugin'; } catch (e) { pluginErr = String(e && e.message || e); }
  return null;
}
async function startHost() {
  const plugin = getPlugin();
  if (!plugin) { $('menuMsg').textContent = 'الاستضافة متاحة داخل تطبيق الأندرويد فقط (' + pluginErr + '). ' + envReport(); return; }
  beginSession(); stopHost();
  const host = new LocalHost(); host.game.cfg.fill = 0;
  $('menuMsg').textContent = 'جارٍ بدء الاستضافة…';
  try { S.lanInfo = await host.startLan(plugin, 3000); }
  catch (e) { host.stop().catch(() => {}); $('menuMsg').textContent = 'تعذّر بدء الاستضافة: ' + (e && e.message ? e.message : e) + ' | ' + envReport(); return; }
  $('menuMsg').textContent = '';
  S.host = host; attach(host.link(), 'host');
}
function joinTo(addr) {
  addr = addr.trim().replace(/^https?:\/\//, '').replace(/\/.*$/, '');
  if (!addr) return;
  if (!/:\d+$/.test(addr)) addr += ':3000';
  beginSession(); S.joinAddr = addr; $('joinMsg').textContent = 'جارٍ الاتصال بـ ' + addr + '…';
  const rec = store.get('recent', []).filter((a) => a !== addr); rec.unshift(addr); store.set('recent', rec.slice(0, 4));
  S.leaving = true; if (S.link) { const o = S.link; S.link = null; try { o.close(); } catch { /* */ } } S.leaving = false;
  let link; try { link = remoteLink(addr, false); } catch { $('joinMsg').textContent = 'عنوان غير صالح.'; return; }
  attach(link, 'join');
}
function joinPage() {
  beginSession(); S.joinAddr = location.host;
  S.leaving = true; if (S.link) { const o = S.link; S.link = null; try { o.close(); } catch { /* */ } } S.leaving = false;
  attach(remoteLink(location.host, location.protocol === 'https:'), 'page');
}

function onMsg(m) {
  switch (m.t) {
    case 'welcome':
      S.id = m.id; S.maps = m.maps || []; S.remote.clear(); S.players.clear(); S.entered = true; fillMaps();
      if (!S.inGame) show('lobby');
      showAddr(); break;
    case 'full': S.leaving = true; if (S.link) { try { S.link.close(); } catch { /* */ } S.link = null; } S.leaving = false; stopHost(); show('join'); $('joinMsg').textContent = 'الغرفة ممتلئة (16 لاعبًا).'; break;
    case 'kicked': S.leaving = true; leaveSession(); showError('تم طردك', 'قام مضيف الغرفة بإخراجك.'); break;
    case 'lobby': onLobby(m); break;
    case 'start': startGame(m); break;
    case 'tolobby': leaveGame(); break;
    case 'spawn': onSpawn(m); break;
    case 'snap': onSnap(m); break;
    case 'shot': onShot(m); break;
    case 'hit': onHit(m); break;
    case 'kill': onKill(m); break;
    case 'boom': onBoom(m); break;
    case 'chat': addChat(m); break;
    case 'seat': onSeat(m); break;
    case 'exit': onExit(m); break;
    case 'vfix': if (me.veh >= 0 && me.seat === 0) { LV.x = m.x; LV.z = m.z; LV.h = m.h; LV.vx = m.vx; LV.vz = m.vz; } break;
    case 'vboom': onVBoom(m); break;
    case 'vcrash': { const v = S.vehs.get(m.v); if (v && !(me.veh === m.v && me.seat === 0)) { const d = Math.hypot(v.x - me.x, v.z - me.z); Snd.crash(clamp(1 - d / 60, 0, 1) * clamp(m.s / 15, 0.3, 1), 0); fx.spark(v.x, 1, v.z, [1, 0.85, 0.4], 8, 4); } break; }
    case 'tire': onTire(m); break;
    case 'kit': { const k = S.kits.get(m.id); if (k) k.up = !!m.up; if (!m.up && m.by !== S.id && k && Math.hypot(k.x - me.x, k.z - me.z) < 30) Snd.pickup(); break; }
    case 'heal': toast('+' + m.n + ' صحة', 1200); Snd.pickup(); vib(30); break;
    case 'zone': toast('تحركت منطقة السيطرة!', 1800); Snd.click(); break;
    case 'ks': onKs(m); break;
    case 'dron': onDron(m); break;
    case 'dfix': if (me.drone) { me.drone.x = m.x; me.drone.y = m.y; me.drone.z = m.z; me.drone.vx = me.drone.vy = me.drone.vz = 0; } break;
    case 'ammo': onAmmo(m); break;
    case 'rl': { const r = S.remote.get(m.id); if (r && S.inGame) { const d = dist3(r, me); Snd.reload(clamp(1 - d / 25, 0, 1) * 0.6, panOf(r)); } break; }
    case 'left': S.remote.delete(m.id); break;
    case 'end': onEnd(m); break;
    case 'pong': S.ping = Math.round(nowMs() - m.c); break;
    default: break;
  }
}
const dist3 = (a, b) => Math.hypot(a.x - b.x, (a.y || 0) - (b.y || 0), a.z - b.z);
function panOf(p) { const dx = p.x - me.x, dz = p.z - me.z, rx = Math.cos(me.yaw), rz = -Math.sin(me.yaw); const l = Math.hypot(dx, dz) || 1; return clamp((dx * rx + dz * rz) / l, -1, 1); }

// ---------- اللوبي والدردشة ----------
function fillMaps() {
  for (const id of ['cfgMap', 'pauseMap']) {
    const sel = $(id); sel.innerHTML = '';
    S.maps.forEach((n, i) => { const o = document.createElement('option'); o.value = i; o.textContent = n; sel.appendChild(o); });
  }
}
function onLobby(m) {
  S.hostId = m.host; S.cfg = m.cfg; S.phase = m.ph;
  S.players.clear(); for (const p of m.ps) S.players.set(p.id, p);
  for (const id of [...S.remote.keys()]) if (!S.players.has(id)) S.remote.delete(id);
  if (S.entered && !S.inGame && m.ph === 'lobby' && $('end').classList.contains('hidden') && $('err').classList.contains('hidden') && $('loadout').classList.contains('hidden') && $('menu').classList.contains('hidden') && $('join').classList.contains('hidden')) show('lobby');
  renderLobby();
  if (S.inGame && !$('board').classList.contains('hidden')) renderBoard();
}
function drawMapPreview(idx) {
  const cv = $('mapPrev'); if (!cv) return; const key = idx; if (drawMapPreview.k === key) return; drawMapPreview.k = key;
  const m = buildMap(idx), c = cv.getContext('2d'), W = cv.width, H = cv.height, sc = Math.min((W - 10) / m.W, (H - 10) / m.D);
  c.clearRect(0, 0, W, H); c.save(); c.translate(W / 2, H / 2); c.scale(sc, sc);
  const th = m.theme; c.fillStyle = `rgb(${th.ground.map((v) => Math.round(v * 255 * 0.55)).join(',')})`; c.fillRect(-m.W / 2, -m.D / 2, m.W, m.D);
  for (const b of m.boxes) { if (b.y0 > 3) continue; const col = th.cols[b.k] || th.cols[0], k = 0.6 + Math.min(0.5, b.y1 * 0.12); c.fillStyle = `rgb(${col.map((v) => Math.min(255, Math.round(v * 255 * k))).join(',')})`; c.fillRect(b.x0, b.z0, b.x1 - b.x0, b.z1 - b.z0); }
  for (const v of m.vehicles || []) { c.fillStyle = '#ffd23b'; c.fillRect(v.x - 1, v.z - 2, 2, 4); }
  c.fillStyle = '#ff4d4d'; for (const [x, z] of m.red) c.fillRect(x - 1.2, z - 1.2, 2.4, 2.4);
  c.fillStyle = '#4d8dff'; for (const [x, z] of m.blue) c.fillRect(x - 1.2, z - 1.2, 2.4, 2.4);
  c.restore();
}
function renderLobby() {
  if (!S.cfg) return;
  const isHost = S.hostId === S.id;
  for (const t of [0, 1]) {
    const ul = $('t' + t); ul.innerHTML = '';
    let n = 0;
    for (const p of S.players.values()) if (p.tm === t) {
      n++;
      const li = document.createElement('li'); if (p.id === S.id) li.className = 'me';
      li.innerHTML = `<span>${p.b ? '<span class="tag">BOT</span> ' : ''}${esc(p.n)}${p.id === S.hostId ? ' <span class="tag host">المضيف</span>' : ''}<small> ${esc(WEAPONS[p.p] ? WEAPONS[p.p].name : '')}</small></span><span class="acts"></span>`;
      if (isHost && !p.b && p.id !== S.id && S.phase === 'lobby') {
        const acts = li.querySelector('.acts');
        const mk = (ic, title, fn) => { const b = document.createElement('button'); b.innerHTML = icon(ic, 13); b.title = title; b.onclick = fn; acts.appendChild(b); };
        mk('swap', 'نقل للفريق الآخر', () => send({ t: 'team', id: p.id, v: 1 - p.tm }));
        mk('crown', 'جعله مضيفًا', () => send({ t: 'give', id: p.id }));
        mk('kick', 'طرد', () => send({ t: 'kick', id: p.id }));
      } else if (isHost && !p.b && p.id !== S.id) {
        const acts = li.querySelector('.acts'); const b = document.createElement('button'); b.innerHTML = icon('kick', 13); b.title = 'طرد'; b.onclick = () => send({ t: 'kick', id: p.id }); acts.appendChild(b);
      }
      ul.appendChild(li);
    }
    $('c' + t).textContent = `(${n})`;
  }
  const set = (id, v) => { const el = $(id); if (el.value !== String(v)) el.value = v; el.disabled = !isHost || S.phase !== 'lobby'; };
  drawMapPreview(S.cfg.map | 0);
  set('cfgMap', S.cfg.map); set('cfgScore', S.cfg.scoreLimit); set('cfgTime', S.cfg.timeLimit); set('cfgFill', S.cfg.fill);
  set('cfgMode', S.cfg.mode || 0); set('cfgTod', S.cfg.tod || 0); set('cfgStreaks', S.cfg.streaks === 0 ? 0 : 1); set('cfgDiff', S.cfg.diff); set('cfgResp', S.cfg.resp); set('cfgVeh', S.cfg.veh); set('cfgFf', S.cfg.ff); set('cfgRegen', S.cfg.regen);
  $('btnStart').disabled = !isHost; $('btnShuffle').disabled = !isHost;
  $('hostTag').textContent = isHost ? '(أنت المضيف)' : '(للمضيف فقط)';
  $('hostMsg').textContent = isHost ? 'اضبط الإعدادات ثم ابدأ المباراة.' : 'بانتظار المضيف لبدء المباراة…';
  document.querySelectorAll('#lobby button[data-team]').forEach((b) => { b.disabled = S.phase !== 'lobby'; });
  if (S.inGame) { $('hostPause').classList.toggle('hidden', !isHost); if (S.cfg) $('pauseMap').value = S.cfg.map; }
}
function showAddr() {
  let txt = '';
  if (S.mode === 'host' && S.lanInfo && S.lanInfo.ips && S.lanInfo.ips.length) txt = 'عنوان لعبتك للأصدقاء: ' + S.lanInfo.ips.map((ip) => `${ip}:${S.lanInfo.port}`).join('  |  ');
  else if (S.mode === 'host') txt = 'الاستضافة تعمل — فعّل الواي فاي أو الهوتسبوت ليجدك الأصدقاء';
  else if (S.mode === 'join') txt = 'متصل بـ ' + S.joinAddr;
  else if (S.mode === 'solo') txt = 'لعب منفرد (بدون شبكة)';
  else txt = 'متصل بخادم الصفحة';
  $('addr').textContent = txt;
}
function addChat(m) {
  if (m.id && m.id !== S.id) Snd.msg();
  S.chat.push(m); if (S.chat.length > 60) S.chat.shift();
  const html = m.id === 0 ? `<span class="sys">${esc(m.m)}</span>` : `${m.to ? '<span class="tt">[فريق] </span>' : ''}<b class="${m.tm === 0 ? 'r' : 'b'}">${esc(m.n)}</b>: ${esc(m.m)}`;
  const log = $('chatLog'), d = document.createElement('div'); d.className = 'cm'; d.innerHTML = html; log.appendChild(d); while (log.children.length > 60) log.removeChild(log.firstChild); log.scrollTop = log.scrollHeight;
  if (S.inGame) { const f = $('chatfeed'), e = document.createElement('div'); e.innerHTML = html; f.appendChild(e); while (f.children.length > 6) f.removeChild(f.firstChild); setTimeout(() => e.remove(), 9000); }
}
function openChat() {
  if (!S.inGame || S.typing || S.ended) return;
  S.typing = true; $('chatbar').classList.remove('hidden'); resetInput();
  try { document.exitPointerLock && document.exitPointerLock(); } catch { /* */ }
  setTimeout(() => $('chatIn').focus(), 30);
}
function closeChat(sendIt) {
  if (!S.typing) return;
  const el = $('chatIn'), txt = el.value.trim();
  if (sendIt && txt) send({ t: 'chat', m: txt, to: Number($('chatTo').value) });
  el.value = ''; el.blur(); S.typing = false; $('chatbar').classList.add('hidden');
  if (!input.touch && S.inGame && !S.paused && !S.ended) { try { $('gl').requestPointerLock(); } catch { /* */ } }
}

// ---------- تجهيز العتاد ----------
let loReturn = 'lobby';
let loSlot = 0;
const KIND_AR = { rifle: 'بندقية هجومية', smg: 'رشاش خفيف', shotgun: 'شوتغن', sniper: 'قناصة', lmg: 'رشاش ثقيل', dmr: 'دقيقة', pistol: 'مسدس', magnum: 'مسدس ثقيل' };
function renderLoadout() {
  const slot = loSlot, ids = slot === 0 ? PRIMARIES : SECONDARIES, id = slot === 0 ? settings.lo.p : settings.lo.s, W = WEAPONS[id];
  const tabs = $('loTabs'); tabs.innerHTML = '';
  ['الرئيسي', 'الثانوي'].forEach((t, i) => { const b = document.createElement('button'); b.className = i === slot ? 'on' : ''; b.textContent = t; b.onclick = () => { loSlot = i; renderLoadout(); }; tabs.appendChild(b); });
  const list = $('loList'); list.innerHTML = '';
  for (const wid of ids) {
    const b = document.createElement('button'); b.className = 'lo-item' + (wid === id ? ' on' : '');
    b.innerHTML = gunIcon(WEAPONS[wid].kind, 74, 24) + '<span>' + WEAPONS[wid].name + '</span>';
    b.onclick = () => { if (slot === 0) settings.lo.p = wid; else settings.lo.s = wid; commitLoadout(); };
    list.appendChild(b);
  }
  $('loName').textContent = W.name; $('loKind').textContent = KIND_AR[W.kind] || W.kind;
  $('loGun').innerHTML = gunIcon(W.kind, 300, 96);
  const st = computeStats(id, settings.lo.a[slot]), bars = $('loBars'); bars.innerHTML = '';
  const row = (n, v, txt) => { bars.insertAdjacentHTML('beforeend', `<span>${n}</span><div class="bar"><i style="width:${clamp(v, 0.04, 1) * 100}%"></i></div><em>${txt}</em>`); };
  row('الضرر', st.dmg * st.pellets * (st.pellets > 1 ? 0.5 : 1) / 110, Math.round(st.dmg) + (st.pellets > 1 ? '×' + st.pellets : ''));
  row('معدل النار', st.rpm / 900, Math.round(st.rpm)); row('المدى', st.range[1] / 200, Math.round(st.range[1]) + 'م');
  row('الثبات', 1 - st.recoilV / 0.05, ''); row('الحركة', (st.move - 0.8) / 0.35, ''); row('الذخيرة', st.mag / 60, st.mag);
  const att = $('loAtt'); att.innerHTML = '';
  const grp = (label, key, opts) => {
    const g = document.createElement('div'); g.className = 'lo-grp'; g.innerHTML = `<small>${label}</small>`;
    const r = document.createElement('div'); r.className = 'chips';
    for (const [k, v] of opts) { const c = document.createElement('button'); c.className = 'chip' + (settings.lo.a[slot][key] === k ? ' on' : ''); c.textContent = v; c.onclick = () => { settings.lo.a[slot][key] = k; commitLoadout(); }; r.appendChild(c); }
    g.appendChild(r); att.appendChild(g);
  };
  grp('المنظار', 'optic', W.optics.map((k) => [k, OPTICS[k].name]));
  grp('الفوهة', 'muzzle', Object.entries(MUZZLES).map(([k, v]) => [k, v.name]));
  grp('أسفل السلاح', 'under', Object.entries(UNDERS).map(([k, v]) => [k, v.name]));
  grp('المخزن', 'mag', Object.entries(MAGS).map(([k, v]) => [k, v.name]));
}
function commitLoadout() {
  settings.lo = sanitizeLoadout(settings.lo); store.set('lo', settings.lo);
  renderLoadout(); send({ t: 'lo', lo: settings.lo }); if (!me.alive && S.inGame) renderDeadWeapons();
}
// لوحة تغيير السلاح عند الموت
function renderDeadWeapons() {
  const box = $('dweapons'); box.innerHTML = '';
  [...PRIMARIES, ...SECONDARIES].forEach((wid, i) => {
    const on = wid === settings.lo.p || wid === settings.lo.s;
    const b = document.createElement('button'); b.className = 'wbtn' + (on ? ' on' : '');
    b.innerHTML = gunIcon(WEAPONS[wid].kind, 70, 22) + `<span>${i + 1}. ${WEAPONS[wid].name}</span>`;
    b.onclick = () => pickDeadWeapon(i);
    box.appendChild(b);
  });
}
function pickDeadWeapon(i) {
  const all = [...PRIMARIES, ...SECONDARIES], wid = all[i]; if (!wid) return;
  if (WEAPONS[wid].type === 'primary') settings.lo.p = wid; else settings.lo.s = wid;
  commitLoadout(); Snd.click(); toast('سيُطبَّق عند الظهور: ' + WEAPONS[wid].name, 1500);
}

// ---------- القائمة ----------
let setReturn = 'menu';
function openSettings(from) { setReturn = from; $('fov').value = settings.fov; $('fovv').textContent = settings.fov; $('xcol').value = settings.xcol; $('showfps').value = String(settings.showfps); $('vib').value = String(settings.vib); show('settings'); }
function applySettings() {
  document.documentElement.style.setProperty('--xc', settings.xcol);
  input.sensMouse = 0.0007 * (settings.sens + 1) * 0.8 + 0.0008;
  input.sensTouch = 0.0012 * (settings.sens + 1) * 0.75 + 0.0006;
  Snd.setVolume(settings.vol);
  if (eng) eng.resize(settings.quality);
}
function bindUI() {
  $('name').value = settings.name; $('quality').value = String(settings.quality);
  $('sens').value = settings.sens; $('vol').value = settings.vol; $('quality2').value = String(settings.quality);
  $('btnSolo').onclick = startSolo;
  $('btnHost').onclick = startHost;
  $('btnJoin').onclick = () => { show('join'); bindJoin(); $('btnScan').click(); };
  $('btnPage').onclick = joinPage;
  $('btnJoinBack').onclick = () => { if (scanCtl) scanCtl.stop = true; show('menu'); };
  $('btnLeave').onclick = leaveSession;
  document.querySelectorAll('#lobby button[data-team]').forEach((b) => { b.onclick = () => { Snd.click(); send({ t: 'team', v: Number(b.dataset.team) }); }; });
  const cfgSend = () => send({ t: 'cfg', map: Number($('cfgMap').value), scoreLimit: Number($('cfgScore').value), timeLimit: Number($('cfgTime').value), fill: Number($('cfgFill').value),
    diff: Number($('cfgDiff').value), resp: Number($('cfgResp').value), veh: Number($('cfgVeh').value), ff: Number($('cfgFf').value), regen: Number($('cfgRegen').value), mode: Number($('cfgMode').value), tod: Number($('cfgTod').value), streaks: Number($('cfgStreaks').value) });
  for (const id of ['cfgMap', 'cfgScore', 'cfgTime', 'cfgFill', 'cfgDiff', 'cfgResp', 'cfgVeh', 'cfgFf', 'cfgRegen', 'cfgMode', 'cfgTod', 'cfgStreaks']) $(id).onchange = cfgSend;
  $('btnStart').onclick = () => { Snd.init(); send({ t: 'start' }); };
  $('btnShuffle').onclick = () => send({ t: 'shuffle' });
  $('btnLoadout').onclick = () => { loReturn = 'lobby'; renderLoadout(); show('loadout'); };
  $('btnLo2').onclick = () => { loReturn = 'pause'; renderLoadout(); show('loadout'); };
  $('btnDeadAtt').onclick = openDeadAtt;
  $('btnLoDone').onclick = () => { if (loReturn === 'dead') hideScreens(); else show(loReturn); };
  $('btnResume').onclick = resume;
  $('btnSettings').onclick = () => openSettings('pause'); $('btnMenuSet').onclick = () => openSettings('menu'); $('btnLobbySet').onclick = () => openSettings('lobby');
  $('btnSetBack').onclick = () => show(setReturn);
  $('fov').oninput = (e) => { settings.fov = Number(e.target.value); $('fovv').textContent = settings.fov; store.set('fov', settings.fov); };
  $('xcol').onchange = (e) => { settings.xcol = e.target.value; store.set('xcol', settings.xcol); applySettings(); };
  $('showfps').onchange = (e) => { settings.showfps = Number(e.target.value); store.set('showfps', settings.showfps); };
  $('vib').onchange = (e) => { settings.vib = Number(e.target.value); store.set('vib', settings.vib); };
  $('btnRestart').onclick = () => { send({ t: 'restart', map: Number($('pauseMap').value) }); S.paused = false; hideScreens(); };
  $('btnBack').onclick = () => { if (S.hostId === S.id) { send({ t: 'back' }); S.paused = false; } else leaveSession(); };
  $('btnEndBack').onclick = () => { if (S.hostId === S.id) send({ t: 'back' }); else { $('endMsg').textContent = 'بانتظار المضيف أو العودة التلقائية…'; } };
  $('btnFull').onclick = () => { try { document.documentElement.requestFullscreen(); } catch { toast('ملء الشاشة غير مدعوم هنا'); } };
  $('btnReload').onclick = () => { S.leaving = true; leaveSession(); };
  $('sens').oninput = (e) => { settings.sens = Number(e.target.value); store.set('sens', settings.sens); applySettings(); };
  $('vol').oninput = (e) => { settings.vol = Number(e.target.value); store.set('vol', settings.vol); applySettings(); };
  $('quality2').onchange = (e) => { settings.quality = Number(e.target.value); store.set('quality', settings.quality); applySettings(); };
  const lobbySend = () => { const el = $('lobbyIn'), t = el.value.trim(); if (t) send({ t: 'chat', m: t, to: 0 }); el.value = ''; };
  $('lobbySend').onclick = lobbySend; $('lobbyIn').onkeydown = (e) => { if (e.key === 'Enter') lobbySend(); };
  $('chatSend').onclick = () => closeChat(true);
  $('chatIn').onkeydown = (e) => { if (e.key === 'Enter') { e.preventDefault(); closeChat(true); } else if (e.key === 'Escape') { e.preventDefault(); e.stopPropagation(); closeChat(false); } };
  if (!APP && location.host) $('btnPage').classList.remove('hidden');
  if (!APP) { $('btnHost').classList.add('hidden'); } else { $('menuHint').textContent = 'اللعب المنفرد يعمل بدون إنترنت. الشبكة المحلية اختيارية (استضافة أو انضمام).'; }
}
function openDeadAtt() { if (me.alive || !S.inGame) return; loReturn = 'dead'; renderLoadout(); show('loadout'); try { document.exitPointerLock && document.exitPointerLock(); } catch { /* */ } }
let scanCtl = null, joinBound = false;
function bindJoin() {
  const list = $('srvlist');
  list.innerHTML = '';
  const addSrv = (s) => {
    const b = document.createElement('button'); b.className = 'srv';
    b.innerHTML = `<span>${icon('wifi', 18)} ${s.local ? 'هذا الجهاز' : 'لعبة'} — ${s.players} لاعب · ${s.phase === 'lobby' ? 'في الغرفة' : 'مباراة جارية'}</span><small>${s.host}:${s.port}</small>`;
    b.onclick = () => { if (scanCtl) scanCtl.stop = true; joinTo(`${s.host}:${s.port}`); };
    list.appendChild(b);
  };
  const rec = store.get('recent', []);
  for (const a of rec) { const b = document.createElement('button'); b.className = 'srv'; b.innerHTML = `<span>آخر اتصال</span><small>${a}</small>`; b.onclick = () => joinTo(a); list.appendChild(b); }
  $('manual').value = rec[0] || $('manual').value;
  if (joinBound) return; joinBound = true;
  $('btnScan').onclick = async () => {
    if (scanCtl) scanCtl.stop = true; scanCtl = { stop: false }; const my = scanCtl;
    list.innerHTML = ''; $('joinMsg').textContent = 'جارٍ البحث في الشبكة…'; $('btnScan').disabled = true;
    let n = 0;
    await scan((s) => { n++; addSrv(s); }, (p) => { $('scanBar').style.width = (p * 100) + '%'; }, my);
    $('btnScan').disabled = false;
    if (!my.stop) $('joinMsg').textContent = n ? 'اختر لعبة من القائمة.' : 'لم يتم العثور على ألعاب. تأكد أن المضيف يعمل أو اكتب العنوان يدويًا.';
  };
  $('btnManual').onclick = () => joinTo($('manual').value);
}
function pause() {
  if (!S.inGame || S.paused || S.ended || S.typing) return;
  S.paused = true; resetInput(); if (input.resetAds) { input.resetAds(); input.resetCrouch(); }
  try { document.exitPointerLock && document.exitPointerLock(); } catch { /* */ }
  $('btnBack').textContent = S.hostId === S.id ? 'إنهاء المباراة والعودة للغرفة' : 'مغادرة الغرفة';
  renderLobby(); show('pause');
}
function resume() {
  S.paused = false; hideScreens();
  if (!input.touch) { try { $('gl').requestPointerLock(); } catch { /* */ } }
}

// ---------- بدء/إنهاء اللعب ----------
function startGame(m) {
  S.map = buildMap(m.map); S.ended = false; S.paused = false; S.score = [0, 0]; S.cfg = m.cfg || S.cfg;
  try { eng.setMap(S.map, (S.cfg && S.cfg.tod) || 0); } catch (e) { showError('خطأ في الرسم', String(e.message || e)); return; }
  S.inGame = true; S.remote.clear(); S.grenades.clear(); S.vehs.clear(); S.drops.clear(); fx = new Effects(); me.alive = false; me.veh = -1;
  S.timeLeft = m.tl; S.typing = false;
  S.kits = new Map((m.kits || []).map((k) => [k[0], { id: k[0], x: k[1], z: k[2], up: true }])); S.zones = m.zones || []; S.zone = null; S.uavUntil = [0, 0]; S.match = { kills: 0, deaths: 0, heads: 0, xp: 0 }; me.shield = 0; me.killerId = 0; me.drone = null; me.droneCd = 0; S.drones.clear(); document.body.classList.remove('drone');
  hideScreens(); $('hud').classList.remove('hidden'); $('board').classList.add('hidden'); $('deadmsg').classList.add('hidden'); $('deadpanel').classList.add('hidden'); $('chatbar').classList.add('hidden');
  if (input.touch) $('touch').classList.remove('hidden');
  document.body.classList.toggle('touch', input.touch);
  $('feed').innerHTML = '';
  resetInput();
  toast('المباراة بدأت — ' + S.map.name, 2500);
}
function leaveGame() {
  cleanupGameUI();
  if (S.entered) show('lobby');
}
function statsFor(info, slot) {
  if (!info) return null;
  const key = slot === 0 ? info.p + JSON.stringify(info.a[0]) : info.s + JSON.stringify(info.a[1]);
  info._sc = info._sc || {};
  if (!info._sc[key]) info._sc[key] = computeStats(slot === 0 ? info.p : info.s, info.a[slot]);
  return info._sc[key];
}
function remoteOf(id) {
  let r = S.remote.get(id);
  if (!r) { r = { id, x: 0, y: 0, z: 0, tx: 0, ty: 0, tz: 0, yaw: 0, tyaw: 0, pitch: 0, tpitch: 0, crouch: false, moving: false, sprint: false, ads: false, alive: false, hp: 100, w: 0, deadAt: -1e9, flash: 0, phase: Math.random() * 6, stepAcc: 0, init: false, reveal: false, veh: 0, seat: 0 }; S.remote.set(id, r); }
  return r;
}

function onSpawn(m) {
  const info = S.players.get(m.id);
  if (info) { info.p = m.p; info.s = m.s; info.a = m.a; info.tm = m.tm; info._sc = null; }
  if (m.id === S.id) {
    me.x = m.x; me.y = m.y; me.z = m.z; me.vy = 0; me.vx = 0; me.vz = 0; me.onGround = true; me.yaw = m.yaw; me.pitch = 0; me.team = m.tm;
    me.epoch = m.e; me.hp = 100; me.alive = true; me.crouch = false; me.eyeH = PLAYER.eye; me.camY = m.y; me.w = 0; me.veh = -1;
    me.st = [computeStats(m.p, m.a[0]), computeStats(m.s, m.a[1])];
    me.ammo = me.st.map((s) => ({ mag: s.mag, res: s.reserve }));
    me.reloading = -1; me.swapAt = nowMs() - 1000; me.grenades = 2; me.adsT = 0; me.recAcc = 0; me.bloom = 0; me.shots = 0; me.nextShot = 0; me.protUntil = nowMs() + 2000;
    if (input.resetAds) { input.resetAds(); input.resetCrouch(); }
    $('deadmsg').classList.add('hidden'); $('deadpanel').classList.add('hidden');
    if (loReturn === 'dead' && !$('loadout').classList.contains('hidden')) hideScreens();
    Snd.spawn(); updateAmmoHud();
  } else {
    const r = remoteOf(m.id); r.x = r.tx = m.x; r.y = r.ty = m.y; r.z = r.tz = m.z; r.yaw = r.tyaw = m.yaw; r.alive = true; r.deadAt = -1e9; r.init = true; r.hp = 100; r.veh = 0;
  }
}
function onSnap(m) {
  S.timeLeft = m.tl; S.score = m.sc;
  const seen = new Set();
  for (const p of m.ps) {
    const id = p[0]; seen.add(id);
    const f = p[6], alive = !!(f & 4);
    if (id === S.id) {
      if (alive && me.alive) { me.hp = p[7]; me.shield = p[12] || 0; me.grenades = p[9]; if (p[10] === 0 && me.veh >= 0 && nowMs() - me.seatAt > 1500) me.veh = -1; }
      continue;
    }
    const r = remoteOf(id);
    r.tx = p[1]; r.ty = p[2]; r.tz = p[3]; r.tyaw = p[4]; r.tpitch = p[5];
    r.crouch = !!(f & 1); r.ads = !!(f & 2); r.moving = !!(f & 8); r.reveal = !!(f & 16); r.sprint = !!(f & 32); r.hp = p[7]; r.w = p[8]; r.veh = p[10] || 0; r.seat = p[11] || 0;
    if (!r.init) { r.x = r.tx; r.y = r.ty; r.z = r.tz; r.yaw = r.tyaw; r.init = true; }
    if (r.alive && !alive) r.deadAt = nowMs();
    r.alive = alive;
  }
  for (const id of [...S.remote.keys()]) if (!seen.has(id)) S.remote.delete(id);
  const gs = new Set();
  for (const g of m.gr) {
    gs.add(g[0]); let o = S.grenades.get(g[0]);
    if (!o) { o = { x: g[1], y: g[2], z: g[3] }; S.grenades.set(g[0], o); }
    o.tx = g[1]; o.ty = g[2]; o.tz = g[3];
  }
  for (const id of [...S.grenades.keys()]) if (!gs.has(id)) S.grenades.delete(id);
  // المركبات
  const vs = new Set();
  for (const a of m.vs || []) {
    const id = a[0]; vs.add(id);
    let v = S.vehs.get(id);
    if (!v || ((v.flags & 2) && !(a[8] & 2))) { // جديدة أو أعيد ظهورها
      v = { id, x: a[1], z: a[2], h: a[3], tx: a[1], tz: a[2], th: a[3], speed: 0, steer: 0, tires: 0, hp: 450, flags: 0, seats: [0, 0, 0, 0, 0], spin: 0 };
      S.vehs.set(id, v);
    }
    const driven = me.veh === id && me.seat === 0;
    v.tx = a[1]; v.tz = a[2]; v.th = a[3];
    if (!driven) { v.speed = a[4]; v.steer = a[5]; v.tires = a[6]; }
    v.hp = a[7]; v.flags = a[8]; v.seats = a.slice(9, 14);
  }
  for (const id of [...S.vehs.keys()]) if (!vs.has(id)) S.vehs.delete(id);
  if (typeof m.kt === 'string') for (let i = 0; i < m.kt.length; i++) { const k = S.kits.get([...S.kits.keys()][i]); if (k) k.up = m.kt[i] === '1'; }
  const dsn = new Set();
  for (const a of m.dn || []) {
    dsn.add(a[0]); let o = S.drones.get(a[0]);
    if (!o) { o = { id: a[0], x: a[1], y: a[2], z: a[3], yaw: a[4], tx: a[1], ty: a[2], tz: a[3], tyaw: a[4], hp: a[5] }; S.drones.set(a[0], o); }
    o.tx = a[1]; o.ty = a[2]; o.tz = a[3]; o.tyaw = a[4]; o.hp = a[5];
  }
  for (const id of [...S.drones.keys()]) if (!dsn.has(id)) S.drones.delete(id);
  if (m.uv) { const t = nowMs(); S.uavUntil = [t + m.uv[0], t + m.uv[1]]; }
  S.zone = m.zn ? { i: m.zn[0], x: m.zn[1], z: m.zn[2], r: m.zn[3], state: m.zn[4], left: m.zn[5] } : null;
  S.drops.clear(); for (const d of m.dr || []) S.drops.set(d[0], { id: d[0], x: d[1], y: d[2], z: d[3], w: d[4], n: d[5] });
}
function onShot(m) {
  const r = S.remote.get(m.id);
  if (r) r.flash = 1;
  for (const e of m.e) {
    fx.tracer(m.o, e, !!m.s, e[3] === 1);
    if (e[3] === 0 || e[3] === undefined) fx.decal(e[0], e[1], e[2]); else if (e[3] === 1) fx.decal(e[0], e[1], e[2], true);
    if (m.id !== S.id && me.alive && (teamOf(m.id) !== me.team || (S.cfg && S.cfg.ff))) suppressCheck(m.o, e);
    if (e[3] === 1) fx.spark(e[0], e[1], e[2], [0.8, 0.1, 0.1], 4, 3); else if (e[3] === 2) fx.spark(e[0], e[1], e[2], [1, 0.85, 0.4], 5, 3.5); else fx.spark(e[0], e[1], e[2], [0.8, 0.75, 0.6], 3, 2.5);
  }
  const d = Math.hypot(m.o[0] - me.x, m.o[2] - me.z), st = WEAPONS[m.w];
  if (r && d < 30 && !m.s && st && st.kind !== 'sniper') fx.shell(m.o[0], m.o[1] - 0.1, m.o[2], r.yaw || 0, st.kind === 'shotgun' || st.kind === 'lmg');
  Snd.shot(st ? st.kind : 'rifle', clamp(1 - d / (m.s ? 40 : 110), 0, 1) * 0.9, panOf({ x: m.o[0], z: m.o[2] }), !!m.s, clamp(d / 110, 0, 1));
}
// قمع النيران: رصاصة عدو تمرّ قرب الرأس => تشويش حواف + طقطقة + اهتزاز
function suppressCheck(o, e) {
  const px = me.x, py = me.y + 1.5, pz = me.z, dx = e[0] - o[0], dy = e[1] - o[1], dz = e[2] - o[2], L2 = dx * dx + dy * dy + dz * dz || 1;
  const t = clamp(((px - o[0]) * dx + (py - o[1]) * dy + (pz - o[2]) * dz) / L2, 0, 1);
  const d = Math.hypot(o[0] + dx * t - px, o[1] + dy * t - py, o[2] + dz * t - pz);
  if (d < 2.6 && t > 0.02 && t < 0.98) { me.supp = Math.min(1, (me.supp || 0) + (1 - d / 2.6) * 0.55); me.shake = Math.max(me.shake, 0.25 * (1 - d / 2.6)); if (nowMs() - (suppressCheck.t || 0) > 90) { suppressCheck.t = nowMs(); Snd.crack(0.5 + 0.4 * (1 - d / 2.6), clamp(((o[0] + dx * t) - px) * 0.2, -1, 1)); } }
}
function onHit(m) {
  if (m.v === S.id) {
    me.hp = Math.max(0, me.hp - m.d);
    const dx = m.fx - me.x, dz = m.fz - me.z, a = Math.atan2(-dx, -dz), rel = angDiff(me.yaw, a);
    const el = document.createElement('i'); el.style.transform = `rotate(${(-rel * 180 / Math.PI).toFixed(0)}deg)`; $('dmgdirs').appendChild(el); setTimeout(() => el.remove(), 1400);
    const v = $('vign'); v.style.opacity = '1'; setTimeout(() => { v.style.opacity = String(clamp((60 - me.hp) / 60, 0, 0.8)); }, 250);
    Snd.hurt();
  } else if (m.a === S.id) {
    const h = $('hitm'); h.className = 'on' + (m.h ? ' head' : ''); clearTimeout(onHit.h); onHit.h = setTimeout(() => { h.className = ''; }, 130);
    Snd.hit(!!m.h);
  }
}
function nameOf(id) { const p = S.players.get(id); return p ? p.n : '؟'; }
function teamOf(id) { const p = S.players.get(id); return p ? p.tm : 0; }
const DRON_WHY = { battery: 'نفد شحن الطائرة', shot: 'أُسقطت طائرتك!', boom: 'انفجرت الطائرة', lost: 'فقدت الاتصال بالطائرة', land: '' };
function onDron(m) {
  if (m.on) {
    if (m.id === S.id) {
      me.drone = { x: me.x, y: me.y + 2, z: me.z, yaw: me.yaw, pitch: 0, vx: 0, vy: 0, vz: 0, until: nowMs() + m.ms, lastSend: 0 };
      document.body.classList.add('drone'); Snd.click();
    }
  } else {
    S.drones.delete(m.id);
    if (m.id === S.id) {
      const was = !!me.drone; me.drone = null; me.droneCd = nowMs() + (m.cd || 20000); document.body.classList.remove('drone'); Snd._dr && Snd.drone(0);
      if (was && DRON_WHY[m.why]) toast(DRON_WHY[m.why], 2200);
    }
  }
}
function toggleDrone(now) {
  if (!me.alive || me.veh >= 0 || S.paused || S.ended || S.typing) return;
  if (me.drone) { send({ t: 'drone', on: 0 }); return; }
  if (now < me.droneCd) { toast(`الطائرة تُشحن… ${Math.ceil((me.droneCd - now) / 1000)} ث`, 1400); return; }
  send({ t: 'drone', on: 1 });
}
function droneStep(dt, now, lx, ly) {
  const d = me.drone;
  d.yaw -= lx; d.pitch = clamp(d.pitch - ly, -1.4, 1.4);
  const cp = Math.cos(d.pitch), sp = Math.sin(d.pitch);
  const fx_ = -Math.sin(d.yaw) * cp, fy = sp, fz = -Math.cos(d.yaw) * cp, rx = Math.cos(d.yaw), rz = -Math.sin(d.yaw);
  const mv = input.moveY, sd = input.moveX, up = (input.jumpHeld ? 1 : 0) - (input.crouch ? 1 : 0);
  const tx = (fx_ * mv + rx * sd) * DRONE.speed, ty = fy * mv * DRONE.speed + up * 6, tz = (fz * mv + rz * sd) * DRONE.speed;
  const k = Math.min(1, 6 * dt);
  d.vx += (tx - d.vx) * k; d.vy += (ty - d.vy) * k; d.vz += (tz - d.vz) * k;
  const nx = d.x + d.vx * dt, ny = d.y + d.vy * dt, nz = d.z + d.vz * dt;
  const tether = (x, z) => Math.hypot(x - me.x, z - me.z) <= DRONE.range;
  if (!droneHit(S.map, nx, d.y, d.z) && tether(nx, d.z)) d.x = nx; else d.vx = 0;
  if (!droneHit(S.map, d.x, ny, d.z)) d.y = ny; else d.vy = 0;
  if (!droneHit(S.map, d.x, d.y, nz) && tether(d.x, nz)) d.z = nz; else d.vz = 0;
  if (now - d.lastSend > 90) { d.lastSend = now; send({ t: 'dp', x: +d.x.toFixed(2), y: +d.y.toFixed(2), z: +d.z.toFixed(2), yaw: +d.yaw.toFixed(3), pitch: +d.pitch.toFixed(3) }); }
  if (consume('firePressed')) { send({ t: 'dboom' }); }
  consume('jump'); consume('reload'); consume('swap'); consume('gren'); consume('slot'); consume('use'); consume('seat'); consume('view');
  me.vx = me.vz = 0;
  me.kick = Math.max(0, me.kick - dt * 8); me.flash = Math.max(0, me.flash - dt * 14); me.shake = Math.max(0, me.shake - dt * 1.6);
}
const KS_TXT = { uav: ['طائرة استطلاع!', 'كشفت أعداءك على الرادار'], shield: ['درع!', 'امتصاص 60 ضررًا'], supply: ['إمداد كامل!', 'صحة + درع + قنابل'] };
function banner(txt) { const b = $('ksbanner'); b.textContent = txt; b.classList.add('on'); clearTimeout(banner.h); banner.h = setTimeout(() => b.classList.remove('on'), 2600); }
function onKs(m) {
  const t = KS_TXT[m.k]; if (!t) return;
  if (m.id === S.id) { banner(`${t[0]} ${t[1]}`); Snd.pickup(); vib(60); me.shield = m.k === 'shield' || m.k === 'supply' ? 60 : me.shield; }
  else if (m.k === 'uav') toast(m.tm === me.team ? `${m.n} أطلق طائرة استطلاع لفريقك` : `⚠ العدو أطلق طائرة استطلاع — أنت مكشوف!`, 2400);
}
function onKill(m) {
  const k = S.players.get(m.k), v = S.players.get(m.v);
  const w = m.w === 'gren' ? 'قنبلة' : m.w === 'veh' ? 'مركبة' : (WEAPONS[m.w] ? WEAPONS[m.w].name : '');
  const d = document.createElement('div'); d.className = teamOf(m.k) === 0 ? 'r' : 'b';
  d.innerHTML = `${esc(k ? k.n : '؟')} ${m.h ? icon('head', 13) : '←'} ${esc(v ? v.n : '؟')} <small>${esc(w)}</small>`;
  const feed = $('feed'); feed.appendChild(d); while (feed.children.length > 5) feed.removeChild(feed.firstChild); setTimeout(() => d.remove(), 6000);
  if (k) k.k++; if (v) v.d++;
  if (S.match) {
    if (m.k === S.id && m.v !== S.id && teamOf(m.v) !== me.team) { S.match.kills++; S.match.xp += 100 + (m.h ? 50 : 0); prof.kills++; prof.xp += 100 + (m.h ? 50 : 0); if (m.h) { S.match.heads++; prof.heads++; } saveProf(); }
    if (m.v === S.id) { S.match.deaths++; prof.deaths++; saveProf(); vib(120); }
  }
  if (m.v === S.id) {
    me.alive = false; me.deadAt = nowMs(); me.hp = 0; me.veh = -1; input.fire = false; if (input.resetAds) { input.resetAds(); input.resetCrouch(); }
    me.killerId = m.k === m.v ? 0 : m.k;
    me.killer = m.k === m.v ? 'نفسك' : (k ? k.n : '؟'); me.killW = w;
    $('dkiller').innerHTML = `قُتلت بواسطة <b>${esc(me.killer)}</b> <small style="color:var(--mut)">${esc(w)}</small>`;
    renderDeadWeapons();
    $('dnote').textContent = input.touch ? '' : 'اضغط 1–8 لاختيار سلاح، X للملحقات';
    $('deadpanel').classList.remove('hidden'); $('scope').classList.add('hidden');
  } else {
    const r = S.remote.get(m.v); if (r && r.alive) { r.alive = false; r.deadAt = nowMs(); r.veh = 0; }
    if (m.k === S.id) { const h = $('hitm'); h.className = 'on kill'; clearTimeout(onHit.h); onHit.h = setTimeout(() => { h.className = ''; }, 300); Snd.kill(); }
  }
}
function onBoom(m) {
  fx.boom(m.x, m.y, m.z);
  const d = Math.hypot(m.x - me.x, m.z - me.z);
  Snd.boom(clamp(1 - d / 70, 0, 1), panOf({ x: m.x, z: m.z }), clamp(d / 70, 0, 1));
  if (d < 20) me.shake = Math.max(me.shake, 1 - d / 20);
  if (d < 9 && me.alive) { Snd.ring(1 - d / 9, 1.5 + 2.5 * (1 - d / 9)); me.supp = Math.max(me.supp || 0, 0.9 * (1 - d / 9)); }
}
function onVBoom(m) {
  fx.boom(m.x, 1.2, m.z); fx.boom(m.x + 1, 0.6, m.z - 1); fx.spark(m.x, 1.5, m.z, [1, 0.6, 0.15], 40, 14); fx.spark(m.x, 1.5, m.z, [0.1, 0.1, 0.1], 20, 8);
  const d = Math.hypot(m.x - me.x, m.z - me.z);
  Snd.boom(clamp(1.2 - d / 90, 0, 1), panOf({ x: m.x, z: m.z }), clamp(d / 90, 0, 1));
  if (d < 35) me.shake = Math.max(me.shake, 1.3 - d / 35);
  if (d < 14 && me.alive) { Snd.ring(1 - d / 14, 2 + 2 * (1 - d / 14)); me.supp = Math.max(me.supp || 0, 0.9 * (1 - d / 14)); }
}
function onTire(m) {
  const v = S.vehs.get(m.v); if (!v) return;
  v.tires = m.m; if (me.veh === m.v && me.seat === 0) LV.tires = m.m;
  const wx = [-0.95, 0.95, -0.95, 0.95][m.i], wz = [-1.55, -1.55, 1.5, 1.5][m.i], c = Math.cos(v.h), s = Math.sin(v.h);
  const x = v.x + c * wx + s * wz, z = v.z - s * wx + c * wz;
  fx.spark(x, 0.4, z, [0.1, 0.1, 0.1], 10, 4); fx.smoke(x, 0.4, z, [0.5, 0.5, 0.5]);
  const d = Math.hypot(x - me.x, z - me.z); Snd.tire(clamp(1 - d / 70, 0, 1), panOf({ x, z }), clamp(d / 70, 0, 1));
  if (me.veh === m.v) toast('انفجر إطار!', 1500);
}
function onAmmo(m) {
  const st = me.st[m.slot], am = me.ammo[m.slot]; if (!st || st.id !== m.w || !am) return;
  const before = am.res; am.res = Math.min(st.reserve, am.res + m.n);
  toast(`+${am.res - before} ذخيرة — ${st.name}`, 1600); Snd.pickup(); updateAmmoHud();
}
function onSeat(m) {
  const was = me.veh, v = S.vehs.get(m.v);
  me.veh = m.v; me.seat = m.s; me.epoch = m.e; me.seatAt = nowMs(); me.crouch = true; me.sprinting = false;
  if (was < 0 && v) { me.yaw = v.h; me.pitch = m.s === 0 ? -0.12 : 0; }
  me.vview = m.s === 0 ? 'chase' : 'fp';
  if (m.s === 0 && v) { const sp = v.speed || 0; LV.x = v.x; LV.z = v.z; LV.h = v.h; LV.vx = -Math.sin(v.h) * sp; LV.vz = -Math.cos(v.h) * sp; LV.steer = 0; LV.tires = v.tires; LV.speed = sp; LV.cr = 0; v.tx = v.x; v.tz = v.z; v.th = v.h; }
  me.reloading = -1; me.adsT = 0; if (input.resetAds) input.resetAds();
  Snd.door(); updateAmmoHud();
}
function onExit(m) {
  me.veh = -1; me.x = m.x; me.y = m.y; me.z = m.z; me.camY = m.y; me.vy = 0; me.vx = me.vz = 0; me.onGround = true; me.epoch = m.e; me.crouch = false; me.eyeH = PLAYER.eye; me.pitch = 0;
  Snd.door();
}
function onEnd(m) {
  S.ended = true; resetInput(); if (input.resetAds) { input.resetAds(); input.resetCrouch(); }
  try { document.exitPointerLock && document.exitPointerLock(); } catch { /* */ }
  $('deadpanel').classList.add('hidden'); $('chatbar').classList.add('hidden'); S.typing = false;
  const mine = me.team;
  let gain = '';
  if (S.match) {
    const bonus = 100 + (m.win === mine ? 300 : 0); S.match.xp += bonus; prof.xp += bonus; prof.matches++; if (m.win === mine) prof.wins++; saveProf();
    gain = `<div class="xpgain">+${S.match.xp} نقطة خبرة · ${S.match.kills} قتلة (${S.match.heads} رأس) · مستوى ${lvlOf(prof.xp)}</div>`; S.match = null;
  }
  $('endTitle').textContent = m.win < 0 ? 'تعادل!' : (m.win === mine ? 'فريقك فاز!' : 'خسر فريقك');
  $('endTitle').style.color = m.win < 0 ? '#ffb020' : (m.win === 0 ? 'var(--red)' : 'var(--blue)');
  $('endBoard').innerHTML = gain + boardHtml(m.sc);
  $('endMsg').textContent = S.hostId === S.id ? 'اضغط للعودة إلى الغرفة (أو يعود الجميع تلقائيًا بعد ثوانٍ).' : 'سيعود الجميع إلى الغرفة معًا بعد ثوانٍ…';
  $('btnEndBack').disabled = S.hostId !== S.id;
  show('end');
}
function boardHtml(sc) {
  const all = [...S.players.values()].sort((a, b) => b.k - a.k || a.d - b.d), mvp = all[0];
  let h = sc && mvp && mvp.k > 0 ? `<div class="xpgain" style="color:#9fe0ff">★ أفضل لاعب: ${esc(mvp.n)} — ${mvp.k} قتلة / ${mvp.d} موت</div>` : '';
  h += '<div style="display:flex;gap:10px;flex-wrap:wrap">';
  for (const t of [0, 1]) {
    const rows = [...S.players.values()].filter((p) => p.tm === t).sort((a, b) => b.k - a.k || a.d - b.d);
    h += `<div class="bt t${t}"><b style="font-size:18px">${t === 0 ? 'الأحمر' : 'الأزرق'} — ${sc ? sc[t] : S.score[t]}</b><table><tr><th>اللاعب</th><th>قتل</th><th>موت</th></tr>`;
    for (const p of rows) h += `<tr class="${p.id === S.id ? 'me' : ''}"><td>${p.b ? '<span class="tag">BOT</span> ' : ''}${esc(p.n)}</td><td>${p.k}</td><td>${p.d}</td></tr>`;
    h += '</table></div>';
  }
  return h + '</div>';
}
function renderBoard() { $('board').innerHTML = boardHtml(); }

// ---------- منطق اللعب المحلي ----------
function curStats() { return me.st[me.w]; }
function updateAmmoHud() {
  const st = curStats(); if (!st) return; const am = me.ammo[me.w];
  $('wname').textContent = st.name; $('mag').textContent = am.mag; $('res').textContent = am.res; $('gcount').innerHTML = icon('gren', 14) + ' ' + me.grenades;
}
function startReload(now) {
  const st = curStats(), am = me.ammo[me.w];
  if (!st || me.reloading >= 0 || am.mag >= st.mag || am.res <= 0 || now < me.swapAt + 350) return;
  me.reloading = me.w; me.reloadStart = now; me.reloadEnd = now + st.reload * 1000; Snd.reload(0.5, 0); send({ t: 'rl' });
}
function aimDir(yaw, pitch) { const cp = Math.cos(pitch); return [-Math.sin(yaw) * cp, Math.sin(pitch), -Math.cos(yaw) * cp]; }
function spreadDir(f, sp) {
  let ux = 0, uy = 1, uz = 0; if (Math.abs(f[1]) > 0.99) { ux = 1; uy = 0; }
  let rx = uy * f[2] - uz * f[1], ry = uz * f[0] - ux * f[2], rz = ux * f[1] - uy * f[0]; const rl = Math.hypot(rx, ry, rz); rx /= rl; ry /= rl; rz /= rl;
  const vx = f[1] * rz - f[2] * ry, vy = f[2] * rx - f[0] * rz, vz = f[0] * ry - f[1] * rx;
  const a = Math.random() * Math.PI * 2, r = Math.sqrt(Math.random()) * sp, c = Math.cos(a) * r, s = Math.sin(a) * r;
  const x = f[0] + rx * c + vx * s, y = f[1] + ry * c + vy * s, z = f[2] + rz * c + vz * s, l = Math.hypot(x, y, z);
  return [x / l, y / l, z / l];
}
const isDriver = () => me.veh >= 0 && me.seat === 0;
function tryFire(now, pressed) {
  const st = curStats(), am = me.ammo[me.w];
  if (!st || me.reloading >= 0 || now < me.nextShot || now < me.swapAt + 350 || now < me.noFireUntil || me.sprinting || isDriver()) return;
  if (am.mag <= 0) {
    if (pressed || !me.emptyClick) { Snd.empty(); me.emptyClick = true; }
    if (am.res > 0) startReload(now);
    return;
  }
  me.emptyClick = false; am.mag--; me.nextShot = now + st.interval * 1000;
  const speed = Math.hypot(me.vx, me.vz), moveFrac = clamp(speed / PLAYER.walk, 0, 1.5);
  const base = lerp(st.hip, st.ads, me.adsT);
  const inV = me.veh >= 0, vs = inV ? S.vehs.get(me.veh) : null;
  const sp = base * (1 + 0.9 * moveFrac + (inV ? 0.8 : 0)) * (me.crouch && !inV ? 0.7 : 1) + (me.onGround ? 0 : 0.04) + me.bloom;
  const f = aimDir(me.yaw, me.pitch), dirs = [];
  for (let i = 0; i < st.pellets; i++) dirs.push(spreadDir(f, sp));
  send({ t: 'shoot', w: me.w, d: dirs.map((d) => [Math.round(d[0] * 1e4) / 1e4, Math.round(d[1] * 1e4) / 1e4, Math.round(d[2] * 1e4) / 1e4]) });
  // مؤثرات محلية
  const eyeY = me.camY + me.eyeH, g = GUNS[st.kind], cy = Math.cos(me.yaw), sy = Math.sin(me.yaw);
  const mz = -g.mz + 0.1, o = [me.x - sy * mz + cy * 0.14, eyeY - 0.1, me.z - cy * mz - sy * 0.14];
  const maxT = Math.min(st.range[1] * 1.6, 300);
  for (const d of dirs) {
    let best = raycastMap(S.map, me.x, eyeY, me.z, d[0], d[1], d[2], maxT); if (best === Infinity) best = maxT;
    let hit = 0;
    for (const v of S.vehs.values()) {
      if (v === vs) continue;
      const q = rayVehicle(v, me.x, eyeY, me.z, d[0], d[1], d[2], best);
      if (q && q.t < best) { best = q.t; hit = 2; }
    }
    for (const r of S.remote.values()) {
      if (!r.alive || (teamOf(r.id) === me.team && !(S.cfg && S.cfg.ff))) continue;
      const q = rayPlayer({ x: r.x, y: r.y, z: r.z, crouch: r.crouch }, me.x, eyeY, me.z, d[0], d[1], d[2], best);
      if (q && q.t < best) { best = q.t; hit = 1; }
    }
    const e = [me.x + d[0] * best, eyeY + d[1] * best, me.z + d[2] * best];
    fx.tracer(o, e, st.muted);
    if (best < maxT && !hit) fx.decal(e[0], e[1], e[2]); else if (best < maxT && hit === 1) fx.decal(e[0], e[1], e[2], true);
    if (best < maxT) fx.spark(e[0], e[1], e[2], hit === 1 ? [0.8, 0.1, 0.1] : hit === 2 ? [1, 0.85, 0.4] : [0.85, 0.8, 0.65], hit ? 4 : 3, 2.5);
  }
  Snd.shot(st.kind, 0.75, 0, st.muted);
  if (!st.muted && st.kind !== 'sniper') fx.shell(me.x + Math.cos(me.yaw) * 0.3, me.y + 1.3, me.z - Math.sin(me.yaw) * 0.3, me.yaw, st.kind === 'shotgun' || st.kind === 'lmg');
  me.flash = 1; me.kick = 1;
  const rv = st.recoilV * (0.8 + Math.random() * 0.4) * (me.adsT > 0.5 ? 0.78 : 1) * (me.crouch && !inV ? 0.85 : 1);
  me.pitch = clamp(me.pitch + rv, -1.5, 1.5); me.recAcc += rv;
  me.yaw += (Math.random() - 0.5) * 2 * st.recoilH + Math.sin(me.shots * 0.7) * st.recoilH * 0.6;
  me.bloom = Math.min(0.025, me.bloom + st.recoilV * 0.25); me.shots++; me.lastShot = now;
  if (am.mag === 0 && am.res > 0) me.autoReloadAt = now + 300;
  updateAmmoHud();
}

// أقرب مركبة يمكن ركوبها (نفس منطق الخادم)
function nearVehicle() {
  if (me.veh >= 0 || !me.alive || me.y > 1.5) return null;
  let best = null, bd = 2.3;
  for (const v of S.vehs.values()) {
    if (v.flags & 2) continue;
    const [lx, lz] = w2l(v, me.x, me.z), d = Math.hypot(Math.max(0, Math.abs(lx) - VEH.hw), Math.max(0, Math.abs(lz) - 2.5));
    const team = v.seats.some(Boolean) ? teamOf(v.seats.find(Boolean)) : -1;
    if (d < bd && (team < 0 || team === me.team)) { bd = d; best = v; }
  }
  return best;
}
// قيادة السيارة (المحاكاة عندي، الخادم يتحقق)
function driveStep(v, dt, now) {
  LV.tires = v.tires;
  LV.dyn = []; for (const o of S.vehs.values()) if (o !== v) LV.dyn.push([o.x, o.z, o.h]);
  const inp = { thr: clamp(input.moveY, -1, 1), steer: clamp(input.moveX, -1, 1), hb: input.jumpHeld };
  const n = Math.max(1, Math.ceil(dt / (1 / 60))), h = dt / n;
  for (let i = 0; i < n; i++) { const cr = vehStep(S.map, LV, inp, h); if (cr > LV.cr) LV.cr = cr; }
  if (LV.cr > VEH.crashMin && now - (driveStep.crashSnd || 0) > 500) { driveStep.crashSnd = now; Snd.crash(clamp(LV.cr / 14, 0.3, 1), 0); me.shake = Math.max(me.shake, clamp(LV.cr / 20, 0.15, 0.7)); }
  v.x = LV.x; v.z = LV.z; v.h = LV.h; v.tx = LV.x; v.tz = LV.z; v.th = LV.h; v.speed = LV.speed; v.steer = LV.steer;
  if (inp.hb && Math.abs(LV.speed) > 8 && now - LV.skidAt > 260) { LV.skidAt = now; Snd.skid(0.35, 0); }
  if (now - LV.lastSend > 50) {
    LV.lastSend = now;
    send({ t: 'v', e: me.epoch, x: Math.round(LV.x * 100) / 100, z: Math.round(LV.z * 100) / 100, h: Math.round(LV.h * 1e3) / 1e3, vx: Math.round(LV.vx * 100) / 100, vz: Math.round(LV.vz * 100) / 100, s: Math.round(LV.steer * 1e3) / 1e3, sp: Math.round(LV.speed * 100) / 100, cr: Math.round(LV.cr * 10) / 10 });
    LV.cr = 0;
  }
}
function nextFreeSeat(v) {
  for (let k = 1; k <= SEATS.length; k++) { const s = (me.seat + k) % SEATS.length; if (!v.seats[s]) return s; }
  return -1;
}

function update(dt, now) {
  me.supp = Math.max(0, (me.supp || 0) - dt * 0.45); { const el = $('supp'); if (el) el.style.opacity = String(Math.min(0.85, me.supp * 0.9)); }
  if (me.alive && S.inGame && !S.paused && me.hp <= 30 && now - (update.hb || 0) > 650 + me.hp * 14) { update.hb = now; Snd.heart(); }
  if (S.inGame) { pollGamepad(dt); if (consume('pause') && !S.typing) { if (S.paused) resume(); else pause(); } }
  const [lx, ly] = takeLook();
  if (consume('drone')) toggleDrone(now);
  if (me.drone && me.alive && !S.paused && !S.ended && !S.typing) { droneStep(dt, now, lx, ly); return; }
  const stC = curStats();
  if (!S.paused && !S.ended && !S.typing) {
    const zf = stC ? 1 + (stC.zoom - 1) * me.adsT : 1;
    me.yaw -= lx / zf; me.pitch = clamp(me.pitch - ly / zf, -1.5, 1.5);
  }
  if (me.yaw > Math.PI * 4 || me.yaw < -Math.PI * 4) me.yaw %= Math.PI * 2;
  if (input.chat) { consume('chat'); if (!S.typing) openChat(); }
  const active = me.alive && !S.paused && !S.ended && !S.typing;
  me.kick = Math.max(0, me.kick - dt * 8); me.flash = Math.max(0, me.flash - dt * 14); me.shake = Math.max(0, me.shake - dt * 1.6);
  me.bloom = Math.max(0, me.bloom - dt * 0.05);
  if (now - me.lastShot > 300) me.shots = 0;
  // اختيار السلاح وقت الموت
  if (!me.alive && S.inGame && !S.paused && !S.typing) {
    const d = consume('dslot'); if (d >= 0) pickDeadWeapon(d);
    if (consume('attach')) openDeadAtt();
    const resp = (S.cfg ? S.cfg.resp : 3) * 1000;
    $('dtimer').textContent = Math.max(0, Math.ceil((me.deadAt + resp - now) / 1000));
  } else { consume('dslot'); consume('attach'); }
  if (!active) { me.vx = me.vz = 0; consume('jump'); consume('reload'); consume('swap'); consume('gren'); consume('firePressed'); consume('slot'); consume('use'); consume('seat'); consume('view'); if (!me.alive) { me.camY += (0.25 - me.camY) * Math.min(1, dt * 3); me.pitch += (-0.5 - me.pitch) * Math.min(1, dt * 2); } return; }
  // تبديل السلاح
  let slot = consume('slot'); if (consume('swap')) slot = 1 - me.w;
  if (slot >= 0 && slot !== me.w) { me.w = slot; me.swapAt = now; me.reloading = -1; me.emptyClick = false; if (input.resetAds) input.resetAds(); updateAmmoHud(); }
  const st = curStats(), am = me.ammo[me.w];
  // ركوب / نزول / تبديل مقعد / كاميرا
  if (consume('use')) send({ t: 'use' });
  const inV = me.veh >= 0, vObj = inV ? S.vehs.get(me.veh) : null;
  if (inV && !vObj) { me.veh = -1; }
  if (consume('seat') && vObj) { const s = nextFreeSeat(vObj); if (s >= 0) send({ t: 'seat', s }); }
  if (consume('view') && inV) me.vview = me.vview === 'fp' ? 'chase' : 'fp';
  let sp = 0;
  if (me.veh >= 0 && vObj) {
    // ---- داخل مركبة ----
    consume('jump');
    if (me.seat === 0) driveStep(vObj, dt, now);
    const [sx, sy, sz] = seatWorld(vObj, me.seat);
    me.x = sx; me.y = sy; me.z = sz; me.camY = sy; me.eyeH = PLAYER.eyec; me.crouch = true; me.vx = me.vz = 0; me.onGround = true; me.sprinting = false;
  } else {
    // القرفصاء
    if (input.crouch) me.crouch = true; else if (me.crouch && canStand(S.map, me)) me.crouch = false;
    const wantFire = input.fire;
    me.sprinting = input.sprint && input.moveY > 0.3 && !me.crouch && !input.ads && !wantFire && me.reloading < 0;
    if (me.sprintPrev && !me.sprinting) me.noFireUntil = now + 150;
    me.sprintPrev = me.sprinting;
    const mlen = Math.min(1, Math.hypot(input.moveX, input.moveY));
    let wx = -Math.sin(me.yaw) * input.moveY + Math.cos(me.yaw) * input.moveX, wz = -Math.cos(me.yaw) * input.moveY - Math.sin(me.yaw) * input.moveX;
    const wl = Math.hypot(wx, wz); if (wl > 0) { wx = wx / wl * mlen; wz = wz / wl * mlen; }
    const spd = (me.crouch ? PLAYER.crouch : me.sprinting ? PLAYER.sprint : PLAYER.walk) * st.move * (me.adsT > 0.5 ? 0.7 : 1);
    const k = Math.min(1, (me.onGround ? 14 : 2.5) * dt);
    me.vx += (wx * spd - me.vx) * k; me.vz += (wz * spd - me.vz) * k;
    const jump = consume('jump'), preVy = me.vy;
    const n = Math.max(1, Math.ceil(dt / (1 / 90))), h = dt / n;
    for (let i = 0; i < n; i++) { moveBody(S.map, me, me.vx, me.vz, h, jump && i === 0, me.crouch); for (const v of S.vehs.values()) pushOutVeh(v, me); }
    if (preVy < -7 && me.onGround) Snd.step(0.5, 0);
    me.eyeH += ((me.crouch ? PLAYER.eyec : PLAYER.eye) - me.eyeH) * Math.min(1, dt * 12);
    if (!me.onGround || Math.abs(me.y - me.camY) > 0.9) me.camY = me.y; else me.camY += (me.y - me.camY) * Math.min(1, dt * 16);
    sp = Math.hypot(me.vx, me.vz);
    if (me.onGround && sp > 0.5) { me.bob += sp * dt * 1.7; me.stepAcc += sp * dt; const stride = me.sprinting ? 3.0 : 2.2; if (me.stepAcc > stride) { me.stepAcc = 0; if (!me.crouch) Snd.step(me.sprinting ? 0.22 : 0.14, 0); } }
  }
  // التصويب
  const adsTarget = input.ads && !me.sprinting && me.reloading < 0 && now > me.swapAt + 250 && !isDriver() ? 1 : 0;
  { const kind = stC ? stC.kind : '', held = input.sprint && me.adsT > 0.6 && (me.breath === undefined || me.breath > 0.02); // Shift أثناء التصويب = حبس النفس
    me.breath = clamp((me.breath === undefined ? 1 : me.breath) + (held ? -dt * 0.28 : dt * 0.2), 0, 1);
    if (me.adsT > 0.3 && me.alive && !isDriver()) { const mvs = Math.hypot(me.vx, me.vz) > 1 ? 2 : 1, amp = (kind === 'sniper' ? 0.0042 : kind === 'lmg' ? 0.0022 : 0.0014) * me.adsT * (me.crouch ? 0.6 : 1) * (held ? 0.12 : 1) * mvs, tt = now / 1000; me.yaw += Math.cos(tt * 1.3) * amp * dt * 30; me.pitch += Math.cos(tt * 1.9 + 1) * amp * dt * 30; } }
  const adsRate = (st.kind === 'sniper' ? 1 / 0.3 : 1 / 0.17) * dt;
  me.adsT = clamp(me.adsT + clamp(adsTarget - me.adsT, -adsRate, adsRate), 0, 1);
  // إعادة التلقيم
  if (consume('reload')) startReload(now);
  if (me.autoReloadAt && now >= me.autoReloadAt) { me.autoReloadAt = 0; startReload(now); }
  if (me.reloading >= 0 && now >= me.reloadEnd) {
    const a = me.ammo[me.reloading], s2 = me.st[me.reloading], take = Math.min(s2.mag - a.mag, a.res); a.mag += take; a.res -= take; me.reloading = -1; updateAmmoHud();
  }
  // النار
  const wantFire2 = input.fire && !isDriver();
  const pressed = consume('firePressed');
  if (!isDriver()) {
    if ((st.auto ? wantFire2 : pressed) && !(me.reloading >= 0)) tryFire(now, pressed);
    else if (pressed && me.reloading < 0 && am.mag <= 0) tryFire(now, true);
  }
  if (!wantFire2) { me.emptyClick = false; const d = me.recAcc * Math.min(1, dt * 2.5); me.pitch -= d * 0.5; me.recAcc -= d; }
  // قنبلة
  if (consume('gren') && me.grenades > 0 && now > me.nextGren && !isDriver()) {
    me.nextGren = now + 700; me.grenades--; send({ t: 'gren', d: aimDir(me.yaw, me.pitch + 0.1) }); Snd.throwG(); updateAmmoHud();
  }
  me.swayX += (clamp(-lx * 3, -0.04, 0.04) - me.swayX) * Math.min(1, dt * 8); me.swayY += (clamp(ly * 3, -0.03, 0.03) - me.swayY) * Math.min(1, dt * 8);
  // إرسال الحالة (يشمل الذخيرة لتتمكن الذخيرة المتساقطة من التحقق)
  if (now - lastSend > 50) {
    lastSend = now;
    const f = (me.crouch ? 1 : 0) | (me.adsT > 0.5 || (me.veh >= 0 && now - me.lastShot < 600) ? 2 : 0) | (sp > 0.5 ? 4 : 0) | (me.sprinting ? 8 : 0);
    send({ t: 'st', e: me.epoch, x: Math.round(me.x * 100) / 100, y: Math.round(me.y * 100) / 100, z: Math.round(me.z * 100) / 100, yaw: Math.round(me.yaw * 1e3) / 1e3, pitch: Math.round(me.pitch * 1e3) / 1e3, f, w: me.w, am: [me.ammo[0].mag, me.ammo[0].res, me.ammo[1].mag, me.ammo[1].res] });
  }
}

function updateRemotes(dt, now) {
  const k = Math.min(1, dt * 14);
  // المركبات
  for (const v of S.vehs.values()) {
    const driven = me.veh === v.id && me.seat === 0;
    if (!driven) {
      const dx = v.tx - v.x, dz = v.tz - v.z;
      if (Math.hypot(dx, dz) > 8) { v.x = v.tx; v.z = v.tz; v.h = v.th; } else { v.x += dx * k; v.z += dz * k; v.h += angDiff(v.h, v.th) * k; }
    }
    v.spin = (v.spin || 0) - (v.speed || 0) * dt / 0.38;
    vehicleFx(fx, v, dt);
  }
  for (const r of S.remote.values()) {
    const dx = r.tx - r.x, dz = r.tz - r.z;
    if (Math.hypot(dx, dz) > 6) { r.x = r.tx; r.z = r.tz; r.y = r.ty; } else { r.x += dx * k; r.z += dz * k; r.y += (r.ty - r.y) * k; }
    r.yaw += angDiff(r.yaw, r.tyaw) * k; r.pitch += (r.tpitch - r.pitch) * k; r.flash = Math.max(0, r.flash - dt * 12);
    r.seated = false;
    if (r.veh > 0 && r.alive) {
      const v = S.vehs.get(r.veh - 1);
      if (v) { const [sx, sy, sz] = seatWorld(v, r.seat); r.x = sx; r.y = sy; r.z = sz; r.seated = true; r.moving = false; continue; }
    }
    if (r.moving && r.alive) {
      r.phase += dt * (r.sprint ? 12 : 9); r.stepAcc += dt;
      if (r.stepAcc > (r.sprint ? 0.3 : 0.42) && !r.crouch && me.alive) {
        r.stepAcc = 0; const d = Math.hypot(r.x - me.x, r.z - me.z);
        if (d < 24) {
          const eyeY = me.camY + me.eyeH, dy = r.y + 1 - eyeY, L = Math.hypot(d, dy) || 1;
          const blocked = raycastMap(S.map, me.x, eyeY, me.z, (r.x - me.x) / L, dy / L, (r.z - me.z) / L, L) !== Infinity;
          Snd.step(Math.pow(1 - d / 24, 2) * (blocked ? 0.25 : 0.55), panOf(r), clamp(d / 24, 0, 1));
        }
      }
    }
  }
  for (const g of S.grenades.values()) { g.x += (g.tx - g.x) * k; g.y += (g.ty - g.y) * k; g.z += (g.tz - g.z) * k; }
  // صوت المحرك: أقرب مركبة عليها سائق
  let best = null, bd = 1e9;
  for (const v of S.vehs.values()) { if ((v.flags & 2) || !v.seats[0]) continue; const d = Math.hypot(v.x - me.x, v.z - me.z); if (d < bd) { bd = d; best = v; } }
  if (best && bd < 70) {
    const own = me.veh === best.id, rpm = clamp(Math.abs(best.speed) / VEH.maxFwd, 0, 1);
    Snd.engine(rpm, own ? 0.9 : Math.pow(1 - bd / 70, 2), own ? 0 : panOf(best));
  } else Snd.engineOff();
}
function leanFor(v, seat, yaw, active) {
  if (!active || seat !== 1) return null;
  const L = 0.4 * Math.sign(SEATS[seat].x || 1);
  return [L * Math.cos(v.h - yaw), L * Math.sin(yaw - v.h)];
}

// ---------- الرسم ----------
const nameEls = new Map(), pt = { x: 0, y: 0, w: 0 };
function render(dt, now) {
  const st = curStats();
  const BF = settings.fov || 76; let fov = BF, cy = me.camY + me.eyeH;
  if (me.alive && st) { const Z = 1 + (st.zoom - 1) * me.adsT; fov = (2 * Math.atan(Math.tan(BF * Math.PI / 360) / Z)) * 180 / Math.PI; }
  const bobY = me.alive && me.onGround && me.veh < 0 ? Math.sin(me.bob * 2) * 0.014 * (me.sprinting ? 1.6 : 1) * (1 - me.adsT * 0.8) : 0;
  let cam = { x: me.x, y: cy + bobY, z: me.z, yaw: me.yaw, pitch: me.pitch, fov };
  const myV = me.alive && me.veh >= 0 ? S.vehs.get(me.veh) : null;
  const chase = !!myV && me.vview === 'chase';
  if (chase) {
    const d = aimDir(me.yaw, me.pitch), tx = myV.x, ty = 1.5, tz = myV.z; let dist = 7.5;
    const t = raycastMap(S.map, tx, ty, tz, -d[0], -d[1], -d[2], dist + 0.4); if (t !== Infinity) dist = Math.max(1.5, t - 0.4);
    cam = { x: tx - d[0] * dist, y: Math.max(0.6, ty - d[1] * dist), z: tz - d[2] * dist, yaw: me.yaw, pitch: me.pitch, fov: 74 };
  }
  const droning = !!me.drone && me.alive;
  if (droning) cam = { x: me.drone.x, y: me.drone.y, z: me.drone.z, yaw: me.drone.yaw, pitch: me.drone.pitch, fov: 88 };
  if (!droning && !me.alive && me.killerId && !S.ended && !chase) {
    const kr = S.remote.get(me.killerId);
    if (kr && kr.alive) {
      const sy = Math.sin(kr.yaw), cyw = Math.cos(kr.yaw), ky = kr.y + 1.5; let dist = 3.2;
      const t = raycastMap(S.map, kr.x, ky, kr.z, sy, 0, cyw, dist + 0.3); if (t !== Infinity) dist = Math.max(0.8, t - 0.3);
      cam = { x: kr.x + sy * dist, y: ky + 0.5, z: kr.z + cyw * dist, yaw: kr.yaw, pitch: -0.12, fov: 70 };
    }
  }
  if (me.shake > 0) { cam.yaw += (Math.random() - 0.5) * 0.03 * me.shake; cam.pitch += (Math.random() - 0.5) * 0.03 * me.shake; }
  eng.begin(cam); eng.drawMap();
  for (const v of S.vehs.values()) { if (!eng.visible(v.x, 1, v.z, 4)) continue; drawVehicle(eng, v, now, Math.hypot(v.x - cam.x, v.z - cam.z) > 55, S.cfg && S.cfg.tod > 0); }
  for (const k of S.kits.values()) if (k.up && Math.hypot(k.x - cam.x, k.z - cam.z) < 80 && eng.visible(k.x, 0.6, k.z, 1)) drawKit(eng, k, now);
  if (S.zone) drawZone(eng, S.zone, now);
  const mineIds = new Set(me.st.filter(Boolean).map((s) => s.id));
  for (const d of S.drops.values()) { if (Math.hypot(d.x - cam.x, d.z - cam.z) < 70 && eng.visible(d.x, d.y, d.z, 1)) drawDrop(eng, d, now, mineIds.has(d.w)); }
  for (const r of S.remote.values()) {
    const dead = !r.alive, dt2 = (now - r.deadAt) / 1000;
    if (dead && dt2 > 4) continue;
    const info = S.players.get(r.id); if (!info) continue;
    const v = r.seated ? S.vehs.get(r.veh - 1) : null, cab = r.seated && r.seat < 2;
    const dd = Math.hypot(r.x - cam.x, r.z - cam.z); if (dd > 110 || !eng.visible(r.x, r.y + 1, r.z, 1.5)) continue;
    drawAvatar(eng, { far: dd > 38, x: r.x, y: r.y - (cab ? 0.36 : 0), z: r.z, yaw: r.yaw, pitch: r.pitch, crouch: r.seated ? !cab : r.crouch, sit: cab, lean: v ? leanFor(v, r.seat, r.yaw, r.ads || r.reveal) : null, team: info.tm, moving: r.moving, sprint: r.sprint, stats: statsFor(info, r.w === 1 ? 1 : 0), deadT: dead ? dt2 : -1, flash: r.flash, phase: r.phase }, now);
  }
  for (const o of S.drones.values()) {
    o.x += (o.tx - o.x) * Math.min(1, dt * 14); o.y += (o.ty - o.y) * Math.min(1, dt * 14); o.z += (o.tz - o.z) * Math.min(1, dt * 14);
    let dy = o.tyaw - o.yaw; while (dy > Math.PI) dy -= 2 * Math.PI; while (dy < -Math.PI) dy += 2 * Math.PI; o.yaw += dy * Math.min(1, dt * 10);
    if (o.id === S.id && droning) continue;
    if (Math.hypot(o.x - cam.x, o.z - cam.z) > 90 || !eng.visible(o.x, o.y, o.z, 1)) continue;
    const inf = S.players.get(o.id); drawDrone(eng, o, inf ? inf.tm : 0, now);
  }
  { let best = 1e9, bo = null; for (const o of S.drones.values()) { const dd2 = Math.hypot(o.x - me.x, o.z - me.z); if (dd2 < best && !(o.id === S.id && droning)) { best = dd2; bo = o; } }
    if (droning) Snd.drone(0.8, 0); else if (bo && best < 45) Snd.drone(Math.pow(1 - best / 45, 2) * 0.9, panOf(bo)); else if (Snd._dr) Snd.drone(0); }
  if (droning || (chase)) { // أرسم نفسي جالسًا عندما تكون الكاميرا خارجية
    const cab = !droning && me.seat < 2, info = S.players.get(S.id);
    drawAvatar(eng, { x: me.x, y: me.y - (cab ? 0.36 : 0), z: me.z, yaw: me.yaw, pitch: me.pitch, crouch: droning ? me.crouch : !cab, sit: cab, lean: null, team: me.team, moving: false, sprint: false, stats: droning || me.seat === 0 ? null : st, deadT: -1, flash: me.flash, phase: 0 }, now);
  }
  for (const g of S.grenades.values()) drawGrenade(eng, g.x, g.y, g.z);
  fx.update(dt); fx.draw(eng); eng.flush();
  const scoped = me.alive && st && me.adsT > 0.92 && st.zoom >= 3.4 && !chase && !droning;
  if (me.alive && st && !scoped && !chase && !isDriver() && !droning) {
    drawViewmodel(eng, { stats: st, ads: me.adsT, kick: me.kick, flash: me.flash, reload: me.reloading >= 0 ? clamp((now - me.reloadStart) / (me.reloadEnd - me.reloadStart), 0, 1) : -1, swap: clamp((now - me.swapAt) / 350, 0, 1), sprint: me.sprinting, team: me.team, bobX: Math.cos(me.bob) * 0.006 * (me.onGround ? 1 : 0), bobY: bobY * 0.8, swayX: me.swayX, swayY: me.swayY });
  }
  $('scope').classList.toggle('hidden', !scoped);
  hud(dt, now, st, chase);
}

function hud(dt, now, st, chase) {
  const slow = now - (hud.last || 0) > 60; if (slow) hud.last = now;
  if (slow) hudSlow(now, st, chase);
  hudFast(now, st, chase);
}
function hudSlow(now, st, chase) {
  $('sc0').textContent = S.score[0]; $('sc1').textContent = S.score[1];
  const tl = Math.max(0, S.timeLeft | 0); $('timer').textContent = `${Math.floor(tl / 60)}:${String(tl % 60).padStart(2, '0')}`;
  const hp = clamp(me.hp, 0, 100);
  $('hpfill').style.width = hp + '%'; $('hpghost').style.width = hp + '%';
  $('hpfill').style.background = hp > 50 ? '' : hp > 25 ? 'linear-gradient(90deg,#f39c12,#f7c46a)' : 'linear-gradient(90deg,#e74c3c,#ff8a7a)';
  $('shfill').style.width = clamp(me.shield / 60 * 100, 0, 100) + '%';
  $('hpnum').textContent = Math.max(0, Math.round(me.hp)); $('hpbox').classList.toggle('low', me.alive && hp <= 30);
  if (me.hp < 60 && me.alive) $('vign').style.opacity = String(clamp((60 - me.hp) / 60, 0, 0.8) * 0.8);
  else if (!me.alive) $('vign').style.opacity = '0.5'; else $('vign').style.opacity = '0';
  if (me.reloading >= 0) $('mag').textContent = '…'; else if (st) $('mag').textContent = me.ammo[me.w].mag;
  $('res').textContent = st ? me.ammo[me.w].res : 0;
  $('prot').classList.toggle('hidden', !(me.alive && now < me.protUntil));
  const zh = $('zonehud'), z = S.zone;
  if (z) {
    const mate = z.state === me.team, txt = z.state === 3 ? 'المنطقة فارغة — اذهب إليها!' : z.state === 2 ? 'المنطقة متنازع عليها' : mate ? 'فريقك يسيطر' : 'العدو يسيطر';
    zh.className = 's' + z.state; zh.textContent = `⚑ ${txt} · تتحرك خلال ${z.left}ث`;
  } else zh.className = 'hidden';
  const dh = $('dronehud'), dr = me.drone && me.alive;
  dh.classList.toggle('hidden', !dr);
  if (dr) {
    const left = Math.max(0, (me.drone.until - now) / 1000); $('dtime').textContent = Math.ceil(left) + 'ث'; $('dbat').style.width = (left / (DRONE.ms / 1000) * 100) + '%';
    $('dhint').textContent = input.touch ? 'زر النار = تفجير · زر الطائرة = هبوط' : 'WASD طيران · Space/C صعود/هبوط · النار تفجير · B هبوط';
  }
  $('uavtag').classList.toggle('hidden', !(S.uavUntil[me.team] > now));
  // مؤشر التقاطع
  const showCross = st && me.alive && !isDriver() && !chase;
  if (showCross) {
    const sp = lerp(st.hip, st.ads, me.adsT) * (1 + 0.9 * clamp(Math.hypot(me.vx, me.vz) / PLAYER.walk, 0, 1.5)) + me.bloom + (me.onGround ? 0 : 0.04);
    const gap = clamp(sp * 700, 3, 60); const c = $('cross');
    c.style.display = me.adsT > 0.6 ? 'none' : '';
    c.querySelector('.cl').style.left = -(gap + 9) + 'px'; c.querySelector('.cr').style.left = gap + 'px';
    c.querySelector('.ct').style.top = -(gap + 9) + 'px'; c.querySelector('.cb').style.top = gap + 'px';
  } else $('cross').style.display = 'none';
  // المركبة: لوحة القيادة والتلميحات
  const near = me.alive ? nearVehicle() : null, myV = me.alive && me.veh >= 0 ? S.vehs.get(me.veh) : null;
  document.body.classList.toggle('invehicle', !!myV); document.body.classList.toggle('driving', !!myV && me.seat === 0); document.body.classList.toggle('canuse', !!near);
  $('vhud').classList.toggle('hidden', !myV);
  if (myV) {
    $('vspd').textContent = Math.round(Math.abs(myV.speed) * 3.6);
    $('vhp').style.width = clamp(myV.hp / 450 * 100, 0, 100) + '%';
    const ti = $('vtires').children; for (let i = 0; i < 4; i++) ti[i].className = (myV.tires >> i & 1) ? 'bad' : 'ok';
    $('vseat').textContent = ['السائق', 'الراكب الأمامي', 'الصندوق (يسار)', 'الصندوق (يمين)', 'الصندوق (خلف)'][me.seat];
    if (myV.flags & 1) { $('prompt').textContent = 'المركبة تحترق! انزل فورًا'; $('prompt').classList.remove('hidden'); }
    else { $('prompt').textContent = input.touch ? '' : 'E نزول · F تبديل المقعد · V الكاميرا' + (me.seat === 0 ? ' · Space فرامل اليد' : ''); $('prompt').classList.toggle('hidden', input.touch || now - me.seatAt > 6000); }
  } else if (near) { $('prompt').textContent = input.touch ? 'اضغط زر الركوب' : 'E — اركب الشاص'; $('prompt').classList.remove('hidden'); }
  else $('prompt').classList.add('hidden');
  // لوحة النتائج
  const showBoard = input.board || S.ended;
  $('board').classList.toggle('hidden', !showBoard || S.ended);
  if (showBoard && !S.ended && (now - hud.lastBoard > 400 || !hud.lastBoard)) { hud.lastBoard = now; renderBoard(); }
}
function hudFast(now, st, chase) {
  const dm = $('dmarks');
  if (me.drone && me.alive) {
    let h = '';
    for (const r of S.remote.values()) {
      const info = S.players.get(r.id); if (!info || !r.alive || info.tm === me.team || !r.reveal) continue;
      if (!eng.project(r.x, r.y + 2.1, r.z, pt)) continue;
      h += `<i style="left:${pt.x * 100}%;top:${pt.y * 100}%">▼<small>${Math.round(Math.hypot(r.x - me.drone.x, r.z - me.drone.z))}م</small></i>`;
    }
    if (dm.innerHTML !== h) dm.innerHTML = h;
  } else if (dm.innerHTML) dm.innerHTML = '';
  // أسماء الزملاء
  const used = new Set();
  if (me.alive) for (const r of S.remote.values()) {
    const info = S.players.get(r.id); if (!info || !r.alive || info.tm !== me.team) continue;
    if (!eng.project(r.x, r.y + (r.seated ? 1.9 : r.crouch ? 1.45 : 2.05), r.z, pt)) continue;
    const d = Math.hypot(r.x - me.x, r.z - me.z); if (d > 70) continue;
    let el = nameEls.get(r.id); if (!el) { el = document.createElement('div'); $('names').appendChild(el); nameEls.set(r.id, el); }
    el.textContent = info.n; el.style.color = info.tm === 0 ? '#ff8b8b' : '#8bbcff';
    el.style.left = (pt.x * 100) + '%'; el.style.top = (pt.y * 100) + '%'; el.style.display = ''; used.add(r.id);
  }
  for (const [id, el] of nameEls) if (!used.has(id)) { if (!S.remote.has(id)) { el.remove(); nameEls.delete(id); } else el.style.display = 'none'; }
  if (now - lastRadar > 100) { lastRadar = now; radar(); }
  if (now - (hudFast.t || 0) > 500) { hudFast.t = now; $('stat').textContent = !settings.showfps ? '' : `${Math.round(1 / emaDt)} fps${S.mode === 'solo' ? '' : ' · ' + S.ping + ' ms'}`;
  $('ping').textContent = S.ping && S.mode !== 'solo' ? `${S.ping}ms` : ''; }
}
function radar() {
  const cv = $('radar'), c = cv.getContext('2d'), W = cv.width, R = 38, sc = W / 2 / R;
  c.clearRect(0, 0, W, W); c.save(); c.beginPath(); c.arc(W / 2, W / 2, W / 2 - 1, 0, Math.PI * 2); c.clip();
  c.translate(W / 2, W / 2); c.rotate(me.yaw); c.scale(sc, sc);
  c.fillStyle = 'rgba(170,190,210,.45)';
  if (radar.map !== S.map) { radar.map = S.map; radar.list = S.map.boxes.filter((b) => b.y1 >= 0.8 && b.y0 <= 2.5); }
  for (const b of radar.list) { if (Math.abs(b.x0 - me.x) > R * 1.5 && Math.abs(b.x1 - me.x) > R * 1.5) continue; if (Math.abs(b.z0 - me.z) > R * 1.5 && Math.abs(b.z1 - me.z) > R * 1.5) continue; c.fillRect(b.x0 - me.x, b.z0 - me.z, b.x1 - b.x0, b.z1 - b.z0); }
  for (const v of S.vehs.values()) { c.save(); c.translate(v.x - me.x, v.z - me.z); c.rotate(-v.h); c.fillStyle = v.flags & 2 ? '#555' : '#ffd23b'; c.fillRect(-1, -2.5, 2, 5); c.restore(); }
  for (const k of S.kits.values()) if (k.up) { c.fillStyle = '#fff'; c.fillRect(k.x - me.x - 1, k.z - me.z - 1, 2, 2); c.fillStyle = '#e74c3c'; c.fillRect(k.x - me.x - 0.25, k.z - me.z - 0.9, 0.5, 1.8); c.fillRect(k.x - me.x - 0.9, k.z - me.z - 0.25, 1.8, 0.5); }
  if (S.zone) { const zc = ['#ff4d4d', '#4d8dff', '#b04dff', '#ffffff'][S.zone.state]; c.strokeStyle = zc; c.lineWidth = 0.9; c.beginPath(); c.arc(S.zone.x - me.x, S.zone.z - me.z, S.zone.r, 0, 6.283); c.stroke(); c.fillStyle = zc + '33'; c.fill(); }
  const mine = new Set(me.st.filter(Boolean).map((s) => s.id));
  c.fillStyle = '#7be0a0'; for (const d of S.drops.values()) if (mine.has(d.w)) c.fillRect(d.x - me.x - 0.7, d.z - me.z - 0.7, 1.4, 1.4);
  for (const r of S.remote.values()) {
    const info = S.players.get(r.id); if (!info || !r.alive) continue;
    const mate = info.tm === me.team; if (!mate && !r.reveal && !(S.uavUntil[me.team] > nowMs())) continue;
    c.fillStyle = info.tm === 0 ? '#ff4d4d' : '#4d8dff'; c.beginPath(); c.arc(r.x - me.x, r.z - me.z, mate ? 1.7 : 2.2, 0, Math.PI * 2); c.fill();
    if (!mate) { c.strokeStyle = '#fff'; c.lineWidth = 0.4; c.stroke(); }
  }
  c.restore();
  c.fillStyle = '#ffd23b'; c.beginPath(); c.moveTo(W / 2, W / 2 - 9); c.lineTo(W / 2 - 6, W / 2 + 7); c.lineTo(W / 2 + 6, W / 2 + 7); c.closePath(); c.fill();
}

// ---------- الحلقة ----------
function frame(ts) {
  requestAnimationFrame(frame);
  if (!eng || eng.lost) return;
  const dt = Math.min(0.05, (ts - lastT) / 1000 || 0.016); lastT = ts;
  emaDt = emaDt * 0.95 + dt * 0.05;
  if (!S.inGame) return;
  if (emaDt > 0.036) { if (++slowCount > 60 && settings.quality > 0.4) { settings.quality = Math.round((settings.quality - 0.1) * 10) / 10; eng.resize(settings.quality); slowCount = 0; toast('تم خفض الجودة لتحسين الأداء'); } } else slowCount = Math.max(0, slowCount - 2);
  const now = nowMs();
  if (now - lastPing > 2000) { lastPing = now; send({ t: 'ping', c: now }); }
  try { update(dt, now); updateRemotes(dt, now); render(dt, now); } catch (e) { console.error(e); try { window.__dbg && window.__dbg('frame: ' + (e && e.stack ? String(e.stack).slice(0, 300) : e)); } catch { /* */ } if (!frame.err) { frame.err = true; toast('خطأ: ' + (e.message || e), 6000); } }
}

// ---------- الإقلاع ----------
function boot() {
  document.querySelectorAll('[data-ic]').forEach((el) => { el.outerHTML = icon(el.dataset.ic, Number(el.dataset.s) || 28); });
  bindUI(); enhanceSelects();
  try { eng = new Engine($('gl')); eng.resize(settings.quality); }
  catch (e) { showError('جهازك لا يدعم الرسم ثلاثي الأبعاد', String(e.message || e), 'إعادة المحاولة'); return; }
  initInput($('gl'), { pause: () => { if (S.typing) return; if (S.paused) resume(); else pause(); }, unlocked: () => { if (S.inGame && !S.ended && !S.paused && !input.touch && me.alive && !S.typing) pause(); }, canLock: () => S.inGame && !S.paused && !S.ended && !S.typing && !!$('gl').requestPointerLock });
  applySettings(); renderProf();
  window.addEventListener('resize', () => eng && eng.resize());
  show('menu');
  requestAnimationFrame(frame);
  try { if (navigator.wakeLock) navigator.wakeLock.request('screen').catch(() => {}); } catch { /* */ }
  window.__wz = { S, me, LV, eng, startSolo, joinTo, send };
}
boot();
