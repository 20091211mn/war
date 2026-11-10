#ifndef THREADS
#define THREADS 2
#endif
#include "game.h"
#include <cstdio>
static void ppm(Renderer&r,const char*fn){FILE*f=fopen(fn,"wb");fprintf(f,"P6\n%d %d\n255\n",r.W,r.H);for(auto p:r.fb){fputc((p>>16)&255,f);fputc((p>>8)&255,f);fputc(p&255,f);}fclose(f);}
int main(){
  srand(3); Game g; g.r.startThreads(THREADS); g.nBots=6; g.menu=0; g.mapIdx=8; g.init(960,540);
  if(!g.air){ puts("FAIL air not active"); return 1; }
  printf("jets %zu ents %zu\n",g.jets.size(),g.e.size());
  Input in; int nan=0,over=0; float maxAlt=0;
  for(int f=0;f<60*240&&!g.over;f++){ in.my=0.05f*sinf(f*0.01f); in.mx=0.3f*sinf(f*0.007f); in.fire=false; g.update(1/60.f,in);
    for(auto&J:g.jets){ if(std::isnan(J.x)||std::isnan(J.y)||std::isnan(J.z)||std::isnan(J.vx))nan++; if(J.alive&&J.y>maxAlt)maxAlt=J.y; }
    if(f==60*40){ g.render(); g.r.flush(); ppm(g.r,"/tmp/air40.ppm"); } }
  int al=0; for(auto&J:g.jets)al+=J.alive;
  printf("after: score %d-%d over %d jetsAlive %d nan %d maxAlt %.0f meAlive %d deaths(me) %d\n",g.score[0],g.score[1],g.over,al,nan,maxAlt,g.e[0].alive,g.e[0].deaths);
  // اختبار الإقلاع: القفز من الطائرة على الأرض ثم ركوب طائرة احتياطية
  Game h; h.r.startThreads(THREADS); h.menu=0; h.mapIdx=8; h.init(960,540); h.over=false;
  int sj=-1; for(int k=0;k<(int)h.jets.size();k++) if(h.jets[k].spare&&h.jets[k].alive&&h.jets[k].team==0){ sj=k; break; }
  // اجعل اللاعب يركب الطائرة الاحتياطية مباشرة عبر الإقلاع الفعلي: ضعه بجانبها كمشاة
  int mj=h.myJet(); h.jets[mj].alive=false; h.jets[mj].pilot=-1; h.entJet[0]=-1; Ent&p=h.e[0]; p.b=Body(); p.b.x=h.jets[sj].x+8; p.b.z=h.jets[sj].z; p.b.y=0.1f; p.alive=true; p.hp=100;
  Input t; t.tri=true; h.update(1/60.f,t); t.tri=false; h.update(1/60.f,t);
  printf("entered jet %d (expected %d)\n",h.myJet(),sj); if(h.myJet()!=sj){ puts("FAIL enter"); return 1; }
  { Input x; for(int f=0;f<10;f++)h.update(1/60.f,x); x.tri=true; h.update(1/60.f,x); x.tri=false; h.update(1/60.f,x); printf("exit on ground: myJet=%d wpn=%s ammo=%d mags=%d\n",h.myJet(),WEAPONS[h.e[0].wpn].id,h.e[0].ammo,h.e[0].mags);
    if(h.myJet()!=-1||strcmp(WEAPONS[h.e[0].wpn].id,"rpg")||h.e[0].mags!=5){ puts("FAIL exit"); return 1; }
    h.e[0].b.x=h.jets[sj].x+6; h.e[0].b.z=h.jets[sj].z; x.tri=true; h.update(1/60.f,x); x.tri=false; h.update(1/60.f,x); printf("re-enter: myJet=%d\n",h.myJet()); if(h.myJet()!=sj){ puts("FAIL re-enter"); return 1; } }
  bool airborne=false; float maxY=0,maxS=0; for(int f=0;f<60*60;f++){ Input k; k.jump=f<120; k.my=(h.jets[sj].vx*h.jets[sj].vx+h.jets[sj].vz*h.jets[sj].vz>55*55)?0.6f:0; if(h.jets[sj].y>150)k.my=-0.1f; h.update(1/60.f,k); const Jet&J=h.jets[sj]; if(!J.alive){ printf("jet destroyed at f=%d y=%.1f\n",f,J.y); break; } if(J.y>5)airborne=true; maxY=std::max(maxY,J.y); maxS=std::max(maxS,sqrtf(J.vx*J.vx+J.vy*J.vy+J.vz*J.vz)); if(f%300==0)printf(" f=%d x=%.0f y=%.0f spd=%.0f pitch=%.2f\n",f,J.x,J.y,sqrtf(J.vx*J.vx+J.vy*J.vy+J.vz*J.vz),J.pitch); }
  printf("takeoff airborne=%d maxY=%.0f maxSpd=%.0f\n",airborne,maxY,maxS);
  return (nan==0&&airborne)?0:1;
}
