#!/usr/bin/env python3
"""يحوّل awm.glb (موديل AWM بتكستشر) إلى بيانات خفيفة بألوان رؤوس لاستخدامها في أندرويد (public/js/awm-data.js)
وPS4 (native/src/awm_data.h).  الاستخدام:  python3 tools/convert_awm.py [awm.glb]
الفضاء الناتج = فضاء السلاح في اللعبة: الأمام -Z ، الفوهة عند z=-0.8 ، محور السبطانة y=0 .
المجموعات: 0 جسم أمامي، 1 مخزن، 2 ترباس (يتحرك عند التعبئة)، 3 الكعب الخلفي (يُخفى أثناء التعبئة كما في بقية الأسلحة)."""
import sys, json, struct, base64, os
import numpy as np
from PIL import Image
Image.MAX_IMAGE_PIXELS = None
src = sys.argv[1] if len(sys.argv) > 1 else 'awm.glb'
root = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
b = open(src, 'rb').read()
jl = struct.unpack_from('<I', b, 12)[0]
J = json.loads(b[20:20 + jl]); bin_ = b[20 + jl + 8:]
def acc(i):
    a = J['accessors'][i]; v = J['bufferViews'][a['bufferView']]
    nc = {'SCALAR': 1, 'VEC2': 2, 'VEC3': 3, 'VEC4': 4}[a['type']]
    dt = {5126: '<f4', 5123: '<u2', 5125: '<u4'}[a['componentType']]
    off = v.get('byteOffset', 0) + a.get('byteOffset', 0); isz = np.dtype(dt).itemsize
    st = v.get('byteStride', nc * isz)
    raw = np.frombuffer(bin_, dtype=np.uint8, count=st * a['count'], offset=off).reshape(a['count'], st)
    return raw[:, :nc * isz].copy().view(dt).reshape(a['count'], nc)
def walk(i, pm, out):
    n = J['nodes'][i]; m = pm @ (np.array(n['matrix']).reshape(4, 4).T if 'matrix' in n else np.eye(4))
    if 'mesh' in n: out.append((n['mesh'], m))
    for c in n.get('children', []): walk(c, m, out)
nodes = []; walk(0, np.eye(4), nodes)
im = Image.open(os.path.join(os.path.dirname(os.path.abspath(src)), 'awm_basecolor.png')) if os.path.exists('awm_basecolor.png') else None
if im is None:
    v = J['bufferViews'][J['images'][J['textures'][J['materials'][0]['pbrMetallicRoughness']['baseColorTexture']['index']]['source']]['bufferView']]
    import io; im = Image.open(io.BytesIO(bin_[v.get('byteOffset', 0):v.get('byteOffset', 0) + v['byteLength']]))
im = im.convert('RGB').resize((1024, 1024), Image.LANCZOS); tex = np.asarray(im).astype(np.float32)
BOLT = {10, 28, 29}; MAG = {4, 20}
P = []; N = []; C = []; G = []; F = []; base = 0
for mi, m in nodes:
    p = J['meshes'][mi]['primitives'][0]
    pos = acc(p['attributes']['POSITION']).astype(np.float64); nor = acc(p['attributes']['NORMAL']).astype(np.float64)
    uv = acc(p['attributes']['TEXCOORD_0']).astype(np.float64); idx = acc(p['indices']).reshape(-1).astype(np.int64)
    w = (np.c_[pos, np.ones(len(pos))] @ m.T)[:, :3]; nn = nor @ m[:3, :3].T
    nn /= np.maximum(np.linalg.norm(nn, axis=1, keepdims=True), 1e-9)
    x = np.clip((uv[:, 0] % 1.0) * 1023, 0, 1023).astype(int); y = np.clip((uv[:, 1] % 1.0) * 1023, 0, 1023).astype(int)
    # متوسط 3x3 لتقليل الضوضاء
    col = np.zeros((len(x), 3), np.float32)
    for dx in (-1, 0, 1):
        for dy in (-1, 0, 1): col += tex[np.clip(y + dy, 0, 1023), np.clip(x + dx, 0, 1023)]
    col /= 9
    P.append(w); N.append(nn); C.append(col); G.append(np.full(len(w), 2 if mi in BOLT else 1 if mi in MAG else 0)); F.append(idx.reshape(-1, 3) + base); base += len(w)
P = np.vstack(P); N = np.vstack(N); C = np.vstack(C); G = np.concatenate(G); F = np.vstack(F)
# إلى فضاء اللعبة
S = 1.2 / (P[:, 2].max() - P[:, 2].min())
Q = np.c_[(P[:, 0] - 0.03) * S, (P[:, 1] - 1.59) * S, (P[:, 2] - P[:, 2].min()) * S - 0.8]
# ترتيب المثلثات حسب المجموعة (مجموعة الرأس الأول)
cz = Q[F][:, :, 2].mean(1); fg = G[F[:, 0]].copy(); fg[(fg == 0) & (cz > 0.17)] = 3; order = np.argsort(fg, kind='stable'); F = F[order]; fg = fg[order]
counts = [int((fg == g).sum()) * 3 for g in range(4)]
# تقدير مركز الترباس للدوران/الإزاحة غير لازم (إزاحة z فقط)
lo = Q.min(0); hi = Q.max(0); rng = np.maximum(hi - lo, 1e-9)
qpos = np.round((Q - lo) / rng * 65535 - 32768).astype(np.int16)
qnor = np.round(N * 127).astype(np.int8); qcol = np.clip(np.round(C), 0, 255).astype(np.uint8)
idx = F.reshape(-1).astype(np.uint16)
print('verts', len(Q), 'tris', len(F), 'groups', counts, 'bbox', lo.round(3), hi.round(3))
meta = dict(lo=[round(float(v), 5) for v in lo], rng=[round(float(v), 5) for v in rng], n=len(Q), groups=counts)
b64 = lambda a: base64.b64encode(a.tobytes()).decode()
js = '// مولَّد من awm.glb بواسطة tools/convert_awm.py — لا تعدّله يدويًا\nexport const AWM = ' + json.dumps(dict(meta, pos=b64(qpos), nor=b64(qnor), col=b64(qcol), idx=b64(idx))) + ';\n'
open(os.path.join(root, 'public/js/awm-data.js'), 'w').write(js)
def arr(a, name, t):
    f = a.reshape(-1).tolist(); return f'static const {t} {name}[{len(f)}]={{' + ','.join(map(str, f)) + '};\n'
h = '// مولَّد من awm.glb بواسطة tools/convert_awm.py — لا تعدّله يدويًا\n#pragma once\n#include <stdint.h>\nnamespace AWMD{\n'
h += f'static const int NV={len(Q)}, NI={len(idx)}; static const int GRP[4]={{{counts[0]},{counts[1]},{counts[2]},{counts[3]}}};\n'
h += f'static const float LO[3]={{{lo[0]:.5f}f,{lo[1]:.5f}f,{lo[2]:.5f}f}}, RNG[3]={{{rng[0]:.5f}f,{rng[1]:.5f}f,{rng[2]:.5f}f}};\n'
h += arr(qpos, 'pos', 'int16_t') + arr(qnor, 'nor', 'int8_t') + arr(qcol, 'col', 'uint8_t') + arr(idx, 'idx', 'uint16_t') + '}\n'
open(os.path.join(root, 'native/src/awm_data.h'), 'w').write(h)
