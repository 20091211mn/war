// اختبار C4 على PS4 (منفرد): زرع، حد الشحنات، تفجير عن بُعد، ضرر، اختفاء بالموت، تجديد
#include "game.h"
#include <cstdio>
static int fails=0; static void ok(bool c,const char*m){ printf("  %s %s\n",c?"OK":"FAIL",m); if(!c)fails++; }
int main(){ srand(7); Game g; g.menu=0; g.init(320,180); g.startMatch(0); Input in; for(int i=0;i<30;i++)g.update(1/60.f,in);
  auto&me=g.e[g.meI]; me.prot=0; ok(me.c4Left==2,"شحنتان");
  Input p=in; p.c4=true; g.update(1/60.f,p); g.update(1/60.f,in); ok(g.c4s.size()==1&&me.c4Left==1,"زُرعت شحنة");
  g.update(1/60.f,p); g.update(1/60.f,in); g.update(1/60.f,p); g.update(1/60.f,in); ok(g.c4s.size()==2&&me.c4Left==0,"الحد شحنتان");
  int en=-1; for(int i=1;i<(int)g.e.size();i++)if(g.e[i].team!=me.team){ en=i; break; }
  C4s c=g.c4s[0]; g.e[en].b.x=c.x-me.b.x+me.b.x; g.e[en].b.z=c.z; g.e[en].b.y=c.y-0.05f; { float best=0; for(int a=0;a<8;a++){ float ox=cosf(a*0.785f)*1.5f,oz=sinf(a*0.785f)*1.5f; float t=raycastMap(g.w,c.x,c.y+0.2f,c.z,ox/1.5f,0.f,oz/1.5f,1.5f); if(t>best&&t>=1.5f){ best=t; g.e[en].b.x=c.x+ox; g.e[en].b.z=c.z+oz; } } } g.e[en].hp=100; g.e[en].alive=true; g.e[en].prot=0;
  me.b.x=c.x+40; me.b.z=c.z; Input d=in; d.det=true; g.update(1/60.f,d); g.update(1/60.f,in);
  printf("en hp %.0f alive %d pos %.1f,%.1f c %.1f,%.1f,%.1f team %d/%d\n",g.e[en].hp,(int)g.e[en].alive,g.e[en].b.x,g.e[en].b.z,c.x,c.y,c.z,g.e[en].team,me.team); ok(g.c4s.empty(),"فُجّرت"); ok(g.e[en].hp<100||!g.e[en].alive,"الخصم تأذّى");
  for(int i=0;i<60*22;i++)g.update(1/60.f,in); ok(me.c4Left==2,"تتجدد بعد 20 ث");
  g.update(1/60.f,p); g.update(1/60.f,in); ok(g.c4s.size()==1,"زرع بعد التجديد"); g.hurt(g.meI,en,1000,false); g.update(1/60.f,in); ok(g.c4s.empty()&&me.c4Left==2,"تختفي بموت صاحبها");
  for(int i=0;i<5;i++)g.render(); printf(fails?"FAILED %d\n":"C4 OK\n",fails); return fails?1:0; }
