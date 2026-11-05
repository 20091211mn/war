"""قراءة GLB (بدون مكتبات خارجية عدا numpy/PIL): رؤوس عالمية + ألوان رؤوس من تكستشر/عامل المادة + رقم الشبكة لكل رأس."""
import json, struct, io
import numpy as np
from PIL import Image
Image.MAX_IMAGE_PIXELS = None
def load(path, tex_size=1024):
    b = open(path, 'rb').read(); jl = struct.unpack_from('<I', b, 12)[0]
    J = json.loads(b[20:20 + jl]); bin_ = b[20 + jl + 8:]
    def acc(i):
        a = J['accessors'][i]; v = J['bufferViews'][a['bufferView']]
        nc = {'SCALAR': 1, 'VEC2': 2, 'VEC3': 3, 'VEC4': 4}[a['type']]
        dt = {5126: '<f4', 5123: '<u2', 5125: '<u4', 5121: 'u1'}[a['componentType']]
        off = v.get('byteOffset', 0) + a.get('byteOffset', 0); isz = np.dtype(dt).itemsize; st = v.get('byteStride', nc * isz)
        raw = np.frombuffer(bin_, dtype=np.uint8, count=st * a['count'], offset=off).reshape(a['count'], st)
        return raw[:, :nc * isz].copy().view(dt).reshape(a['count'], nc)
    texcache = {}
    def tex(ti):
        if ti in texcache: return texcache[ti]
        v = J['bufferViews'][J['images'][J['textures'][ti]['source']]['bufferView']]
        im = Image.open(io.BytesIO(bin_[v.get('byteOffset', 0):v.get('byteOffset', 0) + v['byteLength']])).convert('RGB').resize((tex_size, tex_size), Image.LANCZOS)
        texcache[ti] = np.asarray(im).astype(np.float32); return texcache[ti]
    def matcolor(mi, uv):
        m = J['materials'][mi] if mi is not None else {}; pbr = m.get('pbrMetallicRoughness', {})
        fac = np.array(pbr.get('baseColorFactor', [1, 1, 1, 1])[:3], np.float32) * 255
        ti = None
        if 'baseColorTexture' in pbr: ti = pbr['baseColorTexture']['index']
        else:
            sg = m.get('extensions', {}).get('KHR_materials_pbrSpecularGlossiness', {})
            if 'diffuseTexture' in sg: ti = sg['diffuseTexture']['index']; fac = np.array(sg.get('diffuseFactor', [1, 1, 1, 1])[:3], np.float32) * 255
            elif 'emissiveTexture' in m: ti = m['emissiveTexture']['index']; fac = np.array([255, 255, 255], np.float32)
        if ti is None: return np.tile(fac, (len(uv), 1))
        t = tex(ti); n = t.shape[0]
        x = np.clip((uv[:, 0] % 1.0) * (n - 1), 0, n - 1).astype(int); y = np.clip((uv[:, 1] % 1.0) * (n - 1), 0, n - 1).astype(int)
        c = np.zeros((len(x), 3), np.float32)
        for dx in (-1, 0, 1):
            for dy in (-1, 0, 1): c += t[np.clip(y + dy, 0, n - 1), np.clip(x + dx, 0, n - 1)]
        return c / 9 * (fac / 255)
    nodes = []
    def walk(i, pm):
        n = J['nodes'][i]
        if 'matrix' in n: m = np.array(n['matrix']).reshape(4, 4).T
        else:
            m = np.eye(4)
            if 'scale' in n: m = np.diag(list(n['scale']) + [1]) @ m
            if 'rotation' in n:
                x, y, z, w = n['rotation']; R = np.array([[1-2*(y*y+z*z), 2*(x*y-z*w), 2*(x*z+y*w)], [2*(x*y+z*w), 1-2*(x*x+z*z), 2*(y*z-x*w)], [2*(x*z-y*w), 2*(y*z+x*w), 1-2*(x*x+y*y)]])
                M = np.eye(4); M[:3, :3] = R; m = M @ m
            if 'translation' in n: T = np.eye(4); T[:3, 3] = n['translation']; m = T @ m
        m = pm @ m
        if 'mesh' in n: nodes.append((n['mesh'], m))
        for c in n.get('children', []): walk(c, m)
    for s in J['scenes'][J.get('scene', 0)]['nodes']: walk(s, np.eye(4))
    P = []; N = []; C = []; M = []; F = []; base = 0; k = 0
    for mi, m in nodes:
        for p in J['meshes'][mi]['primitives']:
            pos = acc(p['attributes']['POSITION']).astype(np.float64); nor = acc(p['attributes']['NORMAL']).astype(np.float64)
            uv = acc(p['attributes']['TEXCOORD_0']).astype(np.float64) if 'TEXCOORD_0' in p['attributes'] else np.zeros((len(pos), 2))
            idx = acc(p['indices']).reshape(-1).astype(np.int64)
            w = (np.c_[pos, np.ones(len(pos))] @ m.T)[:, :3]; nn = nor @ np.linalg.inv(m[:3, :3]).T
            nn /= np.maximum(np.linalg.norm(nn, axis=1, keepdims=True), 1e-9)
            P.append(w); N.append(nn); C.append(matcolor(p.get('material'), uv)); M.append(np.full(len(w), k)); F.append(idx.reshape(-1, 3) + base); base += len(w); k += 1
    return np.vstack(P), np.vstack(N), np.vstack(C), np.concatenate(M), np.vstack(F)
