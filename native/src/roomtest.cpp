// اختبار شاشة إعداد الغرفة: يحاكي الأزرار ثم يكتب صورة PPM
#include "game.h"
#include <cstdio>
static void ppm(Game&g,const char*fn){ FILE*f=fopen(fn,"wb"); fprintf(f,"P6\n%d %d\n255\n",g.r.W,g.r.H); for(int i=0;i<g.r.W*g.r.H;i++){ uint32_t p=g.r.fb[i]; fputc(p&255,f); fputc((p>>8)&255,f); fputc((p>>16)&255,f);} fclose(f); }
int main(int argc,char**argv){ srand(3); Game g; g.menu=1; g.init(960,540); Input z,a; float dt=1/60.f;
  g.update(dt,z); g.menuRow=2; g.mode=1; // استضافة LAN
  a.confirm=true; g.update(dt,a); g.update(dt,z); if(g.menu!=4){ printf("FAIL menu=%d\n",g.menu); return 1; }
  auto press=[&](float mx,float my){ Input i; i.mx=mx; i.my=my; g.update(dt,i); g.update(dt,z); };
  press(1,0); press(0,-1); press(1,0); press(1,0); press(0,-1); press(1,0); press(0,-1); press(1,0); press(0,-1); press(0,-1); press(1,0); // style → duel ؟ نرجع
  press(0,1); press(-1,0);
  printf("room: name %s max %d lim %d min %d bots %d style %d row %d\n",Game::rmNameOf(g.rmName),g.rmMaxE(),Game::rmLimV(g.rmLimI),Game::rmMinV(g.rmMinI),g.rmBotsE(),g.rmStyle,g.rmRow);
  g.renderMenu(); if(argc>1)ppm(g,argv[1]);
  Input b; b.back=true; g.update(dt,b); g.update(dt,z); if(g.menu!=1){ printf("FAIL back\n"); return 1; }
  printf("room ui ok\n"); return 0; }
