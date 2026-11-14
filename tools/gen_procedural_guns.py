#!/usr/bin/env python3
"""يولّد موديلات MP7 (1) وMagnum .357 (7) إجرائياً بنفس صيغة godot/assets/guns/<رقم>.bin
(المحور: الماسورة نحو -Z، الأعلى +Y؛ مجموعات: 0 أمامي، 1 مخزن، 2 ترباس، 3 كعب) — لأنه لا يوجد GLB لهما."""
import struct, math, os
root = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))

class M:
    def __init__(s): s.g = [[], [], [], []]   # لكل مجموعة: (pos, nor, col, idx)
        
    def _grp(s, g): return s.g[g]
    def box(s, g, c, sz, col):
        cx, cy, cz = c; hx, hy, hz = (v / 2 for v in sz)
        faces = [((1,0,0),[(1,-1,-1),(1,1,-1),(1,1,1),(1,-1,1)]), ((-1,0,0),[(-1,-1,1),(-1,1,1),(-1,1,-1),(-1,-1,-1)]),
                 ((0,1,0),[(-1,1,-1),(-1,1,1),(1,1,1),(1,1,-1)]), ((0,-1,0),[(-1,-1,1),(-1,-1,-1),(1,-1,-1),(1,-1,1)]),
                 ((0,0,1),[(-1,-1,1),(1,-1,1),(1,1,1),(-1,1,1)]), ((0,0,-1),[(1,-1,-1),(-1,-1,-1),(-1,1,-1),(1,1,-1)])]
        for n, vs in faces:
            s._quad(g, [(cx + a * hx, cy + b * hy, cz + d * hz) for a, b, d in vs], n, col)
    def cyl(s, g, c, r, ln, col, n=12):   # محور Z
        cx, cy, cz = c; z0, z1 = cz - ln / 2, cz + ln / 2
        for i in range(n):
            a0, a1 = 2 * math.pi * i / n, 2 * math.pi * (i + 1) / n
            p = [(cx + r * math.cos(a0), cy + r * math.sin(a0), z0), (cx + r * math.cos(a1), cy + r * math.sin(a1), z0), (cx + r * math.cos(a1), cy + r * math.sin(a1), z1), (cx + r * math.cos(a0), cy + r * math.sin(a0), z1)]
            am = (a0 + a1) / 2; s._quad(g, p, (math.cos(am), math.sin(am), 0), col)
            for z, nz in ((z0, -1), (z1, 1)):
                s._tri(g, [(cx, cy, z), (cx + r * math.cos(a0), cy + r * math.sin(a0), z), (cx + r * math.cos(a1), cy + r * math.sin(a1), z)] if nz > 0 else [(cx, cy, z), (cx + r * math.cos(a1), cy + r * math.sin(a1), z), (cx + r * math.cos(a0), cy + r * math.sin(a0), z)], (0, 0, nz), col)
    def _quad(s, g, ps, n, col): s._tri(g, [ps[0], ps[1], ps[2]], n, col); s._tri(g, [ps[0], ps[2], ps[3]], n, col)
    def _tri(s, g, ps, n, col):
        P, N, C, I = s.g[g] if s.g[g] else (s.g[g].extend([[], [], [], []]) or s.g[g])
        b = len(P)
        for p in ps: P.append(p); N.append(n); C.append(col)
        I.extend([b, b + 1, b + 2])
    def save(s, gid):
        allp, alln, allc, alli, cnts = [], [], [], [], []
        for g in range(4):
            if not s.g[g]: cnts.append(0); continue
            P, N, C, I = s.g[g]; base = len(allp)
            allp += P; alln += N; allc += C; alli += [base + i for i in I]; cnts.append(len(I))
        nv = len(allp)
        b = struct.pack('<II4i', nv, len(alli), *cnts)
        b += struct.pack('<%df' % (nv * 3), *[v for p in allp for v in p]) + struct.pack('<%df' % (nv * 3), *[v for n in alln for v in n])
        b += bytes([c for col in allc for c in col]); b += b'\0' * ((-len(b)) % 4) + struct.pack('<%di' % len(alli), *alli)
        open(root + '/godot/assets/guns/%d.bin' % gid, 'wb').write(b); print('gun', gid, nv, 'verts', cnts)

DARK = (38, 40, 44); GRAY = (70, 74, 80); STEEL = (118, 122, 130); BLACK = (20, 21, 24); TAN = (96, 88, 74); WOOD = (92, 58, 36)

# ---- MP7 ----
m = M()
m.box(0, (0, 0.0, -0.02), (0.05, 0.065, 0.30), DARK)                      # الصندوق
m.box(0, (0, 0.036, -0.03), (0.034, 0.012, 0.22), GRAY)                   # سكة علوية
m.cyl(0, (0, 0.012, -0.235), 0.012, 0.16, BLACK); m.cyl(0, (0, 0.012, -0.33), 0.019, 0.06, BLACK)   # ماسورة + كاتم
m.box(0, (0, 0.05, -0.19), (0.006, 0.02, 0.008), BLACK); m.box(0, (0, 0.05, 0.07), (0.02, 0.016, 0.012), BLACK)   # مرمى أمامي/خلفي
m.box(0, (0, -0.075, 0.075), (0.04, 0.115, 0.048), TAN)                   # القبضة
m.box(0, (0, -0.032, -0.005), (0.026, 0.008, 0.07), DARK)                 # حماية الزناد
m.box(0, (0, -0.045, -0.15), (0.026, 0.075, 0.03), TAN)                   # قبضة أمامية
m.box(1, (0, -0.12, 0.012), (0.03, 0.13, 0.034), BLACK)                   # مخزن مستقيم أمام القبضة
m.box(2, (0.0, 0.03, 0.095), (0.036, 0.022, 0.05), STEEL)                 # ترباس/مقبض التلقيم
m.cyl(3, (0.016, 0.0, 0.22), 0.0045, 0.14, STEEL); m.cyl(3, (-0.016, 0.0, 0.22), 0.0045, 0.14, STEEL)   # قضبان الكعب المنزلق
m.box(3, (0, -0.005, 0.30), (0.034, 0.075, 0.016), BLACK)                  # لوح الكتف
m.save(1)

# ---- Magnum .357 ----
m = M()
m.box(0, (0, 0.0, 0.01), (0.04, 0.07, 0.11), STEEL)                       # الهيكل
m.cyl(0, (0, 0.022, -0.19), 0.013, 0.28, STEEL)                           # الماسورة
m.box(0, (0, 0.042, -0.19), (0.011, 0.009, 0.28), GRAY)                   # سكة الماسورة
m.box(0, (0, 0.05, -0.31), (0.008, 0.016, 0.01), BLACK)                   # مرمى أمامي
m.box(0, (0, -0.088, 0.075), (0.036, 0.13, 0.052), WOOD)                  # قبضة خشبية
m.box(0, (0, -0.036, 0.0), (0.024, 0.008, 0.075), STEEL)                  # حماية الزناد
m.cyl(1, (0, 0.012, -0.045), 0.033, 0.07, GRAY, 14)                       # الأسطوانة (تُنزَع عند التعبئة)
m.box(2, (0, 0.052, 0.078), (0.014, 0.034, 0.018), BLACK)                 # المطرقة
m.save(7)
