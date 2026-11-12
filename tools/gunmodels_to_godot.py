#!/usr/bin/env python3
"""يحوّل native/src/gunmodels.h (موديلات M4/AWM/SVD/RPG المبسّطة) إلى godot/assets/guns/<رقم السلاح>.bin
الصيغة: u32 nv, u32 ni, 4 x i32 (عدد فهارس كل مجموعة: 0 أمامي 1 مخزن 2 ترباس 3 كعب), float32 pos[nv*3], float32 nor[nv*3], u8 col[nv*3], حشو لمحاذاة 4, i32 idx[ni]"""
import re,struct,os
root=os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
src=open(root+'/native/src/gunmodels.h',encoding='utf-8').read()
def arr(name,typ):
    m=re.search(r'static const %s %s\[\d+\]=\{([^}]*)\}'%(typ,name),src); return [float(x) if '.' in x else int(x) for x in m.group(1).split(',') if x.strip()]
os.makedirs(root+'/godot/assets/guns',exist_ok=True)
for nm,gid in [('rifle',0),('sniper',3),('dmr',5),('launcher',8)]:
    grp=[int(x) for x in re.search(r'%s_GRP\[4\]=\{([^}]*)\}'%nm,src).group(1).split(',')]
    lo=[float(x.rstrip('f')) for x in re.search(r'%s_LO\[3\]=\{([^}]*)\}'%nm,src).group(1).split(',')]
    rng=[float(x.rstrip('f')) for x in re.search(r'%s_RNG\[3\]=\{([^}]*)\}'%nm,src).group(1).split(',')]
    pos=arr(nm+'_pos','int16_t'); nor=arr(nm+'_nor','int8_t'); col=arr(nm+'_col','uint8_t'); idx=arr(nm+'_idx','uint16_t')
    P=[lo[k]+(pos[i+k]+32768)/65535.0*rng[k] for i in range(0,len(pos),3) for k in range(3)]
    b=struct.pack('<II4i',len(pos)//3,len(idx),*grp)+struct.pack('<%df'%len(P),*P)+struct.pack('<%df'%len(nor),*[v/127.0 for v in nor])+bytes(col)
    b+=b'\0'*((-len(b))%4)+struct.pack('<%di'%len(idx),*idx)
    open(root+'/godot/assets/guns/%d.bin'%gid,'wb').write(b)
