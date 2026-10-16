// Warzone LAN — العميل: الشبكة، الواجهة، اللعب
import { Engine, tmp } from './gl.js';
import { WEAPONS, PRIMARIES, SECONDARIES, OPTICS, MUZZLES, UNDERS, MAGS, computeStats, sanitizeLoadout, defaultLoadout, sanitizeAtt } from '/shared/weapons.js';
import { buildMap } from '/shared/maps.js';
import { PLAYER, moveBody, canStand, raycastMap, rayPlayer } from '/shared/physics.js';
import { scan, probe } from './lan.js';
import { Snd } from './audio.js';
import { input, consume, takeLook, initInput, resetInput, isTouchDevice } from './input.js';
import { icon, gunIcon } from './icons.js';
import { drawAvatar, drawViewmodel, Effects, drawGrenade, GUNS, sightTop } from './models.js';

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
const settings = { name: store.get('name', ''), quality: store.get('quality', 0.8), sens: store.get('sens', 5), vol: store.get('vol', 0.7), lo: sanitizeLoadout(store.get('lo', null)) };

// ---------- الحالة ----------
const S = {
  ws: null, id: 0, hostId: 0, connected: false, phase: 'lobby', cfg: null, players: new Map(), remote: new Map(), maps: [],
  map: null, inGame: false, paused: false, timeLeft: 0, score: [0, 0], grenades: new Map(), ping: 0, ended: false,
  entered: false, retry: 0,
};
const me = {
  x: 0, y: 0, z: 0, vy: 0, vx: 0, vz: 0, onGround: true, yaw: 0, pitch: 0, crouch: false, eyeH: PLAYER.eye, camY: 0,
  hp: 100, alive: false, w: 0, epoch: 0, team: 0, st: [null, null], ammo: [{ mag: 0, res: 0 }, { mag: 0, res: 0 }],
  reloading: -1, reloadStart: 0, reloadEnd: 0, swapAt: 0, nextShot: 0, noFireUntil: 0, shots: 0, lastShot: 0, recAcc: 0, bloom: 0,
  adsT: 0, grenades: 2, nextGren: 0, stepAcc: 0, bob: 0, deadAt: 0, sprinting: false, sprintPrev: false, kick: 0, flash: 0,
  swayX: 0, swayY: 0, emptyClick: false, autoReloadAt: 0, protUntil: 0, fell: 0,
};
let eng = null, fx = new Effects(), lastT = 0, lastSend = 0, lastPing = 0, lastRadar = 0, emaDt = 0.016, slowCount = 0;

// ---------- الشاشات ----------
const SCREENS = ['join', 'menu', 'lobby', 'loadout', 'pause', 'end', 'err'];
function show(name) { for (const s of SCREENS) $(s).classList.toggle('hidden', s !== name); }
function hideScreens() { for (const s of SCREENS) $(s).classList.add('hidden'); }
function showError(title, msg, btn = 'إعادة المحاولة') { $('errTitle').textContent = title; $('errMsg').textContent = msg; $('btnReload').textContent = btn; show('err'); }
function toast(msg, ms = 2200) { const t = $('toast'); t.textContent = msg; t.classList.add('on'); clearTimeout(toast.h); toast.h = setTimeout(() => t.classList.remove('on'), ms); }

// ---------- الشبكة ----------
function send(o) { if (S.ws && S.ws.readyState === 1) S.ws.send(JSON.stringify(o)); }
const APP = !!(window.Capacitor && (window.Capacitor.isNativePlatform ? window.Capacitor.isNativePlatform() : true));
let serverAddr = APP ? '' : location.host, everOpen = false;
function connect() {
  const proto = !APP && location.protocol === 'https:' ? 'wss' : 'ws';
  let ws;
  try { ws = new WebSocket(`${proto}://${serverAddr}`); } catch (e) { showError('تعذّر الاتصال', 'لا يمكن فتح الاتصال بالخادم.'); return; }
  S.ws = ws;
  const openTimer = setTimeout(() => { if (ws.readyState !== 1) { try { ws.close(); } catch { /* */ } } }, 5000);
  ws.onopen = () => {
    clearTimeout(openTimer); everOpen = true;
    S.connected = true; S.retry = 0;
    send({ t: 'hello', n: settings.name, lo: settings.lo });
    $('menuMsg').textContent = '';
    if (!$('err').classList.contains('hidden') && S.entered) { if (S.inGame) hideScreens(); else show('lobby'); }
  };
  ws.onmessage = (e) => { let m; try { m = JSON.parse(e.data); } catch { return; } try { onMsg(m); } catch (err) { console.error(err); } };
  ws.onclose = () => {
    clearTimeout(openTimer);
    if (S.ws !== ws) return;
    S.connected = false;
    if (APP && !everOpen) { $('joinMsg').textContent = 'تعذّر الاتصال بـ ' + serverAddr + ' — تأكد أن الخادم يعمل وأنكما على نفس الشبكة.'; show('join'); return; }
    if (!S.entered) { $('menuMsg').textContent = 'جارٍ الاتصال بالخادم…'; }
    else showError('انقطع الاتصال', 'نحاول إعادة الاتصال تلقائيًا…', 'تحديث الصفحة');
    S.retry++; setTimeout(connect, Math.min(3000, 600 * S.retry));
  };
  ws.onerror = () => {};
}

function onMsg(m) {
  switch (m.t) {
    case 'welcome': if (APP && $('join') && !$('join').classList.contains('hidden')) show('menu'); S.id = m.id; S.maps = m.maps || []; S.remote.clear(); S.players.clear(); fillMaps(); break;
    case 'full': showError('الغرفة ممتلئة', 'وصلت الغرفة للحد الأقصى من اللاعبين.', 'تحديث الصفحة'); break;
    case 'lobby': onLobby(m); break;
    case 'start': startGame(m); break;
    case 'tolobby': leaveGame(); break;
    case 'spawn': onSpawn(m); break;
    case 'snap': onSnap(m); break;
    case 'shot': onShot(m); break;
    case 'hit': onHit(m); break;
    case 'kill': onKill(m); break;
    case 'boom': onBoom(m); break;
    case 'rl': { const r = S.remote.get(m.id); if (r && S.inGame) { const d = dist3(r, me); Snd.reload(clamp(1 - d / 25, 0, 1) * 0.6, panOf(r)); } break; }
    case 'left': S.remote.delete(m.id); break;
    case 'end': onEnd(m); break;
    case 'pong': S.ping = Math.round(nowMs() - m.c); break;
    default: break;
  }
}
const dist3 = (a, b) => Math.hypot(a.x - b.x, (a.y || 0) - (b.y || 0), a.z - b.z);
function panOf(p) { const dx = p.x - me.x, dz = p.z - me.z, rx = Math.cos(me.yaw), rz = -Math.sin(me.yaw); const l = Math.hypot(dx, dz) || 1; return clamp((dx * rx + dz * rz) / l, -1, 1); }

// ---------- اللوبي ----------
function fillMaps() {
  const sel = $('cfgMap'); sel.innerHTML = '';
  S.maps.forEach((n, i) => { const o = document.createElement('option'); o.value = i; o.textContent = n; sel.appendChild(o); });
}
function onLobby(m) {
  S.hostId = m.host; S.cfg = m.cfg; S.phase = m.ph;
  S.players.clear(); for (const p of m.ps) S.players.set(p.id, p);
  // عند ظهور/اختفاء لاعبين نحدّث الأفاتار
  for (const id of [...S.remote.keys()]) if (!S.players.has(id)) S.remote.delete(id);
  if (S.entered && !S.inGame && m.ph === 'lobby' && $('end').classList.contains('hidden') && $('err').classList.contains('hidden') && $('loadout').classList.contains('hidden')) show('lobby');
  renderLobby();
  if (S.inGame && !$('board').classList.contains('hidden')) renderBoard();
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
      li.innerHTML = `<span>${p.b ? '<span class="tag">BOT</span> ' : ''}${esc(p.n)}${p.id === S.hostId ? ' <span class="tag host">المضيف</span>' : ''}</span><small>${esc(WEAPONS[p.p] ? WEAPONS[p.p].name : '')}</small>`;
      ul.appendChild(li);
    }
    $('c' + t).textContent = `(${n})`;
  }
  const set = (id, v) => { const el = $(id); if (el.value !== String(v)) el.value = v; el.disabled = !isHost; };
  set('cfgMap', S.cfg.map); set('cfgScore', S.cfg.scoreLimit); set('cfgTime', S.cfg.timeLimit); set('cfgFill', S.cfg.fill);
  $('btnStart').disabled = !isHost;
  $('hostMsg').textContent = isHost ? 'أنت المضيف. اضبط الإعدادات ثم ابدأ المباراة.' : 'بانتظار المضيف لبدء المباراة…';
  document.querySelectorAll('#lobby button[data-team]').forEach((b) => { b.disabled = S.phase !== 'lobby'; });
}
async function showAddr() {
  let txt = location.origin;
  try {
    if (/^(localhost|127\.|\[::1\])/.test(location.hostname)) {
      const r = await fetch('/api/info'); const j = await r.json();
      if (j.ips && j.ips.length) txt = j.ips.map((ip) => `http://${ip}:${location.port || 80}`).join('  |  ');
      else txt = location.origin + '  (اكتب عنوان الشبكة لجهازك للأصدقاء)';
    }
  } catch { /* */ }
  $('addr').textContent = 'للأصدقاء: ' + txt;
}

// ---------- تجهيز العتاد ----------
let loReturn = 'lobby';
function renderLoadout() {
  const build = (slot, ids, boxId) => {
    const box = $(boxId); box.innerHTML = '';
    const id = slot === 0 ? settings.lo.p : settings.lo.s;
    const wg = document.createElement('div'); wg.className = 'wgrid';
    for (const wid of ids) {
      const b = document.createElement('button'); b.className = 'wbtn' + (wid === id ? ' on' : ''); b.innerHTML = gunIcon(WEAPONS[wid].kind, 84, 26) + '<span>' + WEAPONS[wid].name + '</span>';
      b.onclick = () => { if (slot === 0) settings.lo.p = wid; else settings.lo.s = wid; commitLoadout(); };
      wg.appendChild(b);
    }
    box.appendChild(wg);
    const st = computeStats(id, settings.lo.a[slot]);
    const bars = document.createElement('div'); bars.className = 'bars';
    const row = (n, v) => { bars.insertAdjacentHTML('beforeend', `<span>${n}</span><div class="bar"><i style="width:${clamp(v, 0.04, 1) * 100}%"></i></div>`); };
    row('الضرر', st.dmg * st.pellets * (st.pellets > 1 ? 0.5 : 1) / 110); row('معدل النار', st.rpm / 900); row('المدى', st.range[1] / 200);
    row('الثبات', 1 - st.recoilV / 0.05); row('الحركة', (st.move - 0.8) / 0.35); row('الذخيرة', st.mag / 60);
    box.appendChild(bars);
    const atts = document.createElement('div'); atts.className = 'atts';
    const sel = (label, key, opts) => {
      const l = document.createElement('label'); l.textContent = label;
      const s = document.createElement('select');
      for (const [k, v] of opts) { const o = document.createElement('option'); o.value = k; o.textContent = v; if (settings.lo.a[slot][key] === k) o.selected = true; s.appendChild(o); }
      s.onchange = () => { settings.lo.a[slot][key] = s.value; commitLoadout(); };
      l.appendChild(s); atts.appendChild(l);
    };
    sel('المنظار', 'optic', WEAPONS[id].optics.map((k) => [k, OPTICS[k].name]));
    sel('الفوهة', 'muzzle', Object.entries(MUZZLES).map(([k, v]) => [k, v.name]));
    sel('أسفل السلاح', 'under', Object.entries(UNDERS).map(([k, v]) => [k, v.name]));
    sel('المخزن', 'mag', Object.entries(MAGS).map(([k, v]) => [k, v.name]));
    box.appendChild(atts);
  };
  build(0, PRIMARIES, 'loP'); build(1, SECONDARIES, 'loS');
}
function commitLoadout() {
  settings.lo = sanitizeLoadout(settings.lo); store.set('lo', settings.lo);
  renderLoadout(); send({ t: 'lo', lo: settings.lo });
}

// ---------- القائمة ----------
function applySettings() {
  input.sensMouse = 0.0007 * (settings.sens + 1) * 0.8 + 0.0008;
  input.sensTouch = 0.0012 * (settings.sens + 1) * 0.75 + 0.0006;
  Snd.setVolume(settings.vol);
  if (eng) eng.resize(settings.quality);
}
function bindUI() {
  $('name').value = settings.name; $('quality').value = String(settings.quality);
  $('sens').value = settings.sens; $('vol').value = settings.vol; $('quality2').value = String(settings.quality);
  $('btnEnter').onclick = () => {
    settings.name = $('name').value.trim().slice(0, 14); settings.quality = Number($('quality').value);
    store.set('name', settings.name); store.set('quality', settings.quality);
    Snd.init(); applySettings();
    try { if (document.documentElement.requestFullscreen && isTouchDevice()) document.documentElement.requestFullscreen().then(() => { try { screen.orientation.lock('landscape').catch(() => {}); } catch { /* */ } }).catch(() => {}); } catch { /* */ }
    S.entered = true; send({ t: 'hello', n: settings.name, lo: settings.lo });
    if (S.phase !== 'lobby' && S.inGame) hideScreens(); else show('lobby');
    showAddr();
  };
  document.querySelectorAll('#lobby button[data-team]').forEach((b) => { b.onclick = () => { Snd.click(); send({ t: 'team', v: Number(b.dataset.team) }); }; });
  const cfgSend = () => send({ t: 'cfg', map: Number($('cfgMap').value), scoreLimit: Number($('cfgScore').value), timeLimit: Number($('cfgTime').value), fill: Number($('cfgFill').value) });
  for (const id of ['cfgMap', 'cfgScore', 'cfgTime', 'cfgFill']) $(id).onchange = cfgSend;
  $('btnStart').onclick = () => { Snd.init(); send({ t: 'start' }); };
  $('btnLoadout').onclick = () => { loReturn = 'lobby'; renderLoadout(); show('loadout'); };
  $('btnLo2').onclick = () => { loReturn = 'pause'; renderLoadout(); show('loadout'); };
  $('btnLoDone').onclick = () => show(loReturn);
  $('btnResume').onclick = resume;
  $('btnBack').onclick = () => { if (S.hostId === S.id) send({ t: 'back' }); else { S.entered = false; location.reload(); } };
  $('btnEndBack').onclick = () => { if (S.hostId === S.id) send({ t: 'back' }); else { $('endMsg').textContent = 'بانتظار المضيف أو العودة التلقائية…'; } };
  $('btnFull').onclick = () => { try { document.documentElement.requestFullscreen(); } catch { toast('ملء الشاشة غير مدعوم هنا'); } };
  $('btnReload').onclick = () => location.reload();
  $('sens').oninput = (e) => { settings.sens = Number(e.target.value); store.set('sens', settings.sens); applySettings(); };
  $('vol').oninput = (e) => { settings.vol = Number(e.target.value); store.set('vol', settings.vol); applySettings(); };
  $('quality2').onchange = (e) => { settings.quality = Number(e.target.value); store.set('quality', settings.quality); applySettings(); };
}
function joinTo(addr) {
  addr = addr.trim().replace(/^https?:\/\//, '').replace(/\/.*$/, '');
  if (!addr) return;
  if (!/:\d+$/.test(addr)) addr += ':3000';
  serverAddr = addr; everOpen = false; $('joinMsg').textContent = 'جارٍ الاتصال بـ ' + addr + '…';
  const rec = store.get('recent', []).filter((a) => a !== addr); rec.unshift(addr); store.set('recent', rec.slice(0, 4));
  if (S.ws) { const o = S.ws; S.ws = null; try { o.close(); } catch { /* */ } }
  connect();
}
let scanCtl = null;
function bindJoin() {
  const list = $('srvlist');
  const addSrv = (s) => {
    const b = document.createElement('button'); b.className = 'srv';
    b.innerHTML = `<span>${icon('wifi', 18)} ${s.local ? 'هذا الجهاز' : 'لعبة'} — ${s.players} لاعب · ${s.phase === 'lobby' ? 'في الغرفة' : 'مباراة جارية'}</span><small>${s.host}:${s.port}</small>`;
    b.onclick = () => { if (scanCtl) scanCtl.stop = true; joinTo(`${s.host}:${s.port}`); };
    list.appendChild(b);
  };
  const rec = store.get('recent', []);
  for (const a of rec) { const b = document.createElement('button'); b.className = 'srv'; b.innerHTML = `<span>آخر اتصال</span><small>${a}</small>`; b.onclick = () => joinTo(a); list.appendChild(b); }
  $('btnScan').onclick = async () => {
    if (scanCtl) scanCtl.stop = true; scanCtl = { stop: false }; const my = scanCtl;
    list.innerHTML = ''; $('joinMsg').textContent = 'جارٍ البحث في الشبكة…'; $('btnScan').disabled = true;
    let n = 0;
    await scan((s) => { n++; addSrv(s); }, (p) => { $('scanBar').style.width = (p * 100) + '%'; }, my);
    $('btnScan').disabled = false;
    if (!my.stop) $('joinMsg').textContent = n ? 'اختر لعبة من القائمة.' : 'لم يتم العثور على ألعاب. تأكد أن الخادم يعمل أو اكتب العنوان يدويًا.';
  };
  $('btnManual').onclick = () => joinTo($('manual').value);
  $('manual').value = rec[0] || '';
}
function pause() {
  if (!S.inGame || S.paused || S.ended) return;
  S.paused = true; resetInput(); if (input.resetAds) { input.resetAds(); input.resetCrouch(); }
  try { document.exitPointerLock && document.exitPointerLock(); } catch { /* */ }
  $('btnBack').textContent = S.hostId === S.id ? 'إنهاء المباراة والعودة للغرفة' : 'مغادرة الغرفة';
  show('pause');
}
function resume() {
  S.paused = false; hideScreens();
  if (!input.touch) { try { $('gl').requestPointerLock(); } catch { /* */ } }
}

// ---------- بدء/إنهاء اللعب ----------
function startGame(m) {
  S.map = buildMap(m.map); S.ended = false; S.paused = false; S.score = [0, 0];
  try { eng.setMap(S.map); } catch (e) { showError('خطأ في الرسم', String(e.message || e)); return; }
  S.inGame = true; S.remote.clear(); S.grenades.clear(); fx = new Effects(); me.alive = false;
  S.timeLeft = m.tl;
  hideScreens(); $('hud').classList.remove('hidden'); $('board').classList.add('hidden'); $('deadmsg').classList.add('hidden');
  if (input.touch) $('touch').classList.remove('hidden');
  document.body.classList.toggle('touch', input.touch);
  $('feed').innerHTML = '';
  resetInput();
  toast('المباراة بدأت — ' + S.map.name, 2500);
}
function leaveGame() {
  S.inGame = false; S.ended = false; S.paused = false; me.alive = false;
  $('hud').classList.add('hidden'); $('touch').classList.add('hidden'); $('scope').classList.add('hidden'); $('board').classList.add('hidden'); $('deadmsg').classList.add('hidden');
  try { document.exitPointerLock && document.exitPointerLock(); } catch { /* */ }
  resetInput();
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
  if (!r) { r = { id, x: 0, y: 0, z: 0, tx: 0, ty: 0, tz: 0, yaw: 0, tyaw: 0, pitch: 0, tpitch: 0, crouch: false, moving: false, sprint: false, ads: false, alive: false, hp: 100, w: 0, deadAt: -1e9, flash: 0, phase: Math.random() * 6, stepAcc: 0, init: false, reveal: false }; S.remote.set(id, r); }
  return r;
}

function onSpawn(m) {
  const info = S.players.get(m.id);
  if (info) { info.p = m.p; info.s = m.s; info.a = m.a; info.tm = m.tm; info._sc = null; }
  if (m.id === S.id) {
    me.x = m.x; me.y = m.y; me.z = m.z; me.vy = 0; me.vx = 0; me.vz = 0; me.onGround = true; me.yaw = m.yaw; me.pitch = 0; me.team = m.tm;
    me.epoch = m.e; me.hp = 100; me.alive = true; me.crouch = false; me.eyeH = PLAYER.eye; me.camY = m.y; me.w = 0;
    me.st = [computeStats(m.p, m.a[0]), computeStats(m.s, m.a[1])];
    me.ammo = me.st.map((s) => ({ mag: s.mag, res: s.reserve }));
    me.reloading = -1; me.swapAt = nowMs() - 1000; me.grenades = 2; me.adsT = 0; me.recAcc = 0; me.bloom = 0; me.shots = 0; me.nextShot = 0; me.protUntil = nowMs() + 2000;
    if (input.resetAds) { input.resetAds(); input.resetCrouch(); }
    $('deadmsg').classList.add('hidden'); Snd.spawn(); updateAmmoHud();
  } else {
    const r = remoteOf(m.id); r.x = r.tx = m.x; r.y = r.ty = m.y; r.z = r.tz = m.z; r.yaw = r.tyaw = m.yaw; r.alive = true; r.deadAt = -1e9; r.init = true; r.hp = 100;
  }
}
function onSnap(m) {
  S.timeLeft = m.tl; S.score = m.sc;
  const seen = new Set();
  for (const p of m.ps) {
    const id = p[0]; seen.add(id);
    const f = p[6], alive = !!(f & 4);
    if (id === S.id) { if (alive && me.alive) { me.hp = p[7]; me.grenades = p[9]; } continue; }
    const r = remoteOf(id);
    r.tx = p[1]; r.ty = p[2]; r.tz = p[3]; r.tyaw = p[4]; r.tpitch = p[5];
    r.crouch = !!(f & 1); r.ads = !!(f & 2); r.moving = !!(f & 8); r.reveal = !!(f & 16); r.sprint = !!(f & 32); r.hp = p[7]; r.w = p[8];
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
}
function onShot(m) {
  const r = S.remote.get(m.id), info = S.players.get(m.id);
  if (r) r.flash = 1;
  for (const e of m.e) {
    fx.tracer(m.o, e, !!m.s, !!e[3]);
    if (e[3]) fx.spark(e[0], e[1], e[2], [0.8, 0.1, 0.1], 4, 3); else fx.spark(e[0], e[1], e[2], [0.8, 0.75, 0.6], 3, 2.5);
  }
  const d = Math.hypot(m.o[0] - me.x, m.o[2] - me.z), st = WEAPONS[m.w];
  Snd.shot(st ? st.kind : 'rifle', clamp(1 - d / (m.s ? 40 : 110), 0, 1) * 0.9, panOf({ x: m.o[0], z: m.o[2] }), !!m.s, clamp(d / 110, 0, 1));
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
function onKill(m) {
  const k = S.players.get(m.k), v = S.players.get(m.v);
  const w = m.w === 'gren' ? 'قنبلة' : (WEAPONS[m.w] ? WEAPONS[m.w].name : '');
  const d = document.createElement('div'); d.className = teamOf(m.k) === 0 ? 'r' : 'b';
  d.innerHTML = `${esc(k ? k.n : '؟')} ${m.h ? icon('head', 13) : '←'} ${esc(v ? v.n : '؟')} <small>${esc(w)}</small>`;
  const feed = $('feed'); feed.appendChild(d); while (feed.children.length > 5) feed.removeChild(feed.firstChild); setTimeout(() => d.remove(), 6000);
  if (k) k.k++; if (v) v.d++;
  if (m.v === S.id) {
    me.alive = false; me.deadAt = nowMs(); me.hp = 0; input.fire = false; if (input.resetAds) { input.resetAds(); input.resetCrouch(); }
    $('deadmsg').innerHTML = `قُتلت بواسطة <b>${esc(k ? k.n : '؟')}</b><small>${esc(w)} — إعادة الظهور خلال 3 ثوانٍ</small>`;
    $('deadmsg').classList.remove('hidden'); $('scope').classList.add('hidden');
  } else {
    const r = S.remote.get(m.v); if (r && r.alive) { r.alive = false; r.deadAt = nowMs(); }
    if (m.k === S.id) { const h = $('hitm'); h.className = 'on kill'; clearTimeout(onHit.h); onHit.h = setTimeout(() => { h.className = ''; }, 300); Snd.kill(); }
  }
}
function onBoom(m) {
  fx.boom(m.x, m.y, m.z);
  const d = Math.hypot(m.x - me.x, m.z - me.z);
  Snd.boom(clamp(1 - d / 70, 0, 1), panOf({ x: m.x, z: m.z }), clamp(d / 70, 0, 1));
}
function onEnd(m) {
  S.ended = true; resetInput(); if (input.resetAds) { input.resetAds(); input.resetCrouch(); }
  try { document.exitPointerLock && document.exitPointerLock(); } catch { /* */ }
  const mine = me.team;
  $('endTitle').textContent = m.win < 0 ? 'تعادل!' : (m.win === mine ? 'فريقك فاز!' : 'خسر فريقك');
  $('endTitle').style.color = m.win < 0 ? '#ffb020' : (m.win === 0 ? 'var(--red)' : 'var(--blue)');
  $('endBoard').innerHTML = boardHtml(m.sc);
  $('endMsg').textContent = S.hostId === S.id ? 'اضغط للعودة إلى الغرفة (أو تعود تلقائيًا بعد ثوانٍ).' : 'ستعود إلى الغرفة تلقائيًا بعد ثوانٍ…';
  $('btnEndBack').disabled = S.hostId !== S.id;
  show('end');
}
function boardHtml(sc) {
  let h = '<div style="display:flex;gap:10px;flex-wrap:wrap">';
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
  // قاعدة متعامدة حول الاتجاه
  let ux = 0, uy = 1, uz = 0; if (Math.abs(f[1]) > 0.99) { ux = 1; uy = 0; }
  let rx = uy * f[2] - uz * f[1], ry = uz * f[0] - ux * f[2], rz = ux * f[1] - uy * f[0]; const rl = Math.hypot(rx, ry, rz); rx /= rl; ry /= rl; rz /= rl;
  const vx = f[1] * rz - f[2] * ry, vy = f[2] * rx - f[0] * rz, vz = f[0] * ry - f[1] * rx;
  const a = Math.random() * Math.PI * 2, r = Math.sqrt(Math.random()) * sp, c = Math.cos(a) * r, s = Math.sin(a) * r;
  const x = f[0] + rx * c + vx * s, y = f[1] + ry * c + vy * s, z = f[2] + rz * c + vz * s, l = Math.hypot(x, y, z);
  return [x / l, y / l, z / l];
}
function tryFire(now, pressed) {
  const st = curStats(), am = me.ammo[me.w];
  if (!st || me.reloading >= 0 || now < me.nextShot || now < me.swapAt + 350 || now < me.noFireUntil || me.sprinting) return;
  if (am.mag <= 0) {
    if (pressed || !me.emptyClick) { Snd.empty(); me.emptyClick = true; }
    if (am.res > 0) startReload(now);
    return;
  }
  me.emptyClick = false; am.mag--; me.nextShot = now + st.interval * 1000;
  const speed = Math.hypot(me.vx, me.vz), moveFrac = clamp(speed / PLAYER.walk, 0, 1.5);
  const base = lerp(st.hip, st.ads, me.adsT);
  const sp = base * (1 + 0.9 * moveFrac) * (me.crouch ? 0.7 : 1) + (me.onGround ? 0 : 0.04) + me.bloom;
  const f = aimDir(me.yaw, me.pitch), dirs = [];
  for (let i = 0; i < st.pellets; i++) dirs.push(spreadDir(f, sp));
  send({ t: 'shoot', w: me.w, d: dirs.map((d) => [Math.round(d[0] * 1e4) / 1e4, Math.round(d[1] * 1e4) / 1e4, Math.round(d[2] * 1e4) / 1e4]) });
  // مؤثرات محلية
  const eyeY = me.camY + me.eyeH, g = GUNS[st.kind], cy = Math.cos(me.yaw), sy = Math.sin(me.yaw);
  const mz = -g.mz + 0.1, o = [me.x - sy * mz + cy * 0.14, eyeY - 0.1, me.z - cy * mz - sy * 0.14];
  const maxT = Math.min(st.range[1] * 1.6, 300);
  for (const d of dirs) {
    let best = raycastMap(S.map, me.x, eyeY, me.z, d[0], d[1], d[2], maxT); if (best === Infinity) best = maxT;
    let hit = false;
    for (const r of S.remote.values()) {
      if (!r.alive || teamOf(r.id) === me.team) continue;
      const q = rayPlayer({ x: r.x, y: r.y, z: r.z, crouch: r.crouch }, me.x, eyeY, me.z, d[0], d[1], d[2], best);
      if (q && q.t < best) { best = q.t; hit = true; }
    }
    const e = [me.x + d[0] * best, eyeY + d[1] * best, me.z + d[2] * best];
    fx.tracer(o, e, st.muted);
    if (best < maxT) fx.spark(e[0], e[1], e[2], hit ? [0.8, 0.1, 0.1] : [0.85, 0.8, 0.65], hit ? 4 : 3, 2.5);
  }
  Snd.shot(st.kind, 0.75, 0, st.muted);
  me.flash = 1; me.kick = 1;
  // ارتداد
  const rv = st.recoilV * (0.8 + Math.random() * 0.4) * (me.adsT > 0.5 ? 0.78 : 1) * (me.crouch ? 0.85 : 1);
  me.pitch = clamp(me.pitch + rv, -1.5, 1.5); me.recAcc += rv;
  me.yaw += (Math.random() - 0.5) * 2 * st.recoilH + Math.sin(me.shots * 0.7) * st.recoilH * 0.6;
  me.bloom = Math.min(0.025, me.bloom + st.recoilV * 0.25); me.shots++; me.lastShot = now;
  if (am.mag === 0 && am.res > 0) me.autoReloadAt = now + 300;
  updateAmmoHud();
}

function update(dt, now) {
  const [lx, ly] = takeLook();
  const stC = curStats();
  if (!S.paused && !S.ended) {
    const zf = stC ? 1 + (stC.zoom - 1) * me.adsT : 1;
    me.yaw -= lx / zf; me.pitch = clamp(me.pitch - ly / zf, -1.5, 1.5);
  }
  if (me.yaw > Math.PI * 4 || me.yaw < -Math.PI * 4) me.yaw %= Math.PI * 2;
  const active = me.alive && !S.paused && !S.ended;
  me.kick = Math.max(0, me.kick - dt * 8); me.flash = Math.max(0, me.flash - dt * 14);
  me.bloom = Math.max(0, me.bloom - dt * 0.05);
  if (now - me.lastShot > 300) me.shots = 0;
  if (!active) { me.vx = me.vz = 0; consume('jump'); consume('reload'); consume('swap'); consume('gren'); consume('firePressed'); consume('slot'); if (!me.alive) { me.camY += (0.25 - me.camY) * Math.min(1, dt * 3); me.pitch += (-0.5 - me.pitch) * Math.min(1, dt * 2); } return; }
  // تبديل السلاح
  let slot = consume('slot'); if (consume('swap')) slot = 1 - me.w;
  if (slot >= 0 && slot !== me.w) { me.w = slot; me.swapAt = now; me.reloading = -1; me.emptyClick = false; if (input.resetAds) input.resetAds(); updateAmmoHud(); }
  const st = curStats(), am = me.ammo[me.w];
  // القرفصاء
  if (input.crouch) me.crouch = true; else if (me.crouch && canStand(S.map, me)) me.crouch = false;
  const wantFire = input.fire;
  me.sprinting = input.sprint && input.moveY > 0.3 && !me.crouch && !input.ads && !wantFire && me.reloading < 0;
  if (me.sprintPrev && !me.sprinting) me.noFireUntil = now + 150;
  me.sprintPrev = me.sprinting;
  // الحركة
  const mlen = Math.min(1, Math.hypot(input.moveX, input.moveY));
  let wx = -Math.sin(me.yaw) * input.moveY + Math.cos(me.yaw) * input.moveX, wz = -Math.cos(me.yaw) * input.moveY - Math.sin(me.yaw) * input.moveX;
  const wl = Math.hypot(wx, wz); if (wl > 0) { wx = wx / wl * mlen; wz = wz / wl * mlen; }
  const spd = (me.crouch ? PLAYER.crouch : me.sprinting ? PLAYER.sprint : PLAYER.walk) * st.move * (me.adsT > 0.5 ? 0.7 : 1);
  const k = Math.min(1, (me.onGround ? 14 : 2.5) * dt);
  me.vx += (wx * spd - me.vx) * k; me.vz += (wz * spd - me.vz) * k;
  const jump = consume('jump'), preVy = me.vy;
  const n = Math.max(1, Math.ceil(dt / (1 / 90))), h = dt / n;
  for (let i = 0; i < n; i++) moveBody(S.map, me, me.vx, me.vz, h, jump && i === 0, me.crouch);
  if (preVy < -7 && me.onGround) Snd.step(0.5, 0);
  // الكاميرا
  me.eyeH += ((me.crouch ? PLAYER.eyec : PLAYER.eye) - me.eyeH) * Math.min(1, dt * 12);
  if (!me.onGround || Math.abs(me.y - me.camY) > 0.9) me.camY = me.y; else me.camY += (me.y - me.camY) * Math.min(1, dt * 16);
  const sp = Math.hypot(me.vx, me.vz);
  if (me.onGround && sp > 0.5) { me.bob += sp * dt * 1.7; me.stepAcc += sp * dt; const stride = me.sprinting ? 3.0 : 2.2; if (me.stepAcc > stride) { me.stepAcc = 0; if (!me.crouch) Snd.step(me.sprinting ? 0.22 : 0.14, 0); } }
  // التصويب
  const adsTarget = input.ads && !me.sprinting && me.reloading < 0 && now > me.swapAt + 250 ? 1 : 0;
  const adsRate = (st.kind === 'sniper' ? 1 / 0.3 : 1 / 0.17) * dt;
  me.adsT = clamp(me.adsT + clamp(adsTarget - me.adsT, -adsRate, adsRate), 0, 1);
  // إعادة التلقيم
  if (consume('reload')) startReload(now);
  if (me.autoReloadAt && now >= me.autoReloadAt) { me.autoReloadAt = 0; startReload(now); }
  if (me.reloading >= 0 && now >= me.reloadEnd) {
    const a = me.ammo[me.reloading], s2 = me.st[me.reloading], take = Math.min(s2.mag - a.mag, a.res); a.mag += take; a.res -= take; me.reloading = -1; updateAmmoHud();
  }
  // النار
  const pressed = consume('firePressed');
  if ((st.auto ? wantFire : pressed) && !(me.reloading >= 0)) tryFire(now, pressed);
  else if (pressed && me.reloading < 0 && am.mag <= 0) tryFire(now, true);
  if (!wantFire) { me.emptyClick = false; const d = me.recAcc * Math.min(1, dt * 2.5); me.pitch -= d * 0.5; me.recAcc -= d; }
  // قنبلة
  if (consume('gren') && me.grenades > 0 && now > me.nextGren) {
    me.nextGren = now + 700; me.grenades--; send({ t: 'gren', d: aimDir(me.yaw, me.pitch + 0.1) }); Snd.throwG(); updateAmmoHud();
  }
  // دوران عرضي للسلاح
  me.swayX += (clamp(-lx * 3, -0.04, 0.04) - me.swayX) * Math.min(1, dt * 8); me.swayY += (clamp(ly * 3, -0.03, 0.03) - me.swayY) * Math.min(1, dt * 8);
  // إرسال الحالة
  if (now - lastSend > 50) {
    lastSend = now;
    const f = (me.crouch ? 1 : 0) | (me.adsT > 0.5 ? 2 : 0) | (sp > 0.5 ? 4 : 0) | (me.sprinting ? 8 : 0);
    send({ t: 'st', e: me.epoch, x: Math.round(me.x * 100) / 100, y: Math.round(me.y * 100) / 100, z: Math.round(me.z * 100) / 100, yaw: Math.round(me.yaw * 1e3) / 1e3, pitch: Math.round(me.pitch * 1e3) / 1e3, f, w: me.w });
  }
}

function updateRemotes(dt, now) {
  const k = Math.min(1, dt * 14);
  for (const r of S.remote.values()) {
    const dx = r.tx - r.x, dz = r.tz - r.z;
    if (Math.hypot(dx, dz) > 6) { r.x = r.tx; r.z = r.tz; r.y = r.ty; } else { r.x += dx * k; r.z += dz * k; r.y += (r.ty - r.y) * k; }
    r.yaw += angDiff(r.yaw, r.tyaw) * k; r.pitch += (r.tpitch - r.pitch) * k; r.flash = Math.max(0, r.flash - dt * 12);
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
}

// ---------- الرسم ----------
const nameEls = new Map(), pt = { x: 0, y: 0, w: 0 };
function render(dt, now) {
  const st = curStats();
  let fov = 76, cy = me.camY + me.eyeH;
  if (me.alive && st) { const Z = 1 + (st.zoom - 1) * me.adsT; fov = (2 * Math.atan(Math.tan(76 * Math.PI / 360) / Z)) * 180 / Math.PI; }
  const bobY = me.alive && me.onGround ? Math.sin(me.bob * 2) * 0.014 * (me.sprinting ? 1.6 : 1) * (1 - me.adsT * 0.8) : 0;
  const cam = { x: me.x, y: cy + bobY, z: me.z, yaw: me.yaw, pitch: me.pitch, fov };
  eng.begin(cam); eng.drawMap();
  for (const r of S.remote.values()) {
    const dead = !r.alive, dt2 = (now - r.deadAt) / 1000;
    if (dead && dt2 > 4) continue;
    const info = S.players.get(r.id); if (!info) continue;
    drawAvatar(eng, { x: r.x, y: r.y, z: r.z, yaw: r.yaw, pitch: r.pitch, crouch: r.crouch, team: info.tm, moving: r.moving, sprint: r.sprint, stats: statsFor(info, r.w === 1 ? 1 : 0), deadT: dead ? dt2 : -1, flash: r.flash, phase: r.phase }, now);
  }
  for (const g of S.grenades.values()) drawGrenade(eng, g.x, g.y, g.z);
  fx.update(dt); fx.draw(eng);
  const scoped = me.alive && st && me.adsT > 0.92 && st.zoom >= 3.4;
  if (me.alive && st && !scoped) {
    drawViewmodel(eng, { stats: st, ads: me.adsT, kick: me.kick, flash: me.flash, reload: me.reloading >= 0 ? clamp((now - me.reloadStart) / (me.reloadEnd - me.reloadStart), 0, 1) : -1, swap: clamp((now - me.swapAt) / 350, 0, 1), sprint: me.sprinting, team: me.team, bobX: Math.cos(me.bob) * 0.006 * (me.onGround ? 1 : 0), bobY: bobY * 0.8, swayX: me.swayX, swayY: me.swayY });
  }
  $('scope').classList.toggle('hidden', !scoped);
  hud(dt, now, st);
}

function hud(dt, now, st) {
  $('sc0').textContent = S.score[0]; $('sc1').textContent = S.score[1];
  const tl = Math.max(0, S.timeLeft | 0); $('timer').textContent = `${Math.floor(tl / 60)}:${String(tl % 60).padStart(2, '0')}`;
  $('hpfill').style.width = clamp(me.hp, 0, 100) + '%'; $('hpfill').style.background = me.hp > 50 ? '' : me.hp > 25 ? 'linear-gradient(90deg,#f39c12,#f7c46a)' : 'linear-gradient(90deg,#e74c3c,#ff8a7a)';
  $('hpnum').textContent = Math.max(0, Math.round(me.hp));
  if (me.hp < 60 && me.alive) $('vign').style.opacity = String(clamp((60 - me.hp) / 60, 0, 0.8) * 0.8);
  else if (!me.alive) $('vign').style.opacity = '0.5'; else $('vign').style.opacity = '0';
  if (me.reloading >= 0) $('mag').textContent = '…'; else if (st) $('mag').textContent = me.ammo[me.w].mag;
  $('res').textContent = st ? me.ammo[me.w].res : 0;
  $('prot').classList.toggle('hidden', !(me.alive && now < me.protUntil));
  // مؤشر التقاطع
  if (st && me.alive) {
    const sp = lerp(st.hip, st.ads, me.adsT) * (1 + 0.9 * clamp(Math.hypot(me.vx, me.vz) / PLAYER.walk, 0, 1.5)) + me.bloom + (me.onGround ? 0 : 0.04);
    const gap = clamp(sp * 700, 3, 60); const c = $('cross');
    c.style.display = me.adsT > 0.6 ? 'none' : '';
    c.querySelector('.cl').style.left = -(gap + 9) + 'px'; c.querySelector('.cr').style.left = gap + 'px';
    c.querySelector('.ct').style.top = -(gap + 9) + 'px'; c.querySelector('.cb').style.top = gap + 'px';
  } else $('cross').style.display = 'none';
  // لوحة النتائج
  const showBoard = input.board || S.ended;
  $('board').classList.toggle('hidden', !showBoard || S.ended);
  if (showBoard && !S.ended && (now - hud.lastBoard > 400 || !hud.lastBoard)) { hud.lastBoard = now; renderBoard(); }
  // العد التنازلي للموت
  if (!me.alive && !$('deadmsg').classList.contains('hidden')) { /* النص ثابت */ }
  // أسماء الزملاء
  const used = new Set();
  if (me.alive) for (const r of S.remote.values()) {
    const info = S.players.get(r.id); if (!info || !r.alive || info.tm !== me.team) continue;
    if (!eng.project(r.x, r.y + (r.crouch ? 1.45 : 2.05), r.z, pt)) continue;
    const d = Math.hypot(r.x - me.x, r.z - me.z); if (d > 70) continue;
    let el = nameEls.get(r.id); if (!el) { el = document.createElement('div'); $('names').appendChild(el); nameEls.set(r.id, el); }
    el.textContent = info.n; el.style.color = info.tm === 0 ? '#ff8b8b' : '#8bbcff';
    el.style.left = (pt.x * 100) + '%'; el.style.top = (pt.y * 100) + '%'; el.style.display = ''; used.add(r.id);
  }
  for (const [id, el] of nameEls) if (!used.has(id)) { if (!S.remote.has(id)) { el.remove(); nameEls.delete(id); } else el.style.display = 'none'; }
  // الرادار
  if (now - lastRadar > 66) { lastRadar = now; radar(); }
  $('stat').textContent = `${Math.round(1 / emaDt)} fps · ${S.ping} ms`;
  $('ping').textContent = S.ping ? `${S.ping}ms` : '';
}
function radar() {
  const cv = $('radar'), c = cv.getContext('2d'), W = cv.width, R = 38, sc = W / 2 / R;
  c.clearRect(0, 0, W, W); c.save(); c.beginPath(); c.arc(W / 2, W / 2, W / 2 - 1, 0, Math.PI * 2); c.clip();
  c.translate(W / 2, W / 2); c.rotate(me.yaw); c.scale(sc, sc);
  c.fillStyle = 'rgba(170,190,210,.45)';
  for (const b of S.map.boxes) { if (b.y1 < 0.8 || b.y0 > 2.5) continue; c.fillRect(b.x0 - me.x, b.z0 - me.z, b.x1 - b.x0, b.z1 - b.z0); }
  for (const r of S.remote.values()) {
    const info = S.players.get(r.id); if (!info || !r.alive) continue;
    const mate = info.tm === me.team; if (!mate && !r.reveal) continue;
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
  // جودة تكيّفية
  if (emaDt > 0.042) { if (++slowCount > 120 && settings.quality > 0.5) { settings.quality = Math.round((settings.quality - 0.1) * 10) / 10; eng.resize(settings.quality); slowCount = 0; toast('تم خفض الجودة لتحسين الأداء'); } } else slowCount = Math.max(0, slowCount - 2);
  const now = nowMs();
  if (now - lastPing > 2000) { lastPing = now; send({ t: 'ping', c: now }); }
  try { update(dt, now); updateRemotes(dt, now); render(dt, now); } catch (e) { console.error(e); if (!frame.err) { frame.err = true; toast('خطأ: ' + (e.message || e), 6000); } }
}

// ---------- الإقلاع ----------
function boot() {
  document.querySelectorAll('[data-ic]').forEach((el) => { el.outerHTML = icon(el.dataset.ic, Number(el.dataset.s) || 28); });
  bindUI();
  try { eng = new Engine($('gl')); eng.resize(settings.quality); }
  catch (e) { showError('جهازك لا يدعم الرسم ثلاثي الأبعاد', String(e.message || e), 'إعادة المحاولة'); return; }
  initInput($('gl'), { pause: () => { if (S.paused) resume(); else pause(); }, unlocked: () => { if (S.inGame && !S.ended && !S.paused && !input.touch) pause(); }, canLock: () => S.inGame && !S.paused && !S.ended && !!$('gl').requestPointerLock });
  applySettings();
  window.addEventListener('resize', () => eng && eng.resize());
  if (APP) { $('btnChange').classList.remove('hidden'); $('btnChange').onclick = () => location.reload(); }
  if (APP) { bindJoin(); show('join'); $('btnScan').click(); } else { show('menu'); connect(); }
  requestAnimationFrame(frame);
  try { if (navigator.wakeLock) navigator.wakeLock.request('screen').catch(() => {}); } catch { /* */ }
  window.__wz = { S, me, eng }; // للاختبارات
}
boot();
