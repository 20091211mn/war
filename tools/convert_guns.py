#!/usr/bin/env python3
"""يحوّل موديلات الأسلحة GLB (M4/SVD/RPG) إلى بيانات خفيفة: تبسيط بالتجميع مع مصفوفة أخطاء (QEM) + ألوان رؤوس.
الاستخدام:  python3 tools/convert_guns.py      (يقرأ tools/models/*.glb)
يولّد: public/js/gunmodels.js (أندرويد/iOS) و native/src/gunmodels.h (PS4).
فضاء السلاح: الأمام -Z ، محور السبطانة y=0 ، الفوهة عند z=mz.  المجموعات: 0 أمامي، 1 مخزن، 2 متحرك (ترباس)، 3 كعب خلفي."""
import os, sys, json, base64
import numpy as np
sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import glbload
root = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
R180Y = np.diag([-1., 1., -1.])
RPG_R = np.array([[0, 0, -1.], [0, 1, 0], [1, 0, 0]])
CFG = {
    'sniper': dict(src='awm.glb', rot=np.eye(3), drop=set(), length=1.2, mz=-0.8, rear=0.17, mag_m={4, 20}, bolt_m={10, 28, 29}),
    'rifle': dict(src='m4_sopmod.glb', rot=R180Y, drop={0, 5}, length=0.92, mz=-0.5, rear=0.12,
                  mag=lambda c: (c[:, 2] > -0.75) & (c[:, 2] < 0.95) & (c[:, 1] < 0.33) & (c[:, 1] > -1.6)),
    'dmr': dict(src='svd_dragunov.glb', rot=np.eye(3), drop=set(), length=1.2, mz=-0.7, rear=0.2,
                mag=lambda c: (c[:, 2] > 0.05) & (c[:, 2] < 0.8) & (c[:, 1] < -0.05) & (c[:, 1] > -0.65),
                bolt=lambda c: (c[:, 0] > 0.08) & (c[:, 1] > 0.04) & (c[:, 1] < 0.24) & (c[:, 2] > -0.2) & (c[:, 2] < 0.9)),
    'launcher': dict(src='rpg7_-_anti_tank.glb', rot=RPG_R, drop=set(), length=0.95, mz=-0.62, rear=0.12, magz=-0.33),
}
def decimate(P, N, C, K, F, target):
    """تجميع رؤوس على شبكة مع موضع أمثل بمصفوفة أخطاء. K = مفتاح مادة/مجموعة لا يُدمج عبره."""
    # مربّعات المستويات لكل رأس
    a, b, c = P[F[:, 0]], P[F[:, 1]], P[F[:, 2]]
    n = np.cross(b - a, c - a); ar = np.linalg.norm(n, axis=1); n /= np.maximum(ar[:, None], 1e-12)
    d = -(n * a).sum(1); pl = np.c_[n, d] * np.sqrt(ar)[:, None]
    Qf = pl[:, :, None] * pl[:, None, :]
    Qv = np.zeros((len(P), 4, 4))
    for k in range(3): np.add.at(Qv, F[:, k], Qf)
    lo, hi = P.min(0), P.max(0); diag = np.linalg.norm(hi - lo)
    cls = (np.clip(C.mean(1), 0, 255) // 64).astype(int)  # فئة سطوع تقريبية
    oct_ = ((N[:, 0] > 0).astype(int) + 2 * (N[:, 1] > 0) + 4 * (N[:, 2] > 0))
    key2 = (K * 4 + cls) * 8 + oct_
    def run(cs):
        cell = np.floor((P - lo) / cs).astype(np.int64); m = cell.max(0) + 1
        ck = ((cell[:, 0] * m[1] + cell[:, 1]) * m[2] + cell[:, 2]) * (key2.max() + 1) + key2
        u, inv = np.unique(ck, return_inverse=True)
        nF = inv[F]; ok = (nF[:, 0] != nF[:, 1]) & (nF[:, 1] != nF[:, 2]) & (nF[:, 0] != nF[:, 2])
        return u, inv, nF[ok], ok
    lo_s, hi_s = diag / 2000, diag / 8
    for _ in range(18):
        mid = (lo_s * hi_s) ** 0.5; u, inv, nF, ok = run(mid)
        if len(nF) > target: lo_s = mid
        else: hi_s = mid
    u, inv, nF, ok = run(hi_s)
    m = len(u); Q = np.zeros((m, 4, 4)); np.add.at(Q, inv, Qv)
    cnt = np.bincount(inv, minlength=m).astype(float)
    mean = np.zeros((m, 3)); np.add.at(mean, inv, P); mean /= cnt[:, None]
    A = Q[:, :3, :3]; bvec = -Q[:, :3, 3]; lam = 1e-3 * (np.trace(A, axis1=1, axis2=2) + 1e-9)[:, None, None]
    Am = A + lam * np.eye(3); rhs = bvec + (lam[:, :, 0] * mean)
    try: pos = np.linalg.solve(Am, rhs[:, :, None])[:, :, 0]
    except np.linalg.LinAlgError: pos = mean
    # نحصر الموضع داخل صندوق أعضاء المجموعة (يمنع الأشواك)
    mn = np.full((m, 3), np.inf); mx = np.full((m, 3), -np.inf); np.minimum.at(mn, inv, P); np.maximum.at(mx, inv, P)
    pos = np.clip(pos, mn, mx); pos[~np.isfinite(pos).all(1)] = mean[~np.isfinite(pos).all(1)]
    nn = np.zeros((m, 3)); np.add.at(nn, inv, N); nn /= np.maximum(np.linalg.norm(nn, axis=1, keepdims=True), 1e-9)
    col = np.zeros((m, 3)); np.add.at(col, inv, C); col /= cnt[:, None]
    # إزالة المثلثات المكرّرة
    key = np.sort(nF, axis=1); _, ui = np.unique(key, axis=0, return_index=True); nF = nF[np.sort(ui)]
    return pos, nn, col, nF
_cache = {}
def build(name, cfg, tris):
    if cfg['src'] not in _cache: _cache[cfg['src']] = glbload.load(os.path.join(root, 'tools/models', cfg['src']), 1024)
    P, N, C, M, F = [a.copy() for a in _cache[cfg['src']]]
    R = cfg['rot']; P = P @ R.T; N = N @ R.T
    keepf = ~np.isin(M[F[:, 0]], list(cfg['drop'])); F = F[keepf]
    used = np.unique(F); remap = -np.ones(len(P), int); remap[used] = np.arange(len(used)); P, N, C, M, F = P[used], N[used], C[used], M[used], remap[F]
    # الأمام = -Z: الفوهة عند أصغر z
    zmin, zmax = P[:, 2].min(), P[:, 2].max(); S = cfg['length'] / (zmax - zmin)
    front = P[:, 2] < zmin + 0.02 * (zmax - zmin); bx, by = P[front, 0].mean(), P[front, 1].mean()
    # مجموعات حسب مركز المثلث (قبل التبسيط لتبقى دقيقة)
    cen = P[F].mean(1); grp = np.zeros(len(F), int)
    if 'bolt' in cfg: grp[cfg['bolt'](cen)] = 2
    if 'mag' in cfg: grp[cfg['mag'](cen)] = 1
    fm = M[F[:, 0]]
    if 'bolt_m' in cfg: grp[np.isin(fm, list(cfg['bolt_m']))] = 2
    if 'mag_m' in cfg: grp[np.isin(fm, list(cfg['mag_m']))] = 1
    gz = (cen[:, 2] - zmin) * S + cfg['mz']
    if 'magz' in cfg: grp[gz < cfg['magz']] = 1  # الرأس الحربي (قذيفة) مجموعة مستقلة لتتحرك أثناء التعبئة
    if cfg.get('rear') is not None: grp[(grp == 0) & (gz > cfg['rear'])] = 3
    # تبسيط كل مجموعة على حدة (تتحرك مستقلة)
    tot = len(F); outs = []
    for g in range(4):
        sel = grp == g
        if not sel.any(): outs.append((np.zeros((0, 3)), np.zeros((0, 3)), np.zeros((0, 3)), np.zeros((0, 3), int))); continue
        Fg = F[sel]; u = np.unique(Fg); rm = -np.ones(len(P), int); rm[u] = np.arange(len(u))
        tgt = max(300, int(tris * sel.sum() / tot))
        if sel.sum() > tgt: outs.append(decimate(P[u], N[u], C[u], M[u], rm[Fg], tgt))
        else: outs.append((P[u], N[u], C[u], rm[Fg]))
    Pa = []; Na = []; Ca = []; Fa = []; counts = []; base = 0
    for pos, nn, col, nF in outs:
        Pa.append(pos); Na.append(nn); Ca.append(col); Fa.append(nF.reshape(-1) + base); counts.append(len(nF) * 3); base += len(pos)
    Pa = np.vstack(Pa); Na = np.vstack(Na); Ca = np.vstack(Ca); Fi = np.concatenate(Fa)
    Q = np.c_[(Pa[:, 0] - bx) * S, (Pa[:, 1] - by) * S, (Pa[:, 2] - zmin) * S + cfg['mz']]
    Nq = Na.copy()
    print(name, 'verts', len(Q), 'tris', len(Fi) // 3, 'groups', counts, 'bbox', Q.min(0).round(3), Q.max(0).round(3))
    meta = {}
    for g, nm in ((1, 'mag'),):
        if counts[g]:
            vi = np.unique(Fi[sum(counts[:g]):sum(counts[:g + 1])]); lo_, hi_ = Q[vi].min(0), Q[vi].max(0)
            meta['mag'] = [round(float(v), 3) for v in ((lo_ + hi_) / 2).tolist() + (hi_ - lo_).tolist()]
    top = Q[:, 1].max(); meta['top'] = round(float(top), 3)
    return Q, Nq, Ca, Fi.astype(np.uint16), counts, meta
LEVELS = {'h': 50000, 'm': 12000, 'l': 3500, 'p': 0}  # h عالية، m متوسطة، l منخفضة (أندرويد/iOS حسب الجودة) ، p لـ PS4 (مُرسِم برمجي: عدد أقل لكل سلاح)
PS4_TRIS = {'sniper': 20000, 'rifle': 26000, 'dmr': 26000, 'launcher': 9000}
def emit_js(level, models):
    b64 = lambda a: base64.b64encode(a.tobytes()).decode()
    js = '// مولَّد من ملفات GLB بواسطة tools/convert_guns.py — لا تعدّله يدويًا\nexport const GM = {\n'
    for name, (Q, Nq, Ca, Fi, counts, meta) in models.items():
        lo = Q.min(0); hi = Q.max(0); rng = np.maximum(hi - lo, 1e-9)
        qpos = np.round((Q - lo) / rng * 65535 - 32768).astype(np.int16); qnor = np.round(Nq * 127).astype(np.int8); qcol = np.clip(np.round(Ca), 0, 255).astype(np.uint8)
        js += f'  {name}: ' + json.dumps(dict(lo=[round(float(v), 5) for v in lo], rng=[round(float(v), 5) for v in rng], n=len(Q), groups=counts, pos=b64(qpos), nor=b64(qnor), col=b64(qcol), idx=b64(Fi))) + ',\n'
def emit_h(models):
    hh = '// generated by tools/convert_guns.py — do not edit\n#pragma once\n#include <stdint.h>\nnamespace GMD{\n'
    for name, (Q, Nq, Ca, Fi, counts, meta) in models.items():
        lo = Q.min(0); hi = Q.max(0); rng = np.maximum(hi - lo, 1e-9)
        qpos = np.round((Q - lo) / rng * 65535 - 32768).astype(np.int16); qnor = np.round(Nq * 127).astype(np.int8); qcol = np.clip(np.round(Ca), 0, 255).astype(np.uint8)
        arr = lambda a, nm, t: f'static const {t} {name}_{nm}[{a.size}]={{' + ','.join(map(str, a.reshape(-1).tolist())) + '};\n'
        hh += f'static const int {name}_NV={len(Q)};static const int {name}_GRP[4]={{{",".join(map(str, counts))}}};\n'
        hh += f'static const float {name}_LO[3]={{{lo[0]:.5f}f,{lo[1]:.5f}f,{lo[2]:.5f}f}},{name}_RNG[3]={{{rng[0]:.5f}f,{rng[1]:.5f}f,{rng[2]:.5f}f}};\n'
        hh += arr(qpos, 'pos', 'int16_t') + arr(qnor, 'nor', 'int8_t') + arr(qcol, 'col', 'uint8_t') + arr(Fi, 'idx', 'uint16_t')
    open(os.path.join(root, 'native/src/gunmodels.h'), 'w').write(hh + '}\n')
if __name__ == '__main__':
    metas = {}
    for lv, tr in LEVELS.items():
        models = {name: build(name, cfg, tr or PS4_TRIS[name]) for name, cfg in CFG.items()}
        if lv == 'p': emit_h(models)
        else: emit_js(lv, models)
        for n, m in models.items(): metas[n] = m[5]
    print(json.dumps(metas))
