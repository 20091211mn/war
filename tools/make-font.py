# يولّد native/src/fontdata.h : أطلس خط DejaVu Sans Bold (لاتيني + عربي بأشكال العرض)
from PIL import ImageFont, Image, ImageDraw
F='/usr/share/fonts/truetype/dejavu/DejaVuSans-Bold.ttf'
lat=list(range(32,127))+[0xB7,0x2014,0x25B6,0x2190,0x2191,0x2192,0x2193,0x25C0]
ar=list(range(0xFE70,0xFEFD))+[0x60C,0x61F,0x640]
out=['#pragma once','#include <cstdint>','struct FGlyph{uint32_t cp;uint8_t w,h;int8_t bx,by;uint16_t adv16;uint32_t off;};']
blob=[];gl={}
for name,size,cps in (('S',16,lat+ar),('M',32,lat+ar),('L',64,lat)):
    f=ImageFont.truetype(F,size);recs=[]
    for cp in sorted(set(cps)):
        ch=chr(cp); adv=f.getlength(ch)
        P=size; im=Image.new('L',(size*3,size*3),0); d=ImageDraw.Draw(im); d.text((P,size*2),ch,font=f,anchor='ls',fill=255)
        bb=im.getbbox()
        if not bb: recs.append((cp,0,0,0,0,int(adv*16),len(blob)));continue
        c=im.crop(bb);w,h=c.size;off=len(blob);blob.extend(bytes(c.tobytes()))
        recs.append((cp,w,h,bb[0]-P,2*size-bb[1],int(adv*16),off))
    gl[name]=recs
out.append('static const uint8_t FDATA[]={'+','.join(map(str,blob))+'};')
for name in 'SML':
    out.append('static const FGlyph FG_%s[]={'%name+','.join('{%d,%d,%d,%d,%d,%d,%d}'%r for r in gl[name])+'};')
    out.append('static const int FN_%s=%d;'%(name,len(gl[name])))
open('native/src/fontdata.h','w').write('\n'.join(out)+'\n')
print(len(blob),{k:len(v) for k,v in gl.items()})
