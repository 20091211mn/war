// إدخال: لوحة مفاتيح/ماوس + شاشة لمس (عصا حركة، سحب للنظر، أزرار)
export const input = {
  moveX: 0, moveY: 0, lookX: 0, lookY: 0,
  fire: false, ads: false, crouch: false, sprint: false, board: false,
  firePressed: false, dslot: -1, attach: false, jump: false, jumpHeld: false, reload: false, swap: false, gren: false, slot: -1, pause: false, use: false, seat: false, view: false, chat: false, drone: false,
  touch: false, locked: false,
  sensMouse: 0.0022, sensTouch: 0.0042,
};
const keys = {};
export const isTouchDevice = () => {
  try {
    const coarse = window.matchMedia && matchMedia('(pointer: coarse)').matches;
    const fine = window.matchMedia && matchMedia('(pointer: fine)').matches;
    return coarse || (('ontouchstart' in window) && !fine);
  } catch { return 'ontouchstart' in window; }
};

export function consume(name) { const v = input[name]; input[name] = (name === 'slot' || name === 'dslot') ? -1 : false; return v; }
export function takeLook() { const x = input.lookX, y = input.lookY; input.lookX = 0; input.lookY = 0; return [x, y]; }
export function resetInput() {
  input.moveX = input.moveY = input.lookX = input.lookY = 0;
  input.fire = input.ads = input.crouch = input.sprint = input.board = false;
  input.dslot = -1; input.attach = false; input.firePressed = input.jump = input.jumpHeld = input.reload = input.swap = input.gren = input.use = input.seat = input.view = input.chat = input.drone = false; input.slot = -1;
  for (const k in keys) keys[k] = false;
}

function recompute() {
  const f = (keys.KeyW || keys.ArrowUp ? 1 : 0) - (keys.KeyS || keys.ArrowDown ? 1 : 0);
  const r = (keys.KeyD || keys.ArrowRight ? 1 : 0) - (keys.KeyA || keys.ArrowLeft ? 1 : 0);
  if (!input.touch) { input.moveY = f; input.moveX = r; input.sprint = !!(keys.ShiftLeft || keys.ShiftRight); input.crouch = !!(keys.KeyC || keys.ControlLeft); }
}

export function initInput(canvas, hooks) {
  const isT = isTouchDevice();
  // ---- لوحة المفاتيح ----
  window.addEventListener('keydown', (e) => {
    if (e.target && /INPUT|SELECT|TEXTAREA/.test(e.target.tagName)) return;
    if (keys[e.code] && e.repeat) return;
    keys[e.code] = true;
    if (/^Digit[1-8]$/.test(e.code)) input.dslot = Number(e.code[5]) - 1;
    switch (e.code) {
      case 'KeyX': input.attach = true; break;
      case 'Space': input.jump = true; input.jumpHeld = true; e.preventDefault(); break;
      case 'KeyE': input.use = true; break;
      case 'KeyF': input.seat = true; break;
      case 'KeyV': input.view = true; break;
      case 'KeyB': input.drone = true; break;
      case 'Enter': case 'KeyT': input.chat = true; e.preventDefault(); break;
      case 'KeyR': input.reload = true; break;
      case 'KeyQ': input.swap = true; break;
      case 'KeyG': input.gren = true; break;
      case 'Digit1': input.slot = 0; break;
      case 'Digit2': input.slot = 1; break;
      case 'Tab': input.board = true; e.preventDefault(); break;
      case 'Escape': hooks.pause && hooks.pause(); break;
      default: break;
    }
    if (['ArrowUp', 'ArrowDown', 'ArrowLeft', 'ArrowRight'].includes(e.code)) e.preventDefault();
    recompute();
  });
  window.addEventListener('keyup', (e) => { keys[e.code] = false; if (e.code === 'Space') input.jumpHeld = false; if (e.code === 'Tab') input.board = false; recompute(); });
  window.addEventListener('blur', () => { for (const k in keys) keys[k] = false; recompute(); input.fire = false; input.ads = false; });
  // ---- الماوس ----
  canvas.addEventListener('mousedown', (e) => {
    if (input.touch) return;
    if (!document.pointerLockElement && hooks.canLock && hooks.canLock()) { canvas.requestPointerLock && canvas.requestPointerLock(); return; }
    if (e.button === 0) { input.fire = true; input.firePressed = true; }
    if (e.button === 2) input.ads = true;
  });
  window.addEventListener('mouseup', (e) => { if (e.button === 0) input.fire = false; if (e.button === 2) input.ads = false; });
  canvas.addEventListener('contextmenu', (e) => e.preventDefault());
  window.addEventListener('mousemove', (e) => {
    if (document.pointerLockElement === canvas) { input.lookX += e.movementX * input.sensMouse; input.lookY += e.movementY * input.sensMouse; }
  });
  window.addEventListener('wheel', (e) => { if (document.pointerLockElement === canvas) input.swap = true; }, { passive: true });
  document.addEventListener('pointerlockchange', () => {
    input.locked = document.pointerLockElement === canvas;
    if (!input.locked && hooks.unlocked) hooks.unlocked();
  });
  // ---- اللمس ----
  if (isT) {
    input.touch = true;
    const tc = document.getElementById('touch');
    document.addEventListener('gesturestart', (e) => e.preventDefault());
    document.addEventListener('dblclick', (e) => e.preventDefault());
    const prevent = (e) => { if (e.cancelable) e.preventDefault(); };
    tc.addEventListener('touchmove', prevent, { passive: false });

    // عصا الحركة
    const jz = document.getElementById('joyzone'), jb = document.getElementById('joybase'), jk = document.getElementById('joyknob');
    let jid = null, jx = 0, jy = 0; const JR = 62;
    jz.addEventListener('touchstart', (e) => {
      prevent(e); if (jid !== null) return;
      const t = e.changedTouches[0]; jid = t.identifier; jx = t.clientX; jy = t.clientY;
      jb.style.display = 'block'; jb.style.left = jx + 'px'; jb.style.top = jy + 'px'; jk.style.transform = 'translate(-50%,-50%)';
    }, { passive: false });
    jz.addEventListener('touchmove', (e) => {
      prevent(e);
      for (const t of e.changedTouches) if (t.identifier === jid) {
        let dx = t.clientX - jx, dy = t.clientY - jy; const l = Math.hypot(dx, dy);
        const k = Math.min(1, l / JR); if (l > 0) { dx = dx / l * k * JR; dy = dy / l * k * JR; }
        jk.style.transform = `translate(calc(-50% + ${dx}px), calc(-50% + ${dy}px))`;
        let mx = dx / JR, my = -dy / JR; const m = Math.hypot(mx, my);
        if (m < 0.12) { mx = 0; my = 0; }
        input.moveX = mx; input.moveY = my; input.sprint = m > 0.97 && my > 0.5;
        jk.classList.toggle('sprint', input.sprint);
      }
    }, { passive: false });
    const jend = (e) => {
      prevent(e);
      for (const t of e.changedTouches) if (t.identifier === jid) { jid = null; jb.style.display = 'none'; input.moveX = input.moveY = 0; input.sprint = false; }
    };
    jz.addEventListener('touchend', jend, { passive: false }); jz.addEventListener('touchcancel', jend, { passive: false });

    // النظر بالسحب (منطقة اليمين + أزرار النار والتصويب)
    const lookTouch = (el, onStart, onEnd) => {
      let id = null, lx = 0, ly = 0;
      el.addEventListener('touchstart', (e) => {
        prevent(e); if (id !== null) return;
        const t = e.changedTouches[0]; id = t.identifier; lx = t.clientX; ly = t.clientY; onStart && onStart(e);
      }, { passive: false });
      el.addEventListener('touchmove', (e) => {
        prevent(e);
        for (const t of e.changedTouches) if (t.identifier === id) {
          input.lookX += (t.clientX - lx) * input.sensTouch; input.lookY += (t.clientY - ly) * input.sensTouch; lx = t.clientX; ly = t.clientY;
        }
      }, { passive: false });
      const end = (e) => { prevent(e); for (const t of e.changedTouches) if (t.identifier === id) { id = null; onEnd && onEnd(e); } };
      el.addEventListener('touchend', end, { passive: false }); el.addEventListener('touchcancel', end, { passive: false });
    };
    lookTouch(document.getElementById('lookzone'));
    lookTouch(document.getElementById('b-fire'), () => { input.fire = true; input.firePressed = true; }, () => { input.fire = false; });
    lookTouch(document.getElementById('b-fire2'), () => { input.fire = true; input.firePressed = true; }, () => { input.fire = false; });
    let adsOn = false;
    lookTouch(document.getElementById('b-ads'), () => { adsOn = !adsOn; input.ads = adsOn; document.getElementById('b-ads').classList.toggle('on', adsOn); });
    input.resetAds = () => { adsOn = false; input.ads = false; document.getElementById('b-ads').classList.remove('on'); };
    const tap = (id, fn, end) => {
      const el = document.getElementById(id);
      el.addEventListener('touchstart', (e) => { prevent(e); fn(); }, { passive: false });
      el.addEventListener('touchend', (e) => { prevent(e); end && end(); }, { passive: false });
      el.addEventListener('touchcancel', (e) => { prevent(e); end && end(); }, { passive: false });
    };
    tap('b-jump', () => { input.jump = true; input.jumpHeld = true; }, () => { input.jumpHeld = false; });
    tap('b-use', () => { input.use = true; });
    tap('b-seat', () => { input.seat = true; });
    tap('b-view', () => { input.view = true; });
    tap('b-chat', () => { input.chat = true; });
    tap('b-drone', () => { input.drone = true; });
    let crouchOn = false;
    tap('b-crouch', () => { crouchOn = !crouchOn; input.crouch = crouchOn; document.getElementById('b-crouch').classList.toggle('on', crouchOn); });
    input.resetCrouch = () => { crouchOn = false; input.crouch = false; document.getElementById('b-crouch').classList.remove('on'); };
    tap('b-reload', () => { input.reload = true; });
    tap('b-swap', () => { input.swap = true; });
    tap('b-gren', () => { input.gren = true; });
    tap('b-board', () => { input.board = true; }, () => { input.board = false; });
    tap('b-pause', () => { hooks.pause && hooks.pause(); });
  }
}

// ---- يد التحكم (DualShock/Xbox/بلوتوث) عبر Gamepad API ----
const padPrev = {}; let padActive = false;
export function pollGamepad(dt) {
  let gp = null;
  try { const l = navigator.getGamepads ? navigator.getGamepads() : []; for (const g of l) if (g && g.connected) { gp = g; break; } } catch { /* */ }
  if (!gp) { if (padActive) { padActive = false; input.moveX = input.moveY = 0; } return false; }
  const dz = (v) => (Math.abs(v) < 0.16 ? 0 : (v - Math.sign(v) * 0.16) / 0.84);
  const ax = gp.axes, b = (i) => !!(gp.buttons[i] && (gp.buttons[i].pressed || gp.buttons[i].value > 0.5));
  const sx = dz(ax[0] || 0), sy = dz(ax[1] || 0), rx = dz(ax[2] || 0), ry = dz(ax[3] || 0);
  if (sx || sy) { padActive = true; input.moveX = sx; input.moveY = -sy; } else if (padActive) { input.moveX = input.moveY = 0; padActive = false; }
  if (rx || ry) { const k = dt * 1000 * (0.6 + Math.hypot(rx, ry)); input.lookX += rx * k; input.lookY += ry * k; }
  const edge = (n, i, fn) => { const v = b(i); if (v && !padPrev[n]) fn(); padPrev[n] = v; };
  const hold = (n, i, on, off) => { const v = b(i); if (v !== !!padPrev[n]) (v ? on : off)(); padPrev[n] = v; };
  hold('fire', 7, () => { input.fire = true; input.firePressed = true; }, () => { input.fire = false; });
  hold('ads', 6, () => { input.ads = true; }, () => { input.ads = false; });
  hold('crouch', 1, () => { input.crouch = true; }, () => { input.crouch = false; });
  hold('sprint', 10, () => { input.sprint = true; }, () => { input.sprint = false; });
  hold('board', 8, () => { input.board = true; }, () => { input.board = false; });
  edge('jump', 0, () => { input.jump = true; input.jumpHeld = true; });
  if (!b(0)) input.jumpHeld = input.jumpHeld && !!padPrev.jumpKb;
  edge('reload', 2, () => { input.reload = true; });
  edge('swap', 3, () => { input.swap = true; });
  edge('gren', 5, () => { input.gren = true; });
  edge('use', 4, () => { input.use = true; });
  edge('pause', 9, () => { input.pause = true; });
  edge('drone', 15, () => { input.drone = true; });
  edge('view', 14, () => { input.view = true; });
  edge('seat', 13, () => { input.seat = true; });
  return true;
}
