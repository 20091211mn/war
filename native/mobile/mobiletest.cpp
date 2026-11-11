// اختبار طبقة الهاتف على الحاسوب: يحاكي لمساً (قائمة → بدء → حركة ونظر وإطلاق) ويكتب صوراً PPM
#include "mobile.h"
#include <cstdio>
static void ppm(const Mobile&m,const char*fn){ FILE*f=fopen(fn,"wb"); fprintf(f,"P6\n%d %d\n255\n",m.W,m.H); for(auto p:m.out){ fputc(p&255,f); fputc((p>>8)&255,f); fputc((p>>16)&255,f); } fclose(f); }
int main(int argc,char**argv){
  const char*pre=argc>1?argv[1]:nullptr; srand(5);
  Mobile m; m.start(2400,1080,4); int bad=0;
  float u=m.U();
  auto tap=[&](int id,float fx,float fy,int frames){ m.touchDown(id,fx*m.sw/m.W,fy*m.sh/m.H); for(int i=0;i<frames;i++)m.frame(1/60.f); m.touchUp(id); m.frame(1/60.f); };
  for(int i=0;i<5;i++)m.frame(1/60.f); if(pre)ppm(m,(std::string(pre)+"_menu.ppm").c_str());
  { int q0=g_quality; for(auto&h:m.g.hits)if(h.kind==1&&h.idx==3){ tap(9,h.x+h.w*0.8f,h.y+h.h/2,2); } for(int i=0;i<3;i++)m.frame(1/60.f); if(g_quality==q0){ printf("FAIL: tap quality row\n"); bad|=64; } g_quality=q0; m.g.qApplied=-1; }
  { float sx=m.W-110*u,sy=m.H-110*u; for(auto&h:m.g.hits)if(h.kind==2){ sx=h.x+h.w/2; sy=h.y+h.h/2; } tap(1,sx,sy,3); } for(int i=0;i<5;i++)m.frame(1/60.f); // OK → ابدأ
  if(m.g.menu!=0){ printf("FAIL: menu did not start (menu=%d)\n",m.g.menu); bad|=1; }
  float x0=m.g.e[m.g.meI].b.x,z0=m.g.e[m.g.meI].b.z,yaw0=m.g.e[m.g.meI].yaw; int ammo0=m.g.e[m.g.meI].ammo;
  // حركة للأمام (عصا يسار) + نظر (سحب يمين) + إطلاق
  m.touchDown(2,200*m.sw/m.W,(m.H-120*u)*m.sh/m.H); for(int i=0;i<60*3;i++){ if(i==5)m.touchMove(2,200*m.sw/m.W,(m.H-120*u-70*u)*m.sh/m.H); if(i==120){ m.touchDown(3,(m.W*0.6f)*m.sw/m.W,m.H*0.4f*m.sh/m.H); } if(i>120&&i<150)m.touchMove(3,(m.W*0.6f+(i-120)*6)*m.sw/m.W,m.H*0.4f*m.sh/m.H); if(i==60)m.touchDown(4,(m.W-130*u)*m.sw/m.W,(m.H-140*u)*m.sh/m.H); if(i==90)m.touchUp(4); m.frame(1/60.f); }
  const Ent&me=m.g.e[m.g.meI]; float d=hypotf(me.b.x-x0,me.b.z-z0);
  printf("moved %.1f m, yaw change %.2f, ammo %d -> %d, res %dx%d\n",d,me.yaw-yaw0,ammo0,me.ammo,m.W,m.H);
  if(d<3.f){ printf("FAIL: no movement\n"); bad|=2; } if(fabsf(me.yaw-yaw0)<0.5f){ printf("FAIL: no look\n"); bad|=4; } if(me.ammo>=ammo0){ printf("FAIL: no fire\n"); bad|=8; }
  m.frame(1/60.f); if(pre)ppm(m,(std::string(pre)+"_game.ppm").c_str());
  { // عجلة الأسلحة: اضغط WPN ثم اختر السلاح رقم 3
    for(int i=0;i<10;i++)m.frame(1/60.f); m.touchDown(6,(m.W/2+190*u)*m.sw/m.W,(m.H-40*u)*m.sh/m.H); m.frame(1/60.f); m.touchUp(6); m.frame(1/60.f);
    if(!m.g.wheelOpen){ printf("FAIL: wheel did not open\n"); bad|=32; } else { if(pre)ppm(m,(std::string(pre)+"_wheel.ppm").c_str()); int rx,ry,rw,rh; m.g.wheelRect(4,m.g.hudSc(),rx,ry,rw,rh);
      m.touchDown(7,(rx+rw/2)*m.sw/m.W,(ry+rh/2)*m.sh/m.H); m.frame(1/60.f); m.touchUp(7); for(int i=0;i<3;i++)m.frame(1/60.f);
      if(m.g.e[m.g.meI].wpn!=4||m.g.wheelOpen){ printf("FAIL: pick weapon (wpn=%d open=%d)\n",m.g.e[m.g.meI].wpn,(int)m.g.wheelOpen); bad|=64; } else { m.g.wsT=2.f; m.frame(1/60.f); if(pre)ppm(m,(std::string(pre)+"_strip.ppm").c_str()); } } }
  m.touchDown(5,(m.W/2+125*u)*m.sw/m.W,28*u*m.sh/m.H); m.frame(1/60.f); m.touchUp(5); m.frame(1/60.f); m.frame(1/60.f); // إيقاف مؤقت
  if(!m.g.paused){ printf("FAIL: pause\n"); bad|=16; } { for(auto&h:m.g.hits)if(h.kind==5){ tap(8,h.x+h.w/2,h.y+h.h/2,2); } m.frame(1/60.f); if(m.g.paused){ printf("FAIL: resume tap\n"); bad|=128; } } if(pre)ppm(m,(std::string(pre)+"_pause.ppm").c_str());
  printf(bad?"MOBILE TEST FAILED %d\n":"mobile ok\n",bad); return bad?1:0;
}
