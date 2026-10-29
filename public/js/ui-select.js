// قوائم منسدلة داخلية داكنة تحل محل نافذة الاختيار الأصلية البيضاء في أندرويد/المتصفح.
// عنصر <select> الأصلي يبقى موجودًا (مخفيًا) فيعمل .value و .onchange و .disabled كما هي.
const valDesc = Object.getOwnPropertyDescriptor(HTMLSelectElement.prototype, 'value');
const disDesc = Object.getOwnPropertyDescriptor(HTMLElement.prototype, 'disabled') || Object.getOwnPropertyDescriptor(HTMLSelectElement.prototype, 'disabled');
let openPop = null;

function closePop() { if (openPop) { openPop.remove(); openPop = null; } }

function openList(sel, title) {
  closePop();
  const ov = document.createElement('div'); ov.className = 'dd-ov';
  const pop = document.createElement('div'); pop.className = 'dd-pop';
  const h = document.createElement('div'); h.className = 'dd-title'; h.textContent = title || ''; pop.appendChild(h);
  const list = document.createElement('div'); list.className = 'dd-list';
  [...sel.options].forEach((o) => {
    const b = document.createElement('button'); b.type = 'button'; b.className = 'dd-opt' + (o.value === sel.value ? ' on' : ''); b.textContent = o.text;
    b.onclick = () => {
      closePop();
      if (sel.value !== o.value) { sel.value = o.value; sel.dispatchEvent(new Event('input', { bubbles: true })); sel.dispatchEvent(new Event('change', { bubbles: true })); }
    };
    list.appendChild(b);
  });
  pop.appendChild(list); ov.appendChild(pop);
  ov.addEventListener('pointerdown', (e) => { if (e.target === ov) closePop(); });
  document.body.appendChild(ov); openPop = ov;
  const on = list.querySelector('.on'); if (on) on.scrollIntoView({ block: 'center' });
}

export function enhanceSelects(root = document) {
  root.querySelectorAll('select').forEach((sel) => {
    if (sel._dd) return;
    const btn = document.createElement('button'); btn.type = 'button'; btn.className = 'dd';
    btn.innerHTML = '<span class="ddv"></span><i class="ddc"></i>';
    sel.classList.add('dd-native'); sel.tabIndex = -1; sel.insertAdjacentElement('afterend', btn); sel._dd = btn;
    const refresh = () => {
      const o = sel.options[sel.selectedIndex]; btn.firstChild.textContent = o ? o.text : '';
      btn.disabled = !!disDesc.get.call(sel);
    };
    Object.defineProperty(sel, 'value', { get() { return valDesc.get.call(this); }, set(v) { valDesc.set.call(this, v); refresh(); }, configurable: true });
    Object.defineProperty(sel, 'disabled', { get() { return disDesc.get.call(this); }, set(v) { disDesc.set.call(this, v); refresh(); }, configurable: true });
    sel.addEventListener('change', refresh);
    new MutationObserver(refresh).observe(sel, { childList: true, subtree: true });
    btn.onclick = () => {
      const lab = sel.closest('label'); let t = '';
      if (lab) for (const n of lab.childNodes) if (n.nodeType === 3 && n.textContent.trim()) { t = n.textContent.trim(); break; }
      openList(sel, t);
    };
    refresh();
  });
}
window.addEventListener('keydown', (e) => { if (e.key === 'Escape' && openPop) { closePop(); e.stopImmediatePropagation(); } }, true);
