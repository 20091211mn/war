#!/usr/bin/env python3
"""يحوّل GLB الأسلحة الجديدة (SPAS-12 / M249 / G17) إلى godot/assets/guns/<رقم>.bin عبر convert_guns.build
الاستخدام: python3 tools/convert_godot_guns.py [--plot]"""
import os, sys, struct
import numpy as np
sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import convert_guns as cg
RPG_R = cg.RPG_R
CFG = {
    2: ('spas', dict(src='franchi_spas-12_shotgun.glb', rot=np.eye(3), drop=set(), length=1.0, mz=-0.55, rear=0.3,
                bolt_q=lambda q: (q[:, 2] > -0.40) & (q[:, 2] < -0.10) & (q[:, 1] < -0.012) & (q[:, 1] > -0.1))),
    4: ('m249', dict(src='m249__freefire.glb', rot=RPG_R, drop=set(), length=1.1, mz=-0.7, rear=0.25,
                mag_q=lambda q: (q[:, 1] < -0.08) & (q[:, 2] > -0.12) & (q[:, 2] < 0.1))),
    6: ('g17', dict(src='g17_pistol.glb', rot=RPG_R, drop={0, 2, 3, 4, 5, 6, 7, 8}, length=0.36, mz=-0.3, rear=None,
               bolt_q=lambda q: (q[:, 1] > -0.004) & (q[:, 2] < 0.03),
               mag_q=lambda q: (q[:, 1] < -0.15) & (q[:, 2] > -0.045))),
}
def write(gid, Q, Nq, Ca, Fi, counts):
    b = struct.pack('<II4i', len(Q), len(Fi), *counts) + Q.astype('<f4').tobytes() + Nq.astype('<f4').tobytes() + np.clip(np.round(Ca), 0, 255).astype(np.uint8).tobytes()
    b += b'\0' * ((-len(b)) % 4) + Fi.astype('<i4').tobytes()
    open(os.path.join(cg.root, 'godot/assets/guns/%d.bin' % gid), 'wb').write(b)
if __name__ == '__main__':
    plot = '--plot' in sys.argv
    for gid, (nm, cfg) in CFG.items():
        Q, Nq, Ca, Fi, counts, meta = cg.build(nm, cfg, 14000)
        write(gid, Q, Nq, Ca, Fi, counts)
        if plot:
            import matplotlib; matplotlib.use('Agg'); import matplotlib.pyplot as plt
            fig, ax = plt.subplots(figsize=(10, 4)); o = 0
            for g, c in enumerate(['gray', 'red', 'blue', 'green']):
                ix = Fi[o:o + counts[g]]; o += counts[g]
                if len(ix): p = Q[ix]; ax.scatter(p[:, 2], p[:, 1], s=0.3, c=c)
            ax.set_aspect('equal'); ax.grid(True); fig.savefig('/tmp/plot_%s.png' % nm, dpi=90)
